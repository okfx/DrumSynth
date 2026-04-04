// chroma.h — Chromatic mode pitch tables, note conversion, envelope filter,
// and dot animation helpers.
//
// Depends on: audiotool.h (audio objects), sysTickMs, chroma state vars
// (all defined in DrumSynth.ino above the #include point).

#pragma once

// --- Extern: audio objects (types from audiotool.h, already in scope) ---
extern AudioSynthWaveformModulated d2Osc;
extern AudioSynthWaveform          d3606Osc1, d3606Osc2, d3606Osc3;
extern AudioSynthWaveform          d3606Osc4, d3606Osc5, d3606Osc6;
extern AudioSynthWaveformModulated d3FmCarrier1, d3FmCarrier2;
extern AudioSynthWaveform          d3FmMod1, d3FmMod2;
extern AudioSynthSimpleDrum        d3Perc;
extern AudioFilterStateVariable    d1LowPass;

// --- Extern: chroma state (defined in .ino) ---
extern bool d1ChromaMode;
extern bool d2ChromaMode;
extern bool d3ChromaMode;
extern float d1ChromaEnvFiltDepth;
extern float d1ChromaEnvFiltBaseHz;
extern uint32_t d1ChromaEnvFiltTrigger;
extern float d1EnvFiltTauMs;
extern float d1EffectiveDecay;
extern volatile uint32_t sysTickMs;

// --- Extern: D3 envelope filter state (defined in .ino) ---
extern AudioFilterLadder        d3MasterFilter;
extern AudioFilterStateVariable d3PercFilter;
extern float    d3EnvFiltDepth;
extern float    d3EnvFiltBaseHz;
extern float    d3EnvFiltPercBaseHz;
extern uint32_t d3EnvFiltTrigger;
extern float    d3EnvFiltTauMs;

// --- Extern: chroma animation state (defined in .ino) ---
extern int8_t chromaAnimDot;
extern uint32_t chromaAnimStart;
extern bool chromaAnimAppearing;

// --- Chroma dot animation ---
// ChromaAnimPhase enum is defined in .ino (before this header) so the
// variable definition is visible at file scope.
extern ChromaAnimPhase chromaAnimPhase;

static constexpr uint32_t CHROMA_COMBO_RAMP_MS = 120;   // fast dither during X+button combo
static constexpr uint32_t CHROMA_SETTLE_MS     = 20;    // snap duration after ramp

static void startChromaRamp(uint8_t dotIndex, bool appearing) {
  chromaAnimDot = dotIndex;
  chromaAnimPhase = CHROMA_ANIM_RAMPING;
  chromaAnimStart = sysTickMs;
  chromaAnimAppearing = appearing;
}

static void startChromaSettle(uint32_t nowMs) {
  if (chromaAnimDot < 0) return;
  chromaAnimPhase = CHROMA_ANIM_SETTLING;
  chromaAnimStart = nowMs;
}

static void cancelChromaAnim() {
  chromaAnimDot = -1;
  chromaAnimPhase = CHROMA_ANIM_NONE;
}

// --- MIDI frequency lookup ---

// D1 chroma note range (MIDI 33=A1 through 75=D#5)
static constexpr uint8_t D1_CHROMA_NOTE_MIN = 33;
static constexpr uint8_t D1_CHROMA_NOTE_MAX = 75;

// D2 chroma mode constants
static constexpr uint8_t D2_CHROMA_NOTE_MIN = 45;  // A2
static constexpr uint8_t D2_CHROMA_NOTE_MAX = 69;  // A4

// D3 chroma mode constants
static constexpr uint8_t D3_HARM_NOTE_MIN = 48;  // C3
static constexpr uint8_t D3_HARM_NOTE_MAX = 84;  // C6

// Shared MIDI-to-Hz lookup table covering MIDI 33 (A1) through 84 (C6).
// Used by D1/D2/D3 chroma modes and wavefolder chroma — replaces per-voice
// expf()/powf() calls with a single 208-byte flash table.
static constexpr uint8_t MIDI_FREQ_MIN = 33;
static constexpr uint8_t MIDI_FREQ_MAX = 84;

static const float MIDI_FREQ[] = {
  // MIDI 33   34       35       36       37       38       39       40
  55.000f, 58.270f, 61.735f, 65.406f, 69.296f, 73.416f, 77.782f, 82.407f,
  // MIDI 41   42       43       44       45       46       47       48
  87.307f, 92.499f, 97.999f, 103.826f, 110.000f, 116.541f, 123.471f, 130.813f,
  // MIDI 49   50       51       52       53       54       55       56
  138.591f, 146.832f, 155.563f, 164.814f, 174.614f, 184.997f, 195.998f, 207.652f,
  // MIDI 57   58       59       60       61       62       63       64
  220.000f, 233.082f, 246.942f, 261.626f, 277.183f, 293.665f, 311.127f, 329.628f,
  // MIDI 65   66       67       68       69       70       71       72
  349.228f, 369.994f, 391.995f, 415.305f, 440.000f, 466.164f, 493.883f, 523.251f,
  // MIDI 73   74       75       76       77       78       79       80
  554.365f, 587.330f, 622.254f, 659.255f, 698.456f, 739.989f, 783.991f, 830.609f,
  // MIDI 81   82       83       84
  880.000f, 932.328f, 987.767f, 1046.502f
};

