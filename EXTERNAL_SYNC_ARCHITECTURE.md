# External Sync Architecture — DrumSynth

Self-contained technical reference for reviewing the external clock synchronization system. Written for an LLM or engineer unfamiliar with the codebase.

---

## 1. Hardware and Signal Model

**Platform:** Teensy 4.0 (ARM Cortex-M7, 600 MHz).

**External clock source:** Korg Volca (or any device sending rising-edge analog clock pulses).
The Volca sends **2 pulses per quarter note (2 PPQN)** — the lowest resolution commonly encountered. The system also supports 1, 4, 8, 24, 48, and 96 PPQN.

**Connection:** Pin 12, configured as `INPUT_PULLDOWN`, with `attachInterrupt(..., RISING)`.

**Sequencer grid:** 16 steps, where 4 steps = 1 beat (16th-note resolution). So 1 bar = 4 beats = 16 steps.

**Relationship between pulses and steps:**
- At 2 PPQN: each pulse = 2 steps (step A fires on the pulse, step B fires at the midpoint via a hardware timer).
- At 4 PPQN: each pulse = 1 step (direct mapping, no subdivision needed).
- At 24 PPQN: 6 pulses = 1 step (accumulator counts pulses until a step is earned).

The fundamental challenge: at 2 PPQN, half the steps must be *synthesized* between pulses. The accuracy of those synthesized steps depends on predicting the interval to the next pulse.

---

## 2. Transport State Machine

```
enum TransportState : uint8_t {
  STOPPED   = 0,
  RUN_INT   = 1,   // Internal timer drives steps
  RUN_EXT   = 2    // External pulse ISR drives steps
};
```

**Transitions:**

```
STOPPED ──[PLAY, no ext clock]──→ RUN_INT
STOPPED ──[PLAY, ext clock detected]──→ RUN_EXT (armed count-in)
RUN_INT ──[ext clock locks in]──→ RUN_EXT (auto-detect)
RUN_EXT ──[2s timeout, still playing]──→ RUN_INT (fallback)
RUN_EXT ──[2s timeout, stopped]──→ STOPPED
RUN_INT/RUN_EXT ──[STOP button]──→ STOPPED
```

Key design choice: **STOP does NOT clear pulse timing state** (`extPulseCount`, `lastPulseMicros`, `extIntervalEMA`, `prevAcceptedInterval`). This lets the PLAY handler detect external clock immediately on the next press without waiting for lock-in again.

---

## 3. Glitch Filter (Two-Part)

Every pulse entering `externalClockISR()` passes through a two-stage filter before acceptance:

**Part 1 — Hard floor (300 µs):** Rejects contact bounce and electrical noise. Any interval < 300 µs is discarded unconditionally.

**Part 2 — Relative (40% of EMA):** Once the EMA is seeded, rejects any interval less than 40% of the current EMA. This filters spurious pulses while permitting musical swing and tempo changes. The 40% threshold is loose enough to allow large tempo jumps (e.g., halving BPM) on the second pulse.

The first pulse is always accepted (no previous timestamp to compare against). The first interval seeds the EMA directly.

**EMA (alpha = 0.5):** After each accepted pulse, the exponential moving average updates:
```
extIntervalEMA = (oldEMA + interval) / 2
```
This serves three purposes:
1. Glitch filter threshold (part 2 above)
2. BPM display source (with light display-side smoothing on top)
3. Subdivision fallback (if `lastPulseInterval` is somehow zero)

---

## 4. Lock-In Detection (Auto-Switch to RUN_EXT)

When the sequencer is playing in `RUN_INT` and external pulses arrive, the ISR checks whether to auto-switch to `RUN_EXT`:

**Requirement:** 3 accepted pulses with 2 consecutive similar intervals.

- Pulse 1: No interval (just a timestamp).
- Pulse 2: First interval computed; seeds `prevAcceptedInterval`. No lock-in yet (only one interval).
- Pulse 3 (earliest possible lock-in): Compares current interval against `prevAcceptedInterval`. If `|curr - prev| < prev/4` (25% tolerance), sets `wantSwitchToExt = true`.

