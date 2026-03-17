# DrumSynth

A 3-voice drum synthesizer with delay line and effects, built on Teensy 4.0. Designed for hands-on use -- capable of classic drum machine sounds, but with wide-open parameter ranges that push into expressive, experimental territory. Firmware v1.0.0.

## Features

**Three drum voices** with deep per-voice synthesis control:
- **D1 Kick** -- Kick with selectable wave shape, wavefolder distortion, and tone shaping
- **D2 Snare/Clap** -- Snare body with tunable pitch, synthesized clap sound, wavefolder distortion, and reverb
- **D3 Hats/Perc** -- Three models crossfaded via mix knob: 606-style analog, FM metallic, and perc (click-to-tone depending on decay); wavefolder distortion; accent pattern selection

**Master Wavefolder** -- Sine and saw oscillators through a waveshaping stage. Frequency sweeps exponentially across four octaves, fold drive has loudness compensation, and the two oscillators diverge across three zones for evolving harmonic content.

**Choke Knob** -- Global envelope tightening/loosening across all voices. Negative values shorten decays, positive values lengthen them. Each voice responds with tuned sensitivity for musical results.

**16-Step Sequencer** with per-step programming and 10 pattern save/load slots.

**Shuffle** -- TR-909-style shuffle with 6 intensity levels. Hold X and press Step 16 to cycle through shuffle amounts.

**Audio Outputs** -- 1/4" and 1/8" line out, headphone out, and USB audio.

**External Clock Sync** -- Accepts an external pulse clock with configurable PPQN, auto lock-in, and armed count-in with beat countdown.

**OLED Display** -- Real-time oscilloscope waveform view and parameter information.

## CHROMA Mode

CHROMA turns the drum machine into a step-sequenced synthesizer. Any voice can be switched from its normal drum sound to a chromatic pitch mode, where each step in the sequence plays its own programmable note. Layer multiple CHROMA voices to build basslines, melodies, and tonal textures on top of (or instead of) the drum pattern.

The master wavefolder can also be placed in CHROMA mode, snapping its frequency to semitones and turning it into a sequenced harmonic voice layered over the full mix.

CHROMA notes are saved per-pattern. Active CHROMA channels are indicated by small dots at the bottom of the OLED.

## MONOBASS Mode

MONOBASS turns the step buttons into a live monophonic keyboard for D1. Hold X for several seconds to enter the mode. The 12 step buttons become chromatic keys covering one octave; the pitch knob shifts octaves. D2 and D3 continue to play normally.

The Body knob becomes a low-pass filter and the Snap knob becomes envelope filter amount. The oscilloscope remains visible. MONOBASS state persists across power cycles.

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
| `Documentation/CHUNKED_OLED_PUSH.md` | Non-blocking SH1106 OLED page-push technique |
| `Documentation/USB_AUDIO_MAC.md` | macOS USB audio clock drift fix |

## Changes (v1.0.0)

- **Non-blocking OLED push** -- `display.display()` replaced with chunked page transfers (one page per `loop()` iteration, ~2 ms each). Eliminates the 15–25 ms blocking window that caused USB audio clicks when recording to a DAW. See [`Documentation/CHUNKED_OLED_PUSH.md`](Documentation/CHUNKED_OLED_PUSH.md) for details.
- **DSB barriers for 600 MHz SPI timing** -- ARM Data Synchronization Barrier instructions added to the software SPI bit-bang. At 600 MHz, `digitalWriteFast()` toggles in ~2 ns — below the SH1106 minimum 100 ns clock cycle. Without DSB barriers the display receives no data.
- **Master audio retuning** -- Master filters, EQ, codec settings, and output gain retuned after A/B/C evaluation:
  - Highpass lowered to 30 Hz (Butterworth) to preserve sub-bass energy
  - Lowpass opened to 12 kHz for more headroom up top
  - Bandpass effectively bypassed (Q 0.01)
  - Final EQ gentler mid cuts, stronger bass shelf (+3.5 dB), reduced top air (+2.5 dB)
  - Output gain reduced from 3.5 to 2.8 (fuller spectrum needs less makeup)
  - SGTL5000: bass EQ doubled (+6 dB at 115 Hz), mud/box cuts halved, AVC safety limiter enabled at boot
- **OLED watchdog** -- ISR-based watchdog detects frozen display (750 ms timeout) and triggers hardware re-init with automatic push restart

## Changes (v1.05)

