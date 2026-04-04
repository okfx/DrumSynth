// buttons.h — Button handler state machine, combo dispatch, and scan loop.
//
// Defines ButtonHandler struct, ComboModState, X-combo dispatch table,
// all btn*Press/Release/Hold handlers, and updateOtherButtons().
//
// Depends on: hw_setup.h (mux, LEDs, constants), audiotool.h (audio objects),
// chroma.h (startChromaRamp, cancelChromaAnim), monobass.h (MonoBassState),
// eeprom.h (save/load), ext_sync.h (resetExternalClockState),
// knob_handlers.h (applyKnobToEngine).
// All included before this header in DrumSynth.ino.

#pragma once

// --- Extern: audio objects (from audiotool.h, already in scope) ---
extern AudioFilterStateVariable d1HighPass;
extern AudioFilterStateVariable d1LowPass;
extern AudioMixer4              d1VoiceMixer;

// --- Extern: state variables (defined in .ino above include point) ---
extern bool d1ChromaMode;
extern bool d2ChromaMode;
extern bool d3ChromaMode;
extern float d1FreqBeforeChroma;
extern float d1BaseFreq;
extern float d1ChromaEnvFiltDepth;
extern uint32_t d1ChromaEnvFiltTrigger;
extern int8_t d1ChromaHeldStep;
extern int8_t d2ChromaHeldStep;
extern int8_t d3ChromaHeldStep;

extern uint8_t d1Sequence[];
extern uint8_t d2Sequence[];
extern uint8_t d3Sequence[];
extern uint8_t d1ChromaNote[];
extern uint8_t d2ChromaNote[];
extern uint8_t d3ChromaNote[];

extern char displayParameter1[];
extern char displayParameter2[];
extern uint32_t parameterOverlayStartTick;
extern RailMode activeRail;

extern bool ppqnModeActive;
extern uint32_t ppqnModeLastActivityTick;
extern uint8_t ppqnModeSelection;
extern volatile uint8_t ppqn;
extern volatile TransportState transportState;
extern volatile bool sequencePlaying;
extern volatile uint32_t sysTickMs;
extern IntervalTimer stepTimer;
extern volatile uint8_t currentStep;

extern MonoAnimPhase monoAnimPhase;
extern uint32_t monoAnimStart;
extern MonoBassState monoBass;

extern volatile SwingMode swingMode;

// --- Extern: functions (defined in .ino or earlier headers) ---
extern void selectTrack(Track t);
extern void updateLEDs();
extern void flashStepLed(uint8_t stepIndex, uint8_t count);
extern void applyD1Freq();
extern void applyMasterGain();
extern void handlePlayStop();
extern void setTransport(TransportState s);
extern void cancelMonoAnim();

// --- Button handler state machine ---

struct ButtonHandler {
  uint32_t pressTick = 0;
  bool enteredMode = false;
  bool enteredMode2 = false;

  void (*onPress)(ButtonHandler& self, uint32_t nowTick);
  void (*onRelease)(ButtonHandler& self, uint32_t nowTick);
  void (*onHoldCheck)(ButtonHandler& self, uint32_t nowTick, uint32_t heldMs);
};

// Combo button (MEM) modifier state machine.
// Combo button is purely a modifier — no short-press action.
// Any button pressed while combo is held fires a combo immediately.
// Combo held alone → MONOBASS timer.
struct ComboModState {
  bool held;               // true while combo button is physically held
  uint32_t pressTick;      // when combo button was pressed
  bool comboFired;         // true if any combo triggered during this hold
  bool monoAnimStarted;    // true if MONOBASS warning animation has begun
  bool monoFired;          // true if MONOBASS mode was entered/exited
};
static ComboModState comboMod = {};

// Track active combo for hold-duration combos (e.g. X+LOAD for PPQN)
static int8_t activeComboIndex = -1;

// --- Combo action functions (combo button held + second button) ---

