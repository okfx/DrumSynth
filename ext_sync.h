#ifndef EXT_SYNC_H
#define EXT_SYNC_H
// ============================================================================
//  External Clock Sync — ext_sync.h
//
//  Pulse-driven external clock for drum machine step sequencer.
//  ISR triggers audio directly (zero-latency) on each accepted pulse.
//  Two-part glitch filter (300µs floor + 25% relative EMA).
//  Lock-in requires 2 consecutive similar intervals.
//  Subdivision timer for ppqn < 4 uses measured interval (not EMA).
//
//  Include AFTER: TransportState enum, stepTimer, sequencer variables,
//  and triggerD1/D2/D3 are declared in the main .ino file.
// ============================================================================

#include <Arduino.h>

// ============================================================================
//  External dependencies — defined in main .ino, referenced here via extern
// ============================================================================

// Transport & sequencer state
extern volatile TransportState transportState;
extern volatile bool sequencePlaying;
extern float bpm;
extern volatile uint8_t ppqn;
extern volatile uint8_t currentStep;

// Sequence data
extern byte drum1Sequence[];
extern byte drum2Sequence[];
extern byte drum3Sequence[];

// Audio triggers
extern void triggerD1();
extern void triggerD2();
extern void triggerD3();

// Bass line mode — ISR reads these to apply per-step frequency
extern bool bassLineModeActive;
extern uint8_t bassLineNote[];
extern float d1BaseFreq;
extern void applyD1Freq();
extern float bassLineFreq(uint8_t midiNote);

// Shared timer (used for both internal clock and ext subdivision)
extern IntervalTimer stepTimer;

// Transport control (defined in main .ino)
extern void setTransport(TransportState s);
extern void applyMasterGainFromState();

// ============================================================================
//  CONCURRENCY CONTRACT
//
//  ISR writes (externalClockISR / subdivISR / triggerStepFromISR):
//    lastPulseMicros           — uint32_t, read in main loop with noInterrupts()
//    lastPulseInterval         — uint32_t, ISR-only (subdivision scheduling)
//    extIntervalEMA            — uint32_t, read in main loop with noInterrupts() (glitch filter + BPM display)
//    prevAcceptedInterval      — uint32_t, ISR-only (lock-in similarity check)
//    extPulseCount             — uint8_t, atomic on ARM
//    extStepAcc                — uint8_t, written by ISR, reset by setTransport()
//    wantSwitchToExt           — bool, atomic on ARM, ISR sets, main loop clears
//    pendingStepCount          — uint8_t, atomic on ARM, written by stepISR only
//    currentStep               — uint8_t, atomic on ARM, ISR writes via triggerStepFromISR
//    ledUpdatePending          — bool, atomic on ARM, ISR sets, main loop clears
//    lastD1/D2/D3TriggerUs     — uint32_t, written by both ISR and main loop
//    subdivRemaining           — uint8_t, ISR writes, setTransport() clears when leaving RUN_EXT
//
//  Main loop writes (read by ISR):
//    transportState            — uint8_t, atomic on ARM, safe for ISR to read
//    sequencePlaying           — bool, atomic on ARM, safe for ISR to read
//    drum1/2/3Sequence[]       — byte arrays, atomic reads from ISR, written by main loop (button presses)
//    ppqn                      — volatile uint8_t, main loop writes (PPQN mode), ISR reads. Atomic on ARM.
//    bassLineModeActive        — bool, atomic on ARM, safe for ISR to read
//    bassLineNote[]            — uint8_t array, atomic reads from ISR, written by main loop (knob handler)
//
//  Rule: All multi-byte (>1 byte) shared variables must be read/written
//  inside noInterrupts()/interrupts() blocks.
// ============================================================================

// ============================================================================
//  Constants
// ============================================================================

static constexpr uint32_t EXT_GLITCH_US = 300;       // Hard floor for glitch rejection (µs)
static constexpr uint32_t EXT_TIMEOUT_US = 2000000;  // No pulse for 2s → fall back to internal
static constexpr uint8_t  STEPS_PER_BEAT = 4;        // 16th-note sequencer: 4 steps per quarter note
static constexpr uint32_t MIN_RETRIGGER_US = 2000;   // 2ms — skip noteOff if retriggered faster

// ============================================================================
//  State variables — external clock
// ============================================================================

