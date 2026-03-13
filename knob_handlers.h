#ifndef KNOB_HANDLERS_H
#define KNOB_HANDLERS_H
// ============================================================================
//  KNOB HANDLERS — knob_handlers.h
//
//  Table-driven dispatch for 32 knob parameters: display + engine functions.
//  Extracted from updateParameterDisplay() and applyKnobToEngine() switch blocks.
//
//  Include AFTER: audiotool.h, hw_setup.h, ext_sync.h, monobass.h,
//                 and all state declarations in DrumSynth.ino.
// ============================================================================

#include <Arduino.h>

// ============================================================================
//  KnobDescriptor — dispatch table entry
// ============================================================================

struct KnobDescriptor {
  void (*display)(uint8_t idx, int knobValue);
  void (*engine)(uint8_t idx, int knobValue);
};

// ============================================================================
//  External dependencies — defined in DrumSynth.ino / audiotool.h / ext_sync.h
// ============================================================================

// --- Audio objects (from audiotool.h) ---
extern AudioSynthWaveformDc     d1WfDrive;
extern AudioMixer4              d1VoiceMixer;
extern AudioMixer4              d1OscMixer;
extern AudioEffectEnvelope      d1AmpEnv;
extern AudioSynthSimpleDrum     d1Snap;
extern AudioFilterBiquad        d1EQ;
extern AudioFilterStateVariable d1LowPass;
extern AudioSynthWaveformModulated d2Osc;
extern AudioEffectEnvelope      d2NoiseEnv;
extern AudioFilterStateVariable d2NoiseFilter;
extern AudioMixer4              d2VoiceMixer;
extern AudioMixer4              snareClapMixer;
extern AudioAmplifier           d2WfAmp;
extern AudioSynthWaveformSine   d2WfSineOsc;
extern AudioFilterStateVariable d2WfLowPass;
extern AudioMixer4              d2MasterMixer;
extern AudioEffectFreeverb      d2Reverb;
extern AudioSynthWaveform       d3606Osc1;
extern AudioSynthWaveform       d3606Osc2;
extern AudioSynthWaveform       d3606Osc3;
extern AudioSynthWaveform       d3606Osc4;
extern AudioSynthWaveform       d3606Osc5;
extern AudioSynthWaveform       d3606Osc6;
extern AudioFilterStateVariable d3606HighPass;
extern AudioFilterStateVariable d3606BandPass;
extern AudioSynthWaveformModulated d3FmCarrier1;
extern AudioSynthWaveformModulated d3FmCarrier2;
extern AudioSynthWaveform       d3FmMod1;
extern AudioSynthWaveform       d3FmMod2;
extern AudioFilterStateVariable d3FmBandPass;
extern AudioFilterStateVariable d3FmHighPass;
extern AudioSynthSimpleDrum     d3Perc;
extern AudioSynthWaveform       d3WfOsc;
extern AudioMixer4              d3MasterMixer;
extern AudioFilterLadder        d3MasterFilter;
extern AudioMixer4              d3VoiceMixer;
extern AudioMixer4              drumMixer;
extern AudioEffectDelay         masterDelay;
extern AudioSynthWaveform       masterWfOscSine;
extern AudioSynthWaveform       masterWfOscSaw;
extern AudioFilterLadder        masterLowPass;
extern AudioMixer4              masterWfInputMixer;
extern AudioMixer4              masterMixer;
extern AudioAmplifier           delayAmp;
extern AudioMixer4              delaySendMixer;
extern IntervalTimer            stepTimer;

// --- State variables ---
extern MonoBassState monoBass;
extern float d1BaseFreq;
extern float d1Volume;
extern float d2Volume;
extern float d3Volume;
extern float d1DelaySend;
extern float d2DelaySend;
extern float d3DelaySend;
extern float d1DecayBase;
extern float d2DecayBase;
extern float d2DecayNorm;
extern float d3DecayBase;
extern uint8_t d3AccentMode;
extern uint16_t d3AccentMask;
extern AccentPreview accentPreview;
extern float bpm;
extern float extBpmDisplay;
extern int chokeOffsetMs;
extern int chokeDisplayPercent;
extern float masterNominalGain;
extern bool wfChromaMode;
extern bool d1ChromaMode;
extern bool d2ChromaMode;
extern bool d3ChromaMode;
extern int8_t d1ChromaHeldStep;
extern int8_t d2ChromaHeldStep;
extern int8_t d3ChromaHeldStep;
extern uint8_t d1ChromaNote[];
extern uint8_t d2ChromaNote[];
extern uint8_t d3ChromaNote[];
extern bool ppqnModeActive;
extern bool patternDirty;
static constexpr size_t kDisplayParamSize = 24;
extern char displayParameter1[kDisplayParamSize];
extern char displayParameter2[kDisplayParamSize];
extern uint32_t parameterOverlayStartTick;
extern volatile uint32_t sysTickMs;
extern RailMode activeRail;
extern volatile TransportState transportState;
extern float uiMixD1Shape;
extern float uiMixD2Voice;
extern float uiMixD3Voice;

// --- Functions (static inline in .ino, visible in this translation unit) ---
// normalizeKnob, mapFloat, d1DecayCurve, d2DecayCurve, d1PitchCurve,
// bpmFromKnob, chokeOffsetFromKnob, delayRatioFromKnob,
// d1ChromaKnobToNote, d2ChromaKnobToNote, d3ChromaKnobToNote,
// accentModeFromKnob, accentMaskFromMode, formatChromaNote,
// midiToFreq — all static inline, no extern needed.

// Non-inline functions with external linkage
extern void applyD1Freq();
extern void applyD1Decay();
extern void applyD2Decay();
extern void applyD3Decay();
extern void applyChokeToDecays();
extern void applyMasterGain();
extern void updateDrumDelayGains();
extern void updateLEDs();

// --- Constants (static constexpr in .ino / ext_sync.h, visible in this TU) ---
// WF_DEADBAND, quantizeRatios[], ratioLabels[], accentNames[],
// STEPS_PER_BEAT — all static constexpr, no extern needed.

// ============================================================================
//  Shared display helpers
// ============================================================================

// For cases that just show "LABEL" + "XX%" — used by cases 12,13,14,15,20,23,28
static inline void displaySimplePercent(const char* label, int knobValue) {
  int percent = (int)(normalizeKnob(knobValue) * 100.0f + 0.5f);
  snprintf(displayParameter1, sizeof(displayParameter1), "%s", label);
  snprintf(displayParameter2, sizeof(displayParameter2), "%d%%", percent);
}

// For D1 cases with MONOBASS large-font branch — used by cases 0,4,5,6,7
static inline void displayMonoBassParam(const char* normalLabel, const char* bkLabel, int knobValue) {
  int percent = (int)(normalizeKnob(knobValue) * 100.0f + 0.5f);
  if (monoBass.active) {
    snprintf(monoBass.paramLabel, sizeof(monoBass.paramLabel), "%s", bkLabel);
    snprintf(monoBass.paramValue, sizeof(monoBass.paramValue), "%d%%", percent);
    monoBass.paramShowStart = sysTickMs;  // ARM 32-bit aligned read — atomic, no guard required
  } else {
    snprintf(displayParameter1, sizeof(displayParameter1), "%s", normalLabel);
    snprintf(displayParameter2, sizeof(displayParameter2), "%d%%", percent);
  }
}

// For D2/D3/master cases disabled during MONOBASS — returns true if handled
static inline bool displayDisabledInMonoBass() {
  if (!monoBass.active) return false;
  snprintf(displayParameter1, sizeof(displayParameter1), "DISABLED FOR");
  snprintf(displayParameter2, sizeof(displayParameter2), "MONOBASS");
  return true;
}

// ============================================================================
//  Display functions (32 total)
// ============================================================================