static void comboChromaD1() {
  startChromaRamp(0, !d1ChromaMode);
  d1ChromaMode = !d1ChromaMode;
  patternDirty = true;
  if (d1ChromaMode) {
    d1FreqBeforeChroma = d1BaseFreq;
    selectTrack(TRACK_D1);
    AudioNoInterrupts();
    d1HighPass.frequency(30.0f);
    d1HighPass.resonance(1.5f);
    // Restore snap transient to ~50% (pitchMod=0.80, gain=0.925)
    d1Snap.pitchMod(0.80f);
    d1VoiceMixer.gain(1, 0.925f);
    AudioInterrupts();
    d1ChromaEnvFiltTrigger = 0;
    // Body knob (6) drives envelope filter, snap knob (5) drives attack
    applyKnobToEngine(6, analog[6]->getValue());
    applyKnobToEngine(5, analog[5]->getValue());
  } else {
    d1ChromaHeldStep = -1;
    d1ChromaEnvFiltTrigger = 0;
    d1BaseFreq = d1FreqBeforeChroma;
    applyD1Freq();
    if (!monoBass.active) {
      AudioNoInterrupts();
      d1HighPass.frequency(85.0f);
      d1HighPass.resonance(2.0f);
      d1LowPass.frequency(1800.0f);
      d1LowPass.resonance(1.5f);
      AudioInterrupts();
    }
    applyKnobToEngine(5, analog[5]->getValue());
    applyKnobToEngine(6, analog[6]->getValue());
  }
}

static void comboChromaD2() {
  startChromaRamp(1, !d2ChromaMode);
  d2ChromaMode = !d2ChromaMode;
  patternDirty = true;
  if (d2ChromaMode) {
    selectTrack(TRACK_D2);
  } else {
    d2ChromaHeldStep = -1;
  }
  applyKnobToEngine(8, analog[8]->getValue());
}

static void comboChromaD3() {
  startChromaRamp(2, !d3ChromaMode);
  d3ChromaMode = !d3ChromaMode;
  patternDirty = true;
  if (d3ChromaMode) {
    selectTrack(TRACK_D3);
  } else {
    d3ChromaHeldStep = -1;
  }
  applyKnobToEngine(16, analog[16]->getValue());
}


// Reset all pattern state to blank defaults (sequences, chroma notes, chroma modes, swing).
// Called when loading an empty or corrupt EEPROM slot.
// swingMode write is intentionally unguarded: the ISR reads it via swungStepPeriodUs()
// but the caller (btnLoadPress / combo+step) always also calls stepTimer.update() which
// re-arms the timer with the new swing setting, so a torn read is harmless.
static void clearPatternState() {
  for (int s = 0; s < numSteps; s++) {
    d1Sequence[s] = 0; d2Sequence[s] = 0; d3Sequence[s] = 0;
    d1ChromaNote[s] = 36; d2ChromaNote[s] = 48; d3ChromaNote[s] = 48;
  }
  d1ChromaMode = false; d2ChromaMode = false;
  d3ChromaMode = false;
  swingMode = SWING_OFF;
}

// Cycle swing mode: OFF → 1 → 2 → … → 14 → OFF.
// Shows overlay "SWING OFF" or "SWING xx%".
// Live-updates the step timer if internal clock is running.
static void cycleSwing(uint32_t nowTick) {
  uint8_t next = (uint8_t)swingMode + 1;
  swingMode = (next > SWING_14) ? SWING_OFF : (SwingMode)next;
  patternDirty = true;
  // Big centered swing overlay (800 ms) instead of standard small overlay
  if (swingMode == SWING_OFF) {
    snprintf(swingOverlayText, sizeof(swingOverlayText), "OFF");
  } else {
    snprintf(swingOverlayText, sizeof(swingOverlayText),
             "%u%%", (unsigned)kSwingPercent[swingMode]);
  }
  swingOverlayStartTick = nowTick;
  // Live-update timer so swing takes effect immediately.
  // Snapshot currentStep under noInterrupts — stepISR can advance it mid-read.
  if (transportState == RUN_INT && sequencePlaying) {
    noInterrupts();
    uint8_t step = currentStep;
    interrupts();
    stepTimer.update(swungStepPeriodUs(step));
  }
}

static void comboPPQN() {
  if (sequencePlaying) {
    noInterrupts();
    sequencePlaying = false;
    setTransport(STOPPED);
    interrupts();
    resetExternalClockState();
    applyMasterGain();
  }
  ppqnModeActive = true;
  ppqnModeLastActivityTick = sysTickMs;
  ppqnModeSelection = ppqn;
}