// Pulse timing (ISR-written, main loop reads with noInterrupts)
volatile uint32_t lastPulseMicros = 0;     // Timestamp of last accepted pulse (µs)
volatile uint32_t lastPulseInterval = 0;   // Interval between last two accepted pulses (µs)
volatile uint32_t extIntervalEMA = 0;      // Fast EMA (alpha=0.5) — glitch filter + BPM display
volatile uint32_t prevAcceptedInterval = 0; // Previous accepted interval — lock-in similarity check
volatile uint8_t  extPulseCount = 0;       // Accepted pulse count (2 needed for lock-in)

// Step generation
volatile uint8_t  extStepAcc = 0;          // Accumulator for fractional step tracking (ppqn >= 4)
volatile bool     wantSwitchToExt = false; // ISR→main-loop handoff: ISR sets, main loop clears

// Subdivision (ppqn < 4): ISR fires Step A immediately, then arms stepTimer
// for the remaining steps at evenly-spaced intervals from lastPulseInterval.
// Both ISRs run at the same ARM priority — no preemption, no race conditions.
volatile uint8_t  subdivRemaining = 0;     // Deferred steps still to fire (0 = idle, max 3)

// ============================================================================
//  State variables — shared (used by both internal and external clock paths)
// ============================================================================

volatile uint8_t  pendingStepCount = 0;    // Internal clock (stepISR) step queue, capped at 255
volatile bool     ledUpdatePending = false; // ISR sets after step trigger, main loop updates LEDs

// Per-voice retrigger timestamps — written by triggerD1/D2/D3 (called from
// both ISR and main loop), used to avoid noteOff() killing a just-triggered
// envelope when two steps fire in rapid succession.
volatile uint32_t lastD1TriggerUs = 0;
volatile uint32_t lastD2TriggerUs = 0;
volatile uint32_t lastD3TriggerUs = 0;

// BPM display (main loop only, not volatile)
float extBpmDisplay = 0.0f;

// ============================================================================
//  ISR Functions
// ============================================================================

// Called from externalClockISR() — triggers audio directly for zero-latency.
// Uses same triggerD1/D2/D3 helpers as playSequenceCore(), but sets
// ledUpdatePending instead of calling updateLEDs() (SPI unsafe from ISR).
void triggerStepFromISR() {
  // Advance step with wrap.  Bitmask (0x0F) works because numSteps is 16.
  currentStep = (currentStep + 1) & 0x0F;

  if (drum1Sequence[currentStep]) {
    if (bassLineModeActive) {
      d1BaseFreq = bassLineFreq(bassLineNote[currentStep]);
      applyD1Freq();
    }
    triggerD1();
  }
  if (drum2Sequence[currentStep]) triggerD2();
  if (drum3Sequence[currentStep]) triggerD3();

  ledUpdatePending = true;
}

// Internal clock step generation — only active when in RUN_INT mode
void stepISR() {
  if (transportState == RUN_INT && sequencePlaying) {
    if (pendingStepCount < 255) {
      pendingStepCount++;
    }
  }
}

// Subdivision timer ISR — fires deferred steps for ppqn < STEPS_PER_BEAT.
// Called by stepTimer at evenly-spaced intervals during RUN_EXT.
// Runs at same priority as externalClockISR (no preemption between them).
void subdivISR() {
  // Guard: if ppqn was changed to >= STEPS_PER_BEAT while subdivisions were in flight, stop
  if (ppqn >= STEPS_PER_BEAT || subdivRemaining == 0) {
    stepTimer.end();
    subdivRemaining = 0;
    return;
  }

  triggerStepFromISR();
  subdivRemaining--;

  if (subdivRemaining == 0) {
    // All subdivision steps fired — stop the timer
    stepTimer.end();
  }
}

