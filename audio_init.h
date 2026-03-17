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
extern float d1Volume;
extern float d2Volume;
extern float d3Volume;

void audioInit() {

  // ============================================================================
  // SGTL5000 CODEC SETUP
  // ============================================================================

  sgtl5000_1.disable();
  sgtl5000_1.enable();

  // Analog output staging
  sgtl5000_1.volume(0.80f);     // Headphone amp gain
  sgtl5000_1.lineOutLevel(13);  // Line out max voltage swing (3.16 Vpp)

  // Enable DAP (required for EQ / AVC on output)
  sgtl5000_1.audioPostProcessorEnable();

  // Digital trim before DAC
  sgtl5000_1.dacVolumeRamp();   // Smooth dacVolume changes
  sgtl5000_1.dacVolume(0.95f);  // Slight headroom

  // 5-band graphic EQ: 115Hz, 330Hz, 990Hz, 3kHz, 9.9kHz
  sgtl5000_1.eqBands(0.50f, -0.08f, -0.05f, 0.05f, 0.03f);

  // Auto Volume Control — fast transparent safety limiter
  sgtl5000_1.autoVolumeControl(0, 1, 1, -3.0f, 150.0f, 500.0f);
  sgtl5000_1.autoVolumeEnable();

  // ============================================================================
  // D1 - KICK DRUM
  // ============================================================================

  // D1 oscillators
  d1OscSine.begin(WAVEFORM_SINE);
  d1OscSine.amplitude(0.85f);
  d1OscSine.frequencyModulation(4);

  d1OscSaw.begin(WAVEFORM_BANDLIMIT_SAWTOOTH);
  d1OscSaw.amplitude(0.85f);
  d1OscSaw.frequencyModulation(4);

  d1OscSquare.begin(WAVEFORM_BANDLIMIT_SQUARE);
  d1OscSquare.amplitude(0.85f);
  d1OscSquare.frequencyModulation(4);

  // D1 oscillator mixer (ch0=sine, ch1=saw, ch2=square; ch3 unused)
  d1OscMixer.gain(0, 0.9f);
  d1OscMixer.gain(1, 0.0f);
  d1OscMixer.gain(2, 0.0f);

  // D1 pitch envelope and modulation
  d1PitchDC.amplitude(0.25f);
  d1WfDrive.amplitude(0.0f);  // wavefolder drive off at boot — set by knob at runtime
  d1PitchEnv.attack(0.0f);   // Instant onset — pitch sweep snaps, no ramp
  d1PitchEnv.hold(2.5f);     // Library default — peak pitch sustains briefly before decay
  d1PitchEnv.decay(25.0f);
  d1PitchEnv.sustain(0.0f);

  // D1 amplitude envelope
  d1AmpEnv.attack(D1_ATTACK_MS);  // defined in DrumSynth.ino before this include
  d1AmpEnv.hold(2.5f);       // Library default — overridden per-trigger by cached params (see updateD1HoldCache)
  d1AmpEnv.decay(100.0f);
  d1AmpEnv.sustain(0.0f);
  d1AmpEnv.release(5.0f);    // Explicit — matches exitMonoBassMode() restore value

  // D1 drum voice (transient layer)
  d1Snap.frequency(150.0f);
  d1Snap.length(15.0f);
  d1Snap.pitchMod(1.0f);
  d1SnapAmp.gain(2.0f);

  // D1 voice mixer (dry oscillators + drum transient + wavefolder)
  d1VoiceMixer.gain(0, 0.3f);   // d1LowPass (dry osc)
  d1VoiceMixer.gain(1, 0.9f);   // d1SnapAmp (transient)
  d1VoiceMixer.gain(2, 0.0f);   // unconnected
  d1VoiceMixer.gain(3, 0.0f);   // d1Wavefolder (wet) — off at boot, knob 0 enables

  // D1 filters
  d1LowPass.frequency(1800.0f);
  d1LowPass.resonance(1.5f);

  d1HighPass.frequency(85.0f);
  d1HighPass.resonance(2.0f);

  // D1 EQ — stages 0-2 set at runtime by knob 6 (D1 Body)
  d1EQ.setLowpass(3, 8000.0f, 0.707f);  // Stage 3: permanent hiss filter

  // D1 output amp
  d1Amp.gain(0.7f);

  // ============================================================================
  // D2 - SNARE / CLAP
  // ============================================================================

  // D2 main oscillators
  d2Osc.begin(WAVEFORM_SINE);
  d2Osc.amplitude(0.75f);
  d2Osc.frequency(200.0f);
  d2Osc.frequencyModulation(1);

  // D2 amplitude envelope
  d2AmpEnv.attack(1.0f);
  d2AmpEnv.decay(93.0f);
  d2AmpEnv.hold(46.5f);  // decay * 0.5
  d2AmpEnv.sustain(0.0f);

  // D2 attack transient
  d2ClickTransient.frequency(1000.0f);
  d2ClickTransient.length(15.0f);
  d2ClickTransient.pitchMod(0.6f);

  d2ClickFilter.frequency(2000.0f);
  d2ClickFilter.resonance(1.5f);

  // D2 noise layer
  d2Noise.amplitude(0.75f);

  d2NoiseEnv.attack(0.1f);
  d2NoiseEnv.hold(7.0f);
  d2NoiseEnv.decay(70.0f);
  d2NoiseEnv.sustain(0.0f);

  d2NoiseFilter.frequency(5000.0f);
  d2NoiseFilter.resonance(0.707f);

  // D2 body transient
  d2Body.frequency(200.0f);
  d2Body.length(15.0f);
  d2Body.pitchMod(0.5f);

  // D2 voice mixer (oscillator + drum transient + noise + attack)
  d2VoiceMixer.gain(0, 0.33f);
  d2VoiceMixer.gain(1, 0.1f);
  d2VoiceMixer.gain(2, 0.1f);
  d2VoiceMixer.gain(3, 0.09f);

  // D2 main filter
  d2VoiceHighPass.frequency(400.0f);
  d2VoiceHighPass.resonance(1.25f);

  // D2 wavefolder
  d2WfSineOsc.amplitude(1.0f);
  d2WfSineOsc.frequency(20.0f);

  d2WfAmp.gain(1.0f);

  d2WfLowPass.frequency(3500.0f);
  d2WfLowPass.resonance(2.0f);

  // D2 reverb
  d2Reverb.damping(1.0f);
  d2Reverb.roomsize(0.3f);

  // --- Clap effects (808-style) ---

  // Clap noise sources
  clapNoise1.amplitude(0.8f);
  clapNoise2.amplitude(0.6f);   // secondary layer quieter

  // Clap bandpass filters — shape noise before delay taps
  // Two different frequencies reduce comb filtering between layers
  clapFilter1.frequency(1150.0f);  // body path
  clapFilter1.resonance(1.8f);

  clapFilter2.frequency(1350.0f);  // brighter path
  clapFilter2.resonance(1.5f);

  // Clap per-burst envelopes — sharp sawtooth spikes, no hold
  clapAmpEnv1.attack(0.1f);
  clapAmpEnv1.hold(0.0f);
  clapAmpEnv1.decay(12.0f);
  clapAmpEnv1.sustain(0.0f);

  clapAmpEnv2.attack(0.1f);
  clapAmpEnv2.hold(0.0f);
  clapAmpEnv2.decay(14.0f);
  clapAmpEnv2.sustain(0.0f);

  // Clap delay lines — tight 808-style spacing (~10ms apart)
  // Two layers interleaved to create dense cluster without overlap
  clapDelay1.delay(0, 0);     // hit 1
  clapDelay1.delay(1, 11);    // hit 2
  clapDelay1.delay(2, 22);    // hit 3
  clapDelay1.delay(3, 35);    // hit 4 (slightly stretched)

  clapDelay2.delay(0, 5);     // fills gap between path 1 hits
  clapDelay2.delay(1, 16);
  clapDelay2.delay(2, 28);
  clapDelay2.delay(3, 42);

  // Clap delay mixers — tapering gains per burst
  clapMixer1.gain(0, 0.35f);
  clapMixer1.gain(1, 0.25f);
  clapMixer1.gain(2, 0.15f);
  clapMixer1.gain(3, 0.08f);

  clapMixer2.gain(0, 0.20f);
  clapMixer2.gain(1, 0.15f);
  clapMixer2.gain(2, 0.10f);
  clapMixer2.gain(3, 0.05f);

  clapBusMixer.gain(0, 1.0f);
  clapBusMixer.gain(1, 0.7f);   // secondary layer lower in mix

  // Clap master filter (highpass output) — removes rumble, low resonance
  clapMasterFilter.frequency(300.0f);
  clapMasterFilter.resonance(0.7f);

  // Clap master envelope — overall tail shape
  clapMasterEnv.attack(0.1f);
  clapMasterEnv.hold(0.5f);
  clapMasterEnv.decay(150.0f);
  clapMasterEnv.sustain(0.0f);

  clapAmp.gain(0.8f);

  // --- D2 output mixing ---

  // Snare / clap mixer (feeds D2 FX and delay)
  // in0: snare body (from d2VoiceHighPass)
  // in1: clap envelope (from clapMasterEnv)
  // in2 / in3: unconnected
  snareClapMixer.gain(0, 0.8f);
  snareClapMixer.gain(1, 0.8f);
  snareClapMixer.gain(2, 0.0f);
  snareClapMixer.gain(3, 0.0f);

  // D2 master mixer: dry bus, verb return, wavefolder output
  // in0: snareClapMixer dry
  // in1: d2Reverb
  // in2: d2Wavefolder
  // in3: unused
  d2MasterMixer.gain(0, 0.8f);  // dry
  d2MasterMixer.gain(1, 0.5f);  // verb
  d2MasterMixer.gain(2, 0.4f);  // wavefolder
  d2MasterMixer.gain(3, 0.0f);  // unused

  // ============================================================================
  // D3 - HI-HAT
  // ============================================================================

  // --- D3 "606" voice: 6-oscillator square bank ---

  static constexpr float d3606baseFreq = 350.0f;

  d3606Osc1.begin(0.5f, d3606baseFreq * 1.00f, WAVEFORM_BANDLIMIT_SQUARE);  // 350.0
  d3606Osc2.begin(0.5f, d3606baseFreq * 1.08f, WAVEFORM_BANDLIMIT_SQUARE);  // 378.0
  d3606Osc3.begin(0.5f, d3606baseFreq * 1.17f, WAVEFORM_BANDLIMIT_SQUARE);  // 409.5
  d3606Osc4.begin(0.5f, d3606baseFreq * 1.26f, WAVEFORM_BANDLIMIT_SQUARE);  // 441.0
  d3606Osc5.begin(0.5f, d3606baseFreq * 1.36f, WAVEFORM_BANDLIMIT_SQUARE);  // 476.0
  d3606Osc6.begin(0.5f, d3606baseFreq * 1.48f, WAVEFORM_BANDLIMIT_SQUARE);  // 518.0

  d3606OscMixer1.gain(0, 0.25f);
  d3606OscMixer1.gain(1, 0.25f);
  d3606OscMixer1.gain(2, 0.25f);
  d3606OscMixer1.gain(3, 0.25f);

  d3606OscMixer2.gain(0, 0.25f);  // Osc5
  d3606OscMixer2.gain(1, 0.25f);  // Osc6
  d3606OscMixer2.gain(2, 0.0f);   // unconnected
  d3606OscMixer2.gain(3, 0.0f);   // unconnected

  d3606BusMixer.gain(0, 1.0f);
  d3606BusMixer.gain(1, 1.0f);

  d3606HighPass.frequency(1000.0f);
  d3606HighPass.resonance(1.25f);

  d3606BandPass.frequency(5000.0f);
  d3606BandPass.resonance(1.25f);

  d3606AmpEnv.attack(0.5f);
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

  static constexpr float carrier1Freq = 500.0f;
  static constexpr float carrier2Freq = 1050.0f;
  static constexpr float ratio1       = 2.236f;   // √5 — maximally inharmonic
  static constexpr float ratio2       = 1.414f;   // √2 — classic metallic ratio
  static constexpr float mod1Freq     = carrier1Freq * ratio1;  // ≈ 1118 Hz
  static constexpr float mod2Freq     = carrier2Freq * ratio2;  // ≈ 1484.7 Hz

  d3FmCarrier1.begin(0.30f, carrier1Freq, WAVEFORM_SINE);  // carrier 1
  d3FmCarrier1.frequencyModulation(8);  // wide FM bandwidth for dense sidebands

  d3FmCarrier2.begin(0.30f, carrier2Freq, WAVEFORM_SINE);  // carrier 2
  d3FmCarrier2.frequencyModulation(8);

  d3FmMod1.begin(0.65f, mod1Freq, WAVEFORM_SINE);  // modulator 1 — aggressive depth
  d3FmMod2.begin(0.55f, mod2Freq, WAVEFORM_SINE);  // modulator 2

  d3FmCarrierMixer.gain(0, 0.75f);  // FM carrier 1
  d3FmCarrierMixer.gain(1, 0.75f);  // FM carrier 2
  d3FmCarrierMixer.gain(2, 0.0f);
  d3FmCarrierMixer.gain(3, 0.0f);

  d3FmBusMixer.gain(0, 1.0f);  // FM voice at full into filter chain
  d3FmBusMixer.gain(1, 0.0f);  // channel 1: unused (no patch cord connected)

  // FM voice shaping filters (d3FmBusMixer → d3FmBandPass → d3FmHighPass → d3FmAmpEnv)
  d3FmBandPass.frequency(4000.0f);     // bandpass: tracks pitch knob (4000–8000 Hz)
  d3FmBandPass.resonance(0.9f);

  d3FmHighPass.frequency(1500.0f);  // highpass: tracks pitch knob (1500–3000 Hz)
  d3FmHighPass.resonance(0.7f);

  d3FmAmpEnv.attack(0.5f);
  d3FmAmpEnv.decay(45.0f);
  d3FmAmpEnv.sustain(0.0f);

  // --- D3 "PERC" voice: noise-based percussion ---

  d3Perc.pitchMod(0.5f);
  d3Perc.frequency(440.0f);   // A4 — matches pitch knob minimum
  d3Perc.length(15.0f);

  // Perc dedicated low-pass filter (AudioFilterStateVariable, lowpass output)
  d3PercFilter.frequency(6000.0f);   // open at boot — knob overrides from EEPROM
  d3PercFilter.resonance(1.2f);      // slight bump for definition

  // --- D3 output mixing ---

  d3WfOsc.begin(0.0f, 400.0f, WAVEFORM_SINE);  // fold-depth modulator — off at boot, knob 19 sets amplitude + freq

  // D3 voice mixer (606 + FM + PERC) — feeds both dry path and wavefolder
  d3VoiceMixer.gain(0, 0.18f);  // 606 voice
  d3VoiceMixer.gain(1, 0.25f);  // FM voice
  d3VoiceMixer.gain(2, 0.10f);  // PERC voice

  // D3 final mixer (dry + wavefolder)
  d3MasterMixer.gain(0, 0.70f);  // d3VoiceMixer (dry) — parity with D1 (0.7) and D2 (0.7)
  d3MasterMixer.gain(1, 0.0f);   // d3Wavefolder (wet) — off at boot, knob 19 enables
  // inputs 2–3: unconnected

  // D3 master filter (1200–12000 Hz range, controlled by D3 Filter knob)
  d3MasterFilter.frequency(12000.0f);  // open at boot (knob overrides from EEPROM)
  d3MasterFilter.resonance(0.4f);

  // ============================================================================
  // MASTER EFFECTS AND ROUTING
  // ============================================================================

  // Drum bus mixer (D1, D2, D3 → master)
  drumMixer.gain(0, d1Volume);  // D1 bus
  drumMixer.gain(1, d2Volume);  // D2 bus (from d2MasterMixer)
  drumMixer.gain(2, d3Volume);  // D3 bus
  drumMixer.gain(3, 0.0f);   // unconnected

  // Master wavefolder oscillators (masterWfOscSine + masterWfOscSaw → masterWfInputMixer → masterWavefolder input 1)
  masterWfOscSine.begin(WAVEFORM_SINE);
  masterWfOscSine.amplitude(1.0f);
  masterWfOscSine.frequency(80.0f);

  masterWfOscSaw.begin(WAVEFORM_SAWTOOTH);
  masterWfOscSaw.amplitude(1.0f);
  masterWfOscSaw.frequency(80.0f);

  masterWfInputMixer.gain(0, 0.0f);          // sine channel — knob 30 controls mix
  masterWfInputMixer.gain(1, 0.0f);          // saw channel
  masterWfInputMixer.gain(2, 0.0f);          // kick envelope
  masterWfInputMixer.gain(3, 0.0f);          // snare/clap envelope

  // Final output amplifier
  finalAmp.gain(6.0f);
  usbTrim.gain(1.0f);

  // Master mixer (dry drums + wavefolder + delay return)
  masterMixer.gain(0, 1.0f);  // dry drums
  masterMixer.gain(1, 0.0f);  // master wavefolder — off at boot, case 30 enables
  masterMixer.gain(2, 0.0f);  // delay return — off at boot, case 31 enables
  masterMixer.gain(3, 0.0f);  // unconnected

  // Master delay
  masterDelay.delay(0, 0);

  delayFilter.frequency(4000.0f);
  delayFilter.resonance(2.5f);

  delayAmp.gain(0.73f);  // matches PEAK_LEVEL in delay amount knob handler (case 31)

  // Delay mixer and feedback
  delaySendMixer.gain(0, 0.0f);  // D1 send
  delaySendMixer.gain(1, 0.0f);  // D2 send from snareClapMixer
  delaySendMixer.gain(2, 0.0f);  // D3 send
  delaySendMixer.gain(3, 0.0f);  // feedback tap

  // Master filters
  masterHighPass.frequency(30.0f);
  masterHighPass.resonance(0.707f);

  masterLowPass.frequency(14000.0f);  // above knob range (1000–7500); initKnobsFromHardware() corrects at boot
  masterLowPass.resonance(0.3f);

  masterBandPass.frequency(1000.0f);
  masterBandPass.resonance(0.01f);   // effectively bypassed — transparent

  // Master EQ
  finalFilter.setLowShelf(0, 150.0f, 0.6f, 3.5f);    // +3.5dB warmth below 150Hz
  finalFilter.setNotch(1, 400.0f, 1.0f);              // gentle cut around 400Hz
  finalFilter.setNotch(2, 2500.0f, 2.0f);             // mild cut around 2.5kHz
  finalFilter.setHighShelf(3, 4500.0f, 0.7f, 2.5f);   // +2.5dB air above 4.5kHz
}

#endif