// Case 0: D1 Drive/Distortion
static void displayD1Distort(uint8_t idx, int knobValue) {
  (void)idx;
  float norm = normalizeKnob(knobValue);
  float active = (norm <= WF_DEADBAND) ? 0.0f
               : (norm - WF_DEADBAND) / (1.0f - WF_DEADBAND);
  if (active > 1.0f) active = 1.0f;
  int percent = (int)(active * 100.0f + 0.5f);
  if (monoBass.active) {
    snprintf(monoBass.paramLabel, sizeof(monoBass.paramLabel), "%s", "DISTORT");
    snprintf(monoBass.paramValue, sizeof(monoBass.paramValue), "%d%%", percent);
    monoBass.paramShowStart = sysTickMs;  // ARM 32-bit aligned read — atomic, no guard required
  } else {
    snprintf(displayParameter1, sizeof(displayParameter1), "D1 DISTORTION");
    snprintf(displayParameter2, sizeof(displayParameter2), "%d%%", percent);
  }
}

// Case 1: D1 Waveform Shape (uses rail display)
static void displayD1Shape(uint8_t idx, int knobValue) {
  (void)idx;
  displayParameter1[0] = 0;
  displayParameter2[0] = 0;
  activeRail = RAIL_D1_SHAPE;
  uiMixD1Shape = normalizeKnob(knobValue);
  if (monoBass.active) parameterOverlayStartTick = sysTickMs;
}

// Case 2: D1 Decay
static void displayD1Decay(uint8_t idx, int knobValue) {
  (void)idx;
  if (monoBass.active) {
    float norm = normalizeKnob(knobValue);
    float releaseMs = 3.0f + norm * 997.0f;
    snprintf(monoBass.paramLabel, sizeof(monoBass.paramLabel), "%s", "RELEASE");
    snprintf(monoBass.paramValue, sizeof(monoBass.paramValue), "%.0fms", releaseMs);
    monoBass.paramShowStart = sysTickMs;  // ARM 32-bit aligned read — atomic, no guard required
  } else {
    float decayMs = d1DecayCurve(knobValue);
    snprintf(displayParameter1, sizeof(displayParameter1), "D1 DECAY");
    snprintf(displayParameter2, sizeof(displayParameter2), "%.0f ms", decayMs);
  }
}

// Case 3: D1 Pitch
static void displayD1Pitch(uint8_t idx, int knobValue) {
  (void)idx;
  if (monoBass.active) {
    // Show current octave in scope overlay on any knob movement —
    // lets user jiggle knob to confirm position without crossing a threshold
    monoBass.showOctave = true;
    monoBass.octaveShowStart = sysTickMs;
    return;
  }
  if (d1ChromaMode) {
    if (d1ChromaHeldStep >= 0) {
      uint8_t note = d1ChromaKnobToNote(knobValue);
      d1ChromaNote[d1ChromaHeldStep] = note;
      char noteName[8];
      formatChromaNote(note, noteName);
      snprintf(displayParameter1, sizeof(displayParameter1),
               "STEP %d", d1ChromaHeldStep + 1);
      snprintf(displayParameter2, sizeof(displayParameter2), "%s", noteName);
    } else {
      snprintf(displayParameter1, sizeof(displayParameter1), "LOCKED FOR");
      snprintf(displayParameter2, sizeof(displayParameter2), "CHROMA");
    }
    return;
  }
  float freqHz = d1PitchCurve(knobValue);
  snprintf(displayParameter1, sizeof(displayParameter1), "D1 FREQUENCY");
  snprintf(displayParameter2, sizeof(displayParameter2), "%d Hz", (int)freqHz);
}

// Case 4: D1 Volume
static void displayD1Volume(uint8_t idx, int knobValue) {
  (void)idx;
  displayMonoBassParam("D1 VOLUME", "VOLUME", knobValue);
}

// Case 5: D1 Snap
static void displayD1Snap(uint8_t idx, int knobValue) {
  (void)idx;
  displayMonoBassParam("D1 SNAP", "SNAP", knobValue);
}

// Case 6: D1 EQ/Body — becomes FILTER in MONOBASS and D1 Chroma modes
static void displayD1Body(uint8_t idx, int knobValue) {
  (void)idx;
  if (monoBass.active || d1ChromaMode) {
    // Filter mode: compute cutoff (same formula as engine, without pitch tracking)
    float norm = normalizeKnob(knobValue);
    float cutoff = 80.0f * expf(norm * 4.317f);
    if (cutoff > 6000.0f) cutoff = 6000.0f;
    if (cutoff < 100.0f)  cutoff = 100.0f;   // match engine floor
    if (monoBass.active) {
      snprintf(monoBass.paramLabel, sizeof(monoBass.paramLabel), "FILTER");
      snprintf(monoBass.paramValue, sizeof(monoBass.paramValue), "%d Hz", (int)cutoff);
      monoBass.paramShowStart = sysTickMs;
    } else {
      snprintf(displayParameter1, sizeof(displayParameter1), "D1 FILTER");
      snprintf(displayParameter2, sizeof(displayParameter2), "%d Hz", (int)cutoff);
    }
  } else {
    snprintf(displayParameter1, sizeof(displayParameter1), "D1 BODY");
    int percent = (int)(normalizeKnob(knobValue) * 100.0f + 0.5f);
    snprintf(displayParameter2, sizeof(displayParameter2), "%d%%", percent);
  }
}

// Case 7: D1 Delay Send
static void displayD1DelaySend(uint8_t idx, int knobValue) {
  (void)idx;
  displayMonoBassParam("D1 DELAY SEND", "DELAY", knobValue);
}

// Case 8: D2 Pitch
static void displayD2Pitch(uint8_t idx, int knobValue) {
  (void)idx;
  if (displayDisabledInMonoBass()) return;
  if (d2ChromaMode) {
    if (d2ChromaHeldStep >= 0) {
      uint8_t note = d2ChromaKnobToNote(knobValue);
      char noteName[8];
      formatChromaNote(note, noteName);
      snprintf(displayParameter1, sizeof(displayParameter1),
               "STEP %d", d2ChromaHeldStep + 1);
      snprintf(displayParameter2, sizeof(displayParameter2), "%s", noteName);
    } else {
      snprintf(displayParameter1, sizeof(displayParameter1), "LOCKED FOR");
      snprintf(displayParameter2, sizeof(displayParameter2), "CHROMA");
    }
    return;
  }
  float freqHz = mapFloat(knobValue, 0, 1023, 110.0f, 440.0f);  // A2-A4
  snprintf(displayParameter1, sizeof(displayParameter1), "D2 SNARE FREQ");
  snprintf(displayParameter2, sizeof(displayParameter2), "%d Hz", (int)freqHz);
}

// Case 9: D2 Decay
static void displayD2Decay(uint8_t idx, int knobValue) {
  (void)idx;
  if (displayDisabledInMonoBass()) return;
  float decayMs = d2DecayCurve(knobValue);
  snprintf(displayParameter1, sizeof(displayParameter1), "D2 DECAY");
  snprintf(displayParameter2, sizeof(displayParameter2), "%.0f ms", decayMs);
}

// Case 10: D2 Voice Mix (uses rail display)
static void displayD2VoiceMix(uint8_t idx, int knobValue) {
  (void)idx;
  if (displayDisabledInMonoBass()) return;
  displayParameter1[0] = 0;
  displayParameter2[0] = 0;
  activeRail = RAIL_D2_VOICE;
  uiMixD2Voice = normalizeKnob(knobValue);
}

// Case 11: D2 Wavefolder Drive
static void displayD2WfDrive(uint8_t idx, int knobValue) {
  (void)idx;
  if (displayDisabledInMonoBass()) return;
  float norm = normalizeKnob(knobValue);
  float active = (norm <= WF_DEADBAND) ? 0.0f
               : (norm - WF_DEADBAND) / (1.0f - WF_DEADBAND);
  if (active > 1.0f) active = 1.0f;
  int percent = (int)(active * 100.0f + 0.5f);
  snprintf(displayParameter1, sizeof(displayParameter1), "D2 DISTORTION");
  snprintf(displayParameter2, sizeof(displayParameter2), "%d%%", percent);
}

// Case 12: D2 Delay Send
static void displayD2DelaySend(uint8_t idx, int knobValue) {
  (void)idx;
  if (displayDisabledInMonoBass()) return;
  displaySimplePercent("D2 DELAY SEND", knobValue);
}