// Combo button dispatch table.
// holdMs == 0: fires immediately on second button press.
// holdMs  > 0: arms on press, fires when second button hold reaches threshold.
//
// | Btn  | holdMs | Action      |
// |------|--------|-------------|
// |  D1  |      0 | D1 Chroma   |
// |  D2  |      0 | D2 Chroma   |
// |  D3  |      0 | D3 Chroma   |
// | LOAD |   1500 | PPQN select |
struct XComboEntry {
  uint8_t  buttonIndex;
  uint32_t holdMs;
  void     (*onTrigger)();
  const char* label;
};

static const XComboEntry comboTable[] = {
  { 0, 0,                   comboChromaD1, "D1 CHROMA" },
  { 1, 0,                   comboChromaD2, "D2 CHROMA" },
  { 2, 0,                   comboChromaD3, "D3 CHROMA" },
  { 8, PPQN_LONG_PRESS_MS,  comboPPQN,    "PPQN"      },
};
static constexpr int COMBO_COUNT = (int)(sizeof(comboTable) / sizeof(comboTable[0]));

// Dispatch X+button combo. Returns true if a combo was fired or armed.
// Sets comboMod.comboFired and cancels the MONOBASS animation automatically.
// Call from any button's press handler when combo button is held.
// For holdMs>0 entries, arms activeComboIndex — caller must set self.pressTick.
static bool dispatchCombo(uint8_t btnIdx, uint32_t nowTick) {
  if (!comboMod.held) return false;
  for (int i = 0; i < COMBO_COUNT; i++) {
    if (comboTable[i].buttonIndex != btnIdx) continue;
    comboMod.comboFired = true;
    cancelMonoAnim();
    if (comboTable[i].holdMs == 0) {
      comboTable[i].onTrigger();
    } else {
      activeComboIndex = i;
    }
    return true;
  }
  return false;
}

// Helper: show "DISABLED FOR MONOBASS" via scope overlay and mark combo as fired.
// Routes through monoBass.paramLabel/paramValue because the standard bottom
// overlay is suppressed in MONOBASS mode (display_ui.h overlay guard).
static void showMonoBassDisabled(uint32_t /*nowTick*/) {
  snprintf(monoBass.paramLabel, sizeof(monoBass.paramLabel), "DISABLED FOR");
  snprintf(monoBass.paramValue, sizeof(monoBass.paramValue), "MONOBASS");
  monoBass.paramShowStart = sysTickMs;
  activeRail = RAIL_NONE;
  comboMod.comboFired = true;
}

// --- D1 button (index 0): press=selectD1, combo+D1=chroma ---

static void btnD1Press(ButtonHandler& self, uint32_t nowTick) {
  if (comboMod.held) {
    if (monoBass.active) { showMonoBassDisabled(nowTick); }
    else { dispatchCombo(0, nowTick); }
    return;
  }
  if (monoBass.active) { showMonoBassDisabled(nowTick); return; }
  self.pressTick = nowTick;
  selectTrack(TRACK_D1);
}

static void btnD1Release(ButtonHandler& self, uint32_t /*nowTick*/) { (void)self; }
static void btnD1Hold(ButtonHandler& self, uint32_t /*nowTick*/, uint32_t /*heldMs*/) { (void)self; }

// --- D2 button (index 1): press=selectD2, combo+D2=chroma ---

static void btnD2Press(ButtonHandler& self, uint32_t nowTick) {
  if (comboMod.held) {
    if (monoBass.active) { showMonoBassDisabled(nowTick); }
    else { dispatchCombo(1, nowTick); }
    return;
  }
  if (monoBass.active) { showMonoBassDisabled(nowTick); return; }
  self.pressTick = nowTick;
  selectTrack(TRACK_D2);
}

static void btnD2Release(ButtonHandler& self, uint32_t /*nowTick*/) { (void)self; }
static void btnD2Hold(ButtonHandler& self, uint32_t /*nowTick*/, uint32_t /*heldMs*/) { (void)self; }

// --- D3 button (index 2): press=selectD3, combo+D3=chroma ---