// External clock ISR — attached to EXT_CLK_PIN (pin 12) rising edge
// Pure pulse-driven: every accepted pulse triggers audio directly.
// Two-part glitch filter (hard floor + relative). Lock-in requires
// two consecutive similar intervals. Subdivision uses measured interval.
void externalClockISR() {
  uint32_t nowUs = micros();
  uint32_t prevUs = lastPulseMicros;

  // Two-part glitch filter:
  //   1. Hard floor: reject < 300µs (contact bounce / electrical noise)
  //   2. Relative: reject < 25% of EMA (absurdly fast edges only)
  // Less aggressive than old 45% — allows swing clocks, jittery analog clocks.
  // Skip interval check on first-ever pulse (no previous timestamp)
  if (prevUs != 0) {
    uint32_t interval = nowUs - prevUs;

    // Part 1: hard floor — always applies
    if (interval < EXT_GLITCH_US) return;

    // Part 2: relative — only when EMA is seeded
    uint32_t ema = extIntervalEMA;
    if (ema > 0 && interval < (ema >> 2)) return;

    // Accepted pulse — update raw interval and fast EMA
    lastPulseInterval = interval;

    // Fast EMA (alpha=0.5): glitch filter threshold + BPM display source.
    // Responsive to tempo changes; display-side smoothing handles jitter.
    // Math: newEMA = (oldEMA + interval) / 2, done as integer shift.
    // The (int32_t) cast makes the subtraction signed so >> 1 rounds toward zero.
    extIntervalEMA = (ema == 0)
        ? interval
        : ema + (((int32_t)(interval - ema)) >> 1);
  }

  // Good pulse — record timestamp
  lastPulseMicros = nowUs;

  // Cap at 255 to prevent uint8_t overflow (only 2 needed for lock-in)
  if (extPulseCount < 255) extPulseCount++;

  // Lock-in: switch to RUN_EXT after 2 consecutive intervals within 25%.
  // Prevents a noise spike + real pulse from false lock-in.
  // prevAcceptedInterval holds previous pulse's interval;
  // lastPulseInterval holds current (0 if this is pulse #1).
  if (extPulseCount >= 2 && transportState != RUN_EXT) {
    uint32_t prev = prevAcceptedInterval;
    uint32_t curr = lastPulseInterval;
    if (prev > 0 && curr > 0) {
      uint32_t diff = (curr > prev) ? (curr - prev) : (prev - curr);
      if (diff < (prev >> 2)) {
        wantSwitchToExt = true;
      }
    }
  }

  // Update previous interval for next lock-in comparison
  prevAcceptedInterval = lastPulseInterval;

  // Step generation — trigger audio directly on each accepted pulse edge.
  // Fires both in steady-state RUN_EXT and on the locking pulse
  // so the first step aligns with a real pulse, not a synthesized main-loop call.
  bool canStep = (transportState == RUN_EXT || wantSwitchToExt);
  if (canStep && sequencePlaying) {

    if (ppqn >= STEPS_PER_BEAT) {
      // --- STANDARD PATH (ppqn >= 4): accumulator, at most 1 step per pulse ---
      uint8_t acc = extStepAcc + STEPS_PER_BEAT;
      while (acc >= ppqn) {
        acc -= ppqn;
        triggerStepFromISR();
      }
      extStepAcc = acc;

    } else {
      // --- SUBDIVISION PATH (ppqn 1 or 2): fire Step A now, defer the rest ---
      // Without this, all steps would bunch at the pulse edge (e.g., ppqn=2
      // fires 2 steps in the same microsecond instead of spacing them out).

      // Cancel any in-flight subdivision from the previous pulse (PLL-like)
      stepTimer.end();
      subdivRemaining = 0;

      // Step A fires immediately on the pulse edge (zero latency)
      triggerStepFromISR();

      // Calculate how many more steps to fire between now and the next pulse:
      //   ppqn=2 → stepsPerPulse=2 → 1 deferred step  (B)
      //   ppqn=1 → stepsPerPulse=4 → 3 deferred steps (B, C, D)
      uint8_t stepsPerPulse = STEPS_PER_BEAT / ppqn;
      uint8_t deferred = stepsPerPulse - 1;

      // Arm subdivision timer using the measured interval (not EMA).
      // Direct: subdivisions match the current pulse spacing immediately,
      // no EMA lag when tempo changes.
      uint32_t measuredInterval = lastPulseInterval;
      if (deferred > 0 && measuredInterval > 0) {
        // Subdivide the pulse interval evenly:
        //   ppqn=2: subInterval = interval / 2  (one step halfway)
        //   ppqn=1: subInterval = interval / 4  (three steps at 1/4, 2/4, 3/4)
        uint32_t subInterval = measuredInterval / stepsPerPulse;

        // Safety floor: don't fire faster than MIN_RETRIGGER_US (2ms)
        if (subInterval < MIN_RETRIGGER_US) subInterval = MIN_RETRIGGER_US;

        subdivRemaining = deferred;
        stepTimer.begin(subdivISR, subInterval);
      }
      // If measuredInterval == 0 (first pulse, no interval yet), only Step A fires.
      // Subsequent pulses will have a valid interval and will arm subdivisions.
    }
  }

}

