# DrumSynth — Project Status

*Last updated: 2026-03-16 · 212 commits · ~7,000 lines across 15 files*

---

## Implemented Features

### Core Synthesis Engine
- **D1 (Kick):** Sine/saw/square oscillator with pitch envelope, resonant filter, wavefolder, and amplitude envelope. Controllable distortion, shape (waveform morph), decay, pitch, snap (click transient), and body/EQ.
- **D2 (Snare/Clap):** Oscillator + 808-style clap engine (dual noise bursts with timed delay for clap texture) + transient click. Dedicated reverb send, wavefolder drive, and noise balance control.
- **D3 (Hi-Hat):** 6× 606-style square wave bank + 2× FM operator pairs + percussive noise source. Routed through wavefolder and ladder filter. Voice mix knob crossfades FM ↔ 606 ↔ Perc. Accent system with per-step dynamics.
- **Master bus:** Drum mixer → master wavefolder (frequency + drive) → lowpass/highpass filter → stereo delay → output. Per-voice delay sends. Global volume and choke (tightens all voice decays simultaneously).
- **Audio memory:** 1200 blocks allocated — handles long delay tails without dropout.

### Step Sequencer
- 16-step sequencer with independent patterns per voice (D1/D2/D3).
- Step buttons toggle hits with LED feedback on the active pattern.
- Play/stop transport with visual step position indicator (LED chase).
- Per-step accent on D3 via dedicated accent mask.

### Timing & Sync
- **Internal clock:** Variable BPM (knob 27), PIT timer-driven step ISR with microsecond precision.
- **External clock:** GPIO interrupt-driven with exponential moving average for jitter smoothing. Auto lock-in detection and timeout fallback. Configurable PPQN (1, 2, 4, 24) with EEPROM persistence.
- **TR-909 shuffle:** 6 audible intensity levels + OFF, applied to even-numbered steps via 96-PPQN subdivision grid. Correct formula: `(24 + delayTicks) / 48 × 100`. Internal clock only; disabled in MONOBASS. Stored per-pattern in EEPROM.

### Chromatic Modes
- **D1 Chroma:** Hold D1 button 1.5s to enter. Per-step pitch assignment via knob 3 (note select). Dot animation on display indicates active chroma notes per step.
- **D2 Chroma:** Same mechanic for snare oscillator pitch.
- **D3 Chroma:** Same mechanic applied to all D3 voice banks simultaneously.
- **WF Chroma:** Hold PLAY 1.5s — master wavefolder frequency quantized to chromatic scale.
- MIDI frequency table covers full range. Chroma state (which modes are active + per-step note arrays) saved/loaded with patterns.

### MONOBASS Mode
- Hold X button 8s to enter. Stops transport, repurposes D1 as a live monophonic bass keyboard across the 16 step buttons.
- Proper key priority (highest note wins), envelope filter tracking note frequency.
- Idle grid animation (timeout to animated display when no keys pressed).
- Oscilloscope waveform display while playing.
- Knobs 0–7 (D1 rail) remain active for live sound shaping; all other knobs/buttons disabled with "DISABLED FOR MONOBASS" feedback.
- MONOBASS active state persists across power cycles via EEPROM.
- White-fill sweep animation on entry, dissolve animation on exit.

### Pattern Storage (EEPROM)
- 10 pattern slots (0–9), selectable via X + step button combos.
- Per-slot: 3 step sequences + 3 chroma note arrays + chroma mode flags + shuffle mode + MONOBASS state.
- CRC-8 integrity check on load — rejects corrupted data.
- Magic number versioning (current `0x4249`) with backward compatibility for two prior formats (`0x4248`, `0x4247`).
- "On deck" slot selection — choosing a slot queues it for load at next bar, with pending blink indicator on LEDs. Replaces old instant-load behavior.
- Factory reset via LOAD hold (clears current pattern state, resets shuffle).

### Display (128×64 SH1106G OLED)
- **Top bar:** Track name (D1/D2/D3), play/stop icon, BPM display (internal or EXT), chroma mode indicators, shuffle level indicator.
- **Oscilloscope:** Real-time audio waveform with auto-scaling, AC coupling, trigger detection, and noise floor suppression.
- **Parameter overlay:** Knob-touch displays parameter name + value with auto-timeout. Two-line format for all 32 knobs with descriptive labels.
- **Chroma dots:** Visual note indicator per step when in chroma mode. Animated ramp/settle transitions.
- **Voice rails:** Caret-style track selector with outlined labels.
- **X-combo overlay:** Full-screen static help diagram showing all available button combos. Appears on X hold.
- **Boot splash:** Version display with Bayer dither dissolve transition into idle UI.
- **MONOBASS displays:** Dedicated idle grid, scope view, knob icons, outlined center text.
- **Chunked OLED push:** 8 pages pushed one-at-a-time, gated by `isSafeToPushOled()` to avoid starving the USB audio pipeline. Only `setup()` uses blocking pushes (before audio init).

