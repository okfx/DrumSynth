# User Manual

A 3-voice drum synthesizer with delay line and effects. Designed for hands-on use. Capable of classic drum machine sounds, with parameter ranges that extend well into experimental, abstract territory.

The device has three drum tracks (D1, D2, D3) and one master channel, driven by a 16-step sequencer. Each channel has eight knobs arranged in a 4x2 grid. All channels share wavefolder distortion, delay send, and volume controls, along with parameters specialized to each voice.

---

## D1: Kick

A synthesized kick voice. Tuned for classic kick drum sounds by default, but capable of a broad range of tonal and textural results well beyond typical drum use.

**Frequency** sets the pitch.
**Decay** controls the length of the sound.
**Oscillator Shape** selects the waveform: sine, square, or sawtooth.
**Body** is an EQ that cuts the mids and enhances the kick-drum-like character.
**Snap** controls the transient click at the attack.
**Wavefolder Distortion** adds harmonics and grit.
**Delay Send** controls how much signal feeds the delay line (pre-distortion).

---

## D2: Clap / Snare

A synthesized snare or clap sound, with a mix knob to blend between the two.

**Snare Body Frequency** tunes the pitched component of the snare.
**Decay** sets the length of the tail.
**Voice Mix** crossfades between snare and clap.
**Snare Noise** adds white noise, like snare wires on a drum, from tight to loose.
**Reverb** adds space to D2's output.
**Wavefolder Distortion** adds harmonic saturation.
**Delay Send** controls the delay feed (pre-distortion).

---

## D3: Hats / Percussion

Three sound models crossfaded with the voice knob: an FM metallic hat, a 606-style analog hat, and a percussion voice that ranges from a short click to a tonal ping depending on decay.

**Tune** sets the pitch across all three models.
**Decay** controls the length of the sound.
**Voice** crossfades between the three models (FM, 606, Perc).
**Accent Pattern** selects from preset accent patterns that add dynamic variation to the sequence.
**Lowpass Filter** controls the high end, useful for darkening hats.
**Wavefolder Distortion** adds edge and harmonic content.
**Delay Send** controls the delay feed (pre-distortion).

---

## Master Channel

**Tempo (BPM)** sets the sequencer speed from 60 to 300 BPM across most of the knob range. Past roughly 80%, it jumps into hyperdrive (800 to 999 BPM) for textural, noise-based territory.

**Lowpass Filter** controls the high end of the master bus with a slight resonance.
**Choke** is a global decay modifier: negative values tighten all voices, positive values loosen them.

**Wavefolder Frequency** sets the frequency of the modulation applied to the wavefolder distortion, controlling grittiness and harmonic content.
**Wavefolder Intensity** sets the depth of the effect.

**Delay Time** sets the interval of the rhythmically synced delay line.
**Delay Intensity** blends feedback and wet level.
Each track's delay send is independent and pre-distortion.

---

## Sequencer

Press **D1**, **D2**, or **D3** to select which track to sequence. Toggle steps on or off with the 16 step buttons.

---

## Shuffle

Shuffle (also called swing) adds TR-909-style groove with six intensity levels. Hold **X** and press **Step 16** to cycle through shuffle amounts.

---

## Memory

The device has 10 memory slots. Hold **X** and press **Step 1 through 10** to select a slot, then use **SAVE** or **LOAD** to write or recall.

Each slot stores the drum pattern, CHROMA notes and mode state, and the shuffle setting.

---

## CHROMA Mode

CHROMA turns a drum voice into a step-sequenced synthesizer. Toggle it per-channel by holding **X** and pressing **D1**, **D2**, **D3**, or **▶** (for the master wavefolder).

When CHROMA is active, each step plays its own programmable note instead of the normal drum sound. To set the pitch of a step, hold the step button and turn that channel's Frequency/Tune knob. Layer multiple CHROMA voices to build basslines, melodies, and tonal textures alongside or instead of the drum pattern.

When **D1 CHROMA** is active, the Body knob becomes a low-pass filter and the Snap knob becomes envelope filter amount.

Active CHROMA channels are indicated by small dots at the bottom of the OLED display.

---

## MONOBASS Mode

Hold **X** for several seconds to enter MONOBASS mode.

MONOBASS turns the step buttons into a live monophonic keyboard for D1. Twelve buttons become chromatic keys spanning one octave; the Frequency knob shifts octaves. D2 and D3 continue to play their sequences normally.

As in D1 CHROMA, the Body knob becomes a low-pass filter and the Snap knob becomes envelope filter amount. The oscilloscope remains visible on the OLED.

---

## The X Button

**X** is a modifier key. Hold it in conjunction with other buttons to toggle modes, select memory slots, and change settings.

---

## Display

The OLED shows a real-time oscilloscope waveform view along with current parameter values and mode indicators.

---

## Audio Outputs

1/4" line out, 1/8" line out, headphone out, and USB audio.

---

## External Clock Sync

The device accepts an external pulse clock input with configurable PPQN. When a clock is detected, the sequencer locks to it automatically. An armed count-in with beat countdown ensures tight sync from the first beat.

---

## Quick Reference

### Control Layout

| D1 (Kick) | | D2 (Clap/Snare) | | D3 (Hats/Perc) | | Master | |
|---|---|---|---|---|---|---|---|
| Frequency | Volume | Snare Body Freq | Volume | Tune | Volume | Tempo (BPM) | Master Volume |
| Decay | Snap (*Envelope filter*) | Decay | Snare Noise | Decay | Accent Pattern | Lowpass Filter | Choke |
| Osc Shape | Body (*Resonant filter*) | Voice Mix | Reverb | Voice | Lowpass Filter | Wavefolder Freq | Wavefolder Intensity |
| Wavefolder | Delay Send | Wavefolder | Delay Send | Wavefolder | Delay Send | Delay Time | Delay Intensity |

### Button Combos

| Action | Combo |
|---|---|
| Cycle shuffle | Hold **X** + **Step 16** |
| Select memory slot | Hold **X** + **Step 1 through 10** |
| Toggle CHROMA (per voice) | Hold **X** + **D1** / **D2** / **D3** / **▶** |
| Enter MONOBASS | Hold **X** (several seconds) |
| Set CHROMA step pitch | Hold **step button** + turn **Frequency/Tune** |