// Case 13: D2 Reverb
static void displayD2Reverb(uint8_t idx, int knobValue) {
  (void)idx;
  if (displayDisabledInMonoBass()) return;
  displaySimplePercent("D2 REVERB", knobValue);
}

// Case 14: D2 Noise
static void displayD2Noise(uint8_t idx, int knobValue) {
  (void)idx;
  if (displayDisabledInMonoBass()) return;
  displaySimplePercent("D2 SNARE NOISE", knobValue);
}

// Case 15: D2 Volume
static void displayD2Volume(uint8_t idx, int knobValue) {
  (void)idx;
  if (displayDisabledInMonoBass()) return;
  displaySimplePercent("D2 VOLUME", knobValue);
}

// Case 16: D3 Pitch
static void displayD3Pitch(uint8_t idx, int knobValue) {
  (void)idx;
  if (displayDisabledInMonoBass()) return;
  if (d3ChromaMode) {
    if (d3ChromaHeldStep >= 0) {
      uint8_t note = d3ChromaKnobToNote(knobValue);
      char noteName[8];
      formatChromaNote(note, noteName);
      snprintf(displayParameter1, sizeof(displayParameter1),
               "STEP %d", d3ChromaHeldStep + 1);
      snprintf(displayParameter2, sizeof(displayParameter2), "%s", noteName);
    } else {
      snprintf(displayParameter1, sizeof(displayParameter1), "LOCKED FOR");
      snprintf(displayParameter2, sizeof(displayParameter2), "CHROMA");
    }
    return;
  }
  int tune = (int)(normalizeKnob(knobValue) * 100.0f + 0.5f);
  snprintf(displayParameter1, sizeof(displayParameter1), "D3 TUNE");
  snprintf(displayParameter2, sizeof(displayParameter2), "%d%%", tune);
}

// Case 17: D3 Decay
static void displayD3Decay(uint8_t idx, int knobValue) {
  (void)idx;
  if (displayDisabledInMonoBass()) return;
  float decayMs = mapFloat(knobValue, 0, 1023, 10.0f, 250.0f);
  snprintf(displayParameter1, sizeof(displayParameter1), "D3 DECAY");
  snprintf(displayParameter2, sizeof(displayParameter2), "%.0f ms", decayMs);
}

// Case 18: D3 Voice Mix (uses rail display)
static void displayD3VoiceMix(uint8_t idx, int knobValue) {
  (void)idx;
  if (displayDisabledInMonoBass()) return;
  displayParameter1[0] = 0;
  displayParameter2[0] = 0;
  activeRail = RAIL_D3_VOICE;
  uiMixD3Voice = normalizeKnob(knobValue);
}

// Case 19: D3 Distort
static void displayD3Distort(uint8_t idx, int knobValue) {
  (void)idx;
  if (displayDisabledInMonoBass()) return;
  float norm = normalizeKnob(knobValue);
  float active = (norm <= WF_DEADBAND) ? 0.0f
               : (norm - WF_DEADBAND) / (1.0f - WF_DEADBAND);
  if (active > 1.0f) active = 1.0f;
  int percent = (int)(active * 100.0f + 0.5f);
  snprintf(displayParameter1, sizeof(displayParameter1), "D3 DISTORTION");
  snprintf(displayParameter2, sizeof(displayParameter2), "%d%%", percent);
}

// Case 20: D3 Delay Send
static void displayD3DelaySend(uint8_t idx, int knobValue) {
  (void)idx;
  if (displayDisabledInMonoBass()) return;
  displaySimplePercent("D3 DELAY SEND", knobValue);
}

// Case 21: D3 Filter
static void displayD3Filter(uint8_t idx, int knobValue) {
  (void)idx;
  if (displayDisabledInMonoBass()) return;
  float cutoffHz = d3FilterCurve(knobValue);

  snprintf(displayParameter1, sizeof(displayParameter1), "D3 LOWPASS");
  snprintf(displayParameter2, sizeof(displayParameter2), "%d Hz", (int)cutoffHz);
}

// Case 22: D3 Accent Pattern
static void displayD3Accent(uint8_t idx, int knobValue) {
  (void)idx;
  if (displayDisabledInMonoBass()) return;
  snprintf(displayParameter1, sizeof(displayParameter1), "D3 ACCENT");
  uint8_t mode = accentModeFromKnob(knobValue);
  snprintf(displayParameter2, sizeof(displayParameter2), "%s", accentNames[mode]);
}

// Case 23: D3 Volume
static void displayD3Volume(uint8_t idx, int knobValue) {
  (void)idx;
  if (displayDisabledInMonoBass()) return;
  displaySimplePercent("D3 VOLUME", knobValue);
}

// Case 24: Master Delay Time
static void displayMasterDelayTime(uint8_t idx, int knobValue) {
  (void)idx;
  float activeBpm = (extBpmDisplay > 0.0f) ? extBpmDisplay : bpm;
  int ratioIdx = delayRatioFromKnob(knobValue, activeBpm);
  if (monoBass.active) {
    snprintf(monoBass.paramLabel, sizeof(monoBass.paramLabel), "DLY TIME");
    snprintf(monoBass.paramValue, sizeof(monoBass.paramValue), "%s", ratioLabels[ratioIdx]);
    monoBass.paramShowStart = sysTickMs;
  } else {
    snprintf(displayParameter1, sizeof(displayParameter1), "DELAY TIME");
    snprintf(displayParameter2, sizeof(displayParameter2), "%s", ratioLabels[ratioIdx]);
  }
}

// Case 25: Wavefold Frequency
static void displayWfFreq(uint8_t idx, int knobValue) {
  (void)idx;
  if (wfChromaMode) {
    uint8_t midiNote = map(knobValue, 0, 1023, 36, 84);
    char noteName[8];
    formatChromaNote(midiNote, noteName);
    if (monoBass.active) {
      snprintf(monoBass.paramLabel, sizeof(monoBass.paramLabel), "WAVEFOLD");
      snprintf(monoBass.paramValue, sizeof(monoBass.paramValue), "%s", noteName);
      monoBass.paramShowStart = sysTickMs;
    } else {
      snprintf(displayParameter1, sizeof(displayParameter1), "WF %s", noteName);
      snprintf(displayParameter2, sizeof(displayParameter2), "CHROMA");
    }
  } else {
    int percent = (int)(normalizeKnob(knobValue) * 100.0f + 0.5f);
    if (monoBass.active) {
      snprintf(monoBass.paramLabel, sizeof(monoBass.paramLabel), "WF FREQ");
      snprintf(monoBass.paramValue, sizeof(monoBass.paramValue), "%d%%", percent);
      monoBass.paramShowStart = sysTickMs;
    } else {
      snprintf(displayParameter1, sizeof(displayParameter1), "WAVEFOLD FREQ");
      snprintf(displayParameter2, sizeof(displayParameter2), "%d%%", percent);
    }
  }
}

// Case 26: Master Lowpass
static void displayMasterLowpass(uint8_t idx, int knobValue) {
  (void)idx;
  int freqHz = map(knobValue, 0, 1023, 1000, 7500);
  if (monoBass.active) {
    snprintf(monoBass.paramLabel, sizeof(monoBass.paramLabel), "LOWPASS");
    snprintf(monoBass.paramValue, sizeof(monoBass.paramValue), "%d Hz", freqHz);
    monoBass.paramShowStart = sysTickMs;
  } else {
    snprintf(displayParameter1, sizeof(displayParameter1), "MASTER LOWPASS");
    snprintf(displayParameter2, sizeof(displayParameter2), "%d Hz", freqHz);
  }
}

// Case 27: Master Tempo
static void displayMasterTempo(uint8_t idx, int knobValue) {
  (void)idx;
  if (displayDisabledInMonoBass()) return;
  if (ppqnModeActive) return;  // PPQN mode uses button cycling, not knob

  if (transportState == RUN_EXT) {
    snprintf(displayParameter1, sizeof(displayParameter1), "EXT MODE");
    displayParameter2[0] = 0;
    return;
  }

  float bpmValue = bpmFromKnob(knobValue);

  snprintf(displayParameter1, sizeof(displayParameter1), "BPM");
  snprintf(displayParameter2, sizeof(displayParameter2), "%.1f", bpmValue);
}

