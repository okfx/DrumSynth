#ifndef MONOBASS_H
#define MONOBASS_H
// ============================================================================
//  MONOBASS — monobass.h
//
//  D1 live keyboard mode: step buttons play chromatic notes through D1 voice,
//  sequencer fully suppressed. Gate-style envelope with configurable release.
//
//  Include AFTER: hw_setup.h (display, numSteps, ledShiftReg),
//                 audiotool.h (d1AmpEnv, d1Osc, etc.),
//                 DrumSynth.ino state declarations (d1BaseFreq, formatChromaNote, etc.)
// ============================================================================

#include <Arduino.h>

// ============================================================================
//  Constants
// ============================================================================

static constexpr uint32_t D1_MONOBASS_LONG_PRESS_MS  = 4000;
static constexpr uint32_t MONOBASS_OCT_DISPLAY_MS    = 1200;
static constexpr uint32_t MONOBASS_PARAM_DISPLAY_MS  = 1200;
static constexpr uint32_t MONOBASS_NOTE_DISPLAY_MS   = 1200;

// ============================================================================
//  State
// ============================================================================

struct MonoBassState {
  static constexpr uint8_t kMaxHeldKeys = 8;

  bool active = false;
  uint8_t octave = 0;           // 0=OCT2 (C2), 1=OCT3 (C3), 2=OCT4 (C4)
  float releaseMs = 30.0f;      // gate-style release time (knob-controlled)
  bool showOctave = false;      // show large OCT display in scope area
  uint32_t octaveShowStart = 0; // when octave display started (sysTickMs)
  char paramLabel[12] = "";     // short label for large-font param display
  char paramValue[12] = "";     // value string for large-font param display
  uint32_t paramShowStart = 0;  // when param display started
  uint32_t noteShowStart = 0;   // when note display was triggered

  // Last-note-priority key stack for polyphonic trill
  int8_t heldKeys[kMaxHeldKeys];
  uint8_t heldCount = 0;

  int8_t topKey() const { return heldCount > 0 ? heldKeys[heldCount - 1] : -1; }
};

extern MonoBassState monoBass;

// ============================================================================
//  External dependencies — defined in main .ino
// ============================================================================

extern Adafruit_SH1106G display;
extern float d1BaseFreq;
extern volatile uint32_t sysTickMs;
extern volatile bool sequencePlaying;
extern volatile bool armed;
extern volatile uint16_t armPulseCountdown;
extern volatile uint8_t pendingStepCount;
extern volatile bool wantSwitchToExt;
extern float d1EffectiveDecay;
extern float d1CachedHoldMs;
extern RailMode activeRail;
extern char displayParameter1[24];
extern char displayParameter2[24];
extern uint32_t parameterOverlayStartTick;
extern ShiftRegister74HC595<2> ledShiftReg;
extern int8_t d1ChromaHeldStep;
extern int8_t d2ChromaHeldStep;
extern int8_t d3ChromaHeldStep;

// Functions defined in .ino
extern void formatChromaNote(uint8_t midiNote, char* outName);
extern float d1ChromaFreq(uint8_t midiNote);
extern void applyD1Freq();
extern void triggerD1();
extern void setTransport(TransportState s);
extern void updateLEDs();
extern void drawOutlinedText(int x, int y, const char* text);

// ============================================================================
//  Entry / exit
// ============================================================================

// Enter MONOBASS mode: stop transport, configure gate envelope.
void enterMonoBassMode() {
  monoBass.active = true;
  // Stop transport if running
  noInterrupts();
  sequencePlaying = false;
  setTransport(STOPPED);
  armed = false;
  armPulseCountdown = 0;
  pendingStepCount = 0;
  wantSwitchToExt = false;
  interrupts();
  monoBass.heldCount = 0;
  memset(monoBass.heldKeys, -1, sizeof(monoBass.heldKeys));
  monoBass.showOctave = false;
  monoBass.noteShowStart = 0;
  // Clear any chroma note-select state so it doesn't persist across mode switch
  d1ChromaHeldStep = -1;
  d2ChromaHeldStep = -1;
  d3ChromaHeldStep = -1;
  // Gate-style bass synth envelope: fast attack, full sustain, short release
  AudioNoInterrupts();
  d1AmpEnv.attack(2.0f);       // Moog-like punch (~2ms)
  d1AmpEnv.hold(0.0f);         // no hold phase — sustain takes over
  d1AmpEnv.decay(0.0f);        // no decay — sustain at full level
  d1AmpEnv.sustain(1.0f);      // hold at full amplitude while key pressed
  d1AmpEnv.release(monoBass.releaseMs);
  AudioInterrupts();
}

// Exit MONOBASS mode: restore drum envelope.
void exitMonoBassMode() {
  monoBass.active = false;
  AudioNoInterrupts();
  d1AmpEnv.attack(D1_ATTACK_MS);
  d1AmpEnv.hold(d1CachedHoldMs);
  d1AmpEnv.decay(d1EffectiveDecay);
  d1AmpEnv.sustain(0.0f);      // drum mode — decay to silence
  d1AmpEnv.release(5.0f);      // library default
  d1AmpEnv.noteOff();          // silence any lingering gate note
  AudioInterrupts();
}

