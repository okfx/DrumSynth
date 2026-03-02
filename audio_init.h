#ifndef AUDIO_INIT_H
#define AUDIO_INIT_H
// ============================================================================
//  Audio Init — audio_init.h
//
//  One-time initialization of mixer gains, envelope parameters, filter
//  settings, and delay tap times. Called from setup() after Audio.begin().
//
//  Include AFTER: audiotool.h (AudioStream objects)
// ============================================================================

// Globals coming from the main file that we use for initial mixer volumes
extern float d1Vol;
extern float d2Vol;
extern float d3Vol;

inline void audioInit() {

  // ============================================================================
  // AUDIT CHANGES (search "AUDIT" to find/revert if sound changes unexpectedly):
  //   - A1: Added clap1AmpEnv.sustain(0.0f) and clap2AmpEnv.sustain(0.0f)
  //         (Teensy default was 1.0 — envelopes never decayed to silence on their own)
  //   - A2: Added d1PitchEnv.attack(0.0f)
  //         (Teensy default was ~10.5ms — kick pitch sweep was ramping instead of snapping)
  // ============================================================================

  // ============================================================================
  // SGTL5000 CODEC SETUP
  // ============================================================================

  sgtl5000_1.disable();
  sgtl5000_1.enable();

  // Analog output staging
  sgtl5000_1.volume(0.75f);     // Headphone amp gain (0.8 ~= max clean)
  sgtl5000_1.lineOutLevel(29);  // Line out voltage swing (Teensy default)

  // Enable DAP (required for EQ / AVC on output)
  sgtl5000_1.audioPostProcessorEnable();

  // Digital trim before DAC (useful if you ever need extra headroom)
  sgtl5000_1.dacVolumeRamp();  // Smooth dacVolume changes
  sgtl5000_1.dacVolume(1.0f);  // Unity gain

  // 5-band graphic EQ: 115Hz, 330Hz, 990Hz, 3kHz, 9.9kHz
  // Range: 1.00 ~= +12 dB, -1.00 ~= -11.75 dB
  sgtl5000_1.eqBands(
    0.25f,   // 115 Hz: slight bass lift
    -0.20f,  // 330 Hz: mud cut
    -0.10f,  // 990 Hz: mild box cut
    0.05f,   // 3 kHz: attack / presence
    0.05f    // 9.9 kHz: sparkle
  );

  // Auto Volume Control used as a safety limiter (no makeup gain)
  sgtl5000_1.autoVolumeControl(
    0,      // maxGain: 0 dB
    2,      // response: 50 ms integration
    1,      // hardLimit: limiter mode
    -6.0f,  // threshold: dBFS
    80.0f,  // attack: dB/s
    300.0f  // decay: dB/s
  );
  sgtl5000_1.autoVolumeEnable();

  // ============================================================================
  // D1 - KICK DRUM
  // ============================================================================

  // D1 oscillators
  d1.begin(WAVEFORM_SINE);
  d1.amplitude(0.85f);
  d1.frequencyModulation(4);

  d1b.begin(WAVEFORM_SAWTOOTH);
  d1b.amplitude(0.85f);
  d1b.frequencyModulation(4);

  d1c.begin(WAVEFORM_SQUARE);
  d1c.amplitude(0.85f);
  d1c.frequencyModulation(4);

  // D1 oscillator mixer (ch0=sine, ch1=saw, ch2=square; ch3 unused)
  d1OscMixer.gain(0, 0.9f);
  d1OscMixer.gain(1, 0.0f);
  d1OscMixer.gain(2, 0.0f);

  // D1 pitch envelope and modulation
  d1DC.amplitude(0.25f);
  d1DCwf.amplitude(0.0f);  // wavefolder drive off at boot — set by knob at runtime
  d1PitchEnv.attack(0.0f);   // AUDIT A2: instant onset (Teensy default was ~10.5ms)
  d1PitchEnv.decay(25.0f);
  d1PitchEnv.sustain(0.0f);

  // D1 amplitude envelope
  d1AmpEnv.decay(100.0f);
  d1AmpEnv.sustain(0.0f);

  // D1 drum voice (transient layer)
  drum1.frequency(150.0f);
  drum1.length(15.0f);
  drum1.pitchMod(1.0f);
  drum1Amp.gain(2.0f);

  // D1 voice mixer (dry oscillators + drum transient + wavefolder)
  d1Mixer.gain(0, 0.3f);   // d1LowPass (dry osc)
  d1Mixer.gain(1, 0.9f);   // drum1Amp (transient)
  // input 2: unconnected
  d1Mixer.gain(3, 0.25f);  // d1Wavefolder (wet)

  // D1 filters
  d1LowPass.frequency(3000.0f);
  d1LowPass.resonance(2.0f);

  d1Filter.frequency(85.0f);
  d1Filter.resonance(2.0f);

  // D1 EQ — pass-through by default; set at runtime by knob 6 (D1 Body)

  // D1 output amp
  d1Amp.gain(0.8f);

  // ============================================================================
  // D2 - SNARE / CLAP
  // ============================================================================

  // D2 main oscillators
  d2.begin(WAVEFORM_SINE);
  d2.amplitude(0.75f);
  d2.frequency(200.0f);
  d2.frequencyModulation(1);

  // D2 amplitude envelope
  d2AmpEnv.decay(75.0f);
  d2AmpEnv.hold(20.0f);
  d2AmpEnv.sustain(0.0f);

  // D2 attack transient
  d2Attack.frequency(1000.0f);
  d2Attack.length(15.0f);
  d2Attack.pitchMod(0.6f);

  d2AttackFilter.frequency(2000.0f);
  d2AttackFilter.resonance(4.0f);

  // D2 noise layer
  d2Noise.amplitude(0.75f);

  d2NoiseEnvelope.attack(10.0f);
  d2NoiseEnvelope.decay(100.0f);
  d2NoiseEnvelope.sustain(0.0f);

  d2NoiseFilter.frequency(5000.0f);
  d2NoiseFilter.resonance(2.0f);

  // D2 drum transient (snare body click)
  drum2.frequency(200.0f);
  drum2.length(15.0f);
  drum2.pitchMod(0.5f);

  // D2 voice mixer (oscillator + drum transient + noise + attack)
  d2Mixer.gain(0, 0.33f);
  d2Mixer.gain(1, 0.1f);
  d2Mixer.gain(2, 0.1f);
  d2Mixer.gain(3, 0.2f);

  // D2 main filter
  d2Filter.frequency(400.0f);
  d2Filter.resonance(1.25f);

  // D2 wavefolder
  d2WfSine.amplitude(1.0f);
  d2WfSine.frequency(20.0f);

  d2WfAmp.gain(1.0f);

  d2WfLowpass.frequency(3500.0f);
  d2WfLowpass.resonance(2.0f);

  // D2 reverb
  d2Verb.damping(1.0f);
  d2Verb.roomsize(0.3f);

  // --- Clap effects ---

  // Clap noise sources
  clapNoise1.amplitude(0.75f);
  clapNoise2.amplitude(0.75f);

  // Clap filters
  clap1Filter.frequency(1100.0f);
  clap1Filter.resonance(1.8f);

  clap2Filter.frequency(900.0f);
  clap2Filter.resonance(1.8f);

  // Clap envelopes
  clap1AmpEnv.attack(0.5f);
  clap1AmpEnv.hold(4.0f);
  clap1AmpEnv.decay(20.0f);
  clap1AmpEnv.sustain(0.0f);  // AUDIT A1: decay to silence (Teensy default was 1.0)

  clap2AmpEnv.attack(0.5f);
  clap2AmpEnv.hold(5.0f);
  clap2AmpEnv.decay(24.0f);
  clap2AmpEnv.sustain(0.0f);  // AUDIT A1: decay to silence (Teensy default was 1.0)

  // Clap delay lines
  clapDelay1.delay(0, 0);
  clapDelay1.delay(1, 18);
  clapDelay1.delay(2, 36);
  clapDelay1.delay(3, 62);

  clapDelay2.delay(0, 7);
  clapDelay2.delay(1, 25);
  clapDelay2.delay(2, 48);
  clapDelay2.delay(3, 70);

  // Clap delay mixers (tapering gains — first hits louder)
  clapMixer1.gain(0, 0.30f);
  clapMixer1.gain(1, 0.25f);
  clapMixer1.gain(2, 0.20f);
  clapMixer1.gain(3, 0.15f);

  clapMixer2.gain(0, 0.25f);
  clapMixer2.gain(1, 0.20f);
  clapMixer2.gain(2, 0.15f);
  clapMixer2.gain(3, 0.10f);

  clapMixerMaster.gain(0, 1.0f);
  clapMixerMaster.gain(1, 1.0f);

  // Clap master filter and envelope
  clapMasterFilter.frequency(1250.0f);
  clapMasterFilter.resonance(1.8f);

  clapMasterEnv.attack(0.5f);
  clapMasterEnv.hold(1.0f);
  clapMasterEnv.decay(200.0f);
  clapMasterEnv.sustain(0.0f);

  clapAmp.gain(1.5f);

  // --- D2 output mixing ---

  // Snare / clap mixer (feeds D2 FX and delay)
  // in0: snare body (from d2Filter)
  // in1: clap envelope (from clapMasterEnv)
  // in2 / in3: spare or extra tone if you want later
  snareClapMixer.gain(0, 0.6f);
  snareClapMixer.gain(1, 0.6f);
  snareClapMixer.gain(2, 0.0f);
  snareClapMixer.gain(3, 0.0f);

  // D2 master mixer: dry bus, verb return, wavefolder output
  // in0: snareClapMixer dry
  // in1: d2Verb
  // in2: d2Wavefolder
  // in3: unused
  d2MasterMixer.gain(0, 0.7f);  // dry
  d2MasterMixer.gain(1, 0.5f);  // verb
  d2MasterMixer.gain(2, 0.4f);  // wavefolder
  d2MasterMixer.gain(3, 0.0f);  // unused

  // ============================================================================
  // D3 - HI-HAT
  // ============================================================================

  // --- D3 "606" voice: 6-oscillator square bank ---

  float d3606baseFreq = 350.0f;

  d3606W1.begin(0.5f, d3606baseFreq * 1.00f, WAVEFORM_SQUARE);  // 350.0
  d3606W2.begin(0.5f, d3606baseFreq * 1.08f, WAVEFORM_SQUARE);  // 378.0
  d3606W3.begin(0.5f, d3606baseFreq * 1.17f, WAVEFORM_SQUARE);  // 409.5
  d3606W4.begin(0.5f, d3606baseFreq * 1.26f, WAVEFORM_SQUARE);  // 441.0
  d3606W5.begin(0.5f, d3606baseFreq * 1.36f, WAVEFORM_SQUARE);  // 476.0
  d3606W6.begin(0.5f, d3606baseFreq * 1.48f, WAVEFORM_SQUARE);  // 518.0

  d3606Mixer1.gain(0, 0.25f);
  d3606Mixer1.gain(1, 0.25f);
  d3606Mixer1.gain(2, 0.25f);
  d3606Mixer1.gain(3, 0.25f);

  d3606Mixer2.gain(0, 0.25f);  // W5
  d3606Mixer2.gain(1, 0.25f);  // W6
  d3606Mixer2.gain(2, 0.0f);   // unconnected
  d3606Mixer2.gain(3, 0.0f);   // unconnected

  d3606MasterMixer.gain(0, 1.0f);
  d3606MasterMixer.gain(1, 1.0f);

  d3606HPF.frequency(1000.0f);
  d3606HPF.resonance(1.25f);

  d3606BPF.frequency(5000.0f);
  d3606BPF.resonance(1.25f);

  d3606AmpEnv.attack(1.0f);
  d3606AmpEnv.decay(45.0f);
  d3606AmpEnv.sustain(0.0f);

  // --- D3 "FM" voice: dual carrier/modulator metallic synthesis ---
  //
  // Two carrier/modulator pairs with irrational ratios for metallic,
  // pitch-free spectra.  Carriers are spaced (500, 1050 Hz) so
  // their sum/difference tones never coincide.  Modulator ratios of
  // √5 ≈ 2.236 and √2 ≈ 1.414 guarantee maximally inharmonic sidebands.
  // High modulation depth (0.65/0.55) fills the spectrum with dense
  // metallic partials — two aggressive pairs rival six mild ones.

  const float carrier1Freq = 500.0f;
  const float carrier2Freq = 1050.0f;
  const float ratio1       = 2.236f;   // √5 — maximally inharmonic
  const float ratio2       = 1.414f;   // √2 — classic metallic ratio
  const float mod1Freq     = carrier1Freq * ratio1;  // ≈ 1118 Hz
  const float mod2Freq     = carrier2Freq * ratio2;  // ≈ 1484.7 Hz

  d3W1.begin(0.30f, carrier1Freq, WAVEFORM_SINE);  // carrier 1
  d3W1.frequencyModulation(8);  // wide FM bandwidth for dense sidebands

  d3W3.begin(0.30f, carrier2Freq, WAVEFORM_SINE);  // carrier 2
  d3W3.frequencyModulation(8);

  d3W2.begin(0.65f, mod1Freq, WAVEFORM_SINE);  // modulator 1 — aggressive depth
  d3W4.begin(0.55f, mod2Freq, WAVEFORM_SINE);  // modulator 2

  d3Mixer1.gain(0, 0.75f);  // FM carrier 1
  d3Mixer1.gain(1, 0.75f);  // FM carrier 2
  d3Mixer1.gain(2, 0.0f);
  d3Mixer1.gain(3, 0.0f);

  d3MasterMixer.gain(0, 1.0f);  // FM voice at full into filter chain
  d3MasterMixer.gain(1, 0.0f);  // channel 1: unused (no patch cord connected)

  // FM voice shaping filters (d3MasterMixer → d3BPF → d3Filter → d3AmpEnv)
  d3BPF.frequency(4000.0f);     // bandpass: tracks pitch knob (4000–8000 Hz)
  d3BPF.resonance(0.9f);

  d3Filter.frequency(1500.0f);  // highpass: tracks pitch knob (1500–3000 Hz)
  d3Filter.resonance(0.7f);

  d3AmpEnv.attack(1.0f);
  d3AmpEnv.decay(45.0f);
  d3AmpEnv.sustain(0.0f);

  // --- D3 "PERC" voice: noise-based percussion ---

  drum3.pitchMod(0.5f);
  drum3.frequency(700.0f);
  drum3.length(15.0f);

  // --- D3 output mixing ---

  d3WfSine.begin(0.0f, 400.0f, WAVEFORM_SINE);  // fold-depth modulator — off at boot, knob 19 sets amplitude + freq

  // D3 wavefolder mixer (606 + FM + PERC)
  d3WfMixer.gain(0, 0.25f);
  d3WfMixer.gain(1, 0.25f);
  d3WfMixer.gain(2, 0.25f);

  // D3 final mixer (dry + wavefolder)
  d3Mixer.gain(0, 0.45f);  // d3WfMixer (dry)
  d3Mixer.gain(1, 0.0f);   // d3Wavefolder (wet) — off at boot, knob 19 enables
  // inputs 2–3: unconnected

  // D3 master filter
  d3MasterFilter.frequency(3000.0f);
  d3MasterFilter.resonance(1.0f);

  // ============================================================================
  // MASTER EFFECTS AND ROUTING
  // ============================================================================

  // Drum bus mixer (D1, D2, D3 → master)
  drumMixer.gain(0, d1Vol);  // D1 bus
  drumMixer.gain(1, d2Vol);  // D2 bus (from d2MasterMixer)
  drumMixer.gain(2, d3Vol);  // D3 bus
  drumMixer.gain(3, 0.0f);   // unused with new D2 routing

  // Master wavefolder oscillators (wfSine + wfSaw → wfMixer → masterWf input 1)
  wfSine.begin(WAVEFORM_SINE);
  wfSine.amplitude(1.0f);
  wfSine.frequency(80.0f);

  wfSaw.begin(WAVEFORM_SAWTOOTH);
  wfSaw.amplitude(1.0f);
  wfSaw.frequency(80.0f);

  wfMixer.gain(0, 0.0f);          // sine channel — knob 30 controls mix
  wfMixer.gain(1, 0.0f);          // saw channel
  wfMixer.gain(2, 0.0f);
  wfMixer.gain(3, 0.0f);

  // Final output amplifier — 3× makeup gain compensates for master filter chain attenuation
  finalAmp.gain(3.0f);

  // Master mixer (dry drums + wavefolder + delay return)
  masterMixer.gain(0, 1.0f);  // dry drums
  masterMixer.gain(1, 1.0f);  // master wavefolder
  masterMixer.gain(2, 1.0f);  // delay return
  masterMixer.gain(3, 0.0f);  // unconnected

  // Master delay
  masterDelay.delay(0, 0);

  delayFilter.frequency(4000.0f);
  delayFilter.resonance(2.5f);

  delayAmp.gain(1.5f);

  // Delay mixer and feedback
  delayMixer.gain(0, 0.0f);  // D1 send
  delayMixer.gain(1, 0.0f);  // D2 send from snareClapMixer
  delayMixer.gain(2, 0.0f);  // D3 send
  delayMixer.gain(3, 0.0f);  // feedback tap

  // Master filters
  masterHiPass.resonance(1.5f);
  masterHiPass.frequency(60.0f);

  masterLowPass.resonance(0.25f);
  masterLowPass.frequency(7500.0f);

  masterBandPass.frequency(1000.0f);  // 1kHz bandpass — intentional coloring of master output
  masterBandPass.resonance(1.0f);

  finalFilter.setHighShelf(0, 3500.0f, 0.5f, 5.0f);  // +5dB air/presence above 3.5kHz
}

#endif