// Case 28: Master Volume
static void displayMasterVolume(uint8_t idx, int knobValue) {
  (void)idx;
  displayMonoBassParam("MASTER VOLUME", "VOLUME", knobValue);
}

// Case 29: Master Choke
static void displayMasterChoke(uint8_t idx, int knobValue) {
  (void)idx;
  if (displayDisabledInMonoBass()) return;
  snprintf(displayParameter1, sizeof(displayParameter1), "CHOKE");
  int offset = chokeOffsetFromKnob(knobValue);
  if (offset == 0) {
    snprintf(displayParameter2, sizeof(displayParameter2), "OFF");
  } else {
    int pct = (int)((normalizeKnob(knobValue) - 0.5f) * 200.0f);
    snprintf(displayParameter2, sizeof(displayParameter2), "%+d%%", pct);
  }
}

// Case 30: Wavefold Drive
static void displayWfDrive(uint8_t idx, int knobValue) {
  (void)idx;
  float norm = normalizeKnob(knobValue);
  float active = (norm <= WF_DEADBAND) ? 0.0f
               : (norm - WF_DEADBAND) / (1.0f - WF_DEADBAND);
  if (active > 1.0f) active = 1.0f;

  // Perceived intensity follows drive^2 curve
  int percent = (int)(active * active * 100.0f + 0.5f);
  if (monoBass.active) {
    snprintf(monoBass.paramLabel, sizeof(monoBass.paramLabel), "WF DRIVE");
    snprintf(monoBass.paramValue, sizeof(monoBass.paramValue), "%d%%", percent);
    monoBass.paramShowStart = sysTickMs;
  } else {
    snprintf(displayParameter1, sizeof(displayParameter1), "WAVEFOLD DRIVE");
    snprintf(displayParameter2, sizeof(displayParameter2), "%d%%", percent);
  }
}

// Case 31: Master Delay Mix
static void displayMasterDelayMix(uint8_t idx, int knobValue) {
  (void)idx;
  float norm = normalizeKnob(knobValue);
  const char* valStr;
  char valBuf[12];
  if (norm <= 0.02f) {
    valStr = "OFF";
  } else {
    int percent = (int)(norm * 100.0f + 0.5f);
    snprintf(valBuf, sizeof(valBuf), "%d%%", percent);
    valStr = valBuf;
  }
  if (monoBass.active) {
    snprintf(monoBass.paramLabel, sizeof(monoBass.paramLabel), "DELAY AMT");
    snprintf(monoBass.paramValue, sizeof(monoBass.paramValue), "%s", valStr);
    monoBass.paramShowStart = sysTickMs;
  } else {
    snprintf(displayParameter1, sizeof(displayParameter1), "DELAY AMOUNT");
    snprintf(displayParameter2, sizeof(displayParameter2), "%s", valStr);
  }
}

// ============================================================================
//  Engine functions (32 total)
// ============================================================================

// Case 0: D1 Drive/Distortion
static void engineD1Distort(uint8_t idx, int knobValue) {
  (void)idx;
  float norm = normalizeKnob(knobValue);
  if (norm <= WF_DEADBAND) {
    AudioNoInterrupts();
    d1WfDrive.amplitude(0.0f);
    d1VoiceMixer.gain(3, 0.0f);   // wavefolder return off
    AudioInterrupts();
    return;
  }
  // Remap above deadband to 0->1 so the effect fades in smoothly
  float active = (norm - WF_DEADBAND) / (1.0f - WF_DEADBAND);
  float wavefolderAmp = 0.1f + active * 1.023f;

  AudioNoInterrupts();
  d1WfDrive.amplitude(wavefolderAmp);
  d1VoiceMixer.gain(3, active * 0.25f);  // wavefolder return ramps in
  AudioInterrupts();
}

// Case 1: D1 Waveform Shape
static void engineD1Shape(uint8_t idx, int knobValue) {
  (void)idx;
  float norm = normalizeKnob(knobValue);

  // Always keep a sine fundamental present
  const float SIN_FLOOR = 0.35f;
  const float SIN_CEIL = 0.85f;
  const float G_SAW_MAX = 0.55f;
  const float G_SQR_MAX = 0.55f;

  // Calculate voice gains
  float gainSine = SIN_CEIL - (SIN_CEIL - SIN_FLOOR) * norm;
  float gainSaw = 0.0f;
  float gainSquare = 0.0f;

  // Harmonic crossfade: sine -> saw -> square
  if (norm <= 0.50f) {
    float blend = norm / 0.50f;
    gainSaw = G_SAW_MAX * blend;
  } else {
    float blend = (norm - 0.50f) / 0.50f;
    gainSaw = G_SAW_MAX * (1.0f - blend);
    gainSquare = G_SQR_MAX * blend;
  }

  // Headroom management
  float sum = gainSine + gainSaw + gainSquare;
  float scale = 1.0f;
  const float SUM_TARGET = 0.95f;
  if (sum > SUM_TARGET) {
    scale = SUM_TARGET / sum;
  }

  AudioNoInterrupts();
  d1OscMixer.gain(0, gainSine * scale);
  d1OscMixer.gain(1, gainSaw * scale);
  d1OscMixer.gain(2, gainSquare * scale);
  AudioInterrupts();
}

// Case 2: D1 Decay (release time in MONOBASS mode)
static void engineD1Decay(uint8_t idx, int knobValue) {
  (void)idx;
  if (monoBass.active) {
    // Decay knob -> release time: 3-1000ms linear
    float norm = normalizeKnob(knobValue);
    monoBass.releaseMs = 3.0f + norm * 997.0f;
    AudioNoInterrupts();
    d1AmpEnv.release(monoBass.releaseMs);
    AudioInterrupts();
    return;
  }
  d1DecayBase = d1DecayCurve(knobValue);
  applyD1Decay();
}

// Case 3: D1 Pitch
static void engineD1Pitch(uint8_t idx, int knobValue) {
  (void)idx;
  // MONOBASS mode: pitch knob becomes stepped octave selector
  if (monoBass.active) {
    uint8_t newOct;
    if (knobValue < 341)       newOct = 0;  // OCT 2 (C2)
    else if (knobValue < 682)  newOct = 1;  // OCT 3 (C3)
    else                       newOct = 2;  // OCT 4 (C4)
    // Always show octave on any knob movement (display handles the timer)
    monoBass.showOctave = true;
    monoBass.octaveShowStart = sysTickMs;
    if (newOct != monoBass.octave) {
      monoBass.octave = newOct;
      // Live-update frequency of held note so octave changes are immediate
      int8_t top = monoBass.topKey();
      if (top >= 0) {
        uint8_t midiNote = (36 + newOct * 12) + top;
        if (midiNote > 75) midiNote = 75;
        d1BaseFreq = d1ChromaFreq(midiNote);
        applyD1Freq();
        monoBass.noteShowStart = sysTickMs;
      }
    }
    return;
  }
  if (d1ChromaMode) {
    if (d1ChromaHeldStep >= 0) {
      uint8_t note = d1ChromaKnobToNote(knobValue);
      d1ChromaNote[d1ChromaHeldStep] = note;
      patternDirty = true;
      // Don't change d1BaseFreq -- frequency set at playback time
    }
    return;
  }
  d1BaseFreq = d1PitchCurve(knobValue);
  applyD1Freq();
}

// Case 4: D1 Volume
static void engineD1Volume(uint8_t idx, int knobValue) {
  (void)idx;
  float norm = normalizeKnob(knobValue);
  d1Volume = norm * norm * 0.65f;  // Log taper (square law), max 0.65
  AudioNoInterrupts();
  drumMixer.gain(0, d1Volume);
  AudioInterrupts();
  updateDrumDelayGains();
}

// Case 5: D1 Snap
static void engineD1Snap(uint8_t idx, int knobValue) {
  (void)idx;
  float norm = normalizeKnob(knobValue);

  // Pitch snap depth increases with knob
  float pitchDepth = 0.60f + (0.40f * norm);
  float transientGain = 0.85f + (0.15f * norm);

  AudioNoInterrupts();
  d1Snap.pitchMod(pitchDepth);
  d1VoiceMixer.gain(1, transientGain);
  AudioInterrupts();
}

