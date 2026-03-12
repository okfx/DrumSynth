#ifndef BASS_KEYS_H
#define BASS_KEYS_H
// ============================================================================
//  BASS KEYS — bass_keys.h
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

static constexpr uint32_t D1_BASS_KEYS_LONG_PRESS_MS = 6000;
static constexpr uint32_t BASS_KEYS_OCT_DISPLAY_MS   = 1200;
static constexpr uint32_t BASS_KEYS_PARAM_DISPLAY_MS  = 1500;

// ============================================================================
//  State
// ============================================================================

struct BassKeysState {
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

extern BassKeysState bassKeys;

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

// Enter BASS KEYS mode: stop transport, configure gate envelope.
void enterBassKeysMode() {
  bassKeys.active = true;
  // Stop transport if running
  noInterrupts();
  sequencePlaying = false;
  setTransport(STOPPED);
  armed = false;
  armPulseCountdown = 0;
  pendingStepCount = 0;
  wantSwitchToExt = false;
  interrupts();
  bassKeys.activeKey = -1;
  bassKeys.showOctave = false;
  // Gate-style bass synth envelope: fast attack, full sustain, short release
  AudioNoInterrupts();
  d1AmpEnv.attack(2.0f);       // Moog-like punch (~2ms)
  d1AmpEnv.hold(0.0f);         // no hold phase — sustain takes over
  d1AmpEnv.decay(0.0f);        // no decay — sustain at full level
  d1AmpEnv.sustain(1.0f);      // hold at full amplitude while key pressed
  d1AmpEnv.release(bassKeys.releaseMs);
  AudioInterrupts();
}

// Exit BASS KEYS mode: restore drum envelope.
void exitBassKeysMode() {
  bassKeys.active = false;
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

// Handle a step button press/release in BASS KEYS mode.
// Returns true if handled (caller should skip normal step logic).
bool handleBassKeysButton(int buttonIndex, bool pressed) {
  if (!bassKeys.active) return false;

  if (pressed) {
    // Press — play note
    uint8_t midiNote = (36 + bassKeys.octave * 12) + buttonIndex;
    if (midiNote > 75) midiNote = 75;  // clamp to D#5 (table max)
    d1BaseFreq = d1ChromaFreq(midiNote);
    applyD1Freq();
    triggerD1();
    bassKeys.activeKey = buttonIndex;
    // Light only the pressed key
    for (int j = 0; j < numSteps; j++) ledShiftReg.set(j, j == buttonIndex);
  } else {
    // Release — gate off, clear active key and LED
    if (bassKeys.activeKey == buttonIndex) {
      AudioNoInterrupts();
      d1AmpEnv.noteOff();
      AudioInterrupts();
      bassKeys.activeKey = -1;
      for (int j = 0; j < numSteps; j++) ledShiftReg.set(j, LOW);
    }
  }
  return true;
}

// ============================================================================
//  Display — scope area renderer
// ============================================================================

// Render BASS KEYS display in scope area (note name, octave, or param).
// Returns true if it drew content (suppresses oscilloscope).
bool renderBassKeysScope(uint32_t nowMs) {
  if (!bassKeys.active) return false;

  if (bassKeys.activeKey >= 0) {
    // Key held — show note name
    uint8_t midiNote = (36 + bassKeys.octave * 12) + bassKeys.activeKey;
    if (midiNote > 75) midiNote = 75;
    char noteName[8];
    formatChromaNote(midiNote, noteName);

    display.setTextSize(1);
    int16_t sx1, sy1;
    uint16_t sw, sh;
    display.getTextBounds("BASS KEYS", 0, 0, &sx1, &sy1, &sw, &sh);
    display.setCursor((128 - sw) / 2, 24);
    display.print("BASS KEYS");

    display.setTextSize(3);
    int16_t nx1, ny1;
    uint16_t nw, nh;
    display.getTextBounds(noteName, 0, 0, &nx1, &ny1, &nw, &nh);
    display.setCursor((128 - nw) / 2, 35);
    display.print(noteName);
    display.setTextSize(1);
    return true;
  }

  if (bassKeys.paramLabel[0] && (nowMs - bassKeys.paramShowStart < BASS_KEYS_PARAM_DISPLAY_MS)) {
    // Parameter knob changed — show label + value
    display.setTextSize(1);
    int16_t sx1, sy1;
    uint16_t sw, sh;
    display.getTextBounds(bassKeys.paramLabel, 0, 0, &sx1, &sy1, &sw, &sh);
    display.setCursor((128 - sw) / 2, 24);
    display.print(bassKeys.paramLabel);

    display.setTextSize(3);
    int16_t nx1, ny1;
    uint16_t nw, nh;
    display.getTextBounds(bassKeys.paramValue, 0, 0, &nx1, &ny1, &nw, &nh);
    display.setCursor((128 - nw) / 2, 35);
    display.print(bassKeys.paramValue);
    display.setTextSize(1);
    return true;
  }

  if (bassKeys.showOctave && (nowMs - bassKeys.octaveShowStart < BASS_KEYS_OCT_DISPLAY_MS)) {
    // Octave knob changed — show large octave
    char octLabel[8];
    snprintf(octLabel, sizeof(octLabel), "OCT %d", bassKeys.octave + 2);

    display.setTextSize(1);
    int16_t sx1, sy1;
    uint16_t sw, sh;
    display.getTextBounds("BASS KEYS", 0, 0, &sx1, &sy1, &sw, &sh);
    display.setCursor((128 - sw) / 2, 24);
    display.print("BASS KEYS");

    display.setTextSize(3);
    int16_t nx1, ny1;
    uint16_t nw, nh;
    display.getTextBounds(octLabel, 0, 0, &nx1, &ny1, &nw, &nh);
    display.setCursor((128 - nw) / 2, 35);
    display.print(octLabel);
    display.setTextSize(1);
    return true;
  }

  // All BASS KEYS displays expired
  bassKeys.showOctave = false;
  bassKeys.paramLabel[0] = '\0';
  return false;
}

#endif // BASS_KEYS_H
