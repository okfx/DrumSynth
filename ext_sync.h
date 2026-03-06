#ifndef EXT_SYNC_H
#define EXT_SYNC_H
// ============================================================================
//  External Clock Sync — ext_sync.h
//
//  Pulse-driven external clock for drum machine step sequencer.
//  ISR advances currentStep at pulse time and queues via pendingStepCount;
//  main loop fires audio via playSequence() → playSequenceCore().
//  For ppqn < 4, ISR queues step A on the pulse; a hardware one-shot
//  timer (subdivTimer) queues deferred steps B/C/D at intervals derived
//  from a slow-filtered period (synthInterval, alpha=1/8) that resists jitter.
//  Two-part glitch filter (300µs floor + 40% relative EMA).
//  Lock-in requires 3 accepted pulses (2 consecutive similar intervals).
//
//  Include AFTER: TransportState enum, stepTimer, sequencer variables
//  are declared in the main .ino file.
// ============================================================================

#include <Arduino.h>

// ============================================================================
//  External dependencies — defined in main .ino, referenced here via extern
// ============================================================================

// Transport & sequencer state
extern volatile TransportState transportState;
extern volatile bool sequencePlaying;
extern volatile bool armed;
extern volatile uint16_t armPulseCountdown;
extern float bpm;
extern volatile uint8_t ppqn;

// Shared timer (used for internal clock)
extern IntervalTimer stepTimer;

// Transport control (defined in main .ino)
extern void setTransport(TransportState s);
extern void applyMasterGain();
extern volatile uint8_t currentStep;

// ============================================================================
//  CONCURRENCY CONTRACT
//
//  ISR writes (externalClockISR / stepISR / subdivTimerCallback):
//    lastPulseMicros           — uint32_t, read in main loop with noInterrupts()
//    lastPulseInterval         — uint32_t, ISR writes, ISR reads (lock-in), reset clears
//    extIntervalEMA            — uint32_t, read in main loop with noInterrupts() (glitch filter + BPM display + subdivision fallback)
//    synthIntervalUs           — uint32_t, ISR writes (externalClockISR), ISR reads (subdivision path). Slow EMA for step B placement. Reset by resetExternalClockState().
//    prevAcceptedInterval      — uint32_t, ISR-only (lock-in similarity check)
//    extPulseCount             — uint8_t, atomic on ARM
//    extStepAcc                — uint8_t, written by ISR, reset by setTransport() and PLAY handler
//    wantSwitchToExt           — bool, atomic on ARM, ISR sets, main loop clears (checkExtClockLockIn, resetExternalClockState, STOP handler)
//    pendingStepCount          — uint8_t, atomic on ARM, ISR writes (stepISR, externalClockISR, subdivTimerCallback); main loop clears (playSequence) and sets (PLAY snap-to-pulse). Always written alongside currentStep.
//    subdivIntervalUs          — uint32_t, ISR sets, timer callback reads (chaining)
//    subdivStepsRemaining      — uint8_t, atomic on ARM, ISR sets, timer callback decrements, main loop resets
//    subdivTimerDueUs          — uint32_t, ISR sets, timer callback updates, main loop reads (SPI push guard)
//
//  Main loop writes (read by ISR):
//    transportState            — uint8_t, atomic on ARM, safe for ISR to read
//    sequencePlaying           — bool, atomic on ARM, safe for ISR to read
//    armed                     — bool, atomic on ARM. Main loop sets (PLAY handler), ISR clears (countdown complete → fires step 0).
//    armPulseCountdown         — uint16_t, atomic on ARM (aligned LDRH/STRH). Main loop sets (PLAY handler), ISR decrements.
//    ppqn                      — volatile uint8_t, main loop writes (PPQN mode), ISR reads. Atomic on ARM.
//
//  ISR writes, main loop reads:
//    currentStep               — uint8_t, atomic on ARM. ISR advances on each step
//                                (stepISR, externalClockISR, subdivTimerCallback).
//                                Main loop reads (playSequence snapshot, LED display).
//                                PLAY handler writes inside noInterrupts (snap-to-pulse init).
//
//  Main loop only (not shared with ISR):
//    d1/d2/d3Sequence[], triggerD1/D2/D3, updateLEDs()
//    — accessed only from main loop via playSequence() → playSequenceCore()
//
//  Rule: On ARM Cortex-M7, naturally-aligned 32-bit loads/stores are
//  single-instruction atomic (LDR/STR).  We still use noInterrupts()
//  when reading multiple related variables that must be consistent with
//  each other (e.g. snapshotting lastPulseMicros + extIntervalEMA together).
//
//  ISR priority: All ISR sources are explicitly configured to NVIC priority
//  128 (GPIO pinned in setup(), IntervalTimer default). Same priority = no
//  preemption, making non-atomic RMW (pendingStepCount++, currentStep) safe.
// ============================================================================