// Case 6: D1 EQ (Body) — Moog-style filter sweep in MONOBASS
static void engineD1Body(uint8_t idx, int knobValue) {
  (void)idx;
  float norm = normalizeKnob(knobValue);

  if (monoBass.active || d1ChromaMode) {
    // Moog-style LPF sweep: 80 Hz – 6000 Hz exponential, with pitch tracking.
    // Uses d1LowPass (AudioFilterStateVariable) as the sweep filter.
    // Resonance at 1.8 gives a conspicuous but stable Moog-like peak.
    float baseCutoff = 80.0f * expf(norm * 4.317f);  // ln(6000/80) ≈ 4.317
    // Pitch tracking: shift cutoff up proportionally to playing frequency
    // so the filter opens with higher notes (classic Moog behavior)
    float trackRatio = d1BaseFreq / 65.41f;  // ratio relative to C2
    float cutoff = baseCutoff * (0.5f + 0.5f * trackRatio);
    if (cutoff > 8000.0f) cutoff = 8000.0f;
    if (cutoff < 100.0f)  cutoff = 100.0f;
    AudioNoInterrupts();
    d1LowPass.frequency(cutoff);
    d1LowPass.resonance(1.8f);
    AudioInterrupts();
    return;
  }

  float eqAmount = 0.4f + 0.6f * norm;

  float blend;
  if (eqAmount <= 0.5f) {
    blend = eqAmount / 0.5f;
  } else {
    blend = 1.0f + (eqAmount - 0.5f) / 0.5f;
  }

  float notchFreq0 = 200.0f + 200.0f * blend;
  float notchFreq1 = notchFreq0 * (1.15f + 0.05f * norm);
  float shelfSlope = 1.25f * blend;

  AudioNoInterrupts();
  d1EQ.setNotch(0, notchFreq0, 3);
  d1EQ.setNotch(1, notchFreq1, 1);
  d1EQ.setHighShelf(2, 3500.0f, shelfSlope, 0.9f);
  AudioInterrupts();
}

// Case 7: D1 Delay Send
static void engineD1DelaySend(uint8_t idx, int knobValue) {
  (void)idx;
  // Capped at 75% to prevent feedback runaway
  float delaySend = normalizeKnob(knobValue) * 0.75f;
  d1DelaySend = delaySend;
  updateDrumDelayGains();
}

// Case 8: D2 Pitch
static void engineD2Pitch(uint8_t idx, int knobValue) {
  (void)idx;
  if (monoBass.active) return;
  if (d2ChromaMode) {
    if (d2ChromaHeldStep >= 0) {
      uint8_t note = d2ChromaKnobToNote(knobValue);
      d2ChromaNote[d2ChromaHeldStep] = note;
      patternDirty = true;
      // Don't set frequency globally -- applied per-step at playback time
    }
    return;
  }
  float freqHz = mapFloat(knobValue, 0, 1023, 110.0f, 440.0f);  // A2-A4
  AudioNoInterrupts();
  d2Osc.frequency(freqHz);
  AudioInterrupts();
}

// Case 9: D2 Decay
static void engineD2Decay(uint8_t idx, int knobValue) {
  (void)idx;
  if (monoBass.active) return;
  d2DecayBase = d2DecayCurve(knobValue);
  d2DecayNorm = normalizeKnob(knobValue);
  applyD2Decay();
}

// Case 10: D2 Voice Mix - Snare/Clap
static void engineD2VoiceMix(uint8_t idx, int knobValue) {
  (void)idx;
  if (monoBass.active) return;
  float norm = normalizeKnob(knobValue);

  float gainSnare = norm;
  float gainClap = 1.0f - norm;

  AudioNoInterrupts();
  snareClapMixer.gain(0, gainSnare);
  snareClapMixer.gain(1, gainClap);
  AudioInterrupts();
}

// Case 11: D2 Wavefolder Drive
static void engineD2WfDrive(uint8_t idx, int knobValue) {
  (void)idx;
  if (monoBass.active) return;
  float norm = normalizeKnob(knobValue);

  // Default values (off state)
  float driveGain = 0.8f;
  float freqHz = 10.0f;
  float lpfFreqHz = 3500.0f;
  float gainDry = 1.0f;
  float gainWet = 0.0f;

  if (norm > WF_DEADBAND) {
    // Remap above deadband to 0->1 so all curves start from zero
    float active = (norm - WF_DEADBAND) / (1.0f - WF_DEADBAND);
    driveGain = 0.75f + 0.15f * active;
    // Frequency: 32-220 Hz (0-75%), 220-440 Hz (75-100%)
    if (active <= 0.75f) {
      float t = active / 0.75f;
      freqHz = 32.0f + t * 188.0f;           // 32 -> 220 Hz
    } else {
      float t = (active - 0.75f) / 0.25f;
      freqHz = 220.0f + t * 220.0f;           // 220 -> 440 Hz
    }

    // Wet mix comes in gradually with quadratic curve
    const float WET_START = 0.10f;
    float wetNorm = 0.0f;
    if (active > WET_START) {
      float blend = (active - WET_START) / (1.0f - WET_START);
      wetNorm = blend * blend;
    }

    // LPF tracks inversely with drive
    if (active <= 0.4f) {
      lpfFreqHz = 2500.0f;
    } else {
      float blend = (active - 0.4f) / 0.6f;
      float blendSquared = blend * blend;
      lpfFreqHz = 2500.0f - blendSquared * (2500.0f - 1500.0f);
    }

    // Dry pulls down as wet rises (1.0 -> 0.70)
    gainDry = 1.0f - 0.30f * wetNorm;
    // Wet ramps up with drive (0.0 -> 0.45)
    gainWet = wetNorm * 0.45f;
    // Loudness comp: starts at 20% knob, ramps down (1.0 -> 0.65)
    float comp = 1.0f;
    if (active > 0.20f) {
      float extra = (active - 0.20f) / 0.80f; // 0->1 over 20%-100%
      comp = 1.0f - 0.35f * extra;
    }
    gainDry *= comp;
    gainWet *= comp;
  }

  // Apply as one atomic update
  AudioNoInterrupts();
  d2WfAmp.gain(driveGain);
  d2WfSineOsc.frequency(freqHz);
  d2WfLowPass.frequency(lpfFreqHz);
  d2MasterMixer.gain(0, gainDry);  // dry
  d2MasterMixer.gain(2, gainWet);  // wavefolder return
  AudioInterrupts();
}

// Case 12: D2 Delay Send
static void engineD2DelaySend(uint8_t idx, int knobValue) {
  (void)idx;
  if (monoBass.active) return;
  // Capped at 75% to prevent feedback runaway
  float delaySend = normalizeKnob(knobValue) * 0.75f;
  d2DelaySend = delaySend;
  updateDrumDelayGains();
}

// Case 13: D2 Reverb
static void engineD2Reverb(uint8_t idx, int knobValue) {
  (void)idx;
  if (monoBass.active) return;
  float norm = normalizeKnob(knobValue);

  if (norm <= 0.02f) {
    // Off
    AudioNoInterrupts();
    d2MasterMixer.gain(1, 0.0f);
    AudioInterrupts();
  } else {
    // Active reverb -- sqrt wet gain, cbrt room, two-stage damping above 50%
    float rawBlend = (norm - 0.02f) / 0.98f;
    float blend = sqrtf(rawBlend);               // ~0.5 at 9 o'clock (25%)
    float roomRaw = (norm - 0.02f) / 0.73f;      // 0->1 over 2-75% knob
    if (roomRaw > 1.0f) roomRaw = 1.0f;
    float roomSize = 0.3f + cbrtf(roomRaw) * 0.45f; // 0.3->0.75, ~0.6 at 25%
    float above50 = (norm > 0.5f) ? (norm - 0.5f) / 0.25f : 0.0f;  // 0->1 over 50-75%
    if (above50 > 1.0f) above50 = 1.0f;
    float above75 = (norm > 0.75f) ? (norm - 0.75f) / 0.25f : 0.0f;  // 0->1 over 75-100%
    float damping = 1.0f - above50 * 0.25f - above75 * 0.25f; // 1.0->0.75->0.5

    AudioNoInterrupts();
    d2Reverb.roomsize(roomSize);
    d2Reverb.damping(damping);
    d2MasterMixer.gain(1, blend);
    AudioInterrupts();
  }
}