**Why not lock in on 2 pulses?** A noise spike followed by a real pulse would produce one valid-looking interval. Requiring two similar intervals prevents false lock-in.

**ISR→main-loop handoff:** The ISR sets the `wantSwitchToExt` flag (bool, atomic on ARM). The main loop's `checkExtClockLockIn()` reads and clears it inside `noInterrupts()`, then calls `setTransport(RUN_EXT)`. The ISR has already queued the first step(s) on the locking pulse via `pendingStepCount`, so the main loop fires audio immediately.

**Guard:** Lock-in only fires while `sequencePlaying == true`. This prevents auto-switching to `RUN_EXT` after the user presses STOP.

---

## 5. Armed Count-In (Phase-Aligned Start)

When the user presses PLAY with an external clock detected, the sequencer does NOT fire step 0 immediately. Instead:

1. **PLAY handler** (main loop, inside `noInterrupts`):
   - Sets `armed = true`
   - Computes `armPulseCountdown = numSteps × ppqn / STEPS_PER_BEAT`
     - At 2 PPQN: 16 × 2 / 4 = 8 pulses (one full 16-step cycle)
   - Sets `sequencePlaying = true`, `transportState = RUN_EXT`
   - **Grace window:** If the user pressed PLAY within the first half of a pulse interval (i.e., just after a pulse), credits that pulse to the countdown by decrementing `armPulseCountdown` by 1. This forgives slightly-late presses. Guards: only applies if `extIntervalEMA > 0` and keeps countdown ≥ 1.

2. **ISR counts down silently:** Each accepted pulse decrements `armPulseCountdown`. While counting, the ISR returns early — no steps are queued, no audio fires.

3. **Countdown reaches 0:** The ISR clears `armed`, sets `currentStep = 0`, queues it via `pendingStepCount`, and arms subdivision for the remaining steps. This fires step 0 on a pulse boundary aligned with the Volca's next step 0 (assuming the user pressed PLAY near step 0).

**Why a full cycle?** The user presses PLAY "near" the Volca's step 0, but we don't know exactly where in the Volca's cycle we are. Counting one full cycle of pulses guarantees that step 0 fires on the same phase as the Volca's step 0, regardless of when PLAY was pressed.

**Display during count-in:** A 4-3-2-1 countdown is rendered on the OLED in large text (textSize 4). The SPI push is done without the timing guard since no steps are firing during count-in — there's nothing to protect.

---

## 6. Step Generation: Two Paths

### Path A — Accumulator (ppqn ≥ 4)

When each pulse provides ≤ 1 step, a simple accumulator tracks fractional steps:

```
acc += STEPS_PER_BEAT (4)
while (acc >= ppqn):
    acc -= ppqn
    steps++
```

Example at 24 PPQN: each pulse adds 4 to the accumulator. After 6 pulses, `4×6 = 24 ≥ 24`, yielding 1 step. Perfect 6:1 division.

Example at 8 PPQN: each pulse adds 4. After 2 pulses, `4×2 = 8 ≥ 8`, yielding 1 step. 2:1 division.

`currentStep` is advanced in the ISR by the total `steps` count. `pendingStepCount` is incremented by the same amount. No subdivision timer is used.

### Path B — Subdivision (ppqn < 4, i.e., ppqn = 1 or 2)

Each pulse carries multiple steps. Step A fires immediately on the pulse (via `pendingStepCount`). The remaining steps are deferred to a hardware one-shot timer (`IntervalTimer subdivTimer`).

At 2 PPQN: 2 steps per pulse → 1 deferred step (step B at the midpoint).
At 1 PPQN: 4 steps per pulse → 3 deferred steps (B, C, D at equal intervals).

**Cancel-and-rearm on every pulse:** When a new pulse arrives, any in-flight subdivision chain is cancelled (`subdivTimer.end()`) before step A is queued and a new chain is armed. This ensures step B is always calculated from the most recent pulse interval, not a stale one.

**Subdivision interval calculation:**
```
subInterval = pulseInterval / stepsPerPulse
```
Where `pulseInterval` is a 2-sample average (see Section 7).