static void btnD3Press(ButtonHandler& self, uint32_t nowTick) {
  if (comboMod.held) {
    if (monoBass.active) { showMonoBassDisabled(nowTick); }
    else { dispatchCombo(2, nowTick); }
    return;
  }
  if (monoBass.active) { showMonoBassDisabled(nowTick); return; }
  self.pressTick = nowTick;
  selectTrack(TRACK_D3);
}

static void btnD3Release(ButtonHandler& self, uint32_t /*nowTick*/) { (void)self; }
static void btnD3Hold(ButtonHandler& self, uint32_t /*nowTick*/, uint32_t /*heldMs*/) { (void)self; }

// --- PLAY button (index 6): press=play/stop, combo+PLAY=WF chroma ---

static void btnPlayPress(ButtonHandler& self, uint32_t nowTick) {
  // Combo+PLAY: WF chroma — NOT disabled in MONOBASS
  if (comboMod.held) { dispatchCombo(6, nowTick); return; }
  if (ppqnModeActive) return;  // block transport while PPQN select screen is active
  if (monoBass.active) { showMonoBassDisabled(nowTick); return; }
  self.pressTick = nowTick;
  handlePlayStop();
}

static void btnPlayRelease(ButtonHandler& self, uint32_t /*nowTick*/) {
  (void)self;
}

static void btnPlayHold(ButtonHandler& self, uint32_t /*nowTick*/, uint32_t /*heldMs*/) {
  (void)self;
}

// --- Combo button (index 7, MEM) ---
//
// Purely a modifier — no short-press action in normal mode.
// Hold alone 3500ms: MONOBASS warning animation begins
// Hold alone 8000ms: enter/exit MONOBASS mode
// Any button pressed while combo is held: combo dispatch (see comboTable)
//
// Exception: in PPQN mode, combo short press cycles PPQN values,
// combo long press (1500ms) saves and exits PPQN mode.

static void btnComboPress(ButtonHandler& self, uint32_t nowTick) {
  self.pressTick = nowTick;
  // Reset hold-phase flags (gate PPQN save and MONOBASS anim re-entry)
  self.enteredMode = false;
  self.enteredMode2 = false;

  comboMod.held = true;
  comboMod.pressTick = nowTick;
  comboMod.comboFired = false;
  comboMod.monoAnimStarted = false;
  comboMod.monoFired = false;

  monoAnimPhase = MONO_ANIM_NONE;

  // In MONOBASS, X only does the exit routine — skip combo LED flash
  if (monoBass.active) return;
  // Combo-active LED flashing is handled by updateLEDs() while comboMod.held
}

static void btnComboRelease(ButtonHandler& self, uint32_t nowTick) {
  // PPQN mode special case: short press cycles PPQN value
  if (ppqnModeActive && !comboMod.comboFired && !self.enteredMode) {
    int idx = -1;
    for (int j = 0; j < PPQN_OPTION_COUNT; j++) {
      if (PPQN_OPTIONS[j] == ppqnModeSelection) { idx = j; break; }
    }
    if (idx < 0) idx = 0;  // corrupted value — reset to first option
    ppqnModeSelection = PPQN_OPTIONS[(idx + 1) % PPQN_OPTION_COUNT];
    ppqnModeLastActivityTick = nowTick;
  }

  // Cancel MONOBASS animation if it was in progress but didn't fire
  if (comboMod.monoAnimStarted && !comboMod.monoFired) {
    monoAnimPhase = MONO_ANIM_NONE;
  }

  // Cancel any pending hold-combo. Also reset in btnSaveRelease —
  // both paths needed because either button can be released first.
  activeComboIndex = -1;

  comboMod.held = false;
  self.enteredMode = false;
  self.enteredMode2 = false;
  updateLEDs();  // restore step LEDs from combo-active pattern
}