// Case 14: D2 Noise
static void engineD2Noise(uint8_t idx, int knobValue) {
  (void)idx;
  if (monoBass.active) return;
  if (knobValue >= 10) {
    float norm = normalizeKnob(knobValue);
    // 0-25%: gain/filter ramp, 30-45%: hold/decay ramp, 45-100%: final ramp
    float noiseScale = norm * 4.0f;            // 0->1 over 0-25% knob
    if (noiseScale > 1.0f) noiseScale = 1.0f;
    float above30 = (norm > 0.3f) ? (norm - 0.3f) / 0.15f : 0.0f;  // 0->1 over 30-45%
    if (above30 > 1.0f) above30 = 1.0f;
    float above45 = (norm > 0.45f) ? (norm - 0.45f) / 0.55f : 0.0f; // 0->1 over 45-100%
    float attackMs = 0.1f;                                            // fixed -- 0.1ms to avoid click/pop
    float holdMs = 7.0f + above30 * 15.5f + above45 * 22.5f;  // 7->22.5->45ms
    float decayMs = 25.0f + above30 * 3.75f + above45 * 61.25f; // 25->28.75->90ms
    float filterFreqHz = 3000.0f + 2000.0f * noiseScale;
    float noiseScaleCapped = (noiseScale > 0.5f) ? 0.5f : noiseScale;
    float noiseGain = 0.043f + 0.209f * noiseScaleCapped;  // gain caps at ~12.5% knob

    AudioNoInterrupts();
    d2NoiseEnv.attack(attackMs);
    d2NoiseEnv.hold(holdMs);
    d2NoiseEnv.decay(decayMs);
    d2NoiseFilter.frequency(filterFreqHz);
    d2VoiceMixer.gain(2, noiseGain);
    AudioInterrupts();

  } else {
    // Off
    d2VoiceMixer.gain(2, 0.0f);
  }
}

// Case 15: D2 Volume
static void engineD2Volume(uint8_t idx, int knobValue) {
  (void)idx;
  if (monoBass.active) return;
  float norm = normalizeKnob(knobValue);
  d2Volume = norm * norm * 1.25f;  // Log taper (square law), max 1.25
  AudioNoInterrupts();
  drumMixer.gain(1, d2Volume);
  AudioInterrupts();
  updateDrumDelayGains();
}

// Case 16: D3 Pitch
static void engineD3Pitch(uint8_t idx, int knobValue) {
  (void)idx;
  if (monoBass.active) return;
  float norm = normalizeKnob(knobValue);

  // --- Shared pitch bend curve (all voices use the same shape) ---
  const float BEND_START = 0.25f;
  float pitchBend;

  if (norm <= BEND_START) {
    float blend = norm / BEND_START;
    pitchBend = 0.60f * blend;
  } else {
    float blend = (norm - BEND_START) / (1.0f - BEND_START);
    pitchBend = 0.60f + 0.40f * (blend * blend);
  }

  // Gentle filter tracking -- shared by both modes (keeps volume steadier)
  const float hpfHz = 6800.0f * (1.0f + 0.06f * norm);
  const float bpfHz = 9800.0f * (1.0f + 0.05f * norm);
  const float fmBpfHz = 4000.0f + 4000.0f * pitchBend;   // 4000->8000 Hz
  const float fmHpfHz = 1500.0f + 1500.0f * pitchBend;   // 1500->3000 Hz

  if (d3ChromaMode) {
    // --- D3 CHROMA MODE: per-step note select ---
    if (d3ChromaHeldStep >= 0) {
      // Step held -- set that step's note from knob position
      uint8_t note = d3ChromaKnobToNote(knobValue);
      d3ChromaNote[d3ChromaHeldStep] = note;
      patternDirty = true;
      // Don't set frequencies globally -- applied per-step at playback time
    }
    // Filter tracking still applies in D3 chroma mode (keeps volume steadier)
    AudioNoInterrupts();
    d3606HighPass.frequency(hpfHz);
    d3606BandPass.frequency(bpfHz);
    d3FmBandPass.frequency(fmBpfHz);
    d3FmHighPass.frequency(fmHpfHz);
    AudioInterrupts();

  } else {
    // --- NORMAL MODE: inharmonic 606, irrational FM ---
    // Precomputed log of constant ratio -- expf(x*log) is ~2x faster than powf on Cortex-M7
    static const float LOG_HAT_RATIO  = logf(6000.0f / 400.0f);   // hatMax/hatMin

    const float hatCurve = pitchBend * pitchBend;
    const float hatBaseHz = 400.0f * expf(hatCurve * LOG_HAT_RATIO);

    // FM carriers and irrational ratios
    static const float LOG_FM_C1_RATIO = logf(3000.0f / 500.0f);  // 500->3000 Hz
    static const float LOG_FM_C2_RATIO = logf(6300.0f / 1050.0f); // 1050->6300 Hz

    const float carrier1Hz = 500.0f * expf(pitchBend * LOG_FM_C1_RATIO);
    const float carrier2Hz = 1050.0f * expf(pitchBend * LOG_FM_C2_RATIO);
    const float modulator1Hz = carrier1Hz * 2.236f;  // sqrt(5) fixed ratio
    const float modulator2Hz = carrier2Hz * 1.414f;  // sqrt(2) fixed ratio

    // FM depth: increases with pitch to maintain brightness at high end
    const float depth1 = 0.45f + 0.35f * pitchBend;
    const float depth2 = 0.35f + 0.30f * pitchBend;

    AudioNoInterrupts();

    // FM voice
    d3FmCarrier1.frequency(carrier1Hz);
    d3FmCarrier2.frequency(carrier2Hz);
    d3FmMod1.frequency(modulator1Hz);
    d3FmMod2.frequency(modulator2Hz);
    d3FmMod1.amplitude(depth1);
    d3FmMod2.amplitude(depth2);
    d3FmBandPass.frequency(fmBpfHz);
    d3FmHighPass.frequency(fmHpfHz);

    // 606 voice oscillator bank
    d3606Osc1.frequency(hatBaseHz * 1.00f);
    d3606Osc2.frequency(hatBaseHz * 1.08f);
    d3606Osc3.frequency(hatBaseHz * 1.17f);
    d3606Osc4.frequency(hatBaseHz * 1.26f);
    d3606Osc5.frequency(hatBaseHz * 1.36f);
    d3606Osc6.frequency(hatBaseHz * 1.48f);
    d3606HighPass.frequency(hpfHz);
    d3606BandPass.frequency(bpfHz);

    // PERC voice -- A3(220) -> A6(1760), independent of FM carriers
    static const float LOG_NOISE_RATIO = logf(1760.0f / 220.0f);  // A3->A6
    d3Perc.frequency(220.0f * expf(pitchBend * LOG_NOISE_RATIO));

    AudioInterrupts();
  }
}

// Case 17: D3 Decay
static void engineD3Decay(uint8_t idx, int knobValue) {
  (void)idx;
  if (monoBass.active) return;
  float decayMs = mapFloat(knobValue, 0, 1023, 10.0f, 250.0f);
  d3DecayBase = decayMs;
  applyD3Decay();
}