static_assert(sizeof(MIDI_FREQ) / sizeof(MIDI_FREQ[0]) == (MIDI_FREQ_MAX - MIDI_FREQ_MIN + 1),
              "MIDI_FREQ must have one entry per MIDI note in range");

// Clamp-and-lookup: safe for any uint8_t input.
static inline float midiToFreq(uint8_t midiNote) {
  uint8_t idx = constrain(midiNote, MIDI_FREQ_MIN, MIDI_FREQ_MAX) - MIDI_FREQ_MIN;
  return MIDI_FREQ[idx];
}

static inline float d1ChromaFreq(uint8_t midiNote) {
  return midiToFreq(midiNote);
}

// Chroma pitch: writes note name string for display.
// outName must be >= 6 chars (worst case: "C#10\0" = 5 + null).
static inline void formatChromaNote(uint8_t midiNote, char* outName) {
  static const char* NOTE_NAMES[] = {
    "C","C#","D","D#","E","F","F#","G","G#","A","A#","B"
  };
  snprintf(outName, 6, "%s%d", NOTE_NAMES[midiNote % 12], (midiNote / 12) - 1);
}

// --- Knob-to-note conversion ---

// Maps knob 0–1023 to MIDI note in A1(33)–D#5(75) range, quantized.
static inline uint8_t d1ChromaKnobToNote(int knobValue) {
  float semi = (knobValue / 1023.0f) * (float)(D1_CHROMA_NOTE_MAX - D1_CHROMA_NOTE_MIN);
  int note = constrain((int)(semi + 0.5f), 0, D1_CHROMA_NOTE_MAX - D1_CHROMA_NOTE_MIN);
  return (uint8_t)(note + D1_CHROMA_NOTE_MIN);
}

// Maps knob 0–1023 linearly to MIDI note in A2(45)–A4(69) range, quantized.
static inline uint8_t d2ChromaKnobToNote(int knobValue) {
  float semi = (knobValue / 1023.0f) * (float)(D2_CHROMA_NOTE_MAX - D2_CHROMA_NOTE_MIN);
  return (uint8_t)constrain((int)(semi + 0.5f) + D2_CHROMA_NOTE_MIN,
                             D2_CHROMA_NOTE_MIN, D2_CHROMA_NOTE_MAX);
}

// Maps knob 0–1023 to MIDI note in C3(48)–C6(84) range using the same
// pitchBend curve as the D3 harmonic engine (BEND_START breakpoint at 0.25).
static inline uint8_t d3ChromaKnobToNote(int knobValue) {
  float norm = knobValue / 1023.0f;
  const float BEND_START = 0.25f;
  float pitchBend;
  if (norm <= BEND_START) {
    pitchBend = 0.60f * (norm / BEND_START);
  } else {
    float blend = (norm - BEND_START) / (1.0f - BEND_START);
    pitchBend = 0.60f + 0.40f * (blend * blend);
  }
  float semi = pitchBend * (float)(D3_HARM_NOTE_MAX - D3_HARM_NOTE_MIN);
  return (uint8_t)constrain((int)(semi + 0.5f) + D3_HARM_NOTE_MIN,
                             D3_HARM_NOTE_MIN, D3_HARM_NOTE_MAX);
}

// --- Frequency application ---

// Sets d2Osc frequency from MIDI note. Called per-step during playback
// when D2 chroma mode is active. Only d2Osc is pitched — d2Body stays fixed.
static inline void applyD2ChromaFreq(uint8_t midiNote) {
  AudioNoInterrupts();
  d2Osc.frequency(midiToFreq(midiNote));
  AudioInterrupts();
}

// Sets all D3 oscillators (606 bank, FM, perc) to harmonic ratios for a given
// MIDI note.  Called per-step during playback when D3 chroma mode is active.
// Filter tracking is handled separately by the knob handler — not per-step.
static inline void applyD3ChromaFreq(uint8_t midiNote) {
  float baseHz = midiToFreq(midiNote);
  AudioNoInterrupts();
  // 606 bank: power chord spacing
  d3606Osc1.frequency(baseHz * 1.0f);   // root
  d3606Osc2.frequency(baseHz * 1.5f);   // fifth
  d3606Osc3.frequency(baseHz * 2.0f);   // octave
  d3606Osc4.frequency(baseHz * 3.0f);   // fifth + octave
  d3606Osc5.frequency(baseHz * 4.0f);   // two octaves
  d3606Osc6.frequency(baseHz * 6.0f);   // fifth + two octaves
  // FM voice: octave-spaced carrier-modulator pairs (2:1 ratios)
  d3FmCarrier1.frequency(baseHz * 2.0f);
  d3FmMod1.frequency(baseHz * 4.0f);
  d3FmMod1.amplitude(0.35f);
  d3FmCarrier2.frequency(baseHz * 3.0f);
  d3FmMod2.frequency(baseHz * 6.0f);
  d3FmMod2.amplitude(0.30f);
  // Perc: same base pitch
  d3Perc.frequency(baseHz);
  AudioInterrupts();
}