// ============================================================================
//  Helper Functions
// ============================================================================

// (Re)start the internal clock timer based on current BPM
void rearmStepTimer() {
  // 60,000,000 µs/min ÷ STEPS_PER_BEAT ÷ bpm = µs per step
  float stepPeriodUs = 60000000.0f / (STEPS_PER_BEAT * bpm);
  if (stepPeriodUs < 500.0f) stepPeriodUs = 500.0f;
  stepTimer.end();
  stepTimer.begin(stepISR, (uint32_t)stepPeriodUs);
}

// Clear all external clock state — called after setTransport() on timeout.
// extStepAcc and subdivRemaining are already cleared by setTransport(),
// so only pulse-tracking and display state is reset here.
static inline void resetExternalClockState() {
  noInterrupts();
  extPulseCount = 0;
  lastPulseMicros = 0;
  lastPulseInterval = 0;
  extIntervalEMA = 0;
  prevAcceptedInterval = 0;
  ledUpdatePending = false;
  interrupts();
  extBpmDisplay = 0.0f;
}

// ============================================================================
//  Main-loop helpers — call these from loop() instead of inline blocks
// ============================================================================

// Handle ISR→main-loop lock-in handoff (wantSwitchToExt flag)
void checkExtClockLockIn() {
  bool switchToExt = false;
  noInterrupts();
  if (wantSwitchToExt) {
    wantSwitchToExt = false;
    switchToExt = true;
  }
  interrupts();

  if (switchToExt) {
    // The ISR already fired the first step on the locking pulse (extPulseCount == 2).
    // Main loop just needs to formalize the transport state. Don't reset currentStep
    // here — ISR already advanced it. Only reset extStepAcc if the ISR hasn't
    // touched it yet (it has, so we leave it alone).
    setTransport(RUN_EXT);
  }
}

// Detect external clock timeout — fall back to internal or stop
void checkExtClockTimeout() {
  if (transportState == RUN_EXT) {
    uint32_t now = micros();
    uint32_t lastPulseCopy;

    noInterrupts();
    lastPulseCopy = lastPulseMicros;
    interrupts();

    if ((now - lastPulseCopy) > EXT_TIMEOUT_US) {
      if (sequencePlaying) {
        setTransport(RUN_INT);
      } else {
        setTransport(STOPPED);
        applyMasterGainFromState();
      }
      resetExternalClockState();
    }
  }
}

// Check if external clock is running steadily — call from main loop only.
// Returns true if ISR has been tracking a valid clock within the timeout window.
// Used by the PLAY button to skip RUN_INT and go straight to RUN_EXT,
// so the first step fires on the next real pulse with no internal-clock gap.
bool isExtClockRunning() {
  noInterrupts();
  uint8_t  count = extPulseCount;
  uint32_t ema   = extIntervalEMA;
  uint32_t lastP = lastPulseMicros;
  interrupts();

  if (count < 2 || ema == 0 || lastP == 0) return false;
  return (micros() - lastP) < EXT_TIMEOUT_US;
}

// Derive EXT BPM from the fast EMA (alpha=0.5) — same interval the engine
// uses for glitch filtering, so the display matches what the engine hears.
// Light display-side smoothing (alpha=0.25) damps jitter without the long
// settle time the old slow EMA had.
void updateExtBpmDisplay() {
  if (transportState == RUN_EXT) {
    noInterrupts();
    uint32_t emaCopy = extIntervalEMA;
    interrupts();
    if (emaCopy > 0) {
      float rawBpm = 60000000.0f / ((float)emaCopy * (float)ppqn);
      if (extBpmDisplay <= 0.0f) {
        extBpmDisplay = rawBpm;           // First reading — snap immediately
      } else {
        extBpmDisplay += 0.25f * (rawBpm - extBpmDisplay);  // Light smoothing
      }
    }
  }
}

#endif // EXT_SYNC_H