**Timer callback chaining:** `subdivTimerCallback()` fires step B, then chains to step C (and C to D) by computing the absolute timestamp of the next step and rearming the one-shot timer. Using absolute timestamps avoids drift from callback processing time.

**Staleness guard:** If a new pulse arrives and reconfigures the timer just as an old callback is NVIC-pending, the old callback would fire stale. The callback checks:
```
int32_t early = (subdivTimerDueUs - micros());
if (early > 1000) return;  // stale — don't fire
```
A valid callback fires at or after its due time (early ≤ 0, or small positive from NVIC latency). A stale callback fires microseconds after the ISR reconfigured the timer, with `early ≈ subInterval` (many milliseconds).

---

## 7. Subdivision Smoothing (2-Sample Average)

**Problem:** At 2 PPQN and low BPM, step B is scheduled at the midpoint of the pulse interval. If `lastPulseInterval` jitters by ±5 ms (common with analog clocks), step B shifts by ±2.5 ms — audible as flamming.

**Solution:** Average the current and previous pulse intervals:
```
pulseInterval = (lastPulseInterval + prevInterval) / 2
```

This halves the jitter: a 5 ms error in one measurement moves step B by only 1.25 ms instead of 2.5 ms. The 2-sample window is short enough to track tempo changes within 2 pulses.

**Implementation detail:** `prevInterval` is captured at the very top of the ISR (before `lastPulseInterval` is overwritten):
```
uint32_t prevInterval = lastPulseInterval;  // Capture before overwrite
```

