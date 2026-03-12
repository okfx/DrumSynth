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

static constexpr uint32_t D1_MONOBASS_LONG_PRESS_MS = 6000;
static constexpr uint32_t MONOBASS_OCT_DISPLAY_MS   = 1200;
static constexpr uint32_t MONOBASS_PARAM_DISPLAY_MS  = 1500;

// ============================================================================
//  State
// ============================================================================

struct MonoBassState {
  bool active = false;
  uint8_t octave = 0;           // 0=OCT2 (C2), 1=OCT3 (C3), 2=OCT4 (C4)
  int8_t activeKey = -1;        // currently pressed key index (-1 = none)
  float releaseMs = 30.0f;      // gate-style release time (knob-controlled)
  bool showOctave = false;      // show large OCT display in scope area
  uint32_t octaveShowStart = 0; // when octave display started (sysTickMs)
  char paramLabel[12] = "";     // short label for large-font param display
  char paramValue[12] = "";     // value string for large-font param display
  uint32_t paramShowStart = 0;  // when param display started
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
  monoBass.activeKey = -1;
  monoBass.showOctave = false;
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
// Returns true if handled (caller should skip normal step logic).
bool handleMonoBassButton(int buttonIndex, bool pressed) {
  if (!monoBass.active) return false;

  if (pressed) {
    // Press — play note
    uint8_t midiNote = (36 + monoBass.octave * 12) + buttonIndex;
    if (midiNote > 75) midiNote = 75;  // clamp to D#5 (table max)
    d1BaseFreq = d1ChromaFreq(midiNote);
    applyD1Freq();
    triggerD1();
    monoBass.activeKey = buttonIndex;
    // Light only the pressed key
    for (int j = 0; j < numSteps; j++) ledShiftReg.set(j, j == buttonIndex);
  } else {
    // Release — gate off, clear active key and LED
    if (monoBass.activeKey == buttonIndex) {
      AudioNoInterrupts();
      d1AmpEnv.noteOff();
      AudioInterrupts();
      monoBass.activeKey = -1;
      for (int j = 0; j < numSteps; j++) ledShiftReg.set(j, LOW);
    }
  }
  return true;
}

// ============================================================================
//  Display — scope area renderer
// ============================================================================

// Render MONOBASS display in scope area (note name, octave, or param).
// Returns true if it drew content (suppresses oscilloscope).
bool renderMonoBassScope(uint32_t nowMs) {
  if (!monoBass.active) return false;

  if (monoBass.activeKey >= 0) {
    // Key held — show note name
    uint8_t midiNote = (36 + monoBass.octave * 12) + monoBass.activeKey;
    if (midiNote > 75) midiNote = 75;
    char noteName[8];
    formatChromaNote(midiNote, noteName);

    display.setTextSize(1);
    int16_t sx1, sy1;
    uint16_t sw, sh;
    display.getTextBounds("MONOBASS", 0, 0, &sx1, &sy1, &sw, &sh);
    display.setCursor((128 - sw) / 2, 24);
    display.print("MONOBASS");

    display.setTextSize(3);
    int16_t nx1, ny1;
    uint16_t nw, nh;
    display.getTextBounds(noteName, 0, 0, &nx1, &ny1, &nw, &nh);
    display.setCursor((128 - nw) / 2, 35);
    display.print(noteName);
    display.setTextSize(1);
    return true;
  }

  if (monoBass.paramLabel[0] && (nowMs - monoBass.paramShowStart < MONOBASS_PARAM_DISPLAY_MS)) {
    // Parameter knob changed — show label + value
    display.setTextSize(1);
    int16_t sx1, sy1;
    uint16_t sw, sh;
    display.getTextBounds(monoBass.paramLabel, 0, 0, &sx1, &sy1, &sw, &sh);
    display.setCursor((128 - sw) / 2, 24);
    display.print(monoBass.paramLabel);

    display.setTextSize(3);
    int16_t nx1, ny1;
    uint16_t nw, nh;
    display.getTextBounds(monoBass.paramValue, 0, 0, &nx1, &ny1, &nw, &nh);
    display.setCursor((128 - nw) / 2, 35);
    display.print(monoBass.paramValue);
    display.setTextSize(1);
    return true;
  }

  if (monoBass.showOctave && (nowMs - monoBass.octaveShowStart < MONOBASS_OCT_DISPLAY_MS)) {
    // Octave knob changed — show large octave
    char octLabel[8];
    snprintf(octLabel, sizeof(octLabel), "OCT %d", monoBass.octave + 2);

    display.setTextSize(1);
    int16_t sx1, sy1;
    uint16_t sw, sh;
    display.getTextBounds("MONOBASS", 0, 0, &sx1, &sy1, &sw, &sh);
    display.setCursor((128 - sw) / 2, 24);
    display.print("MONOBASS");

    display.setTextSize(3);
    int16_t nx1, ny1;
    uint16_t nw, nh;
    display.getTextBounds(octLabel, 0, 0, &nx1, &ny1, &nw, &nh);
    display.setCursor((128 - nw) / 2, 35);
    display.print(octLabel);
    display.setTextSize(1);
    return true;
  }

  // All MONOBASS displays expired
  monoBass.showOctave = false;
  monoBass.paramLabel[0] = '\0';
  return false;
}

#endif // MONOBASS_H
