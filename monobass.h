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

static constexpr uint32_t MONOBASS_OCT_DISPLAY_MS    = 1200;
static constexpr uint32_t MONOBASS_PARAM_DISPLAY_MS  = 1200;
static constexpr uint32_t MONOBASS_NOTE_DISPLAY_MS   = 1200;
// kEnvFiltCeiling defined in DrumSynth.ino before this include
static constexpr uint32_t GRID_SHOW_MS     = 10000;  // idle grid visible for 10s
static constexpr uint32_t GRID_DISSOLVE_MS = 1000;   // Bayer dissolve animation duration

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
  char paramLabel[16] = "";     // short label for large-font param display
  char paramValue[16] = "";     // value string for large-font param display
  uint32_t paramShowStart = 0;  // when param display started
  uint32_t noteShowStart = 0;   // when note display was triggered
  bool savedWfChroma = false;   // wfChromaMode state before entering MONOBASS
  uint32_t modeEnteredAt = 0;   // sysTickMs when MONOBASS was entered (for splash grid)

  // Envelope filter state (snap knob repurposed)
  float envFiltDepth = 0.0f;    // 0–1 from snap knob — how far filter opens above base
  float envFiltBaseHz = 100.0f; // base cutoff in Hz (cached from body knob)
  uint32_t envFiltTrigger = 0;  // sysTickMs of last noteOn

  // Last-note-priority key stack for polyphonic trill
  int8_t heldKeys[kMaxHeldKeys];
  uint8_t heldCount = 0;

  int8_t topKey() const { return heldCount > 0 ? heldKeys[heldCount - 1] : -1; }
};

extern MonoBassState monoBass;

// Idle grid dissolve state
static uint32_t monoGridShowStartMs = 0;  // when the idle grid first appeared
static bool monoGridDismissed = false;     // true after dissolve completes

// ============================================================================
//  External dependencies — defined in main .ino
// ============================================================================

extern Adafruit_SH1106G display;
extern float d1BaseFreq;
extern volatile uint32_t sysTickMs;
extern AudioFilterStateVariable d1LowPass;
extern AudioFilterStateVariable d1HighPass;
extern AudioFilterBiquad        d1EQ;
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
// chroma.h is included before monobass.h — chroma vars/funcs are in scope.
// Functions defined in eeprom.h (included before monobass.h)
extern void saveMonoBassStatusToEEPROM(bool active);
extern void applyD1Freq();
extern void triggerD1();
extern void setTransport(TransportState s);
extern void updateLEDs();
extern void drawOutlinedText(int x, int y, const char* text);
extern void applyKnobToEngine(uint8_t idx, int knobValue);
extern ResponsiveAnalogRead* analog[];
extern bool monoBassKeyEvent;
// Animation enum and state — defined in .ino before this header is included
extern MonoAnimPhase monoAnimPhase;
// bayerDither() is static inline in DrumSynth.ino, visible here via single TU

// ============================================================================
//  Entry / exit
// ============================================================================

// Enter MONOBASS mode: stop transport, configure gate envelope.
void enterMonoBassMode() {
  monoBass.active = true;
  monoBass.modeEnteredAt = sysTickMs;
  saveMonoBassStatusToEEPROM(true);
  // Force wavefolder into chroma mode (save previous state for exit)
  monoBass.savedWfChroma = wfChromaMode;
  wfChromaMode = true;
  // Stop transport if running
  noInterrupts();
  sequencePlaying = false;
  setTransport(STOPPED);
  armed = false;
  armPulseCountdown = 0;
  pendingStepCount = 0;
  wantSwitchToExt = false;
  interrupts();
  monoBass.heldCount = 0;  // heldKeys contents don't matter when heldCount == 0
  monoBass.showOctave = false;
  monoBass.noteShowStart = 0;
  monoGridShowStartMs = 0;
  monoGridDismissed = false;
  // Clear any chroma note-select state so it doesn't persist across mode switch
  d1ChromaHeldStep = -1;
  d2ChromaHeldStep = -1;
  d3ChromaHeldStep = -1;
  // Gate-style bass synth envelope: fixed fast attack, full sustain, short release
  AudioNoInterrupts();
  d1AmpEnv.attack(0.5f);       // fixed minimum — snap knob repurposed as env filter
  d1AmpEnv.hold(0.0f);         // no hold phase — sustain takes over
  d1AmpEnv.decay(0.0f);        // no decay — sustain at full level
  d1AmpEnv.sustain(1.0f);      // hold at full amplitude while key pressed
  d1AmpEnv.release(monoBass.releaseMs);
  // Retune filters for bass: lower the highpass to preserve sub-bass fundamentals,
  // open the lowpass wider, and flatten EQ for cleaner bass tone
  d1HighPass.frequency(30.0f);   // 30 Hz — passes bass fundamentals down to B0
  d1HighPass.resonance(0.7f);    // flat passband, no resonant peak
  d1LowPass.frequency(4000.0f);  // open wider so harmonics breathe
  d1LowPass.resonance(0.8f);     // gentle resonance
  d1EQ.setNotch(0, 600.0f, 0.5f);          // mild notch — tames boxiness
  d1EQ.setHighShelf(1, 2000.0f, 0.0f, 1.0f); // flat shelf — no cut
  d1EQ.setHighShelf(2, 3500.0f, 0.0f, 1.0f); // flat shelf — no cut
  // Stage 3 intentionally unchanged — retains pre-entry EQ setting.
  // exitMonoBassMode() restores it to lowpass 8 kHz.
  d1VoiceMixer.gain(1, 0.0f);  // mute snap transient — knob 5 is now env filter depth
  d1AmpEnv.noteOff();          // kill any note held over from the sequencer
  AudioInterrupts();
  // Init envelope filter depth from current snap knob position
  float snapNorm = normalizeKnob(analog[5]->getValue());
  monoBass.envFiltDepth = snapNorm * snapNorm;  // square curve, matches engineD1Snap
  monoBass.envFiltTrigger = 0;
  // Light first 12 LEDs as usable-key indicators (12 notes per octave)
  for (int i = 0; i < numSteps; i++) ledShiftReg.setNoUpdate(i, i < 12);
  ledShiftReg.updateRegisters();
}

