#ifndef EXT_SYNC_H
#define EXT_SYNC_H
// ============================================================================
//  External Clock Sync — ext_sync.h
//
//  Pulse-driven external clock for drum machine step sequencer.
//  ISR queues steps via pendingStepCount; main loop fires audio via
//  playSequence() → playSequenceCore() (same path as internal clock).
//  For ppqn < 4, ISR queues step A on the pulse; main loop fires
//  deferred steps B/C/D at evenly-spaced timestamps (checkExtSubdivision).
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
extern float bpm;
extern volatile uint8_t ppqn;

// Shared timer (used for internal clock)
extern IntervalTimer stepTimer;

// Transport control (defined in main .ino)
extern void setTransport(TransportState s);
extern void applyMasterGainFromState();
extern void playSequenceCore();
extern uint8_t currentStep;

// ============================================================================
//  CONCURRENCY CONTRACT
//
//  ISR writes (externalClockISR / stepISR):
//    lastPulseMicros           — uint32_t, read in main loop with noInterrupts()
//    lastPulseInterval         — uint32_t, ISR writes, ISR reads (subdivision + lock-in), reset clears
//    extIntervalEMA            — uint32_t, read in main loop with noInterrupts() (glitch filter + BPM display)
//    prevAcceptedInterval      — uint32_t, ISR-only (lock-in similarity check)
//    extPulseCount             — uint8_t, atomic on ARM
//    extStepAcc                — uint8_t, written by ISR, reset by setTransport()
//    wantSwitchToExt           — bool, atomic on ARM, ISR sets, main loop clears
//    pendingStepCount          — uint8_t, atomic on ARM, written by both stepISR and externalClockISR
//    extSubdivRemaining        — uint8_t, atomic on ARM, ISR sets, main loop decrements
//    extSubdivNextUs           — uint32_t, ISR sets, main loop reads with noInterrupts()
//    extSubdivIntervalUs       — uint32_t, ISR sets, main loop reads with noInterrupts()
//
//  Main loop writes (read by ISR):
//    transportState            — uint8_t, atomic on ARM, safe for ISR to read
//    sequencePlaying           — bool, atomic on ARM, safe for ISR to read
//    ppqn                      — volatile uint8_t, main loop writes (PPQN mode), ISR reads. Atomic on ARM.
//
//  Main loop reads/writes (not shared with ISR):
//    currentStep, drum1/2/3Sequence[], triggerD1/D2/D3, updateLEDs()
//    — all accessed only from main loop via playSequence() → playSequenceCore()
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

// ============================================================================
//  State variables — external clock
// ============================================================================

// Pulse timing (ISR-written, main loop reads with noInterrupts)
volatile uint32_t lastPulseMicros = 0;     // Timestamp of last accepted pulse (µs)
volatile uint32_t lastPulseInterval = 0;   // Interval between last two accepted pulses (µs)
volatile uint32_t extIntervalEMA = 0;      // Fast EMA (alpha=0.5) — glitch filter + BPM display
volatile uint32_t prevAcceptedInterval = 0; // Previous accepted interval — lock-in similarity check
volatile uint8_t  extPulseCount = 0;       // Accepted pulse count (3 needed for lock-in — see below)

// Step generation
volatile uint8_t  extStepAcc = 0;          // Accumulator for fractional step tracking (ppqn >= 4)
volatile bool     wantSwitchToExt = false; // ISR→main-loop handoff: ISR sets, main loop clears

// Subdivision scheduling (ppqn < 4): ISR queues step A immediately and sets
// these for main loop to fire deferred steps B/C/D at the right timestamps.
// ISR writes all three on each pulse; main loop reads and decrements remaining.
volatile uint8_t  extSubdivRemaining = 0;  // Deferred steps still to fire (0 = idle)
volatile uint32_t extSubdivNextUs = 0;     // Timestamp when next deferred step is due
volatile uint32_t extSubdivIntervalUs = 0; // Time between subdivided steps (µs)

// ============================================================================
//  State variables — shared (used by both internal and external clock paths)
// ============================================================================

volatile uint8_t  pendingStepCount = 0;    // Step queue: both stepISR and externalClockISR write, main loop consumes

// BPM display (main loop only, not volatile)
float extBpmDisplay = 0.0f;

// ============================================================================
//  ISR Functions
// ============================================================================