**Fallback chain:**
1. If both `prevInterval > 0` and `lastPulseInterval > 0` → use average
2. If only `lastPulseInterval > 0` (pulse 2, no prev yet) → use raw interval
3. If `lastPulseInterval == 0` (shouldn't happen, but safety) → use `extIntervalEMA`

**Why not a longer EMA?** A display-side EMA (alpha = 0.25) is too slow — it would lag behind tempo changes and misplace steps. The 2-sample average responds within one pulse.

**Why not use `extIntervalEMA` (alpha = 0.5)?** It blends in older pulses and would take longer to converge after a tempo change. The raw 2-sample average is the most responsive option that still provides meaningful smoothing.

This smoothing logic appears in two places (armed count-in path and normal subdivision path) with cross-reference comments to keep them in sync.

---

## 8. Step Queue: ISR → Main Loop

The ISR (any of `externalClockISR`, `subdivTimerCallback`, `stepISR`) advances `currentStep` and increments `pendingStepCount`. The main loop's `playSequence()` function consumes the queue:

```
noInterrupts();
toDo = pendingStepCount;
pendingStepCount = 0;
targetStep = currentStep;
interrupts();
```

**Consumption logic:**
- `toDo == 0` → nothing to do, return.
- `toDo == 1` → fire `targetStep` (the common case).
- `toDo == 2` AND `RUN_EXT` → fire both steps (back-to-back `playSequenceCore` calls). This preserves rhythmic density when the main loop was briefly blocked (e.g., by an OLED push).
- `toDo >= 3` OR internal clock → fire only `targetStep` (skip ahead). A missing step is less noticeable than an off-beat one.

**Why advance `currentStep` in the ISR instead of main loop?** To anchor the step boundary to the hardware event (pulse or timer tick), not the variable-latency main loop. Audio fires a few microseconds later in the main loop, which is inaudible.

---

## 9. OLED SPI Push Guard

The SH1106 OLED uses software SPI bitbang, which takes 15–25 ms per frame. During this time, interrupts are still enabled, but the CPU is busy bit-banging and can't service `pendingStepCount` in the main loop. If a step fires during a push, it waits in the queue — adding up to 25 ms latency.

**`isSafeToPushOled(nowMs)` checks:**

1. **`pendingStepCount > 0`** — Steps waiting. Consume them first. Never block with pending audio work.

2. **`displayBlockedUntilMs`** — 500 ms suppression window after PLAY press. Prevents OLED push during the critical drop-in/count-in phase.

3. **Step B proximity** — If `subdivTimerDueUs` is within 25 ms, skip the push. The subdivision timer callback will fire step B; don't block it with SPI.

4. **Step A proximity** — If `lastPulseMicros + extIntervalEMA` (predicted next pulse) is within 25 ms, skip the push. Don't let SPI bitbang delay step A.

If any check fails, the frame is skipped. A skipped frame is cosmetic; a late step is audible.

---

## 10. Concurrency Model

**ARM Cortex-M7 atomicity:** Naturally-aligned 32-bit and 16-bit loads/stores are single-instruction atomic (LDR/STR, LDRH/STRH). All shared variables are aligned by default.

**ISR priority:** All three ISRs (`externalClockISR`, `stepISR`, `subdivTimerCallback`) run at default NVIC priority 128. Same priority = no preemption between ISRs. The `pendingStepCount++` and `currentStep` mutations are read-modify-write (LDRB/ADD/STRB) which are NOT atomic — but they're safe because no ISR can preempt another at the same priority.

**`noInterrupts()` usage:** Used when the main loop must read multiple related ISR-written variables consistently (e.g., `lastPulseMicros` + `extIntervalEMA` together). Not needed for single-variable reads of aligned types.

**Key invariant:** `pendingStepCount` and `currentStep` are always written together in the same ISR context. The main loop snapshots both inside `noInterrupts()` to get a consistent pair.

**Detailed ownership table:** See the CONCURRENCY CONTRACT comment block at the top of `ext_sync.h` for which variables are ISR-written, main-loop-written, or ISR-only.

---

## 11. Timeout and Fallback

If no pulse arrives for 2 seconds (`EXT_TIMEOUT_US = 2000000`), `checkExtClockTimeout()` fires:
- If still playing → switch to `RUN_INT` (internal clock takes over at current BPM)
- If stopped → switch to `STOPPED`
- In either case, `resetExternalClockState()` clears all pulse tracking, subdivision state, and display BPM

---

## 12. File Map

| File | Role |
|------|------|
| `ext_sync.h` | All ISRs, state variables, glitch filter, lock-in, subdivision timer, count-in countdown, reset/timeout/display helpers |
| `DrumSynth.ino` | Transport state enum, PLAY/STOP handler, `playSequence()`/`playSequenceCore()`, `isSafeToPushOled()`, count-in display rendering, main loop calls to ext_sync helpers |
| `hw_setup.h` | Pin assignments including `EXT_CLK_PIN 12` |
| `eeprom.h` | Pattern save/load (not involved in sync) |

---

## 13. Design Principles

1. **External pulses are hard timing anchors.** Step A fires in the ISR at pulse time. No main-loop latency affects the step boundary.

2. **Better to skip a step than play it off-beat.** If the main loop falls behind by 3+ steps, only the latest step fires.

3. **Audio and timing always take priority over display.** OLED pushes are skipped if a step is imminent. The SPI push guard enforces this.

4. **Cancel and rearm, don't accumulate.** Each new pulse cancels any in-flight subdivision and starts fresh. This prevents stale intervals from producing incorrectly-placed steps.

5. **Phase alignment over immediate gratification.** PLAY doesn't fire step 0 immediately — it waits one full cycle to align with the master clock's phase.

6. **Preserve timing state across STOP.** Pulse tracking survives STOP so the next PLAY detects external clock instantly without re-locking.

---

## 14. Known Constraints and Trade-Offs

- **2 PPQN is the hardest case.** Step B accuracy depends entirely on interval prediction. The 2-sample average is the best compromise between jitter reduction and tempo tracking speed.

- **OLED bitbang latency (15–25 ms)** is the primary source of main-loop blocking. The SPI push guard mitigates this, but at the cost of frame rate. A hardware SPI upgrade would eliminate the problem.

- **Grace window is 50% of pulse interval.** This is generous — at 120 BPM / 2 PPQN, the window is 125 ms. If the user is more than half a pulse late, they genuinely pressed on the wrong beat and the system correctly doesn't credit it.

- **No swing/shuffle support.** Subdivision steps are evenly spaced within each pulse interval. Adding swing would require an additional parameter per step or per beat.

- **Single-clock assumption.** The system assumes one external clock source. Multiple clocks on pin 12 would confuse the glitch filter and lock-in detection.