// Case 18: D3 Voice Mix -- 606 / FM / PERC
static void engineD3VoiceMix(uint8_t idx, int knobValue) {
  (void)idx;
  if (monoBass.active) return;
  // Zone boundaries (pure solo regions for each voice)
  const int ZONE_606_MAX  = int(1023 * 0.06f);
  const int ZONE_FM_MIN   = int(1023 * 0.46f);
  const int ZONE_FM_MAX   = int(1023 * 0.65f);
  const int ZONE_PERC_MIN = int(1023 * 0.94f);

  const float MIX_SCALE = 0.9f;

  // Voice level trims (matched so each voice solos at similar perceived level)
  const float TRIM_606  = 3.5f;
  const float TRIM_FM   = 2.5f;
  const float TRIM_PERC = 0.35f;

  float gain606  = 0.0f;
  float gainFM   = 0.0f;
  float gainPerc = 0.0f;

  // Calculate voice gains based on knob position
  if (knobValue <= ZONE_606_MAX) {
    gain606 = 1.0f;
  } else if (knobValue < ZONE_FM_MIN) {
    const float blend = float(knobValue - ZONE_606_MAX) / float(ZONE_FM_MIN - ZONE_606_MAX);
    gain606 = 1.0f - blend;
    gainFM = blend;
  } else if (knobValue <= ZONE_FM_MAX) {
    gainFM = 1.0f;
  } else if (knobValue < ZONE_PERC_MIN) {
    const float blend = float(knobValue - ZONE_FM_MAX) / float(ZONE_PERC_MIN - ZONE_FM_MAX);
    gainFM = 1.0f - blend;
    gainPerc = blend;
  } else {
    gainPerc = 1.0f;
  }

  AudioNoInterrupts();
  d3VoiceMixer.gain(0, gain606  * MIX_SCALE * TRIM_606);
  d3VoiceMixer.gain(1, gainFM   * MIX_SCALE * TRIM_FM);
  d3VoiceMixer.gain(2, gainPerc * MIX_SCALE * TRIM_PERC);
  AudioInterrupts();
}

// Case 19: D3 Distort (sine-driven wavefolder)
static void engineD3Distort(uint8_t idx, int knobValue) {
  (void)idx;
  if (monoBass.active) return;
  float norm = normalizeKnob(knobValue);
  if (norm <= WF_DEADBAND) {
    AudioNoInterrupts();
    d3WfOsc.amplitude(0.0f);
    d3MasterMixer.gain(0, 0.70f);  // dry at default (parity with D1/D2)
    d3MasterMixer.gain(1, 0.0f);   // wavefolder return off
    AudioInterrupts();
    return;
  }
  // Remap above deadband to 0->1 so all curves start from zero
  float active = (norm - WF_DEADBAND) / (1.0f - WF_DEADBAND);

  // Amplitude: 0.05 -> 0.50 (linear ramp, full range)
  float drive = 0.05f + 0.45f * active;

  // Frequency: 16.352 -> 110 Hz (exponential, sub-bass waveshaping range — C0 to A2)
  float freqHz = 16.352f * expf(active * 1.905f);  // ln(110/16.352) ≈ 1.905

  // Dry pulls down slightly as drive rises (0.70 -> 0.62, before loudness comp)
  float dryGain = 0.70f - 0.08f * active;
  // Wet return ramps up with drive (0.0 -> 0.35)
  float wetGain = active * 0.35f;
  // Loudness comp: gentle to 55%, steeper above (1.0 -> 0.934 -> 0.684)
  float comp;
  if (active <= 0.55f) {
    comp = 1.0f - 0.12f * active;
  } else {
    float base = 1.0f - 0.12f * 0.55f;  // 0.934
    float extra = (active - 0.55f) / 0.45f; // 0->1 over 55%-100%
    comp = base - 0.25f * extra;
  }
  dryGain *= comp;
  wetGain *= comp;

  AudioNoInterrupts();
  d3WfOsc.amplitude(drive);
  d3WfOsc.frequency(freqHz);
  d3MasterMixer.gain(0, dryGain);  // dry
  d3MasterMixer.gain(1, wetGain);  // wavefolder return
  AudioInterrupts();
}

// Case 20: D3 Delay Send
static void engineD3DelaySend(uint8_t idx, int knobValue) {
  (void)idx;
  if (monoBass.active) return;
  // Capped at 75% to prevent feedback runaway (same as D1/D2)
  float delaySend = normalizeKnob(knobValue) * 0.75f;
  d3DelaySend = delaySend;
  updateDrumDelayGains();
}

// Case 21: D3 Filter
static void engineD3Filter(uint8_t idx, int knobValue) {
  (void)idx;
  if (monoBass.active) return;
  float norm = normalizeKnob(knobValue);
  float cutoffHz = d3FilterCurve(knobValue);

  // Resonance: gentle bump at low cutoffs for volume compensation, clean when open
  float resonance = 0.35f + 0.15f * (1.0f - norm);  // 0.50 -> 0.35

  AudioNoInterrupts();
  d3MasterFilter.frequency(cutoffHz);
  d3MasterFilter.resonance(resonance);
  AudioInterrupts();
}

// Case 22: D3 Accent Pattern
static void engineD3Accent(uint8_t idx, int knobValue) {
  (void)idx;
  if (monoBass.active) return;
  uint8_t prevMode = d3AccentMode;
  d3AccentMode = accentModeFromKnob(knobValue);
  d3AccentMask = accentMaskFromMode(d3AccentMode);

  // Only recompute decay + preview when mode actually changes
  if (d3AccentMode != prevMode) {
    applyD3Decay();

    // Show the new pattern briefly on step LEDs
    accentPreview.mask = d3AccentMask;  // already computed above

    uint32_t nowTick;
    noInterrupts();
    nowTick = sysTickMs;  // sysTickMs is ISR-written
    interrupts();

    accentPreview.startTick = nowTick;
    accentPreview.active = true;  // Main-loop only -- no guard needed

    updateLEDs();
  }
}

// Case 23: D3 Volume
static void engineD3Volume(uint8_t idx, int knobValue) {
  (void)idx;
  if (monoBass.active) return;
  float norm = normalizeKnob(knobValue);
  d3Volume = norm * norm * 0.75f;  // Log taper (square law), max 0.75
  AudioNoInterrupts();
  drumMixer.gain(2, d3Volume);
  AudioInterrupts();
  updateDrumDelayGains();
}

// Case 24: Master Delay Time (quantized to tempo)
static void engineMasterDelayTime(uint8_t idx, int knobValue) {
  (void)idx;
  // Use external BPM when synced, fall back to internal knob BPM
  float activeBpm = (extBpmDisplay > 0.0f) ? extBpmDisplay : bpm;
  int ratioIdx = delayRatioFromKnob(knobValue, activeBpm);
  float msPerBeat = 60000.0f / activeBpm;

  float delayMs = msPerBeat * quantizeRatios[ratioIdx];
  if (delayMs > 1400.0f) delayMs = 1400.0f;

  AudioNoInterrupts();
  masterDelay.delay(0, delayMs);
  AudioInterrupts();
}

// Case 25: Wavefold Frequency
static void engineWfFreq(uint8_t idx, int knobValue) {
  (void)idx;
  if (wfChromaMode) {
    // Quantize wavefold frequency to chromatic notes (C2-C6, MIDI 36-84)
    uint8_t midiNote = map(knobValue, 0, 1023, 36, 84);
    float freq = midiToFreq(midiNote);
    AudioNoInterrupts();
    masterWfOscSine.frequency(freq);
    masterWfOscSaw.frequency(freq);
    AudioInterrupts();
  } else {
    float norm = normalizeKnob(knobValue);
    // C2->C6 exponential: 4 octaves, every 25% = one octave
    float baseFreq = 65.41f * expf(norm * 2.7725887f);

    float sineFreq, sawFreq;

    if (norm <= 0.5f) {
      // 0-50%: sine at baseFreq, saw one octave below
      sineFreq = baseFreq;
      sawFreq  = baseFreq * 0.5f;
    } else if (norm <= 0.75f) {
      // 50-75%: saw locks to baseFreq, sine diverges up to 2x baseFreq
      float blend = (norm - 0.5f) / 0.25f;
      sawFreq  = baseFreq;
      sineFreq = baseFreq * (1.0f + blend);
    } else {
      // 75-100%: sine locks to baseFreq, saw diverges up to 4x baseFreq
      float blend = (norm - 0.75f) / 0.25f;
      sineFreq = baseFreq;
      sawFreq  = baseFreq * (1.0f + blend * 3.0f);
    }

    AudioNoInterrupts();
    masterWfOscSine.frequency(sineFreq);
    masterWfOscSaw.frequency(sawFreq);
    AudioInterrupts();
  }
}

// Case 26: Master Lowpass Filter
static void engineMasterLowpass(uint8_t idx, int knobValue) {
  (void)idx;
  int freqHz = map(knobValue, 0, 1023, 1000, 7500);
  AudioNoInterrupts();
  masterLowPass.frequency(freqHz);
  AudioInterrupts();
}