// ============================================================================
//  Constants
// ============================================================================

static constexpr uint32_t EXT_GLITCH_US = 300;            // Hard floor for glitch rejection (µs)
static constexpr uint32_t EXT_TIMEOUT_US = 2000000;       // No pulse for 2s → fall back to internal
static constexpr uint8_t  STEPS_PER_BEAT = 4;             // 16th-note sequencer: 4 steps per quarter note
static constexpr uint32_t SUBDIV_MIN_DELAY_US = 50;       // Floor for one-shot timer to avoid immediate refire
static constexpr uint8_t  SYNTH_INTERVAL_SHIFT = 3;       // EMA shift for synthInterval (3 = alpha 1/8, 4 = alpha 1/16)

// ============================================================================
//  State variables — external clock
// ============================================================================

// Pulse timing (ISR-written, main loop reads with noInterrupts)
volatile uint32_t lastPulseMicros = 0;     // Timestamp of last accepted pulse (µs)
volatile uint32_t lastPulseInterval = 0;   // Interval between last two accepted pulses (µs)
volatile uint32_t extIntervalEMA = 0;      // Fast EMA (alpha=0.5) — glitch filter + BPM display
volatile uint32_t synthIntervalUs = 0;     // Slow EMA (alpha=1/2^SYNTH_INTERVAL_SHIFT) — subdivision placement only
volatile uint32_t prevAcceptedInterval = 0; // Previous accepted interval — lock-in similarity check
volatile uint8_t  extPulseCount = 0;       // Accepted pulse count (3 needed for lock-in — see below)

// Step generation
volatile uint8_t  extStepAcc = 0;          // Accumulator for fractional step tracking (ppqn >= 4)
volatile bool     wantSwitchToExt = false; // ISR→main-loop handoff: ISR sets, main loop clears

// Subdivision scheduling (ppqn < 4): ISR queues step A on the pulse, then arms
// a hardware one-shot timer to queue steps B (and C/D for ppqn=1) at precise
// intervals. Timer callback chains to the next step if more remain.
IntervalTimer     subdivTimer;              // One-shot hardware timer for subdivision steps
volatile uint8_t  subdivStepsRemaining = 0; // Countdown: deferred steps left to fire (0 = idle)
volatile uint32_t subdivIntervalUs = 0;     // Time between subdivision steps (µs)
volatile uint32_t subdivTimerDueUs = 0;     // Absolute timestamp when next one-shot fires (0 = not armed)

// ============================================================================
//  State variables — shared (used by both internal and external clock paths)
// ============================================================================

volatile uint8_t  pendingStepCount = 0;    // Step queue: stepISR, externalClockISR, and subdivTimerCallback write; main loop consumes

// BPM display (main loop only, not volatile)
float extBpmDisplay = 0.0f;

// ============================================================================
//  ISR Functions
// ============================================================================

