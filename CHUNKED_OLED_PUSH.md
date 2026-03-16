# Chunked OLED Push — USB Audio Click Fix

## Problem
Software SPI `display.display()` bit-bangs the entire 1024-byte SH1106
framebuffer in one call, blocking the CPU for 15–25 ms. During that window
USB audio interrupts can't fire, causing buffer underruns heard as clicks
when recording via Teensy USB audio into a DAW.

## Solution
Split the single blocking push into 8 page-level pushes (~2 ms each).
One page is sent per `loop()` iteration. USB audio interrupts can fire
between pages, eliminating the 15–25 ms starvation window.

### How it works
- `oledPushOnePage()` is called once per `loop()` iteration (before input
  scanning). If no push is in progress it's a no-op.
- `oledStartPush()` initiates a new 8-page transfer. It's called from
  `updateDisplay()` wherever `display.display()` was previously used.
- While a push is in progress (`oledPushInProgress()` returns true),
  `updateDisplay()` is gated from running — the framebuffer is frozen
  until all 8 pages have been sent to prevent tearing.
- `isSafeToPushOled()` still gates the *start* of new pushes to protect
  musical timing, but the guard window is reduced from 25 ms to 3 ms.

### Timing
| Metric               | Before          | After            |
|----------------------|-----------------|------------------|
| Max CPU block        | 15–25 ms        | ~2 ms (1 page)   |
| Full frame push      | 15–25 ms (1 call)| ~16–24 ms (8 calls)|
| OLED_GUARD_US        | 25000           | 3000             |
| OLED_SUBDIV_GUARD_US | 30000           | 5000             |
| Effective frame rate | ~24 fps         | ~24 fps (unchanged)|

### Files changed
- **DrumSynth.ino**: Added `oledSwSpiWrite()`, `oledSwSpiCmd()`,
  `oledPushOnePage()`, `oledStartPush()`, `oledPushInProgress()` before
  the `display_ui.h` include. Added `oledPushOnePage()` call at top of
  `loop()`. Added `!oledPushInProgress()` gate to frame interval check.
  Reduced `OLED_GUARD_US` and `OLED_SUBDIV_GUARD_US`.
- **display_ui.h**: Replaced `display.display()` with `oledStartPush()` in
  count-in, PPQN mode, and main display paths (3 sites).
- **xcombo_overlay.h**: Replaced `display.display()` with `oledStartPush()`
  (1 site), added `lastFrameDrawTick` update.

### What was NOT changed
- **Setup/boot paths** (splash, EEPROM clear): Still use blocking
  `display.display()` — no audio is running yet.
- **Watchdog recovery**: Still uses blocking `display.display()` — rare
  emergency path, acceptable.

### SH1106 page-write protocol
Each page push sends: page address command (0xB0|page), lower column
command (0x02 — SH1106 has 2-pixel horizontal offset), upper column
command (0x10), then 128 bytes of pixel data. This matches the Adafruit
library's internal protocol for SH1106 software SPI.

## How to revert
If this causes display issues (tearing, corruption, visual glitches):

```bash
git revert <commit-hash>   # see git log for the chunked push commit
```

Or manually: replace all `oledStartPush()` calls back to `display.display()`,
remove the page-push functions from DrumSynth.ino, remove `oledPushOnePage()`
from loop(), remove `!oledPushInProgress()` from the frame gate, restore
`OLED_GUARD_US` to 25000 and `OLED_SUBDIV_GUARD_US` to 30000.

## Other potential click sources
If clicks persist after this change, the cause may be the Teensy's USB
audio sample rate mismatch: the PLL outputs 44117.647 Hz, not 44100 Hz.
Some DAWs handle this with adaptive resampling; others don't. This is a
known Teensy firmware limitation with no device-side fix. Try increasing
the DAW's USB audio buffer size as a workaround.
