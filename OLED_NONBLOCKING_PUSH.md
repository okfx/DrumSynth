# Non-Blocking SH1106 OLED Push for Teensy 4.x

## The Problem

The Adafruit SH1106 library's `display.display()` writes the entire 1024-byte
framebuffer to the screen in a single blocking call. On software SPI this takes
**15–25 ms** — long enough to starve time-critical interrupts.

On Teensy 4.0/4.1 with USB Audio, the USB stack must service packets every
1 ms. A 20 ms blocking write causes **audible clicks, pops, and dropouts**
because the USB audio buffer underruns while the CPU is bit-banging SPI.

The same problem affects any interrupt-driven workload that can't tolerate
15+ ms of dead time: MIDI output, DMA transfers, motor control, sensor
sampling, etc.

## Who Needs This

- Teensy 4.x projects using **USB Audio + SH1106/SSD1306 OLED** (software SPI)
- Any project where `display.display()` causes timing glitches in
  interrupt-driven code
- Projects hitting audio clicks/pops that disappear when you comment out
  the display update

If you're using hardware SPI or I2C with DMA, you probably don't need this —
those transfers happen in the background. This specifically fixes the
**software SPI bit-bang** path that the Adafruit library uses when you pass
pin numbers to the constructor.

## The Fix

### 1. Push one page per loop iteration instead of all eight at once

The SH1106 framebuffer is organized as 8 pages of 128 bytes each. Instead of
writing all 1024 bytes in one shot, write one page (~131 bytes including
commands) per `loop()` call. Each page takes about **2 ms** — short enough
that USB audio interrupts fire between pages.

```cpp
static int8_t oledPushPage = -1;   // -1 = idle, 0–7 = next page to push

static void oledPushOnePage() {
  if (oledPushPage < 0 || oledPushPage >= 8) return;

  uint8_t* buf = display.getBuffer();
  int page = oledPushPage;

  oledSwSpiCmd(0xB0 | page);   // set page address
  oledSwSpiCmd(0x02);          // lower column start (SH1106 offset)
  oledSwSpiCmd(0x10);          // upper column start

  for (int col = 0; col < 128; col++) {
    oledSwSpiWrite(buf[page * 128 + col]);
  }

  oledPushPage++;
  if (oledPushPage >= 8) oledPushPage = -1;   // done
}
```

Call `oledPushOnePage()` at the top of every `loop()`. When you want to start
a new frame, set `oledPushPage = 0`. The full screen transfer completes over
8 loop iterations instead of blocking for one long stretch.

To check whether a transfer is still in progress:

```cpp
static inline bool oledPushInProgress() { return oledPushPage >= 0; }
```

Don't start a new frame while one is still pushing — you'll get tearing.
Build your framebuffer with the normal Adafruit drawing calls, then set
`oledPushPage = 0` only when `oledPushInProgress()` returns false.

### 2. Bit-bang SPI with DSB barriers for Teensy 4.x at 600 MHz

The Adafruit library's `display.display()` uses `digitalWrite()` internally,
which is slow enough (~150 ns per call) that the SH1106 can latch data
reliably. But when you write your own bit-bang with `digitalWriteFast()`, the
Cortex-M7 at 600 MHz toggles GPIO in **~2 ns** — far below the SH1106's
minimum 100 ns clock cycle.

The symptom: your code runs, the page push executes, but **the display shows
nothing** or garbage. The fix is an ARM Data Synchronization Barrier (`dsb`)
between transitions:

```cpp
static inline void oledSwSpiWrite(uint8_t data) {
  for (int8_t bit = 7; bit >= 0; bit--) {
    digitalWriteFast(OLED_MOSI, (data >> bit) & 1);
    __asm__ volatile("dsb");   // ~10 ns hold time
    digitalWriteFast(OLED_CLK, HIGH);
    __asm__ volatile("dsb");
    digitalWriteFast(OLED_CLK, LOW);
  }
}

static inline void oledSwSpiCmd(uint8_t cmd) {
  digitalWriteFast(OLED_DC, LOW);
  oledSwSpiWrite(cmd);
  digitalWriteFast(OLED_DC, HIGH);
}
```

The `dsb` instruction forces the CPU to complete all pending memory/register
writes before continuing. On Cortex-M7 this costs about 10 ns — enough for
the display to see clean edges, fast enough that a full page still takes
only ~2 ms.

