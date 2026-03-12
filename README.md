# DrumSynth

A 3-voice drum synthesizer + wavefolder built on Teensy 4.1. Firmware v1.03.

## Features

**Three drum voices** with deep per-voice synthesis control:
- **D1 Kick** — Sine-based kick with pitch sweep, drive, and tone shaping (55–440 Hz)
- **D2 Snare/Clap** — Snare body with tunable pitch, clap with multi-burst envelope, and a wavefolder oscillator sub-voice (27.5–880 Hz)
- **D3 Hats** — Three hat models crossfaded via mix knob: 606-style analog, FM metallic, and noise

**Master Wavefolder** — Sine + saw oscillators through a waveshaping stage. Knob 25 sweeps frequency exponentially across 4 octaves (C2–C6). Knob 30 controls fold drive with loudness compensation. The two oscillators diverge across three zones for evolving harmonic content.

**CHROMA Modes** — Per-step chromatic note programming for any voice. Long-press a track button (2s) to enter CHROMA mode, hold a step (300ms) to select its note with the pitch knob. Each step plays its own tuned pitch while the sequencer runs. Active channels are shown as small dots at the bottom of the display.

| Mode | Activation | Note Range |
|------|-----------|------------|
| D1 CHROMA | Hold D1 (2s) | A1–A4 |
| D2 CHROMA | Hold D2 (2s) | A2–A4 |
| D3 CHROMA | Hold D3 (2s) | C3–C6 |
| WF CHROMA | Hold PLAY (2s) | C2–C6 |

**Choke Knob** — Global envelope tightening/loosening across all voices. Negative values shorten decays (down to -55ms), positive values lengthen them. Each voice responds with tuned sensitivity for musical results.

**16-Step Sequencer** with per-step programming, 10 pattern save/load slots (EEPROM), and accent patterns (straight, offbeat, bossa).

**USB Audio Output** — Stereo audio over USB alongside the headphone/line output. Trim knob for USB level.

**External Clock Sync** — Pulse input on pin 12 with configurable PPQN (1/2/4/8/24/48/96), auto lock-in, armed count-in with beat countdown, and glitch-filtered pulse rejection. See [`EXTERNAL_SYNC_ARCHITECTURE.md`](EXTERNAL_SYNC_ARCHITECTURE.md) for details.

**OLED Display** — 128×64 SH1106 with real-time oscilloscope waveform, large track number, parameter overlays, and full-screen note-select during CHROMA step editing. Display updates yield to audio timing to prevent sequencer jitter.

**Animated Splash** — Waveform morph animation on power-up.

## Hardware

- **MCU:** Teensy 4.1 (ARM Cortex-M7 @ 600 MHz)
- **Audio:** Teensy Audio Library → SGTL5000 codec (headphone + line out) + USB audio
- **Display:** 128×64 SH1106 OLED (software SPI)
- **Controls:** 32 knobs (2× 16-ch analog mux), 16 step buttons, 10 control buttons
- **LEDs:** 16 step LEDs (74HC595 shift register)
- **Clock:** Internal BPM (60–400) + external pulse sync on pin 12
- **Storage:** EEPROM with `update()` wear-leveling — 10 pattern slots storing drum steps + CHROMA notes for D1/D2/D3, PPQN validated before write

## File Structure

| File | Purpose |
|------|---------|
| `DrumSynth.ino` | Main firmware — sequencer, UI, knob engine, display |
| `audiotool.h` | Audio graph (Teensy Audio Design Tool export) |
| `audio_init.h` | Mixer gains, envelope params, filter settings |
| `hw_setup.h` | Pin assignments, mux/LED/OLED hardware config |
| `ext_sync.h` | External clock sync — ISRs, glitch filter, lock-in, subdivisions |
| `eeprom.h` | Pattern save/load, CHROMA notes + PPQN persistence |
| `oscilloscope.h` | Scrolling waveform display (decimation, auto-scale) |
| `bitmaps.h` | OLED transport icons (play/stop) |

## Changes (v1.03)

- CHROMA modes for all voices (D1/D2/D3/WF) with per-step note selection
- WF CHROMA mode (PLAY button 2s hold) quantizes wavefolder to chromatic pitches
- Dot indicator replaces text status bar for active CHROMA channels
- Large track digit display, cleaner note-select overlay
- Choke knob with deeper negative range (-55ms) for tighter envelopes
- Clap master highpass lowered (600→300 Hz) for fuller transients
- EEPROM PPQN validation, numSteps linkage to hardware step count
- Chroma step-handling refactored into shared helpers
- Animated waveform morph splash screen
- USB audio output with trim control

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
