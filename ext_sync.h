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
extern volatile int currentStep;

// Sequence data
extern byte drum1Sequence[];
extern byte drum2Sequence[];
extern byte drum3Sequence[];

// Audio triggers
extern void triggerD1();
extern void triggerD2();
extern void triggerD3();

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
//    currentStep               — int (32-bit aligned), atomic on ARM, ISR writes via triggerStepFromISR
//    ledUpdatePending          — bool, atomic on ARM, ISR sets, main loop clears
//    lastD1/D2/D3TriggerUs     — uint32_t, written by both ISR and main loop
//    subdivRemaining           — uint8_t, ISR + resetExternalClockState (inside noInterrupts)
//    subdivIntervalUs          — uint32_t, ISR + resetExternalClockState (inside noInterrupts)
//
//  Main loop writes (read by ISR):
//    transportState                    — uint8_t, atomic on ARM, safe for ISR to read
//    sequencePlaying           — bool, atomic on ARM, safe for ISR to read
//    drum1/2/3Sequence[]       — byte arrays, atomic reads from ISR, written by main loop (button presses)
//    ppqn                      — volatile uint8_t, main loop writes (PPQN mode), ISR reads. Atomic on ARM.
//
//  Rule: All multi-byte (>1 byte) shared variables must be read/written
//  inside noInterrupts()/interrupts() blocks.
// ============================================================================

// ============================================================================
//  State variables
// ============================================================================

// ISR-written external clock state
volatile uint32_t lastPulseMicros = 0;     // Timestamp of last valid pulse (ISR+main)
volatile uint32_t lastPulseInterval = 0;   // Interval between last two accepted pulses (µs)
volatile uint32_t extIntervalEMA = 0;      // Fast EMA (alpha=0.5) — glitch filter + BPM display source
volatile uint32_t prevAcceptedInterval = 0; // Previous accepted interval — lock-in similarity check
volatile uint8_t  extPulseCount = 0;       // Counts valid pulses (2 needed to switch to EXT)
volatile uint8_t  extStepAcc = 0;          // Step accumulator (fractional step tracking)
volatile uint8_t  pendingStepCount = 0;    // For internal clock (stepISR), capped at 255
volatile bool     ledUpdatePending = false; // ISR sets after step trigger, main loop updates LEDs

// ISR→main-loop handoff: ISR sets this flag, main loop acts on it
volatile bool wantSwitchToExt = false;

// Per-voice retrigger timestamps — ISR-written, used to avoid noteOff()
// killing a just-triggered envelope when two steps fire in rapid succession
volatile uint32_t lastD1TriggerUs = 0;
volatile uint32_t lastD2TriggerUs = 0;
volatile uint32_t lastD3TriggerUs = 0;
static constexpr uint32_t MIN_RETRIGGER_US = 2000;  // 2ms — skip noteOff if retriggered faster

// BPM derived from external clock interval (main loop only, not volatile)
float extBpmDisplay = 0.0f;

// Hard floor for glitch rejection: 300µs catches contact bounce / electrical noise
static constexpr uint32_t EXT_GLITCH_US = 300;

// Timeout: if no pulse for 2 seconds, fall back to internal clock
static constexpr uint32_t EXT_TIMEOUT_US = 2000000;

// Subdivision timer state — for ppqn < 4, the external clock ISR fires Step A
// immediately, then arms stepTimer to fire the remaining steps (B, C, D) at
// evenly-spaced intervals derived from lastPulseInterval (measured, not EMA).
// Both ISRs (externalClockISR + subdivISR) run at the same ARM priority — they
// cannot preempt each other, so no race conditions between them.
volatile uint8_t  subdivRemaining = 0;     // Deferred steps still to fire (0 = idle, max 3)
volatile uint32_t subdivIntervalUs = 0;    // Microseconds between subdivision steps

// ============================================================================
//  ISR Functions
// ============================================================================

// Called from externalClockISR() — triggers audio directly for zero-latency.
// Uses same triggerD1/D2/D3 helpers as playSequenceCore(), but sets
// ledUpdatePending instead of calling updateLEDs() (SPI unsafe from ISR).
void triggerStepFromISR() {
  // Guard against corruption: clamp to valid range, then advance.
  // Setting to numSteps-1 means the +1 below wraps to step 0 (clean restart).
  if (currentStep < 0 || currentStep >= numSteps) {
    currentStep = numSteps - 1;
  }
  currentStep = (currentStep + 1) % numSteps;

  if (drum1Sequence[currentStep]) triggerD1();
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

// Subdivision timer ISR — fires deferred steps for ppqn < 4.
// Called by stepTimer at subdivIntervalUs intervals during RUN_EXT.
// Runs at same priority as externalClockISR (no preemption between them).
void subdivISR() {
  // Guard: if ppqn was changed to >= 4 while subdivisions were in flight, stop
  if (ppqn >= 4 || subdivRemaining == 0) {
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
    // Equivalent to (ema + interval) / 2 — fast integer average
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

    if (ppqn >= 4) {
      // --- STANDARD PATH (ppqn >= 4): accumulator, at most 1 step per pulse ---
      uint8_t acc = extStepAcc + 4;
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
      uint8_t stepsPerPulse = 4 / ppqn;
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
        subdivIntervalUs = subInterval;
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
  float stepPeriodUs = 15000000.0f / bpm;
  if (stepPeriodUs < 500.0f) stepPeriodUs = 500.0f;
  stepTimer.end();
  stepTimer.begin(stepISR, (uint32_t)stepPeriodUs);
}

// Clear all external clock state — called on timeout or transport reset
static inline void resetExternalClockState() {
  noInterrupts();
  extPulseCount = 0;
  extStepAcc = 0;
  lastPulseMicros = 0;
  lastPulseInterval = 0;
  extIntervalEMA = 0;
  prevAcceptedInterval = 0;
  ledUpdatePending = false;
  // Per-voice retrigger timestamps (lastD1/D2/D3TriggerUs) not reset here —
  // they self-correct on next trigger.
  subdivRemaining = 0;
  subdivIntervalUs = 0;
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
