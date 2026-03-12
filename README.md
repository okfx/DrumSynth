# DrumSynth

A 3-voice drum synthesizer with delay line and effects, built on Teensy 4.0. Designed for hands-on use -- capable of classic drum machine sounds, but with wide-open parameter ranges that push into expressive, experimental territory. Firmware v1.04.

## Features

**Three drum voices** with deep per-voice synthesis control:
- **D1 Kick** -- Sine-based kick with pitch sweep, drive, and tone shaping
- **D2 Snare/Clap** -- Snare body with tunable pitch, clap with multi-burst envelope, and a wavefolder oscillator sub-voice
- **D3 Hats/Perc** -- Three models crossfaded via mix knob: 606-style analog, FM metallic, and perc (click-to-tone depending on decay)

**Master Wavefolder** -- Sine and saw oscillators through a waveshaping stage. Frequency sweeps exponentially across four octaves, fold drive has loudness compensation, and the two oscillators diverge across three zones for evolving harmonic content.

**Choke Knob** -- Global envelope tightening/loosening across all voices. Negative values shorten decays, positive values lengthen them. Each voice responds with tuned sensitivity for musical results.

**16-Step Sequencer** with per-step programming, 10 pattern save/load slots, and per-hat accent patterns.

**Audio Outputs** -- 1/4" and 1/8" line out, headphone out, and USB audio.

**External Clock Sync** -- Accepts an external pulse clock with configurable PPQN, auto lock-in, and armed count-in with beat countdown. See [`EXTERNAL_SYNC_ARCHITECTURE.md`](EXTERNAL_SYNC_ARCHITECTURE.md) for details.

**OLED Display** -- Real-time oscilloscope waveform view and parameter information.

## CHROMA Mode

CHROMA turns the drum machine into a step-sequenced synthesizer. Any voice can be switched from its normal drum sound to a chromatic pitch mode, where each step in the sequence plays its own programmable note. Layer multiple CHROMA voices to build basslines, melodies, and tonal textures on top of (or instead of) the drum pattern.

The master wavefolder can also be placed in CHROMA mode, snapping its frequency to semitones and turning it into a sequenced harmonic voice layered over the full mix.

CHROMA notes are saved per-pattern. Active CHROMA channels are indicated by small dots at the bottom of the OLED.

## Hardware

- **MCU:** Teensy 4.0 (ARM Cortex-M7 @ 600 MHz)
- **Audio:** Teensy Audio Library -> SGTL5000 codec (headphone + line out) + USB audio
- **Display:** 128x64 OLED (software SPI)
- **Controls:** 32 knobs (2x 16-ch analog mux), 16 step buttons, 10 control buttons
- **LEDs:** 16 step LEDs (74HC595 shift register)
- **Clock:** Internal BPM with external pulse clock sync
- **Storage:** EEPROM with wear-leveling -- 10 pattern slots storing drum steps + CHROMA notes

## File Structure

| File | Purpose |
|------|---------|
| `DrumSynth.ino` | Main firmware -- sequencer, UI, knob engine, display |
| `audiotool.h` | Audio graph (Teensy Audio Design Tool export) |
| `audio_init.h` | Mixer gains, envelope params, filter settings |
| `hw_setup.h` | Pin assignments, mux/LED/OLED hardware config |
| `ext_sync.h` | External clock sync -- ISRs, glitch filter, lock-in, subdivisions |
| `eeprom.h` | Pattern save/load, CHROMA notes + PPQN persistence |
| `knob_handlers.h` | Table-driven knob dispatch -- 32 display + 32 engine functions |
| `monobass.h` | MONOBASS live keyboard mode -- entry/exit, button handler, scope |
| `oscilloscope.h` | Scrolling waveform display (decimation, auto-scale) |
| `bitmaps.h` | OLED transport icons (play/stop) |

## Changes (v1.04)

