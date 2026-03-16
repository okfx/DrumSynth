# DrumSynth — Claude Code Context

## What This Is

Teensy 4.0 drum synthesizer: 3-voice (kick/snare/hi-hat) step sequencer with
chromatic modes, external clock sync, OLED display, and EEPROM pattern storage.
Hardware: PJRC Audio Library, 128×64 SH1106G OLED (software SPI), 32 knobs via
2×16 mux, 16 step buttons + 10 control buttons via mux, 2×74HC595 LED shift register.

## Source Files

Single translation unit — Arduino IDE compiles `.ino` + all `#include`d headers
as one file. Headers are inline code segments, not independent modules.

| File | Purpose | Notes |
|------|---------|-------|
| `DrumSynth.ino` | Main firmware (~1750 lines) | State, setup, loop, transport, triggers, knobs |
| `audiotool.h` | Audio Design Tool output | **Auto-generated — NEVER full-file rewrite** |
| `audio_init.h` | One-time audio object init | Codec, envelopes, filters, mixer gains |
| `hw_setup.h` | Pin assignments, peripheral objects | Mux, shift register, OLED, constants |
| `ext_sync.h` | External clock ISR + helpers | Concurrency contract documented at top |
| `eeprom.h` | Pattern save/load, PPQN persistence | CRC-8, magic number versioning (0x4249) |
| `chroma.h` | Chromatic mode pitch/note/envelope | MIDI freq table, knob-to-note, dot animation |
| `monobass.h` | MONOBASS mode (live keyboard) | Entry/exit, button handler, scope renderer |
| `splash.h` | Boot splash + dissolve | Version display, Bayer dissolve into idle UI |
| `xcombo_overlay.h` | X-combo help overlay | Full-screen diagram + scrolling marquee |
| `buttons.h` | Button state machine + combos | ButtonHandler, ComboModState, dispatch table |
| `knob_handlers.h` | 32 display + 32 engine functions | Table-driven dispatch via `knobTable[32]` |
| `display_ui.h` | OLED rendering + updateDisplay() | Top bar, scope, chroma dots, parameter overlay |
| `oscilloscope.h` | Scope display | Circular buffer, auto-scale, AC-coupled |
| `bitmaps.h` | OLED icons | Play/stop bitmaps |

### Include Order (matters — each header depends on prior declarations)

```
audiotool.h → audio_init.h → bitmaps.h → hw_setup.h
  → ext_sync.h → oscilloscope.h → eeprom.h
  → chroma.h → monobass.h → splash.h → xcombo_overlay.h
  → buttons.h → knob_handlers.h → display_ui.h
```

## Build Command

```bash
arduino-cli compile --fqbn "teensy:avr:teensy40:usb=audio" \
  --build-property "build.flags.optimize=-O2" .
```

Always compile after edits. Zero warnings expected.

## Architecture

### Audio Signal Chain

- **D1 (Kick):** sine/saw/square → pitch envelope → filter → wavefolder → amp
- **D2 (Snare/Clap):** osc + 808-style clap (dual noise + delay burst) + click → reverb/wavefolder → amp
- **D3 (Hi-Hat):** 6× 606 square bank + 2× FM pairs + perc → wavefolder → ladder filter → amp
- **Master:** drumMixer → master wavefolder → filters → delay → output

### Concurrency Model

All ISRs pinned to NVIC priority 128 (same priority = no preemption).
ISR sources: `stepISR` (PIT), `subdivTimerCallback` (PIT), `externalClockISR` (GPIO).

- **ISR writes, main reads:** `currentStep`, `pendingStepCount`, `sysTickMs`, ext clock timing vars
- **Main writes, ISR reads:** `transportState`, `sequencePlaying`, `armed`, `ppqn`, `bpm` (via `shuffledStepPeriodUs()`)
- **Main only:** sequences, trigger functions, display, knob state, EEPROM

Rule: aligned 32-bit reads/writes are atomic on ARM Cortex-M7. Use `noInterrupts()`
when snapshotting multiple related ISR variables that must be consistent together.

**`setTransport()` interrupt contract:** This function does NOT manage interrupt
guards — `transportState` is `volatile uint8_t` (ARM-atomic). Callers that bundle
`setTransport()` with other ISR-shared writes must provide their own
`noInterrupts()`/`interrupts()` wrapper. See the comment block in `setTransport()`
for the full per-callsite contract. ARM `noInterrupts()`/`interrupts()` are
non-nestable; never add an `interrupts()` call inside a wider critical section.

### Dispatch Tables