### Hardware Interface
- 32 knobs via 2×16-channel analog mux with `ResponsiveAnalogRead` filtering (activity threshold prevents drift).
- Round-robin scanning: 8 knobs per loop iteration across 4 groups, with `delayMicroseconds()` settling time.
- 16 step buttons + 10 control buttons via digital mux.
- 2×74HC595 shift register for 16 step LEDs (active step chase + pattern display).
- Software SPI bit-bang for OLED (dedicated pins, no DMA contention with audio).
- Button state machine with press/release/hold detection and debouncing.
- X (MEM) button modifier system: hold X + press another button for combos (pattern slots, shuffle, PPQN, clear, chroma toggles).

---

## What Needs Improvement

### Audio / Sound Design
- **AVC limiter is disabled.** `audio_init.h` has the SGTL5000 auto volume control configured but turned off. Could serve as a safety limiter to prevent clipping on master output — needs testing to find settings that limit without pumping.
- **D2 clap texture is basic.** The dual-noise + delay burst approach works but the clap lacks the multi-tap randomness of a real 808/909 clap. Could benefit from more burst stages or slight per-burst pitch variation.
- **No per-voice filter envelopes on D2.** D1 has a pitch envelope and envelope filter; D3 has a ladder filter. D2's filter options are limited to the global reverb send and wavefolder.
- **Delay is mono.** A ping-pong or stereo spread option would add depth.

### Sequencer
- **No pattern chaining.** Only single 16-step loops — no song mode or pattern queue for live performance.
- **No per-step velocity/probability.** D3 has accent but D1/D2 don't. No probability per step for generative patterns.
- **No swing per voice.** Shuffle applies globally to all voices equally. Per-voice swing would allow tighter hats over swung kick/snare.
- **No step copy/paste.** No way to duplicate or shift patterns within slots.

### UX / Display
- **No undo for pattern edits.** One wrong combo can clear a pattern with no recovery (factory reset clears immediately).
- **No visual slot preview.** When selecting a slot to load, there's no indication whether it contains a pattern or is empty.
- **Parameter overlay blocks scope.** When turning knobs, the two-line text overlay replaces the oscilloscope. A non-blocking mini display (single-line or sidebar) could keep both visible.
- **BPM display precision.** External BPM shows as integer — fractional BPM (e.g., 120.5) would be useful for external sync verification.

### Code / Architecture
- **Single translation unit.** The entire firmware compiles as one file via Arduino `#include` chain. This means every edit recompiles everything (~7k lines). Switching to proper `.cpp`/`.h` separation with forward declarations would enable incremental compilation, but would require significant restructuring of the shared state model.
- **No unit tests.** All validation is compile + hardware testing. The dispatch-table architecture and pure curve functions are testable in isolation if a test harness were added.
- **3 null button handlers.** Buttons 3, 4, 5 in the mux have no physical hardware — their handler slots are wired to no-ops. Could be repurposed if hardware is added.

---

## What Needs Polish

All prior audit findings have been addressed. No outstanding polish items at this time.

---

## Project Assessment

### Where We Are

**The firmware is feature-complete for its v1.x scope and production-stable.** The synth plays, sequences, stores patterns, syncs to external clock, and provides responsive UI feedback — all without audio dropouts thanks to the chunked OLED push system.

The codebase has been through **5+ full audit passes** (225 commits), progressively tightening concurrency safety, eliminating dead code, deduplicating logic, and improving naming. The most recent audit identified 23 items; analysis proved 8 were non-issues, and the remainder have fixes ready.

### Maturity
- **Audio engine:** Solid. D1 and D3 are expressive instruments with deep sound-shaping. D2 is functional but has the least tonal range.
- **Sequencer:** Reliable for basic step programming. Lacks features expected in more advanced drum machines (chaining, probability, copy/paste).
- **Sync:** Rock-solid internal timing. External clock works with configurable PPQN and handles jitter well via EMA smoothing.
- **Storage:** Robust with CRC validation and backward-compatible format versioning. 10 slots is adequate for the 1080-byte Teensy 4.0 EEPROM.
- **Display:** Well-optimized. The chunked push was the hardest problem solved — USB audio and bit-banged SPI competing for CPU time is now handled gracefully.
- **Code quality:** Clean — zero TODO/FIXME comments, zero commented-out code blocks, no dead functions, consistent naming. The main risk area is the single-TU architecture making refactors expensive.

### What Would Make This a v2.0
1. Pattern chaining / song mode
2. Per-step probability and velocity
3. MIDI in/out (note + clock)
4. Stereo delay with ping-pong
5. Sample import (flash storage) for D2 or a 4th voice
6. Proper multi-file compilation with `.cpp` separation

### Risk Areas
- **EEPROM is 94% full** (1024 of 1080 bytes). Any new per-pattern data requires either reducing slots or moving to external storage.
- **AudioMemory at 1200 blocks** is high. Long delay tails with all voices active could still cause allocation pressure under extreme settings.
- **`noInterrupts()` sections** are brief and correct today, but adding more shared state between ISR and main loop increases the surface area for concurrency bugs.