// Case 27: Master Tempo (BPM) -- inactive during external clock / PPQN mode
static void engineMasterTempo(uint8_t idx, int knobValue) {
  (void)idx;
  if (monoBass.active) return;
  if (ppqnModeActive) return;     // Don't change BPM while selecting PPQN
  if (transportState == RUN_EXT) return;  // Ignore knob when ext clock drives tempo

  bpm = bpmFromKnob(knobValue);

  // If internal clock is currently running, adjust period without restarting.
  // update() finishes the current interval, then applies the new period --
  // no gap, no stutter.
  if (transportState == RUN_INT) {
    float stepPeriodUs = 60000000.0f / (STEPS_PER_BEAT * bpm);
    if (stepPeriodUs < 500.0f) stepPeriodUs = 500.0f;
    stepTimer.update((uint32_t)stepPeriodUs);
  }
}

// Case 28: Master Volume
static void engineMasterVolume(uint8_t idx, int knobValue) {
  (void)idx;
  masterNominalGain = normalizeKnob(knobValue) * 7.0f;  // 0-7x range
  applyMasterGain();
}

// Case 29: Master Choke Offset
static void engineMasterChoke(uint8_t idx, int knobValue) {
  (void)idx;
  if (monoBass.active) return;
  chokeOffsetMs = chokeOffsetFromKnob(knobValue);
  if (chokeOffsetMs == 0) {
    chokeDisplayPercent = 0;  // Match engine deadband -- display shows "OFF"
  } else {
    chokeDisplayPercent = (int)((normalizeKnob(knobValue) - 0.5f) * 200.0f);
  }
  applyChokeToDecays();
}

// Case 30: Wavefold (master wavefolder drive)
static void engineWfDrive(uint8_t idx, int knobValue) {
  (void)idx;
  float norm = normalizeKnob(knobValue);

  if (norm <= WF_DEADBAND) {
    AudioNoInterrupts();
    masterWfInputMixer.gain(0, 0.0f);
    masterWfInputMixer.gain(1, 0.0f);
    masterMixer.gain(0, 1.0f);
    masterMixer.gain(1, 0.0f);
    AudioInterrupts();
    return;
  }

  float active = (norm - WF_DEADBAND) / (1.0f - WF_DEADBAND);
  if (active > 1.0f) active = 1.0f;
  active *= 0.5f;  // full knob = old 50%

  float sq      = active * active;
  float oscGain = sq * 0.55f;
  float dry     = 1.0f - 0.12f * sq;
  float wet     = 0.30f + 0.28f * sq;
  float comp    = (1.0f / (dry + wet)) * (1.0f - 0.45f * sq);

  AudioNoInterrupts();
  masterWfInputMixer.gain(0, oscGain);
  masterWfInputMixer.gain(1, oscGain);
  masterMixer.gain(0, dry);
  masterMixer.gain(1, wet * comp);
  AudioInterrupts();
}

// Case 31: Master Delay Mix/Feedback
static void engineMasterDelayMix(uint8_t idx, int knobValue) {
  (void)idx;
  float norm = normalizeKnob(knobValue);
  const float PEAK_LEVEL = 0.73f;

  // Compute all gains before applying as one atomic update
  float ampGain, returnGain, fbGain;

  // Dead band below 2% -- delay fully OFF
  if (norm <= 0.02f) {
    ampGain = 0.0f;
    returnGain = 0.0f;
    fbGain = 0.0f;
  } else if (norm <= 0.25f) {
    // Early zone (2%-25%, ~7-9 o'clock): volume ramp only, no feedback.
    // sqrt curve for smoother audible control in the quiet zone.
    float blend = (norm - 0.02f) / (0.25f - 0.02f);  // 0->1 over 2%-25%
    float level = sqrtf(blend) * PEAK_LEVEL;
    ampGain = level;
    returnGain = level;
    fbGain = 0.0f;
  } else if (norm <= 0.97f) {
    // Main zone (25%-97%, ~9 o'clock-5 o'clock): level locked at peak,
    // feedback ramps 0->0.5. Linear curve so echoes are audible by noon
    // and clearly building through 3 o'clock.
    ampGain = PEAK_LEVEL;
    returnGain = PEAK_LEVEL;
    float fbBlend = (norm - 0.25f) / (0.97f - 0.25f);  // 0->1 over 25%-97%
    fbGain = fbBlend * 0.5f;
  } else {
    // Top 3% (97%-100%): gentle oscillation zone. Feedback ramps 0.5->0.65.
    // Just past the oscillation threshold -- sustained but not blowout.
    ampGain = PEAK_LEVEL;
    returnGain = PEAK_LEVEL;
    float oscBlend = (norm - 0.97f) / 0.03f;  // 0->1 over 97%-100%
    fbGain = 0.5f + oscBlend * 0.15f;   // 0.5->0.65
  }

  AudioNoInterrupts();
  delayAmp.gain(ampGain);
  masterMixer.gain(2, returnGain);
  delaySendMixer.gain(3, fbGain);
  AudioInterrupts();
}

// ============================================================================
//  Dispatch table
// ============================================================================

static const KnobDescriptor knobTable[32] = {
  /* 0  D1 Distort    */ { displayD1Distort,       engineD1Distort       },
  /* 1  D1 Shape      */ { displayD1Shape,         engineD1Shape         },
  /* 2  D1 Decay      */ { displayD1Decay,         engineD1Decay         },
  /* 3  D1 Pitch      */ { displayD1Pitch,         engineD1Pitch         },
  /* 4  D1 Volume     */ { displayD1Volume,        engineD1Volume        },
  /* 5  D1 Snap       */ { displayD1Snap,          engineD1Snap          },
  /* 6  D1 Body       */ { displayD1Body,          engineD1Body          },
  /* 7  D1 Delay Send */ { displayD1DelaySend,     engineD1DelaySend     },
  /* 8  D2 Pitch      */ { displayD2Pitch,         engineD2Pitch         },
  /* 9  D2 Decay      */ { displayD2Decay,         engineD2Decay         },
  /* 10 D2 Voice Mix  */ { displayD2VoiceMix,      engineD2VoiceMix      },
  /* 11 D2 WF Drive   */ { displayD2WfDrive,       engineD2WfDrive       },
  /* 12 D2 Delay Send */ { displayD2DelaySend,     engineD2DelaySend     },
  /* 13 D2 Reverb     */ { displayD2Reverb,        engineD2Reverb        },
  /* 14 D2 Noise      */ { displayD2Noise,         engineD2Noise         },
  /* 15 D2 Volume     */ { displayD2Volume,        engineD2Volume        },
  /* 16 D3 Pitch      */ { displayD3Pitch,         engineD3Pitch         },
  /* 17 D3 Decay      */ { displayD3Decay,         engineD3Decay         },
  /* 18 D3 Voice Mix  */ { displayD3VoiceMix,      engineD3VoiceMix      },
  /* 19 D3 Distort    */ { displayD3Distort,       engineD3Distort       },
  /* 20 D3 Delay Send */ { displayD3DelaySend,     engineD3DelaySend     },
  /* 21 D3 Filter     */ { displayD3Filter,        engineD3Filter        },
  /* 22 D3 Accent     */ { displayD3Accent,        engineD3Accent        },
  /* 23 D3 Volume     */ { displayD3Volume,        engineD3Volume        },
  /* 24 Delay Time    */ { displayMasterDelayTime, engineMasterDelayTime },
  /* 25 WF Freq       */ { displayWfFreq,          engineWfFreq          },
  /* 26 Master LP     */ { displayMasterLowpass,   engineMasterLowpass   },
  /* 27 Tempo         */ { displayMasterTempo,     engineMasterTempo     },
  /* 28 Master Vol    */ { displayMasterVolume,    engineMasterVolume    },
  /* 29 Choke         */ { displayMasterChoke,     engineMasterChoke     },
  /* 30 WF Drive      */ { displayWfDrive,         engineWfDrive         },
  /* 31 Delay Mix     */ { displayMasterDelayMix,  engineMasterDelayMix  },
};

#endif // KNOB_HANDLERS_H