// --- D1 chroma envelope filter ---

// Per-frame envelope filter update for D1 chroma mode.
// Drives d1LowPass frequency + resonance from an exponential decay envelope
// triggered on each D1 note. Depth controlled by body knob.
static inline void updateD1ChromaEnvFilter(uint32_t nowMs) {
  if (!d1ChromaMode) return;
  if (d1ChromaEnvFiltDepth < 0.01f) return;

  if (d1ChromaEnvFiltTrigger == 0) {
    // No trigger yet — hold at resting cutoff so body-knob moves are heard.
    AudioNoInterrupts();
    d1LowPass.frequency(d1ChromaEnvFiltBaseHz);
    d1LowPass.resonance(1.5f);
    AudioInterrupts();
    return;
  }

  uint32_t elapsed = nowMs - d1ChromaEnvFiltTrigger;
  // Filter tau scales with attack knob: 60ms (punchy) → 250ms (slow pad)
  float decay = expf(-(float)elapsed / d1EnvFiltTauMs);

  // Sweep from ceiling down to base cutoff — the "bow" is on the attack
  float peak = d1ChromaEnvFiltBaseHz
             + d1ChromaEnvFiltDepth * (kEnvFiltCeiling - d1ChromaEnvFiltBaseHz);
  if (peak > kEnvFiltCeiling) peak = kEnvFiltCeiling;
  float cutoff = d1ChromaEnvFiltBaseHz + (peak - d1ChromaEnvFiltBaseHz) * decay;
  if (cutoff < 20.0f) cutoff = 20.0f;

  // Resonance bump gives the acid character
  float resonance = 1.5f + 4.0f * d1ChromaEnvFiltDepth * decay;

  AudioNoInterrupts();
  d1LowPass.frequency(cutoff);
  d1LowPass.resonance(resonance);
  AudioInterrupts();
}

// --- D3 envelope filter ---

// Per-frame envelope filter update for D3 (hi-hat).
// Always active (not mode-gated). Drives d3MasterFilter + d3PercFilter
// from exponential decay triggered on each D3 hit.
static inline void updateD3EnvFilter(uint32_t nowMs) {
  if (d3EnvFiltDepth < 0.01f) return;

  if (d3EnvFiltTrigger == 0) {
    // No trigger yet — hold at resting cutoff so knob moves are heard
    AudioNoInterrupts();
    d3MasterFilter.frequency(d3EnvFiltBaseHz);
    d3MasterFilter.resonance(0.35f);
    d3PercFilter.frequency(d3EnvFiltPercBaseHz);
    d3PercFilter.resonance(2.5f);
    AudioInterrupts();
    return;
  }

  uint32_t elapsed = nowMs - d3EnvFiltTrigger;
  float decay = expf(-(float)elapsed / d3EnvFiltTauMs);

  // Master filter sweep: peak → base (D3 uses higher ceiling than D1)
  float peak = d3EnvFiltBaseHz
             + d3EnvFiltDepth * (kD3EnvFiltCeiling - d3EnvFiltBaseHz);
  if (peak > kD3EnvFiltCeiling) peak = kD3EnvFiltCeiling;
  float cutoff = d3EnvFiltBaseHz + (peak - d3EnvFiltBaseHz) * decay;
  if (cutoff < 20.0f) cutoff = 20.0f;

  // Master resonance bump for acid character
  float resonance = 0.35f + 0.35f * d3EnvFiltDepth * decay;

  // Perc filter sweep — low range to intersect perc energy (440–1760 Hz)
  // Ceiling at 4000 Hz, base tracks knob (down to 600 Hz at full depth)
  static constexpr float kPercFiltCeiling = 4000.0f;
  float percPeak = d3EnvFiltPercBaseHz
                 + d3EnvFiltDepth * (kPercFiltCeiling - d3EnvFiltPercBaseHz);
  if (percPeak > kPercFiltCeiling) percPeak = kPercFiltCeiling;
  float percCutoff = d3EnvFiltPercBaseHz + (percPeak - d3EnvFiltPercBaseHz) * decay;
  if (percCutoff < 20.0f) percCutoff = 20.0f;

  AudioNoInterrupts();
  d3MasterFilter.frequency(cutoff);
  d3MasterFilter.resonance(resonance);
  d3PercFilter.frequency(percCutoff);
  d3PercFilter.resonance(2.5f + 8.0f * d3EnvFiltDepth * decay);
  AudioInterrupts();
}
