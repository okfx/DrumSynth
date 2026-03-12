# DrumSynth

A 3-voice drum synthesizer with delay line and effects, built on Teensy 4.0. Designed for hands-on use -- capable of classic drum machine sounds, but with wide-open parameter ranges that push into expressive, experimental territory. Firmware v1.03.

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

**OLED Display** -- Real-time oscilloscope waveform view.

## CHROMA Mode

CHROMA turns the drum machine into a step-sequenced synthesizer. Any voice can be switched from its normal drum sound to a chromatic pitch mode, where each step in the sequence plays its own programmable note. Layer multiple CHROMA voices to build basslines, melodies, and tonal textures on top of (or instead of) the drum pattern.

The master wavefolder can also be placed in CHROMA mode, snapping its frequency to semitones and turning it into a sequenced harmonic voice layered over the full mix.

CHROMA notes are saved per-pattern. Active CHROMA channels are indicated by small dots at the bottom of the OLED.

## Hardware

- **MCU:** Teensy 4.0 (ARM Cortex-M7 @ 600 MHz)
- **Audio:** Teensy Audio Library -> SGTL5000 codec (headphone + line out) + USB audio
- **Display:** 128x64 OLED (software SPI)
- **Controls:** 32 knobs (2x 16-ch analog mux), 16 step buttons, 7 control buttons
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
| `oscilloscope.h` | Scrolling waveform display (decimation, auto-scale) |
| `bitmaps.h` | OLED transport icons (play/stop) |

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