// Internal clock step generation — only active when in RUN_INT mode.
// Advances currentStep at ISR time so the step boundary is anchored to the
// timer tick, not the main loop. Main loop fires audio via playSequence().
// NOTE: pendingStepCount++ and currentStep mutation are non-atomic RMW
// (LDRB/ADD/STRB). This is safe because all three ISR sources run at NVIC
// priority 128: stepISR and subdivTimerCallback share IRQ_PIT (same physical
// interrupt, cannot preempt each other by definition), and externalClockISR
// on IRQ_GPIO6789 is explicitly pinned to 128 in setup(). Same priority =
// no preemption. If ISR priorities are ever differentiated, protect these
// with __LDREXB/__STREXB or noInterrupts().
void stepISR() {
  if (transportState == RUN_INT && sequencePlaying && !wantSwitchToExt) {
    if (pendingStepCount < 255) {
      currentStep = (currentStep + 1) % numSteps;
      pendingStepCount++;
    }
  }
}

// One-shot subdivision timer callback — advances currentStep and queues the
// next deferred step, then chains to the following one if more remain.
// Uses absolute timestamps to avoid drift from callback processing time.
void subdivTimerCallback() {
  // Staleness guard: if a new pulse reconfigured the timer after this
  // callback was NVIC-pending, subdivTimerDueUs now points far into the
  // future.  A valid callback fires at or after its due time (early <= 0).
  // A stale one fires microseconds after the ISR, with early ≈ subInterval.
  // 1ms threshold gives huge margin — musical subdivisions are always > 10ms.
  int32_t early = (int32_t)(subdivTimerDueUs - micros());
  if (early > 1000) return;  // stale — don't call end(), new timer is running

  subdivTimer.end();  // one-shot: stop (this is the valid callback)

  if (pendingStepCount < 255) {
    currentStep = (currentStep + 1) % numSteps;
    pendingStepCount++;
  }

  subdivStepsRemaining--;

  // Chain: arm next one-shot if more deferred steps remain
  if (subdivStepsRemaining > 0) {
    uint32_t nextDue = subdivTimerDueUs + subdivIntervalUs;
    uint32_t now = micros();
    int32_t remaining = (int32_t)(nextDue - now);
    if (remaining < (int32_t)SUBDIV_MIN_DELAY_US) remaining = SUBDIV_MIN_DELAY_US;
    subdivTimerDueUs = nextDue;
    subdivTimer.begin(subdivTimerCallback, (uint32_t)remaining);
  } else {
    subdivTimerDueUs = 0;
  }
}