// Exit MONOBASS mode: restore drum envelope.
void exitMonoBassMode() {
  monoBass.active = false;
  saveMonoBassStatusToEEPROM(false);
  // Restore wavefolder chroma mode to pre-MONOBASS state
  wfChromaMode = monoBass.savedWfChroma;
  AudioNoInterrupts();
  d1AmpEnv.attack(D1_ATTACK_MS);
  d1AmpEnv.hold(d1CachedHoldMs);
  d1AmpEnv.decay(d1EffectiveDecay);
  d1AmpEnv.sustain(0.0f);      // drum mode — decay to silence
  d1AmpEnv.release(5.0f);      // library default
  d1AmpEnv.noteOff();          // silence any lingering gate note
  // Restore HPF: chroma mode keeps 30 Hz for bass fundamentals,
  // normal drum mode uses 85 Hz with resonant peak for body.
  if (d1ChromaMode) {
    d1HighPass.frequency(30.0f);
    d1HighPass.resonance(0.7f);
  } else {
    d1HighPass.frequency(85.0f);
    d1HighPass.resonance(2.0f);
  }
  d1LowPass.frequency(1800.0f);
  d1LowPass.resonance(1.5f);
  d1EQ.setLowpass(3, 8000.0f, 0.707f);
  AudioInterrupts();
  // Re-apply Body/EQ knob to restore d1EQ stages 0-2 from current knob position.
  // enterMonoBassMode() flattened these stages — without this, the kick's EQ
  // stays flat until the user physically moves knob 6.
  applyKnobToEngine(6, analog[6]->getValue());
  // Re-apply snap knob to restore snap transient (was repurposed as attack)
  applyKnobToEngine(5, analog[5]->getValue());
}

// ============================================================================
//  Envelope filter update — call from loop() every ~5ms
// ============================================================================