- **MONOBASS mode** -- hold D1 for 6 seconds to enter a live mono keyboard using the step buttons as keys, with oscilloscope waveform display. D2/D3 knobs, BPM, choke, and memory/load/save are disabled during MONOBASS mode.
- MONOBASS automatically enables WF CHROMA mode on entry; restores previous WF CHROMA state on exit
- MONOBASS Snap knob repurposed as D1 attack control (0.5–100 ms); snap transient muted for cleaner bass tone. Original snap settings restored on exit.
- MONOBASS D1 filter retuned: 40–4000 Hz Moog-style LPF sweep, resonance 1.5, softened pitch tracking (0.6+0.4× ratio), floor 20 Hz / ceiling 6000 Hz with modulation
- MONOBASS idle display: knob reference grid shown for 4 seconds after mode entry, then clears to unobstructed scope view
- MONOBASS note display repositioned to sit 4 px lower for better border clearance
- All chroma indicator dots suppressed during MONOBASS mode
- **D3 perc voice** now has a dedicated low-pass filter (`d3PercFilter`, AudioFilterStateVariable) separate from the shared `d3MasterFilter`. Perc filter range 400–6000 Hz with slight resonance bump; both filters controlled by the same D3 Filter knob. D3 master filter retuned to 1200–12000 Hz (was 800–7500 Hz). Perc pitch range narrowed to A4–A6 (440–1760 Hz).
- **Codebase audit:** null check + halt on failed knob allocator (`new`), `uint32_t` promotion on `armPulseCountdown` multiplication, `extern` declaration added for `monoBassKeyEvent`, `memset` replaced with range-for on key stack init, `static constexpr` on FM synthesis constants, loop counters standardized to `int` across all files, redundant `inline` removed from 7 single-TU functions, clarifying comments added to date-parsing chain and choke knob range math
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
- EEPROM safety: `loadStateFromEEPROM()` and `saveStateToEEPROM()` cleaned up—unnecessary `noInterrupts()` wrappers removed (ISR does not access EEPROM)
- Display robustness: D1/D2/D3 decay display functions confirmed not to factor in choke knob (show base decay only)
- WF Chromatic mode state now persisted in EEPROM pattern slots (backward-compatible with V1/V2 slots)
- MONOBASS mode persists across power cycles (dedicated EEPROM address, independent of pattern slots)
- MONOBASS chroma dot display: only WF dot shown during MONOBASS (D1/D2/D3 sequencer dots suppressed)
- MONOBASS parameter display: WF Intensity/Frequency, Delay Time/Amount, Master Volume, and Master LPF now route through scope overlay instead of being suppressed
- D1 oscillator Shape rail now renders during MONOBASS mode
- MONOBASS note display moved to bottom-left corner for more scope visibility
- MONOBASS octave changes now apply immediately to held notes (live pitch update)
- MONOBASS bass filter retuning: highpass lowered to 30 Hz, lowpass opened to 4 kHz, EQ flattened to preserve sub-bass energy
- D1 CHROMA mode: highpass lowered to 30 Hz to match MONOBASS; BODY knob becomes FILTER sweep (same Moog LPF curve as MONOBASS)
- D3 wavefolder frequency range extended down to 16.352 Hz (C0) for deeper sub-bass waveshaping
- MONOBASS keyboard limited to 12 keys (buttons 13-16 dead); first 12 step LEDs stay lit as usable-key indicators
- MONOBASS note display repositioned to avoid bottom border occlusion
- Oscilloscope rewritten: triggered waveform display with rising zero-crossing lock — sine, saw, and square shapes now visually distinct and stable
- Chroma indicator dots no longer blank the oscilloscope; each dot clears only the pixels behind itself
- Chroma indicator dots hidden while parameter overlay is active (no overlap)
- Fix: D1 chroma HPF state no longer corrupted by MONOBASS entry/exit (HPF stays at 30 Hz when chroma is active)
- Fix: toggling D1 chroma OFF during MONOBASS no longer overwrites MONOBASS HPF
- Fix: pattern load now re-applies Body knob engine state on D1 chroma transitions
- Audio thread safety: `AudioNoInterrupts()` guards added to `engineMasterLowpass()` and `engineMasterDelayTime()`
- Oscilloscope module variables qualified `static` for single-TU safety
- Splash animation wavefold uses `floorf()` for well-defined int truncation
- Knob display: `displayDisabledInMonoBass()` helper replaces 16 duplicate "DISABLED FOR MONOBASS" blocks
- Knob display: `displayD1Body()` filter cutoff formula deduplicated between MONOBASS and chroma paths
- `accentModeFromKnob()` simplified from 14-case switch to sequential enum cast
- **D1 Chroma envelope filter** -- Snap knob repurposed as envelope filter depth in D1 Chroma mode (same behavior as MONOBASS). Exponential decay from peak cutoff back to body knob base position, with resonance sweep.
- **MONOBASS envelope filter** -- legato fallback now uses the same peak formula and resonance as the initial key press (was using a weaker multiplicative formula)
- MONOBASS WF Chroma toggle locked during MONOBASS mode (shows "LOCKED") to prevent user changes from being silently overwritten on exit
- `finalAmp` gain reduced from 5.0 to 3.5 to prevent DAC clipping on bright patches with high-shelf EQ boost
- Audio thread safety: `AudioNoInterrupts()` guard added to `engineD2Noise` off-branch
- D1 Body knob now responds immediately in Chroma mode when envelope filter is active but no note is decaying
- Envelope filter sweep range widened: ceiling raised from 5000 Hz to 8000 Hz for more pronounced wah character
- Envelope filter resonance peak raised from 3.5 to 4.0 at full depth (applies to both D1 Chroma and MONOBASS)
- Master lowpass raised from 7500 Hz to 9000 Hz for a slightly brighter overall mix
- Envelope filter minimum sweep time clamped to 100ms so the wah is always audible on short-decay kicks
- Envelope filter depth knob uses a square curve — sweet spot is in the mid-range, full sweep still accessible at top
- D1 Chroma snap transient reduced to 20% on chroma entry, restored to knob position on exit
- D1 chroma toggle deferred to button release so holding past 6s for MONOBASS no longer also fires the chroma toggle
- Chroma mode text overlays removed — state is already indicated by the dot indicators at the bottom of the display

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
arduino-cli compile --fqbn "teensy:avr:teensy40:usb=audio" --build-property "build.flags.optimize=-O2" .
arduino-cli upload --fqbn "teensy:avr:teensy40:usb=audio" -p /dev/ttyACM0 .
```