static void btnComboHold(ButtonHandler& self, uint32_t nowTick, uint32_t heldMs) {
  if (comboMod.comboFired) return;

  // PPQN mode special case: long press saves and exits
  if (ppqnModeActive && !self.enteredMode && heldMs >= PPQN_SAVE_HOLD_MS) {
    self.enteredMode = true;
    ppqn = ppqnModeSelection;
    savePpqnToEEPROM(ppqn);
    ppqnModeActive = false;
    // Clean up stale subdivision timer from any prior external clock session
    subdivTimer.end();
    subdivStepsRemaining = 0;
    subdivTimerDueUs = 0;
    snprintf(displayParameter1, sizeof(displayParameter1), "PPQN %d", ppqn);
    snprintf(displayParameter2, sizeof(displayParameter2), "SAVED");
    parameterOverlayStartTick = nowTick;
    activeRail = RAIL_NONE;
    comboMod.comboFired = true;
    return;
  }

  // MONOBASS warning animation at 3500ms (only if no combo fired, combo button held alone)
  if (!comboMod.monoAnimStarted && heldMs >= COMBO_MONO_ANIM_MS) {
    monoAnimPhase = monoBass.active ? MONO_ANIM_EXITING : MONO_ANIM_ENTERING;
    monoAnimStart = nowTick - (heldMs - COMBO_MONO_ANIM_MS);  // backdate to threshold crossing
    comboMod.monoAnimStarted = true;
  }

  // MONOBASS mode entry/exit at 4000ms
  if (!comboMod.monoFired && heldMs >= COMBO_MONO_ENTER_MS) {
    comboMod.monoFired = true;
    comboMod.comboFired = true;
    cancelMonoAnim();
    cancelChromaAnim();
    if (!monoBass.active) {
      enterMonoBassMode();
    } else {
      exitMonoBassMode();
    }
    updateLEDs();
  }
}

// --- LOAD button (index 8): press=load pattern, combo+LOAD hold=PPQN.

static void btnLoadPress(ButtonHandler& self, uint32_t nowTick) {
  self.pressTick = nowTick;
  if (comboMod.held) {
    if (monoBass.active) { showMonoBassDisabled(nowTick); }
    else { dispatchCombo(8, nowTick); }
    return;
  }
  if (monoBass.active) { showMonoBassDisabled(nowTick); return; }
  slotPending = false;
  LoadResult result = loadStateFromEEPROM(activeSaveSlot);
  if (result != LOAD_OK) {
    clearPatternState();
    updateLEDs();
    patternDirty = false;
    activeRail = RAIL_NONE;
    if (result == LOAD_CORRUPT) {
      // CRC mismatch — slot has valid magic but damaged data.
      // Write a clean empty pattern so the slot is usable again.
      initializeEepromSlot(activeSaveSlot);
      snprintf(displayParameter1, sizeof(displayParameter1), "SLOT %d CORRUPT",
               activeSaveSlot + 1);
      snprintf(displayParameter2, sizeof(displayParameter2), "INITIALIZED");
    } else {
      snprintf(displayParameter1, sizeof(displayParameter1), "SLOT %d",
               activeSaveSlot + 1);
      snprintf(displayParameter2, sizeof(displayParameter2), "EMPTY");
    }
    parameterOverlayStartTick = nowTick;
  }
  flashStepLed(activeSaveSlot, 2);
}

static void btnLoadRelease(ButtonHandler& self, uint32_t /*nowTick*/) {
  (void)self;
  if (activeComboIndex >= 0) activeComboIndex = -1;
}

static void btnLoadHold(ButtonHandler& self, uint32_t /*nowTick*/, uint32_t heldMs) {
  (void)self;
  if (monoBass.active) return;
  if (activeComboIndex >= 0 && activeComboIndex < COMBO_COUNT
      && heldMs >= comboTable[activeComboIndex].holdMs) {
    int idx = activeComboIndex;
    activeComboIndex = -1;
    comboTable[idx].onTrigger();
  }
}

// --- SAVE button (index 9): double-press to confirm, X+SAVE suppressed ---

static uint32_t saveConfirmTick = 0;          // when "press again" was shown
static constexpr uint32_t SAVE_CONFIRM_MS = 2000;  // confirmation window