// Decays d1LowPass from its peak (set on noteOn) back toward envFiltBaseHz.
// Only runs when a note is held and envFiltDepth > 0.
void updateMonoBassEnvFilter(uint32_t nowMs) {
  if (!monoBass.active) return;
  if (monoBass.envFiltDepth < 0.01f) return;
  if (monoBass.topKey() < 0) return;  // no note held — body knob owns the filter
  if (monoBass.envFiltTrigger == 0) return;

  uint32_t elapsed = nowMs - monoBass.envFiltTrigger;
  // Time constant: 0.6× release time, minimum 100ms — floor ensures audible wah sweep
  float tauMs = fmaxf(monoBass.releaseMs * 0.6f + 40.0f, 100.0f);
  float decay = expf(-(float)elapsed / tauMs);  // 1.0 at trigger → 0.0 as time → ∞

  // Fixed ceiling: always sweep to kEnvFiltCeiling at full depth regardless of base.
  // This guarantees a wide, audible wah regardless of where the body knob is set.
  float peak = monoBass.envFiltBaseHz
             + monoBass.envFiltDepth * (kEnvFiltCeiling - monoBass.envFiltBaseHz);
  if (peak > kEnvFiltCeiling) peak = kEnvFiltCeiling;
  float cutoff = monoBass.envFiltBaseHz + (peak - monoBass.envFiltBaseHz) * decay;
  if (cutoff < 20.0f) cutoff = 20.0f;

  // High resonance during the sweep gives the vocal "waaah" character
  float resonance = 1.5f + 2.5f * monoBass.envFiltDepth * decay;  // peaks at 4.0, settles to 1.5

  AudioNoInterrupts();
  d1LowPass.frequency(cutoff);
  d1LowPass.resonance(resonance);
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
  if (buttonIndex >= 12) return true;  // only 12 notes per octave — ignore top 4 keys

  if (pressed) {
    // Skip if already held (debounce edge case — prevents ghost stack entries)
    for (int k = 0; k < monoBass.heldCount; k++) {
      if (monoBass.heldKeys[k] == buttonIndex) return true;
    }
    // Push key onto held-key stack
    if (monoBass.heldCount < MonoBassState::kMaxHeldKeys) {
      monoBass.heldKeys[monoBass.heldCount++] = buttonIndex;
    } else {
      // Stack full — drop oldest, shift left, push new at end
      for (int k = 1; k < MonoBassState::kMaxHeldKeys; k++) {
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
    // Envelope filter: open to peak cutoff at note attack
    monoBass.envFiltTrigger = sysTickMs;
    if (monoBass.envFiltDepth > 0.01f) {
      float peak = monoBass.envFiltBaseHz
                 + monoBass.envFiltDepth * (kEnvFiltCeiling - monoBass.envFiltBaseHz);
      if (peak > kEnvFiltCeiling) peak = kEnvFiltCeiling;
      AudioNoInterrupts();
      d1LowPass.frequency(peak);
      d1LowPass.resonance(1.5f + 2.5f * monoBass.envFiltDepth);  // up to 4.0 at full depth
      AudioInterrupts();
    }

  } else {
    // Remove released key from stack
    bool found = false;
    for (int k = 0; k < monoBass.heldCount; k++) {
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
      // Envelope filter: re-trigger on legato fallback (same formula as press)
      monoBass.envFiltTrigger = sysTickMs;
      if (monoBass.envFiltDepth > 0.01f) {
        float peak = monoBass.envFiltBaseHz
                   + monoBass.envFiltDepth * (kEnvFiltCeiling - monoBass.envFiltBaseHz);
        if (peak > kEnvFiltCeiling) peak = kEnvFiltCeiling;
        AudioNoInterrupts();
        d1LowPass.frequency(peak);
        d1LowPass.resonance(1.5f + 2.5f * monoBass.envFiltDepth);
        AudioInterrupts();
      }
    } else {
      // No keys held — gate off
      AudioNoInterrupts();
      d1AmpEnv.noteOff();
      AudioInterrupts();
    }
  }

  // Light first 12 LEDs (usable keys) — single SPI transaction.
  // Common to all paths: press, legato fallback, and gate-off.
  for (int i = 0; i < numSteps; i++) ledShiftReg.setNoUpdate(i, i < 12);
  ledShiftReg.updateRegisters();
  monoBassKeyEvent = true;  // tell loop() to skip next OLED frame for lower latency
  return true;
}

// ============================================================================
//  Display — scope area renderer
// ============================================================================

// Helper: draw a 7×7 pixel knob icon centered at (cx, cy).
static inline void drawKnobIcon(int cx, int cy) {
  // Top/bottom arcs
  display.drawPixel(cx - 1, cy - 3, 1);
  display.drawPixel(cx,     cy - 3, 1);
  display.drawPixel(cx + 1, cy - 3, 1);
  display.drawPixel(cx - 1, cy + 3, 1);
  display.drawPixel(cx,     cy + 3, 1);
  display.drawPixel(cx + 1, cy + 3, 1);
  // Corner pixels
  display.drawPixel(cx - 2, cy - 2, 1);
  display.drawPixel(cx + 2, cy - 2, 1);
  display.drawPixel(cx - 2, cy + 2, 1);
  display.drawPixel(cx + 2, cy + 2, 1);
  // Side columns
  display.drawPixel(cx - 3, cy - 1, 1);
  display.drawPixel(cx - 3, cy,     1);
  display.drawPixel(cx - 3, cy + 1, 1);
  display.drawPixel(cx + 3, cy - 1, 1);
  display.drawPixel(cx + 3, cy,     1);
  display.drawPixel(cx + 3, cy + 1, 1);
  // Center dot + indicator line up
  display.drawPixel(cx, cy,     1);
  display.drawPixel(cx, cy - 1, 1);
  display.drawPixel(cx, cy - 2, 1);
}

// Draw the 4×2 knob reference grid in the scope area (idle state).
// After GRID_SHOW_MS, dissolves via Bayer dither over GRID_DISSOLVE_MS.
static void renderMonoBassIdleGrid(uint32_t nowMs) {
  if (monoGridDismissed) return;

  if (monoGridShowStartMs == 0) {
    monoGridShowStartMs = nowMs;
  }

  uint32_t elapsed = nowMs - monoGridShowStartMs;
  if (elapsed >= GRID_SHOW_MS + GRID_DISSOLVE_MS) {
    monoGridDismissed = true;
    return;
  }
  static constexpr int kRowStartY  = 24;
  static constexpr int kRowHeight  = 10;
  static constexpr int kLeftX      = 4;
  static constexpr int kRightEdge  = 125;
  static constexpr int kLeftKnobCx = 56;
  static constexpr int kRightKnobCx = 68;

  static const char* const leftLabels[]  = { "OCTAVE", "DECAY", "OSC", "WAVFLDR" };
  static const char* const rightLabels[] = { "VOLUME", "ENV FLT", "FILTER", "DLY SND" };

  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);

  for (int row = 0; row < 4; row++) {
    int rowY = kRowStartY + row * kRowHeight;

    // Left label — left-justified
    display.setCursor(kLeftX, rowY);
    display.print(leftLabels[row]);

    // Right label — right-justified
    int rLen = strlen(rightLabels[row]);
    display.setCursor(kRightEdge - rLen * 6, rowY);
    display.print(rightLabels[row]);

    // Two knob icons per row, vertically centered with text
    int knobCy = rowY + 3;
    drawKnobIcon(kLeftKnobCx, knobCy);
    drawKnobIcon(kRightKnobCx, knobCy);
  }

  // Bayer dissolve: erase content area pixels progressively after GRID_SHOW_MS
  if (elapsed > GRID_SHOW_MS) {
    float dp = (float)(elapsed - GRID_SHOW_MS) / (float)GRID_DISSOLVE_MS;
    for (int dy = 22; dy < 64; ++dy)
      for (int dx = 0; dx < 128; ++dx)
        if (bayerDither(dx, dy, dp))
          display.drawPixel(dx, dy, SH110X_BLACK);
  }
}

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
// Draws on top of scope waveform using outlined text.
void renderMonoBassScope(uint32_t nowMs) {
  if (!monoBass.active) return;

  // Reset idle grid timer whenever a note is playing or an overlay is active
  if (monoBass.topKey() >= 0) {
    monoGridShowStartMs = 0;
    monoGridDismissed = false;
  }

  // Priority 1: Parameter knob changed recently
  if (monoBass.paramLabel[0] && (nowMs - monoBass.paramShowStart < MONOBASS_PARAM_DISPLAY_MS)) {
    monoGridShowStartMs = 0;
    monoGridDismissed = false;
    monoBassOutlinedCenter(monoBass.paramLabel, 24, 1);
    monoBassOutlinedCenter(monoBass.paramValue, 37, 2);
    return;
  }

  // Priority 2: Octave changed recently
  if (monoBass.showOctave && (nowMs - monoBass.octaveShowStart < MONOBASS_OCT_DISPLAY_MS)) {
    char octLabel[8];
    snprintf(octLabel, sizeof(octLabel), "OCT %d", monoBass.octave + 2);
    if (monoAnimPhase != MONO_ANIM_EXITING)
      monoBassOutlinedCenter("MONOBASS", 24, 1);
    monoBassOutlinedCenter(octLabel, 37, 2);
    return;
  }

  // Priority 3: Note display — bottom-left corner to maximize scope visibility
  if (monoBass.topKey() >= 0 && (nowMs - monoBass.noteShowStart < MONOBASS_NOTE_DISPLAY_MS)) {
    int8_t top = monoBass.topKey();
    uint8_t midiNote = (36 + monoBass.octave * 12) + top;
    if (midiNote > 75) midiNote = 75;
    char noteName[8];
    formatChromaNote(midiNote, noteName);
    display.setTextSize(1);
    drawOutlinedText(8, 54, noteName);
    return;
  }

  // All displays expired — clear flags
  monoBass.showOctave = false;
  monoBass.paramLabel[0] = '\0';

  // Priority 4: note still playing → let scope waveform show through
  if (monoBass.topKey() >= 0) return;

  // Priority 5: idle — show knob reference grid (with timed dissolve)
  renderMonoBassIdleGrid(nowMs);
}

#endif // MONOBASS_H