// Internal clock step generation — only active when in RUN_INT mode
void stepISR() {
  if (transportState == RUN_INT && sequencePlaying) {
    if (pendingStepCount < 255) {
      pendingStepCount++;
    }
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
  }

  // Good pulse — record timestamp
  lastPulseMicros = nowUs;

  // Cap at 255 to prevent uint8_t overflow (3 needed for lock-in)
  if (extPulseCount < 255) extPulseCount++;

  // Lock-in: switch to RUN_EXT after 2 consecutive similar intervals (3 pulses).
  // Pulse 1 has no interval. Pulse 2 seeds prevAcceptedInterval but prev==0
  // so the similarity test can't pass. Pulse 3 is the earliest possible lock-in.
  // Prevents a noise spike + real pulse from false lock-in.
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

  // Step generation — queue steps for main loop via pendingStepCount.
  // Main loop fires audio via playSequence() → playSequenceCore(), same
  // path as the internal clock.  This avoids triggering audio from ISR
  // context (which blocks audio DMA and causes envelope glitches).
  // Fires both in steady-state RUN_EXT and on the locking pulse
  // so the first step aligns with a real pulse, not a synthesized main-loop call.
  bool canStep = (transportState == RUN_EXT || wantSwitchToExt);
  if (canStep && sequencePlaying) {

    if (ppqn >= STEPS_PER_BEAT) {
      // --- STANDARD PATH (ppqn >= 4): accumulator, at most 1 step per pulse ---
      uint8_t acc = extStepAcc + STEPS_PER_BEAT;
      uint8_t steps = 0;
      while (acc >= ppqn) {
        acc -= ppqn;
        steps++;
      }
      extStepAcc = acc;
      if (steps > 0 && pendingStepCount <= (255 - steps)) {
        pendingStepCount += steps;
      }

    } else {
      // --- SUBDIVISION PATH (ppqn 1 or 2): queue step A now, defer the rest ---
      // Step A fires immediately via pendingStepCount. Remaining steps fire
      // at evenly-spaced timestamps checked by the main loop (checkExtSubdivision).
      //   ppqn=2 → 2 steps/pulse → 1 deferred (step B at pulse + interval/2)
      //   ppqn=1 → 4 steps/pulse → 3 deferred (steps B, C, D)
      if (pendingStepCount < 255) pendingStepCount++;  // step A

      uint8_t stepsPerPulse = STEPS_PER_BEAT / ppqn;
      uint8_t deferred = stepsPerPulse - 1;
      if (deferred > 0 && lastPulseInterval > 0) {
        uint32_t subInterval = lastPulseInterval / stepsPerPulse;
        extSubdivIntervalUs = subInterval;
        extSubdivNextUs = nowUs + subInterval;
        extSubdivRemaining = deferred;
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
// Called from checkExtClockTimeout() after switching transport away from RUN_EXT.
static inline void resetExternalClockState() {
  noInterrupts();
  extPulseCount = 0;
  lastPulseMicros = 0;
  lastPulseInterval = 0;
  extIntervalEMA = 0;
  prevAcceptedInterval = 0;
  pendingStepCount = 0;
  extSubdivRemaining = 0;
  wantSwitchToExt = false;
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
        applyMasterGainFromState();
      }
      resetExternalClockState();
    }
  }
}

// Fire deferred subdivision steps at the right timestamps (ppqn < 4 only).
// Called from main loop after playSequence(). If multiple steps are overdue
// (main loop was slow), skip ahead to the latest one — better to drop a step
// than play it off-beat.
void checkExtSubdivision() {
  // Snapshot all subdivision state atomically — the early-return check must
  // be inside the same noInterrupts block as the reads.  If the ISR fires
  // between an unguarded remaining==0 check and the snapshot, we'd proceed
  // with a stale "not zero" decision but read the NEW pulse's values.
  uint32_t nextDue, subInterval;
  uint8_t remaining;
  noInterrupts();
  remaining = extSubdivRemaining;
  nextDue = extSubdivNextUs;
  subInterval = extSubdivIntervalUs;
  interrupts();

  if (remaining == 0) return;

  // Not yet time for the next deferred step
  if ((int32_t)(micros() - nextDue) < 0) return;

  // Count how many deferred steps are overdue
  uint32_t now = micros();
  uint8_t overdue = 0;
  uint32_t checkTime = nextDue;
  while (overdue < remaining && (int32_t)(now - checkTime) >= 0) {
    overdue++;
    checkTime += subInterval;
  }

  // Skip past any extra overdue steps (advance currentStep silently).
  // numSteps is always 16 (constexpr in hw_setup.h).
  if (overdue > 1) {
    currentStep = (currentStep + overdue - 1) & 0x0F;
  }

  // Fire only the latest overdue step
  playSequenceCore();

  noInterrupts();
  extSubdivRemaining -= overdue;
  extSubdivNextUs = nextDue + (uint32_t)overdue * subInterval;
  interrupts();
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
// Light display-side smoothing (alpha=0.25) damps jitter for stable readout.
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