- **MONOBASS mode** -- hold D1 for 6 seconds to enter a live mono keyboard using the step buttons as keys, with oscilloscope waveform display. D2/D3 knobs, BPM, choke, and memory/load/save are disabled during MONOBASS mode.
- D2 and D3 wavefolder carrier frequencies lowered to sub-bass range (32-110 Hz for D3, 32-440 Hz for D2)
- Table-driven knob dispatch replaces switch blocks -- all 32 knobs handled via `knobTable[]` with paired display/engine function pointers
- Table-driven button dispatch replaces `updateOtherButtons()` -- 10 control buttons handled via `btnHandlers[]` with press/release/hold callbacks
- Display rendering extracted into focused sub-renderers (`renderTopBar`, `renderStepRow`, etc.)
- Accent preview and MONOBASS UI state grouped into structs
- Unified MIDI-to-Hz lookup across all voices
- D3 filter curve unified between display and engine (was showing wrong range on display)
- Choke display type corrected from float to int
- Replaced `strncpy` with `snprintf` throughout for safe string handling
- CHROMA note-select state cleared on MONOBASS mode entry to prevent stale state
- EEPROM backward compatibility preserved across format changes
- ISR robustness: `setTransport()` refactored to remove non-nestable interrupt pair; `applyMasterGain()` guarded with `AudioNoInterrupts()`; count-in beat display snapshots `armPulseCountdown` atomically with `ppqn`
- WF Chromatic mode now works during MONOBASS — indicator dot and parameter overlay display correctly
- MONOBASS key responsiveness: LED updates batched to 1 SPI transaction (was 16); OLED frame skipped on key events to prevent SPI blocking
- MONOBASS release time minimum lowered from 10ms to 3ms for tighter note cutoff
- Oscilloscope scroll speed reduced for easier waveform shape comparison across oscillators
- LED updates batched to single SPI transaction across all `updateLEDs()` paths (accent preview, normal pattern)
- Audio thread safety: `AudioNoInterrupts()` guards added to `applyD2ChromaFreq()`, volume engines, and `engineD2Pitch()`
- WF Chromatic mode state now persisted in EEPROM pattern slots (backward-compatible with V1/V2 slots)

## Changes (v1.03.1)

- CHROMA mode state (on/off per track) now saved and restored with each pattern slot
- Entering CHROMA mode on a voice auto-selects that track for immediate step editing
- CHROMA long-press time reduced to 1.5s (was 2s)
- D2 snare decay curve rescaled -- wider useful range across the full knob travel
- D2 snare and clap attack times tightened across all envelope stages
- Clap bandpass filter frequencies adjusted for tone
- D3 lowpass filter minimum cutoff lowered for darker perc sounds
- Splash animation plays faster
- Sequencer catch-up logic extended to internal clock -- rare missed triggers at high BPM eliminated

## Changes (v1.03)

- CHROMA modes for all voices (D1/D2/D3/WF) with per-step note selection
- WF CHROMA mode quantizes wavefolder to chromatic pitches
- Dot indicator for active CHROMA channels
- Choke knob with deeper negative range for tighter envelopes
- Clap master highpass lowered for fuller transients
- USB audio output
- Animated waveform morph splash screen

## Dependencies

- [Teensy Audio Library](https://www.pjrc.com/teensy/td_libs_Audio.html)
- [Mux](https://github.com/stechio/arduino-ad-mux-lib) (analog multiplexer)
- [ResponsiveAnalogRead](https://github.com/dxinteractive/ResponsiveAnalogRead)
- [ShiftRegister74HC595](https://github.com/Simsso/ShiftRegister74HC595)
- [Adafruit GFX](https://github.com/adafruit/Adafruit-GFX-Library) + [Adafruit SH110X](https://github.com/adafruit/Adafruit_SH110x)

## Build

```bash
arduino-cli compile --fqbn "teensy:avr:teensy41:usb=audio" --build-property "build.flags.optimize=-O2" .
arduino-cli upload --fqbn "teensy:avr:teensy41:usb=audio" -p /dev/ttyACM0 .
```