// External clock ISR — attached to EXT_CLK_PIN (pin 12) rising edge
// Every accepted pulse queues steps via pendingStepCount for main loop.
// Two-part glitch filter (hard floor + relative). Lock-in requires
// two consecutive similar intervals (3 pulses).
void externalClockISR() {
  uint32_t nowUs = micros();
  uint32_t prevUs = lastPulseMicros;
  // Two-part glitch filter:
  //   1. Hard floor: reject < 300µs (contact bounce / electrical noise)
  //   2. Relative: reject < 40% of EMA (filters noise but allows swing/jitter)
  // Skip interval check on first-ever pulse (no previous timestamp)
  if (prevUs != 0) {
    uint32_t interval = nowUs - prevUs;

    // Part 1: hard floor — always applies
    if (interval < EXT_GLITCH_US) return;

    // Part 2: relative — only when EMA is seeded
    // 40% of EMA = ema * 2 / 5 (integer math, no float)
    uint32_t ema = extIntervalEMA;
    if (ema > 0 && interval < (ema * 2 / 5)) return;

    // Accepted pulse — update raw interval and fast EMA
    lastPulseInterval = interval;

    // Fast EMA (alpha=0.5): glitch filter threshold + BPM display source.
    // Responsive to tempo changes; display-side smoothing handles jitter.
    // First interval (ema==0) seeds the EMA; subsequent intervals smooth.
    // Math: newEMA = (oldEMA + interval) / 2, done as integer shift.
    // The (int32_t) cast makes the subtraction signed so >> 1 rounds toward zero.
    extIntervalEMA = (ema == 0)
        ? interval
        : ema + (((int32_t)(interval - ema)) >> 1);

    // Slow EMA for subdivision placement — doesn't chase jitter like the fast EMA.
    // At 80 BPM / 2 PPQN, 5 ms of pulse jitter shifts synthInterval by only 0.625 ms.
    // Seeded on first interval; subsequent intervals smooth with alpha = 1/2^SYNTH_INTERVAL_SHIFT.
    synthIntervalUs = (synthIntervalUs == 0)
        ? interval
        : synthIntervalUs + (((int32_t)(interval - synthIntervalUs)) >> SYNTH_INTERVAL_SHIFT);
  }

  // Good pulse — record timestamp
  lastPulseMicros = nowUs;

  // Cap at 255 to prevent uint8_t overflow (3 needed for lock-in)
  if (extPulseCount < 255) extPulseCount++;

  // Lock-in: switch to RUN_EXT after 2 consecutive similar intervals (3 pulses).
  // Pulse 1 has no interval. Pulse 2 seeds prevAcceptedInterval. Pulse 3 is the
  // earliest possible lock-in (first time both prev and curr intervals are valid).
  // Prevents a noise spike + real pulse from false lock-in.
  // Guard: only lock in while playing — prevents auto-switch to RUN_EXT after
  // the user presses STOP. Pulse timing state is preserved across STOP so
  // the PLAY handler detects ext clock immediately on the next press.
  if (extPulseCount >= 3 && transportState != RUN_EXT && sequencePlaying) {
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

  // ---- Armed count-in: count one full cycle of pulses, then fire step 0 ----
  // PLAY sets armed=true and armPulseCountdown=pulsesPerCycle. The DrumSynth
  // is silent while counting down. Each accepted pulse decrements the counter.
  // When it reaches 0, we fire step 0 on a pulse boundary aligned with the
  // Volca's next step 0 (assuming the user pressed PLAY near step 0).
  if (armed && transportState == RUN_EXT && sequencePlaying) {
    armPulseCountdown--;
    if (armPulseCountdown > 0) return;  // Still counting — stay silent

    // Countdown complete — fire step 0
    armed = false;

    subdivTimer.end();  // safety — should already be ended by PLAY handler

    currentStep = 0;
    if (pendingStepCount < 255) pendingStepCount++;

    // Arm subdivision for deferred steps (step B for ppqn=2, B/C/D for ppqn=1).
    // Uses slow-filtered synthInterval — same source as the normal subdivision path.
    uint8_t p = ppqn;
    if (p < STEPS_PER_BEAT) {
      uint8_t stepsPerPulse = STEPS_PER_BEAT / p;
      uint8_t deferred = stepsPerPulse - 1;
      uint32_t pulseInterval = synthIntervalUs;
      if (pulseInterval == 0) pulseInterval = extIntervalEMA;
      if (deferred > 0 && pulseInterval > 0) {
        uint32_t subInterval = pulseInterval / stepsPerPulse;
        subdivIntervalUs = subInterval;
        subdivStepsRemaining = deferred;
        subdivTimerDueUs = nowUs + subInterval;
        subdivTimer.begin(subdivTimerCallback, subInterval);
      }
    }

    extStepAcc = 0;  // Reset accumulator for clean start
    return;
  }

  // ---- Normal step generation (steady-state) ----
  // Advance currentStep and queue for main loop via pendingStepCount.
  // currentStep is advanced HERE at pulse time so the step boundary is
  // anchored to the external pulse, not the main loop. Main loop fires
  // audio via playSequence() → playSequenceCore().
  // Fires both in steady-state RUN_EXT and on the locking pulse
  // so the first step aligns with a real pulse, not a synthesized main-loop call.
  bool canStep = (transportState == RUN_EXT || wantSwitchToExt);
  if (canStep && sequencePlaying) {

    // Snapshot ppqn once so the branch and arithmetic see the same value.
    // ppqn is volatile (main loop writes it in PPQN mode).
    uint8_t p = ppqn;

    if (p >= STEPS_PER_BEAT) {
      // --- STANDARD PATH (ppqn >= 4): accumulator, at most 1 step per pulse ---
      uint8_t acc = extStepAcc + STEPS_PER_BEAT;
      uint8_t steps = 0;
      while (acc >= p) {
        acc -= p;
        steps++;
      }
      extStepAcc = acc;
      if (steps > 0 && pendingStepCount <= (255 - steps)) {
        currentStep = (currentStep + steps) % numSteps;
        pendingStepCount += steps;
      }

    } else {
      // --- SUBDIVISION PATH (ppqn 1 or 2): queue step A now, defer the rest ---
      // Step A fires immediately via pendingStepCount. Remaining steps are
      // queued by a hardware one-shot timer at precise intervals.
      //   ppqn=2 → 2 steps/pulse → 1 deferred (step B)
      //   ppqn=1 → 4 steps/pulse → 3 deferred (steps B, C, D)

      // Cancel any in-flight subdivision chain from the previous pulse.
      // Prevents stale deferred steps from firing after a tempo change or
      // jitter event. The staleness guard in subdivTimerCallback handles
      // the rare case where the old callback is already NVIC-pending.
      subdivTimer.end();

      if (pendingStepCount < 255) {
        currentStep = (currentStep + 1) % numSteps;  // step A — advance at pulse time
        pendingStepCount++;
      }

      uint8_t stepsPerPulse = STEPS_PER_BEAT / p;
      uint8_t deferred = stepsPerPulse - 1;
      // Subdivision placement uses the slow-filtered synthInterval (alpha=1/8)
      // instead of raw or 2-sample-averaged intervals. This prevents pulse
      // jitter from shifting step B — at 80 BPM, 5 ms of jitter only moves
      // synthInterval by 0.625 ms. Falls back to fast EMA before seeded.
      uint32_t pulseInterval = synthIntervalUs;
      if (pulseInterval == 0) pulseInterval = extIntervalEMA;
      if (deferred > 0 && pulseInterval > 0) {
        uint32_t subInterval = pulseInterval / stepsPerPulse;
        subdivIntervalUs = subInterval;
        subdivStepsRemaining = deferred;
        subdivTimerDueUs = nowUs + subInterval;
        subdivTimer.begin(subdivTimerCallback, subInterval);
      }
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

// Clear all external clock state — pulse tracking, subdivision, and display.
// Called from checkExtClockTimeout() (clock lost) and PPQN mode entry.
// NOT called from STOP — STOP does a partial reset preserving pulse timing
// so the PLAY handler detects ext clock immediately on the next press.
void resetExternalClockState() {
  subdivTimer.end();
  noInterrupts();
  extPulseCount = 0;
  lastPulseMicros = 0;
  lastPulseInterval = 0;
  extIntervalEMA = 0;
  synthIntervalUs = 0;
  prevAcceptedInterval = 0;
  pendingStepCount = 0;
  subdivStepsRemaining = 0;
  subdivTimerDueUs = 0;
  wantSwitchToExt = false;
  armed = false;
  armPulseCountdown = 0;
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
    // The ISR already queued the first step(s) on the locking pulse via
    // pendingStepCount. Main loop just needs to formalize the transport state.
    // playSequence() will consume those queued steps on this or next iteration.
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
        applyMasterGain();
      }
      resetExternalClockState();
    }
  }
}

// Derive EXT BPM from the fast EMA (alpha=0.5) — same interval the engine
// uses for glitch filtering, so the display matches what the engine hears.
// Light display-side smoothing (alpha=0.25) damps jitter for stable readout.
// Updates whenever external pulses are being received (even while STOPPED),
// so the user can see the incoming sync tempo before pressing PLAY.
// Clears to 0 when pulses stop (timeout).
void updateExtBpmDisplay() {
  noInterrupts();
  uint32_t emaCopy = extIntervalEMA;
  uint32_t lastPCopy = lastPulseMicros;
  interrupts();

  // Show ext BPM if we've received pulses recently (within timeout)
  if (lastPCopy != 0 && (micros() - lastPCopy) < EXT_TIMEOUT_US && emaCopy > 0) {
    float rawBpm = 60000000.0f / ((float)emaCopy * (float)ppqn);
    if (extBpmDisplay <= 0.0f) {
      extBpmDisplay = rawBpm;           // First reading — snap immediately
    } else {
      extBpmDisplay += 0.25f * (rawBpm - extBpmDisplay);  // Light smoothing
    }
  } else {
    extBpmDisplay = 0.0f;  // No recent pulses — clear display
  }
}

#endif // EXT_SYNC_H