**Do not use `delayNanoseconds()` or `NOP` loops** — `dsb` is the correct
tool here because GPIO writes go through a write buffer that can reorder
relative to subsequent operations. `dsb` guarantees the pin state is
committed to the peripheral bus before the next write.

### 3. SSD1306 variant

If you're using an SSD1306 instead of SH1106, change the column start
commands:

```cpp
// SH1106: 2-pixel column offset
oledSwSpiCmd(0x02);   // lower nibble
oledSwSpiCmd(0x10);   // upper nibble

// SSD1306: no offset
oledSwSpiCmd(0x00);   // lower nibble
oledSwSpiCmd(0x10);   // upper nibble
```

Everything else is identical — both use the same page addressing scheme.

## Putting It Together

Minimal integration with `Adafruit_SH1106G`:

```cpp
#include <Adafruit_SH110X.h>

// Your pin assignments
#define OLED_MOSI  4
#define OLED_CLK   5
#define OLED_DC    6

Adafruit_SH1106G display(128, 64, OLED_MOSI, OLED_CLK, OLED_DC, -1, -1);

static int8_t oledPushPage = -1;

static inline void oledSwSpiWrite(uint8_t data) {
  for (int8_t bit = 7; bit >= 0; bit--) {
    digitalWriteFast(OLED_MOSI, (data >> bit) & 1);
    __asm__ volatile("dsb");
    digitalWriteFast(OLED_CLK, HIGH);
    __asm__ volatile("dsb");
    digitalWriteFast(OLED_CLK, LOW);
  }
}

static inline void oledSwSpiCmd(uint8_t cmd) {
  digitalWriteFast(OLED_DC, LOW);
  oledSwSpiWrite(cmd);
  digitalWriteFast(OLED_DC, HIGH);
}

static void oledPushOnePage() {
  if (oledPushPage < 0 || oledPushPage >= 8) return;
  uint8_t* buf = display.getBuffer();
  int page = oledPushPage;
  oledSwSpiCmd(0xB0 | page);
  oledSwSpiCmd(0x02);          // 0x00 for SSD1306
  oledSwSpiCmd(0x10);
  for (int col = 0; col < 128; col++) {
    oledSwSpiWrite(buf[page * 128 + col]);
  }
  if (++oledPushPage >= 8) oledPushPage = -1;
}

static inline bool oledPushInProgress() { return oledPushPage >= 0; }

void setup() {
  display.begin(0, true);
  // ... your init code ...
}

void loop() {
  // Always service the page push first
  oledPushOnePage();

  // Your main loop work (read inputs, process audio, etc.)
  // ...

  // Only start a new frame when the previous one is done
  if (!oledPushInProgress()) {
    display.clearDisplay();
    // ... draw your UI with normal Adafruit calls ...
    oledPushPage = 0;   // kick off the chunked transfer
  }
}
```

## Performance

| Metric | `display.display()` | Chunked push |
|---|---|---|
| Blocking time per call | 15–25 ms | ~2 ms (one page) |
| Total transfer time | 15–25 ms | ~16 ms (spread over 8 iterations) |
| Max interrupt latency | 15–25 ms | ~2 ms |
| Effective frame rate | ~40–60 fps (if nothing else runs) | 24 fps typical (loop-rate dependent) |
| USB Audio compatible | No (causes underruns) | Yes |

The total transfer time is similar — you're still sending the same 1024 bytes.
The difference is that interrupts get serviced between pages instead of being
locked out for the entire transfer.

## Troubleshooting

**Display stays blank after switching to chunked push:**
You're almost certainly missing the DSB barriers. At 600 MHz the clock
transitions are invisible to the SH1106. Add `__asm__ volatile("dsb")` as
shown above.

**Display works but shows previous frame / tearing:**
You're starting a new push before the previous one finishes. Check
`oledPushInProgress()` before setting `oledPushPage = 0`.

**Works at first, then freezes after a few seconds:**
Your `loop()` is taking too long between `oledPushOnePage()` calls. The
display doesn't have a timeout, but if your loop takes >100 ms, the display
update will be visibly slow. Keep your loop fast.

**Audio still clicks occasionally:**
A single page push (~2 ms) might still be too long for your specific
interrupt timing. You can push half a page (64 bytes) per iteration instead
— split the inner loop at `col < 64` and `col < 128` across two calls.