- **Knobs:** `knobTable[32]` in `knob_handlers.h` — each entry has `.display()` and `.engine()` function pointers
- **Buttons:** `btnHandlers[10]` in `DrumSynth.ino` — each entry has `.onPress()`, `.onRelease()`, `.onHoldCheck()`

### Modes

| Mode | Entry | Effect |
|------|-------|--------|
| **MONOBASS** | D1 hold 6s | Stops transport, D1 becomes live mono keyboard, D2/D3/BPM/choke knobs + MEM/LOAD/SAVE disabled |
| **D1 Chroma** | D1 hold 1.5s | Per-step pitch, knob 3 → note select |
| **D2 Chroma** | D2 hold 1.5s | Per-step pitch for snare osc |
| **D3 Chroma** | D3 hold 1.5s | Per-step pitch for all D3 voices |
| **WF Chroma** | PLAY hold 1.5s | Master wavefolder freq quantized to chromatic notes |
| **PPQN Select** | X+LOAD hold 1.5s | Stops transport, cycles PPQN values, hold again to save |
| **Shuffle** | X+step15 | TR-909 shuffle (7 levels), internal clock only, disabled in MONOBASS |
| **X-Combo Overlay** | Hold X (MEM) | Full-screen help overlay showing available combos, scrolling marquee |

### EEPROM

- 10 pattern slots × 102 bytes + 2 bytes PPQN + 2 bytes MONOBASS = 1024 of 1080 bytes (Teensy 4.0)
- Slots 0–9 accessible via combo+step buttons; compile-time `static_assert` guards capacity
- Magic `0x4249` (current), `0x4248`/`0x4247` (legacy backward compat)
- CRC-8 over `PatternStore` — rejects corrupted slots
- Stores: 3 sequences + 3 chroma note arrays + chroma mode flags (bits 0-3) + shuffle mode (bits 4-6) + MONOBASS active state

### Knob Map

| 0–7 (D1) | 8–15 (D2) | 16–23 (D3) | 24–31 (Master) |
|-----------|-----------|------------|----------------|
| 0 Distort | 8 Pitch | 16 Pitch | 24 Delay Time |
| 1 Shape | 9 Decay | 17 Decay | 25 WF Freq |
| 2 Decay | 10 Voice Mix | 18 Voice Mix | 26 Lowpass |
| 3 Pitch | 11 WF Drive | 19 Distort | 27 BPM |
| 4 Volume | 12 Delay Send | 20 Delay Send | 28 Volume |
| 5 Snap | 13 Reverb | 21 Filter | 29 Choke |
| 6 Body/EQ | 14 Noise | 22 Accent | 30 WF Drive |
| 7 Delay Send | 15 Volume | 23 Volume | 31 Delay Mix |

## Editing Rules

1. **Targeted edits only.** Never rewrite entire files. Work on specific functions.
2. **Wrap audio changes** in `AudioNoInterrupts()`/`AudioInterrupts()`.
3. **Compile after every change.** Use the build command above.
4. **When adding shared curves** (knob → value), put the curve function in
   `DrumSynth.ino` with the other `*Curve()` helpers and call it from both
   display and engine functions. Never duplicate the formula.
5. **When disabling knobs/buttons in MONOBASS mode**, follow the existing pattern:
   display shows `"DISABLED FOR"` / `"MONOBASS"`, engine returns early.
6. **When asked to discuss or conceptualize**, do NOT explore the codebase unless asked.

## Sharp Edges

- `audiotool.h` is generated by the [Teensy Audio Design Tool](https://www.pjrc.com/teensy/gui/).
  Edit individual lines only. Regenerate via the web tool if restructuring audio routing.
- `noInterrupts()`/`interrupts()` don't nest. See concurrency hazard above.
- OLED `display.display()` blocks for 15–25 ms (software SPI bit-bang).
  `isSafeToPushOled()` gates the push — respect it.
- `delayMicroseconds()` in knob/button scanning adds up. Round-robin scanning
  (8 knobs per loop) keeps it manageable.
- LSP diagnostics will show false errors (unknown types like `uint8_t`, `Adafruit_SH1106G`)
  because clang doesn't see Arduino/Teensy headers. These are expected — compile to validate.
- EEPROM magic number must be bumped when `PatternStore` layout changes.

## When to Update This File

- New header added or include order changed
- New mode added (like MONOBASS was)
- Knob assignments changed
- EEPROM format version bumped
- Concurrency contract changed (new ISR, new shared variable)
- Build command or hardware target changed
