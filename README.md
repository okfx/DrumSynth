# DrumSynth

A 3-voice drum synthesizer + bass line built on Teensy 4.0.

## Voices

| Voice | Name | Frequency Range |
|-------|------|-----------------|
| D1 | Kick | 55–440 Hz (A1–A4) |
| D1 Bass Line | Chromatic bass | A1–A4 (MIDI 33–69) |
| D2 Snare | Snare body | 110–440 Hz (A2–A4) |
| D2 Clap | Clap | Fixed |
| D2 Wavefolder | Wavefolder osc | 27.5–880 Hz (A0–A5) |
| D3 606 Hats | Analog-style hats | 400–6000 Hz |
| D3 FM Hats | FM carriers | 440–3520 Hz (A4–A7) |
| D3 Noise Hat | Noise hat | 110–1760 Hz (A2–A6) |

## Bass Line Mode

Long-press (3s) the track-select button on D1 to toggle bass line mode. Each step gets its own chromatic pitch (A1–A4). Hold a step button for 300ms to enter note-select, then turn the D1 pitch knob to set that step's note. Bass line notes are saved with patterns to EEPROM.

## Hardware

- **MCU:** Teensy 4.0 (ARM Cortex-M7 @ 600MHz)
- **Audio:** Teensy Audio Library → on-board DAC
- **Display:** 128x64 SH1106 OLED (software SPI) with oscilloscope waveform view
- **Controls:** 32 knobs (2x 16-ch analog mux), 16 step buttons, 10 control buttons
- **LEDs:** 16 step LEDs (74HC595 shift register)
- **Clock:** Internal BPM (60–400) with external pulse sync on pin 12
- **Storage:** 10 EEPROM pattern save/load slots with per-step bass line notes

## File Structure

| File | Purpose |
|------|---------|
| `DrumSynth.ino` | Main firmware — sequencer, UI, knob handling, display |
| `audiotool.h` | Audio graph (Teensy Audio Design Tool export) |
| `audio_init.h` | Mixer gains, envelope params, filter settings |
| `hw_setup.h` | Pin assignments, mux/LED/OLED hardware config |
| `ext_sync.h` | External clock ISR, glitch filter, lock-in logic |
| `eeprom.h` | Pattern save/load, bass line + PPQN persistence |
| `oscilloscope.h` | Scrolling waveform display (decimation, auto-scale) |
| `bitmaps.h` | OLED transport icons (play/stop) |

## Dependencies

- [Teensy Audio Library](https://www.pjrc.com/teensy/td_libs_Audio.html)
- [Mux](https://github.com/stechio/arduino-ad-mux-lib) (analog multiplexer)
- [ResponsiveAnalogRead](https://github.com/dxinteractive/ResponsiveAnalogRead)
- [ShiftRegister74HC595](https://github.com/Simsso/ShiftRegister74HC595)
- [Adafruit GFX](https://github.com/adafruit/Adafruit-GFX-Library) + [Adafruit SH110X](https://github.com/adafruit/Adafruit_SH110x)

## Build

Open in Arduino IDE or compile with arduino-cli:

```bash
arduino-cli compile --fqbn teensy:avr:teensy40 .
arduino-cli upload --fqbn teensy:avr:teensy40 -p /dev/ttyACM0 .
```