static void btnSavePress(ButtonHandler& self, uint32_t nowTick) {
  (void)self;
  if (comboMod.held) {
    // X+SAVE has no action — suppress MONOBASS timer only
    comboMod.comboFired = true;
    cancelMonoAnim();
    return;
  }
  if (monoBass.active) { showMonoBassDisabled(nowTick); return; }

  slotPending = false;
  activeRail = RAIL_NONE;

  if (!patternDirty) {
    snprintf(displayParameter1, sizeof(displayParameter1), "NOTHING TO");
    snprintf(displayParameter2, sizeof(displayParameter2), "SAVE");
    parameterOverlayStartTick = nowTick;
    return;
  }

  // Double-press confirmation: first press shows prompt, second press saves
  if ((uint32_t)(nowTick - saveConfirmTick) <= SAVE_CONFIRM_MS) {
    // Second press within window — save
    saveConfirmTick = 0;
    saveStateToEEPROM(activeSaveSlot);
    flashStepLed(activeSaveSlot, 2);
  } else {
    // First press — show confirmation prompt
    saveConfirmTick = nowTick;
    snprintf(displayParameter1, sizeof(displayParameter1), "PRESS AGAIN");
    snprintf(displayParameter2, sizeof(displayParameter2), "TO SAVE");
    parameterOverlayStartTick = nowTick;
  }
}

// --- Null handler for unassigned buttons (3, 4, 5) ---
static void btnNullPress(ButtonHandler& s, uint32_t t) { (void)s; (void)t; }
static void btnNullRelease(ButtonHandler& s, uint32_t t) { (void)s; (void)t; }
static void btnNullHold(ButtonHandler& s, uint32_t t, uint32_t h) { (void)s; (void)t; (void)h; }

// --- Handler table (indices 0–9 match otherButtonsMux channels) ---
static ButtonHandler btnHandlers[otherButtonsCount] = {
  /* 0 D1     */ { 0, false, false, btnD1Press,   btnD1Release,   btnD1Hold   },
  /* 1 D2     */ { 0, false, false, btnD2Press,   btnD2Release,   btnD2Hold   },
  /* 2 D3     */ { 0, false, false, btnD3Press,   btnD3Release,   btnD3Hold   },
  /* 3 n/a    */ { 0, false, false, btnNullPress, btnNullRelease, btnNullHold },
  /* 4 n/a    */ { 0, false, false, btnNullPress, btnNullRelease, btnNullHold },
  /* 5 n/a    */ { 0, false, false, btnNullPress, btnNullRelease, btnNullHold },
  /* 6 PLAY   */ { 0, false, false, btnPlayPress, btnPlayRelease, btnPlayHold },
  /* 7 COMBO  */ { 0, false, false, btnComboPress,    btnComboRelease,    btnComboHold    },
  /* 8 LOAD   */ { 0, false, false, btnLoadPress, btnLoadRelease, btnLoadHold },
  /* 9 SAVE   */ { 0, false, false, btnSavePress, btnNullRelease, btnNullHold },
};

// --- Generic button scan loop ---

void updateOtherButtons() {
  static uint32_t lastDebounceTick[otherButtonsCount] = { 0 };
  static bool btnState[otherButtonsCount] = { false };
  static bool btnLastState[otherButtonsCount] = { false };

  uint32_t nowTick = sysTickMs;  // 32-bit aligned, ARM-atomic read

  for (int i = 0; i < otherButtonsCount; i++) {
    otherButtonsMux.channel(i);
    delayMicroseconds(5);
    bool rawPressed = !otherButtonsMux.read();

    if (rawPressed != btnLastState[i]) {
      lastDebounceTick[i] = nowTick;
    }

    uint16_t debounceMs = (i == 6) ? PLAY_DEBOUNCE_MS : BUTTON_DEBOUNCE_MS;
    if ((uint32_t)(nowTick - lastDebounceTick[i]) > debounceMs && (rawPressed != btnState[i])) {
      btnState[i] = rawPressed;
      if (btnState[i]) {
        btnHandlers[i].onPress(btnHandlers[i], nowTick);
      } else {
        btnHandlers[i].onRelease(btnHandlers[i], nowTick);
      }
    }

    // Continuous hold check — runs every scan iteration while button is held
    if (btnState[i]) {
      uint32_t heldMs = (uint32_t)(nowTick - btnHandlers[i].pressTick);
      btnHandlers[i].onHoldCheck(btnHandlers[i], nowTick, heldMs);
    }

    btnLastState[i] = rawPressed;
  }
}
