# DrumSynth

A 3-voice drum synthesizer + bass line built on Teensy 4.1. Firmware version 1.02.

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
| D3 Noise Hat | Noise hat | 220–1760 Hz (A3–A6) |
| Master Wavefolder | Sine + saw fold drive | 65.41–1046.50 Hz (C2–C6) |

## Master Wavefolder

Knob 25 sweeps the fold oscillator frequency exponentially from C2 to C6 (4 octaves — every 25% of knob travel = one octave). Knob 30 controls drive intensity with a halved range and loudness compensation. Sine and saw oscillators diverge across three zones: 0–50% (saw one octave below sine), 50–75% (sine diverges up to 2×), 75–100% (saw diverges up to 4×).

## D2 Momentary Reverb

Hold button 1 (D2 track select) for 200ms to arm a reverb slam. The reverb maxes out on the next D2 trigger (roomsize 0.75, damping 0.5, full wet) and stays maxed until the button is released. Short press still selects the D2 track.

## Bass Line Mode

Long-press (2s) the track-select button on D1 to toggle bass line mode. Each step gets its own chromatic pitch (A1–A4, default C2). Hold a step button for 300ms to enter note-select, then turn the D1 pitch knob to set that step's note. Bass line notes are saved with patterns to EEPROM.

## Hardware

- **MCU:** Teensy 4.1 (ARM Cortex-M7 @ 600MHz)
- **Audio:** Teensy Audio Library → SGTL5000 codec (headphone + line out) + USB audio output
- **Display:** 128x64 SH1106 OLED (software SPI) with oscilloscope waveform view
- **Controls:** 32 knobs (2x 16-ch analog mux), 16 step buttons, 10 control buttons
- **LEDs:** 16 step LEDs (74HC595 shift register)
- **Clock:** Internal BPM (60–400) with external pulse clock sync on pin 12
- **Storage:** 10 EEPROM pattern save/load slots with per-step bass line notes (empty slots load as blank patterns)

## External Clock Sync

External clock input on pin 12 (RISING edge). Configurable PPQN (1, 2, 4, 8, 24, 48, 96) — default 2 PPQN (Korg Volca standard). Long-press the slot button (2s) to enter PPQN selection mode, tap to cycle through values, long-press again to save. 5-second timeout exits without saving.

**Transport states:** STOPPED, RUN_INT (internal timer), RUN_EXT (external pulses).

**Lock-in:** 3 accepted pulses (2 consecutive intervals within 25%) required to auto-switch from RUN_INT to RUN_EXT. Two-part glitch filter rejects noise (300 µs hard floor + 40% of EMA relative threshold).

**Step generation:** ISR advances `currentStep` and queues via `pendingStepCount`; main loop fires audio via `playSequence()`. For low PPQN (1 or 2), step A fires on the pulse, then a hardware one-shot timer schedules deferred steps B/C/D at precise intervals. A slow EMA (alpha=1/8) smooths step B placement to resist pulse jitter. Each new pulse cancels and rearms any in-flight subdivision. If steps are backlogged, skip ahead to the latest — a skipped step is less noticeable than an off-beat one.

**Armed count-in:** PLAY with external clock arms a full-cycle count-in (e.g. 8 pulses at 2 PPQN). The OLED shows a 4-3-2-1 beat countdown. Step 0 fires on the aligned pulse boundary. A grace window (50% of pulse interval) forgives slightly-late PLAY presses so the user doesn't end up one pulse out of phase.

**OLED timing guard:** The SH1106 software SPI push takes 15–25 ms. An `isSafeToPushOled()` guard skips the frame if a step is pending, a subdivision timer is due within 25 ms, or the next pulse is predicted within 25 ms. Audio timing always wins over display updates.

**Timeout:** 2 seconds without a pulse falls back to RUN_INT (if playing) or STOPPED. Pulse timing state is preserved across STOP so PLAY detects external clock immediately on the next press.

See [`EXTERNAL_SYNC_ARCHITECTURE.md`](EXTERNAL_SYNC_ARCHITECTURE.md) for a comprehensive technical reference covering the full sync system.

## File Structure

| File | Purpose |
|------|---------|
| `DrumSynth.ino` | Main firmware — sequencer, UI, knob handling, display |
| `audiotool.h` | Audio graph (Teensy Audio Design Tool export) |
| `audio_init.h` | Mixer gains, envelope params, filter settings |
| `hw_setup.h` | Pin assignments, mux/LED/OLED hardware config |
| `ext_sync.h` | External clock sync — ISRs, glitch filter, lock-in, subdivision scheduling |
| `eeprom.h` | Pattern save/load, bass line + PPQN persistence |
| `EXTERNAL_SYNC_ARCHITECTURE.md` | Self-contained technical reference for external sync review |
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
arduino-cli compile --fqbn teensy:avr:teensy41 --build-property "build.flags.optimize=-O2" .
arduino-cli upload --fqbn teensy:avr:teensy41 -p /dev/ttyACM0 .
```