// ============================================================================
//  Keyboard handler — call from updateStepButtons() for each button
// ============================================================================

// Handle a step button press/release in MONOBASS mode.
// Last-note priority: most recently pressed key sounds. Releasing it
// falls back to the previous still-held key, enabling trills.
// Returns true if handled (caller should skip normal step logic).
bool handleMonoBassButton(int buttonIndex, bool pressed) {
  if (!monoBass.active) return false;

  if (pressed) {
    // Push key onto held-key stack
    if (monoBass.heldCount < MonoBassState::kMaxHeldKeys) {
      monoBass.heldKeys[monoBass.heldCount++] = buttonIndex;
    } else {
      // Stack full — drop oldest, shift left, push new at end
      for (uint8_t k = 1; k < MonoBassState::kMaxHeldKeys; k++) {
        monoBass.heldKeys[k - 1] = monoBass.heldKeys[k];
      }
      monoBass.heldKeys[MonoBassState::kMaxHeldKeys - 1] = buttonIndex;
    }

    // Play the new top key
    uint8_t midiNote = (36 + monoBass.octave * 12) + buttonIndex;
    if (midiNote > 75) midiNote = 75;
    d1BaseFreq = d1ChromaFreq(midiNote);
    applyD1Freq();
    triggerD1();
    monoBass.noteShowStart = sysTickMs;

    // Light only the sounding key
    for (int j = 0; j < numSteps; j++) ledShiftReg.set(j, j == buttonIndex);
  } else {
    // Remove released key from stack
    bool found = false;
    for (uint8_t k = 0; k < monoBass.heldCount; k++) {
      if (monoBass.heldKeys[k] == buttonIndex) found = true;
      if (found && k + 1 < monoBass.heldCount) {
        monoBass.heldKeys[k] = monoBass.heldKeys[k + 1];
      }
    }
    if (found) monoBass.heldCount--;

    int8_t top = monoBass.topKey();
    if (top >= 0) {
      // Fall back to previous held key
      uint8_t midiNote = (36 + monoBass.octave * 12) + top;
      if (midiNote > 75) midiNote = 75;
      d1BaseFreq = d1ChromaFreq(midiNote);
      applyD1Freq();
      triggerD1();
      monoBass.noteShowStart = sysTickMs;
      for (int j = 0; j < numSteps; j++) ledShiftReg.set(j, j == top);
    } else {
      // No keys held — gate off
      AudioNoInterrupts();
      d1AmpEnv.noteOff();
      AudioInterrupts();
      for (int j = 0; j < numSteps; j++) ledShiftReg.set(j, LOW);
    }
  }
  return true;
}

// ============================================================================
//  Display — scope area renderer
// ============================================================================

// Helper: draw centered outlined text at given y position and text size.
static inline void monoBassOutlinedCenter(const char* text, int y, uint8_t textSize) {
  display.setTextSize(textSize);
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  drawOutlinedText((128 - w) / 2, y, text);
  display.setTextSize(1);
}

// Render MONOBASS overlay in scope area (note, param, or octave).
// Draws on top of scope waveform using outlined text. Always returns false
// so the oscilloscope continues to render behind.
void renderMonoBassScope(uint32_t nowMs) {
  if (!monoBass.active) return;

  // Priority 1: Parameter knob changed recently
  if (monoBass.paramLabel[0] && (nowMs - monoBass.paramShowStart < MONOBASS_PARAM_DISPLAY_MS)) {
    monoBassOutlinedCenter(monoBass.paramLabel, 24, 1);
    monoBassOutlinedCenter(monoBass.paramValue, 37, 2);
    return;
  }

  // Priority 2: Octave changed recently
  if (monoBass.showOctave && (nowMs - monoBass.octaveShowStart < MONOBASS_OCT_DISPLAY_MS)) {
    char octLabel[8];
    snprintf(octLabel, sizeof(octLabel), "OCT %d", monoBass.octave + 2);
    monoBassOutlinedCenter("MONOBASS", 24, 1);
    monoBassOutlinedCenter(octLabel, 37, 2);
    return;
  }

  // Priority 3: Note display (temporary — shown for MONOBASS_NOTE_DISPLAY_MS after key press)
  if (monoBass.topKey() >= 0 && (nowMs - monoBass.noteShowStart < MONOBASS_NOTE_DISPLAY_MS)) {
    int8_t top = monoBass.topKey();
    uint8_t midiNote = (36 + monoBass.octave * 12) + top;
    if (midiNote > 75) midiNote = 75;
    char noteName[8];
    formatChromaNote(midiNote, noteName);
    monoBassOutlinedCenter(noteName, 34, 2);
    return;
  }

  // All displays expired — clear flags
  monoBass.showOctave = false;
  monoBass.paramLabel[0] = '\0';
}

#endif // MONOBASS_H
