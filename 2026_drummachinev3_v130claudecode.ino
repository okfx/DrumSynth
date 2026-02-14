// ============================================================================
//  Drum Machine Firmware v130
//  Teensy 4.0 (ARM Cortex-M7 @ 600MHz)
//  Last modified: 2026-02-14 (Session 9 — v130, BPM display fix)
//
//  3 synthesized drum voices, 32 knobs (2x 16-ch mux), 16 step buttons,
//  10 control buttons, 16 LEDs (shift register), 128x64 OLED over I2C,
//  internal clock with external pulse clock sync on pin 12.
//
//  Transport: STOPPED / RUN_INT / RUN_EXT
//  External clock ISR triggers audio directly (zero-latency) with two-part
//  glitch filter (300µs floor + 25% relative). Lock-in requires 2 consecutive
//  similar intervals. Subdivision scheduling uses measured interval (not EMA).
//  ISR signals main loop via wantSwitchToExt flag.
//  Timeout falls back to RUN_INT (if playing) or STOPPED.
// ============================================================================

#include <Arduino.h>
#include <Mux.h>
#include <ResponsiveAnalogRead.h>
#include <ShiftRegister74HC595.h>
#include <Adafruit_GFX.h>
#include <Fonts/picopixel.h>
#include <Adafruit_SH110X.h>
#include <EEPROM.h>

#include "audiotool.h"
#include "audio_init.h"
#include "bitmaps.h"
#include "hw_setup.h"

#define FIRMWARE_VERSION ".130"

// Track enum — declared early so Arduino auto-prototypes can reference it
enum Track : uint8_t {
  TRACK_D1 = 0,
  TRACK_D2 = 1,
  TRACK_D3 = 2
};

// overlay and debounce timing
const uint16_t parameterOverlayDurationTicks = 500;
const uint16_t debounceDelayTicks = 10;  // 10ms — fast response for sync

// D3 accent LED preview state — Main-loop only, no ISR access
bool accentPreviewActive = false;
uint32_t accentPreviewUntilTick = 0;
uint16_t accentPreviewMask = 0;  // 16-bit mask, bit15 = step0 ... bit0 = step15
const uint16_t accentPreviewDurationTicks = 2000;

// PPQN selection mode state — main-loop only
bool ppqnModeActive = false;
uint32_t ppqnModeLastActivityTick = 0;  // Last knob/button interaction
uint8_t ppqnModeSelection = 2;          // Currently selected value in mode
static constexpr uint32_t PPQN_MODE_TIMEOUT_MS = 5000;
static constexpr uint32_t PPQN_LONG_PRESS_MS = 5000;

// PPQN options table
static const uint8_t PPQN_OPTIONS[] = {1, 2, 4, 24, 48, 96};
static constexpr uint8_t PPQN_OPTION_COUNT = 6;

// sequences
byte drum1Sequence[numSteps] = {
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0
};

byte drum2Sequence[numSteps] = {
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0
};

byte drum3Sequence[numSteps] = {
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0
};

// pattern persistence flag
bool patternDirty = false;

// pattern struct for EEPROM
struct PatternStore {
  byte drum1[numSteps];
  byte drum2[numSteps];
  byte drum3[numSteps];
};

// EEPROM save slots
const uint16_t EEPROM_MAGIC = 0x4242;

struct EepromSlot {
  uint16_t magic;
  uint16_t seq;
  PatternStore patterns;
  uint8_t reserved0;
  //  uint8_t reserved[3];  // Can be used for future features (swing, pattern chaining, etc)
};

const uint8_t SAVE_SLOT_COUNT = 10;
const int EEPROM_BASE_ADDR = 0;
const int EEPROM_SLOT_SIZE = sizeof(EepromSlot);

// PPQN stored after all save slots: address = 10 * sizeof(EepromSlot)
static const int EEPROM_PPQN_ADDR = SAVE_SLOT_COUNT * EEPROM_SLOT_SIZE;
static const uint8_t EEPROM_PPQN_MAGIC = 0xAA;  // Validate stored value
// Layout: [EEPROM_PPQN_ADDR] = magic byte, [EEPROM_PPQN_ADDR+1] = PPQN value

uint16_t eepromSeq = 0;
uint8_t activeSaveSlot = 0;
// timers
IntervalTimer tickTimer1k;
IntervalTimer stepTimer;

// global time and step
volatile uint32_t sysTickMs = 0;  // Written by sysTickISR, read by main loop

// tempo
// Main-loop only — not accessed from any ISR. rearmStepTimer() is only
// called from setTransport() and setup(), both main-loop context.
float bpm = 120.0f;
volatile int currentStep = 0;  // Written by ISR (triggerStepFromISR), read by main loop

// transport — three states: internal clock, external clock, or stopped
enum TransportState : uint8_t {
  STOPPED   = 0,
  RUN_INT   = 1,   // Internal timer drives steps
  RUN_EXT   = 2    // External pulse ISR drives steps
};

volatile TransportState tstate = STOPPED;  // Read by stepISR, externalClockISR
volatile bool sequencePlaying = false;     // Read by stepISR, externalClockISR

// ISR→main-loop handoff: ISR sets this flag, main loop acts on it
volatile bool wantSwitchToExt = false;

// ============================================================================
//  CONCURRENCY CONTRACT — External Clock
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
//    subdivRemaining           — uint8_t, ISR-only (externalClockISR + subdivISR, same priority)
//    subdivIntervalUs          — uint32_t, ISR-only (externalClockISR writes, subdivISR reads)
//
//  Main loop writes (read by ISR):
//    tstate                    — uint8_t, atomic on ARM, safe for ISR to read
//    sequencePlaying           — bool, atomic on ARM, safe for ISR to read
//    drum1/2/3Sequence[]       — byte arrays, atomic reads from ISR, written by main loop (button presses)
//    ppqn                      — volatile uint8_t, main loop writes (PPQN mode), ISR reads (externalClockISR). Atomic on ARM.
//
//  Rule: All multi-byte (>1 byte) shared variables must be read/written
//  inside noInterrupts()/interrupts() blocks.
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

// PPQN for external clock — loaded from EEPROM at boot, selectable via UI.
// ISR reads (externalClockISR), main loop writes (PPQN selection mode).
// uint8_t is atomic on ARM Cortex-M7 — no interrupt guards needed.
volatile uint8_t ppqn = 2;

// Subdivision timer state — for ppqn < 4, the external clock ISR fires Step A
// immediately, then arms stepTimer to fire the remaining steps (B, C, D) at
// evenly-spaced intervals derived from lastPulseInterval (measured, not EMA).
// Both ISRs (externalClockISR + subdivISR) run at the same ARM priority — they
// cannot preempt each other, so no race conditions between them.
volatile uint8_t  subdivRemaining = 0;     // Deferred steps still to fire (0 = idle, max 3)
volatile uint32_t subdivIntervalUs = 0;    // Microseconds between subdivision steps

// Debug pulse counter
#ifdef DEBUG_MODE
volatile uint32_t extClockDebugPulseCount = 0;
#endif

// OLED watchdog and frame timing
// Written: main loop (updateDisplay). Read: sysTickISR (watchdog), main loop (frame limiter).
volatile uint32_t lastFrameDrawTick = 0;
const uint32_t oledWatchdogTimeoutTicks = 750;
const uint32_t oledFrameIntervalTicks = 67;  // ~15 fps
volatile bool requestOledReinit = false;  // Set by sysTickISR, cleared by main loop

// Oscilloscope (AC waveform via AudioRecordQueue)
#define SCOPE_DISPLAY_WIDTH 124
#define SCOPE_DISPLAY_HEIGHT 40
float scopeBuffer[SCOPE_DISPLAY_WIDTH] = { 0 };
uint8_t scopeBufferWriteIndex = 0;

// master UI values
float masterVolumeDisp = 10.0f;  // consistent with masterNominalGain default
int masterLPFDisp = 78;

// drum decays
float d1DecayBase = 75.0f;
float d1Decay = 75.0f;
float d2DecayBase = 75.0f;
float d3DecayBase = 25.0f;

// global choke offset in ms
int chokeOffsetMs = 0;

// D2 noise and clap bases
float clapDecayBase = 120.0f;

// parameter overlay text — Main-loop only, no ISR access
char displayParameter1[24] = "";
char displayParameter2[24] = "";
int lastActiveKnob = 255;
uint32_t parameterOverlayStartTick = 0;

// UI voice rails
float uiMixD1Shape = 0.0f;
float uiMixD2Voice = 0.0f;
float uiMixD3Voice = 0.0f;

enum RailMode : uint8_t {
  RAIL_NONE = 0,
  RAIL_D1_SHAPE,
  RAIL_D2_VOICE,
  RAIL_D3_VOICE
};

RailMode activeRail = RAIL_NONE;

// delay ratios (in quarter-note units)
static constexpr float quantizeRatios[] = {
  0.125f,       // 1/8  of a quarter = 1/32
  0.25f,        // 1/4  of a quarter = 1/16
  0.33333333f,  // 1/3  of a quarter = 1/8T
  0.5f,         // 1/2  of a quarter = 1/8
  0.66666667f,  // 2/3  of a quarter = 1/4T
  0.75f,        // 3/4  of a quarter = 1/8.
  1.0f,         // 1    quarter      = 1/4
  1.5f,         // 3/2  quarters     = 1/4.
  2.0f,         // 2    quarters     = 1/2
  2.5f,         // 5/2  quarters     = 5/8
  3.0f          // 3    quarters     = 1/2.
};

static constexpr const char* ratioLabels[] = {
  "1/32",
  "1/16",
  "1/8T",
  "1/8",
  "1/4T",
  "1/8.",
  "1/4",
  "1/4.",
  "1/2",
  "5/8",
  "1/2."
};

const int numRatios = sizeof(quantizeRatios) / sizeof(quantizeRatios[0]);

// master gain and wavefolder compensation
float masterNominalGain = 1.0f;
float masterWfComp = 1.0f;

// drum levels
float d1Vol = 0.75f;
float d2Vol = 0.75f;
float d3Vol = 0.75f;

// delay sends
float d1DelaySend = 0.0f;
float d2DelaySend = 0.0f;
float d3DelaySend = 0.0f;

// accents
enum AltMode : uint8_t {
  ALT_OFF = 0,
  ALT_HALF,
  ALT_QUARTER,
  ALT_EIGHT,
  ALT_EIGHTUP,
  ALT_VARI1,
  ALT_VARI2,
  ALT_VARI3,
  ALT_VARI4,
  ALT_ALT5,
  ALT_ALT6,
  ALT_ALT7,
  ALT_ALT8,
  ALT_ALT9
};

uint8_t d3AltMode = ALT_OFF;

const float d3AltFactor = 2.5f;

// patterns
const uint16_t PAT_VARI1 = 0b1000100000101000;
const uint16_t PAT_VARI2 = 0b1000100010001000;  // NOTE: intentionally same as QUARTER
const uint16_t PAT_VARI3 = 0b1010010010100101;
const uint16_t PAT_VARI4 = 0b0111011101110111;
const uint16_t PAT_ALT5 = 0b0011001100110011;
const uint16_t PAT_ALT6 = 0b0010010010010010;
const uint16_t PAT_ALT7 = 0b1001001001001001;
const uint16_t PAT_ALT8 = 0b0000111100001111;
const uint16_t PAT_ALT9 = 0b1111000011110000;

Track activeTrack = TRACK_D1;  // Main-loop only — no ISR access

// knob smoothing
ResponsiveAnalogRead* analog[knobCount];

// Boot-lock: suppress knob updates until user intentionally moves a knob.
// Prevents ADC drift on power-up from changing parameter values.
static int16_t knobBootValue[knobCount];   // ADC value at boot (after filter settles)
static bool    knobUnlocked[knobCount];    // true once the user has moved this knob
static constexpr int KNOB_UNLOCK_THRESHOLD = 10;  // ADC counts of intentional movement

// helpers
static inline float mapf(int v, int inMin, int inMax, float outMin, float outMax) {
  return outMin + (outMax - outMin) * float(v - inMin) / float(inMax - inMin);
}

static inline float normKnob(int v) {
  float t = float(v) / 1023.0f;
  if (t < 0.0f) return 0.0f;
  if (t > 1.0f) return 1.0f;
  return t;
}

// D1 osc and pitch
float d1BaseFreq = 100.0f;
float d1DistPitchBoost = 0.0f;

static inline void applyD1Freq() {
  float f = d1BaseFreq * (1.0f + d1DistPitchBoost);
  AudioNoInterrupts();
  d1.frequency(f);
  d1b.frequency(f);
  d1c.frequency(f);
  d1d.frequency(f);
  AudioInterrupts();
}

float d1AttackAmt = 0.0f;

// choke mapping helper
static inline int chokeOffsetFromKnob(int v) {
  const int CENTER = 512;
  const int DEADBAND = 50;
  const int MAX_NEG = -40;  // slightly more extreme CCW
  const int MAX_POS = 150;

  if (v >= CENTER - DEADBAND && v <= CENTER + DEADBAND) {
    return 0;
  }

  if (v < CENTER - DEADBAND) {
    int lowEnd = CENTER - DEADBAND;
    float t = float(lowEnd - v) / float(lowEnd - 0);
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return (int)(MAX_NEG * t);
  } else {
    int highStart = CENTER + DEADBAND;
    float t = float(v - highStart) / float(1023 - highStart);
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return (int)(MAX_POS * t);
  }
}

// D3 decay helper (includes choke and a small accent boost)
static inline float d3EffectiveBaseDecay() {
  float base = d3DecayBase + chokeOffsetMs;
  if (base < 7.0f) base = 7.0f;
  if (d3AltMode != ALT_OFF) base += 7.0f;
  return base;
}

// D2 decay helper (includes choke)
static inline float d2EffectiveBaseDecay() {
  float base = d2DecayBase + chokeOffsetMs;
  if (base < 15.0f) base = 15.0f;
  return base;
}

// sequence routing
inline byte* seqByTrack(Track t) {
  switch (t) {
    case TRACK_D1: return drum1Sequence;
    case TRACK_D2: return drum2Sequence;
    case TRACK_D3: return drum3Sequence;
    default:       return drum1Sequence;
  }
}

// forward declarations
void updateOtherButtons();
void updateStepButtons();
void updateLEDs();
void updateDisplay();
void selectSequence(int index);
void playSequence();
void playSequenceCore();
void triggerStepFromISR();
void triggerD1();
void triggerD2();
void triggerD3();
void rearmStepTimer();
inline int readKnobRaw(byte idx);
inline void updateParameterDisplay(byte idx, int knobValue);
inline void applyKnobToEngine(byte idx, int knobValue);
void updateKnobs();
static inline void setTransport(TransportState s);
inline void updateDrumDelayGains();
void initKnobsFromHardware();
inline void applyMasterGainFromState();
void applyChokeToDecays();
void updateScopeData();
void drawScopeWaveform(int x, int y, int w, int h);
void drawOutlinedText(int x, int y, const char* text);
void externalClockISR();
void subdivISR();
static inline void resetExternalClockState();

// EEPROM state
bool loadStateFromEEPROM(uint8_t slotIndex);
void saveStateToEEPROM(uint8_t slotIndex);

// accent evaluation
static inline bool isAccent(uint8_t mode, uint8_t step) {
  uint8_t s = step & 15;
  switch (mode) {
    case ALT_HALF: return (s % 8) == 0;
    case ALT_QUARTER: return (s % 4) == 0;
    case ALT_EIGHT: return (s % 2) == 0;
    case ALT_EIGHTUP: return (s & 1) == 1;
    case ALT_VARI1: return (PAT_VARI1 >> (15 - s)) & 1;
    case ALT_VARI2: return (PAT_VARI2 >> (15 - s)) & 1;
    case ALT_VARI3: return (PAT_VARI3 >> (15 - s)) & 1;
    case ALT_VARI4: return (PAT_VARI4 >> (15 - s)) & 1;
    case ALT_ALT5: return (PAT_ALT5 >> (15 - s)) & 1;
    case ALT_ALT6: return (PAT_ALT6 >> (15 - s)) & 1;
    case ALT_ALT7: return (PAT_ALT7 >> (15 - s)) & 1;
    case ALT_ALT8: return (PAT_ALT8 >> (15 - s)) & 1;
    case ALT_ALT9: return (PAT_ALT9 >> (15 - s)) & 1;
    default: return false;
  }
}

// Safe even if 32 bit volatile reads can tear on your target.
// On Teensy 3.x and 4.x aligned 32 bit is atomic, but this keeps the logic portable.
static inline uint32_t atomicReadU32(volatile uint32_t& v) {
  uint32_t a, b;
  do {
    a = v;
    b = v;
  } while (a != b);
  return a;
}

// timers and interrupts
void sysTickISR();

void stepISR() {
  // Internal clock step generation — only active when in RUN_INT mode
  if (tstate == RUN_INT && sequencePlaying) {
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

void sysTickISR() {
  uint32_t now = sysTickMs + 1;
  sysTickMs = now;

  // stable read of lastFrameDrawTick (handles non atomic 32 bit on some MCUs)
  uint32_t lastDraw = atomicReadU32(lastFrameDrawTick);

  if ((uint32_t)(now - lastDraw) > oledWatchdogTimeoutTicks) {
    requestOledReinit = true;
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
    extIntervalEMA = (ema == 0)
        ? interval
        : ema + (((int32_t)(interval - ema)) >> 1);
  }

  // Good pulse — record timestamp
  lastPulseMicros = nowUs;

  if (extPulseCount < 255) extPulseCount++;

  // Lock-in: switch to RUN_EXT after 2 consecutive intervals within 25%.
  // Prevents a noise spike + real pulse from false lock-in.
  // prevAcceptedInterval holds previous pulse's interval;
  // lastPulseInterval holds current (0 if this is pulse #1).
  if (extPulseCount >= 2 && tstate != RUN_EXT) {
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
  bool canStep = (tstate == RUN_EXT || wantSwitchToExt);
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

#ifdef DEBUG_MODE
  extClockDebugPulseCount++;
#endif
}

// audio helpers

inline void updateDrumDelayGains() {
  // Decouple delay sends from the drum output levels.
  // This keeps delay tails and throws consistent even if a drum is turned down or muted.

  float s1 = d1DelaySend;  // 0..1
  float s2 = d2DelaySend;  // 0..1
  float s3 = d3DelaySend;  // 0..1

  // Optional protection: keep the combined delay input from getting too hot.
  // (Prevents clipping when multiple sends are high at once.)
  const float SEND_SUM_TARGET = 0.90f;
  float sum = s1 + s2 + s3;
  if (sum > SEND_SUM_TARGET) {
    float k = SEND_SUM_TARGET / sum;
    s1 *= k;
    s2 *= k;
    s3 *= k;
  }

  delayMixer.gain(0, s1);
  delayMixer.gain(1, s2);
  delayMixer.gain(2, s3 * 1.75f);
}

inline void applyMasterGainFromState() {
  // always honor masterNominalGain and masterWfComp, even when stopped
  float g = masterNominalGain * masterWfComp;
  masterAmp.gain(g);
}

// choke application on all relevant decays

void applyChokeToDecays() {
  // D1: floor at 17 ms
  float eff1 = d1DecayBase + chokeOffsetMs;
  if (eff1 < 17.0f) eff1 = 17.0f;
  d1Decay = eff1;

  AudioNoInterrupts();
  d1AmpEnv.hold(d1Decay * 0.75f);
  d1AmpEnv.decay(d1Decay);
  AudioInterrupts();

  // D2 main decay using helper
  float base2 = d2EffectiveBaseDecay();
  AudioNoInterrupts();
  d2AmpEnv.hold(base2 * 0.5f);
  d2AmpEnv.decay(base2);
  drum2.length(base2 * 0.25f);
  AudioInterrupts();

  // D2 clap family
  float clapEff = clapDecayBase + chokeOffsetMs;
  if (clapEff < 10.0f) clapEff = 10.0f;

  AudioNoInterrupts();
  clap1AmpEnv.decay(clapEff);
  clap2AmpEnv.decay(clapEff);
  clapMasterEnv.decay(clapEff);
  AudioInterrupts();
}

// EEPROM load/save

bool loadStateFromEEPROM(uint8_t slotIndex) {
  if (slotIndex >= SAVE_SLOT_COUNT) return false;

  // Calculate and verify address (type-safe)
  size_t addr = (size_t)EEPROM_BASE_ADDR + (size_t)slotIndex * sizeof(EepromSlot);
  if (addr + sizeof(EepromSlot) > EEPROM.length()) return false;

  // Read slot from EEPROM
  EepromSlot slot;
  EEPROM.get((int)addr, slot);

  // Verify magic number
  if (slot.magic != EEPROM_MAGIC) return false;

  // Load pattern data
  for (int step = 0; step < numSteps; step++) {
    drum1Sequence[step] = slot.patterns.drum1[step];
    drum2Sequence[step] = slot.patterns.drum2[step];
    drum3Sequence[step] = slot.patterns.drum3[step];
  }

  // Refresh step LEDs so they reflect the loaded pattern
  updateLEDs();

  // Update sequence number (keep monotonic for debugging)
  if (slot.seq > eepromSeq) {
    eepromSeq = slot.seq;
  }

  patternDirty = false;

  // Show "PATTERN LOADED" overlay message
  activeRail = RAIL_NONE;
  lastActiveKnob = 255;  // sentinel: no active knob
  snprintf(displayParameter1, sizeof(displayParameter1), "PATTERN");
  snprintf(displayParameter2, sizeof(displayParameter2), "LOADED");

  // Start overlay timer atomically
  noInterrupts();
  parameterOverlayStartTick = sysTickMs;
  interrupts();

  return true;
}

void saveStateToEEPROM(uint8_t slotIndex) {
  if (slotIndex >= SAVE_SLOT_COUNT) return;

  // Should be size_t, not int
  size_t addr = (size_t)EEPROM_BASE_ADDR + (size_t)slotIndex * sizeof(EepromSlot);
  if (addr + sizeof(EepromSlot) > EEPROM.length()) return;

  EepromSlot slot = {};
  slot.magic = EEPROM_MAGIC;
  slot.seq = eepromSeq + 1;

  for (int step = 0; step < numSteps; step++) {
    slot.patterns.drum1[step] = drum1Sequence[step];
    slot.patterns.drum2[step] = drum2Sequence[step];
    slot.patterns.drum3[step] = drum3Sequence[step];
  }

  EEPROM.put((int)addr, slot);  // Cast to int here for EEPROM.put()

  eepromSeq++;
  patternDirty = false;
}

void loadPpqnFromEEPROM() {
  uint8_t magic = EEPROM.read(EEPROM_PPQN_ADDR);
  if (magic == EEPROM_PPQN_MAGIC) {
    uint8_t val = EEPROM.read(EEPROM_PPQN_ADDR + 1);
    // Validate: must be one of the allowed values
    for (uint8_t i = 0; i < PPQN_OPTION_COUNT; i++) {
      if (PPQN_OPTIONS[i] == val) { ppqn = val; return; }
    }
  }
  ppqn = 2;  // Default if nothing saved or invalid
}

void savePpqnToEEPROM(uint8_t val) {
  EEPROM.update(EEPROM_PPQN_ADDR, EEPROM_PPQN_MAGIC);
  EEPROM.update(EEPROM_PPQN_ADDR + 1, val);
}

void setup() {
  delay(200);

  // ============================================================================
  // STARTUP LED BLINK (3x flash)
  // ============================================================================

  pinMode(LED_BUILTIN, OUTPUT);
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(150);
    digitalWrite(LED_BUILTIN, LOW);
    delay(150);
  }

  // ============================================================================
  // OLED DISPLAY INITIALIZATION
  // ============================================================================

  bool displayOk = display.begin(0, true);
  if (displayOk) {
    display.clearDisplay();

    // Startup splash screen
    display.setTextColor(1);
    display.setTextWrap(false);
    display.setFont(NULL);
    display.setTextSize(1);
    display.setCursor(40, 20);
    display.print("VERSION");
    display.setTextSize(2);
    display.setCursor(46, 36);
    display.print("v" FIRMWARE_VERSION);
    display.display();
    delay(800);

    display.clearDisplay();
    display.display();
  }

  // ============================================================================
  // AUDIO SYSTEM INITIALIZATION
  // ============================================================================

  AudioMemory(500);
  tickTimer1k.begin(sysTickISR, 1000);
  audioInit();
  scopeQueue.begin();

  // ============================================================================
  // UI STATE INITIALIZATION
  // ============================================================================

  // Clear overlay and parameter strings at boot (now sysTickMs is valid)
  activeRail = RAIL_NONE;
  lastActiveKnob = 255;
  displayParameter1[0] = 0;
  displayParameter2[0] = 0;

  noInterrupts();
  parameterOverlayStartTick = sysTickMs - parameterOverlayDurationTicks - 1;
  interrupts();

  // Load saved PPQN from EEPROM (defaults to 2 if nothing saved)
  loadPpqnFromEEPROM();

  // ============================================================================
  // HARDWARE INITIALIZATION
  // ============================================================================

  // Initialize shift register and LEDs
  sr.setAllLow();
  updateLEDs();

  // Initialize knob smoothing filters
  for (byte i = 0; i < knobCount; i++) {
    analog[i] = new ResponsiveAnalogRead(0, true);
    analog[i]->setActivityThreshold(20);
  }

  // ============================================================================
  // EXTERNAL CLOCK PIN
  // ============================================================================

  pinMode(EXT_CLK_PIN, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(EXT_CLK_PIN), externalClockISR, RISING);

  // ============================================================================
  // LOAD STATE AND START SEQUENCER
  // ============================================================================

  initKnobsFromHardware();
  loadStateFromEEPROM(activeSaveSlot);
  rearmStepTimer();
  applyMasterGainFromState();
}

void loop() {

  // ============================================================================
  // ACCENT PREVIEW TIMEOUT
  // ============================================================================

  // Expire accent preview and restore normal LEDs
  if (accentPreviewActive) {
    uint32_t nowTick;
    noInterrupts();
    nowTick = sysTickMs;  // sysTickMs is ISR-written, needs guard
    interrupts();

    if ((int32_t)(nowTick - accentPreviewUntilTick) >= 0) {
      accentPreviewActive = false;  // Main-loop only — no guard needed
      updateLEDs();
    }
  }

  // PPQN mode timeout — auto-exit without saving after 5s inactivity
  if (ppqnModeActive) {
    uint32_t nowTick;
    noInterrupts();
    nowTick = sysTickMs;
    interrupts();
    if ((uint32_t)(nowTick - ppqnModeLastActivityTick) >= PPQN_MODE_TIMEOUT_MS) {
      ppqnModeActive = false;
      // No save — just exit silently. Normal display resumes automatically.
    }
  }

  // ============================================================================
  // OLED WATCHDOG RECOVERY
  // ============================================================================

  // Atomic check-and-clear prevents losing a flag set between the
  // if-check and the clear (ISR could fire in between otherwise)
  {
    bool needsReinit;
    noInterrupts();
    needsReinit = requestOledReinit;
    requestOledReinit = false;
    interrupts();

    if (needsReinit) {
      display.begin(0, true);
      display.clearDisplay();
      display.display();

      uint32_t t;
      noInterrupts();
      t = sysTickMs;
      lastFrameDrawTick = t;
      interrupts();
    }
  }

  // ============================================================================
  // EXTERNAL CLOCK: ISR→main-loop handoff
  // ============================================================================

  {
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

  // ============================================================================
  // EXTERNAL CLOCK: timeout detection — fall back to internal or stop
  // ============================================================================

  if (tstate == RUN_EXT) {
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

  // ============================================================================
  // SEQUENCER STEP PROCESSING (before input polling to minimize latency)
  // ============================================================================

  if (sequencePlaying) {
    if (tstate == RUN_EXT) {
      // External clock: audio triggers fire directly in ISR (triggerStepFromISR).
      // Main loop handles deferred LED update only.
      if (ledUpdatePending) {
        // Clear atomically: if the ISR sets the flag between our if-test
        // and this clear, we'd lose that update without the guard.
        noInterrupts();
        ledUpdatePending = false;
        interrupts();
        updateLEDs();
      }
    } else {
      // Internal clock: consume steps queued by stepISR
      playSequence();
    }
  } else {
    applyMasterGainFromState();
  }

  // Derive EXT BPM from the fast EMA (alpha=0.5) — same interval the engine
  // uses for glitch filtering, so the display matches what the engine hears.
  // Light display-side smoothing (alpha=0.25) damps jitter without the long
  // settle time the old slow EMA had.
  if (tstate == RUN_EXT) {
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

  // ============================================================================
  // USER INPUT POLLING
  // ============================================================================

  updateOtherButtons();
  updateKnobs();
  updateStepButtons();

  // ============================================================================
  // OSCILLOSCOPE DATA ACQUISITION
  // ============================================================================

  updateScopeData();

  // ============================================================================
  // THROTTLED OLED REFRESH (15 FPS to reduce lag)
  // ============================================================================

  uint32_t tickCopy, lastDrawCopy;

  noInterrupts();
  tickCopy = sysTickMs;
  lastDrawCopy = lastFrameDrawTick;
  interrupts();

  if ((uint32_t)(tickCopy - lastDrawCopy) >= oledFrameIntervalTicks) {
    updateDisplay();
  }
}

// transport helpers — v126 pattern: clean state transitions
static inline void setTransport(TransportState s) {
  if (tstate == s) return;
  tstate = s;

  // When leaving RUN_EXT, clear step accumulator and cancel subdivisions
  if (tstate != RUN_EXT) {
    extStepAcc = 0;
    stepTimer.end();        // Cancel any in-flight subdivision timer
    subdivRemaining = 0;
  }

  switch (tstate) {
    case RUN_INT:
      rearmStepTimer();
      break;
    case RUN_EXT:
      stepTimer.end();
      // Don't clear extStepAcc here — the ISR may have already written a valid
      // accumulator value on the locking pulse before main loop calls setTransport().
      // Clearing happens when *leaving* RUN_EXT (see above).
      break;
    case STOPPED:
    default:
      stepTimer.end();
      break;
  }
}

void rearmStepTimer() {
  float stepPeriodUs = 60000000.0f / (bpm * 4.0f);
  if (stepPeriodUs < 500.0f) stepPeriodUs = 500.0f;
  stepTimer.end();
  stepTimer.begin(stepISR, (uint32_t)stepPeriodUs);
}

static inline void resetExternalClockState() {
  noInterrupts();
  extPulseCount = 0;
  extStepAcc = 0;
  lastPulseMicros = 0;
  lastPulseInterval = 0;
  extIntervalEMA = 0;
  prevAcceptedInterval = 0;
  ledUpdatePending = false;
  lastD1TriggerUs = 0;
  lastD2TriggerUs = 0;
  lastD3TriggerUs = 0;
  subdivRemaining = 0;
  subdivIntervalUs = 0;
  interrupts();
  extBpmDisplay = 0.0f;
}

// sequencing and triggers

void playSequence() {
  // pendingStepCount is uint8_t (atomic on ARM), but read-and-clear
  // must be done atomically to avoid losing a count
  uint8_t toDo;
  noInterrupts();
  toDo = pendingStepCount;
  pendingStepCount = 0;
  interrupts();

  if (toDo == 0) return;

  if (toDo > 32) toDo = 32;
  while (toDo--) {
    playSequenceCore();
  }
}

// Called from externalClockISR() — triggers audio directly for zero-latency.
// Uses same triggerD1/D2/D3 helpers as playSequenceCore(), but sets
// ledUpdatePending instead of calling updateLEDs() (SPI unsafe from ISR).
void triggerStepFromISR() {
  if (currentStep < 0 || currentStep >= numSteps) {
    currentStep = numSteps - 1;
  }
  currentStep = (currentStep + 1) % numSteps;

  if (drum1Sequence[currentStep]) triggerD1();
  if (drum2Sequence[currentStep]) triggerD2();
  if (drum3Sequence[currentStep]) triggerD3();

  ledUpdatePending = true;
}

// playSequenceCore — used by internal clock path (main loop context).
// Same triggerD1/D2/D3 helpers, but safe to call updateLEDs() directly.
void playSequenceCore() {
  if (currentStep < 0 || currentStep >= numSteps) {
    currentStep = numSteps - 1;
  }
  currentStep = (currentStep + 1) % numSteps;

  if (drum1Sequence[currentStep]) triggerD1();
  if (drum2Sequence[currentStep]) triggerD2();
  if (drum3Sequence[currentStep]) triggerD3();

  // Update LEDs — safe in main-loop context (SPI not safe from ISR)
  updateLEDs();
}

void triggerD1() {
  uint32_t now = micros();
  bool skipNoteOff = (now - lastD1TriggerUs) < MIN_RETRIGGER_US;
  lastD1TriggerUs = now;

  float attackMs = 0.5f + 20.0f * d1AttackAmt;
  float holdMs = d1Decay * 0.75f + 12.0f * d1AttackAmt;
  float decayMs = d1Decay * (1.0f - 0.25f * d1AttackAmt);

  AudioNoInterrupts();
  d1AmpEnv.attack(attackMs);
  d1AmpEnv.hold(holdMs);
  d1AmpEnv.decay(decayMs);
  AudioInterrupts();

  if (!skipNoteOff) {
    d1AmpEnv.noteOff();
    d1PitchEnv.noteOff();
  }
  d1AmpEnv.noteOn();
  d1PitchEnv.noteOn();
  drum1.noteOn();
}

// Trigger D2 (snare/clap) — shared by ISR and main-loop step functions.
// Handles retrigger guard, decay application, envelope noteOff/noteOn.
void triggerD2() {
  uint32_t now = micros();
  bool skipNoteOff = (now - lastD2TriggerUs) < MIN_RETRIGGER_US;
  lastD2TriggerUs = now;

  float applyDecay2 = d2EffectiveBaseDecay();
  float clapEff = clapDecayBase + chokeOffsetMs;
  if (clapEff < 10.0f) clapEff = 10.0f;

  AudioNoInterrupts();
  d2AmpEnv.hold(applyDecay2 * 0.5f);
  d2AmpEnv.decay(applyDecay2);
  drum2.length(applyDecay2 * 0.25f);

  clap1AmpEnv.decay(clapEff);
  clap2AmpEnv.decay(clapEff);
  clapMasterEnv.decay(clapEff);
  AudioInterrupts();

  if (!skipNoteOff) {
    d2AmpEnv.noteOff();
    d2bAmpEnv.noteOff();
    clap1AmpEnv.noteOff();
    clap2AmpEnv.noteOff();
    clapMasterEnv.noteOff();
    d2NoiseEnvelope.noteOff();
  }
  d2AmpEnv.noteOn();
  d2bAmpEnv.noteOn();
  clap1AmpEnv.noteOn();
  clap2AmpEnv.noteOn();
  clapMasterEnv.noteOn();
  drum2.noteOn();
  d2Attack.noteOn();
  d2NoiseEnvelope.noteOn();
}

// Trigger D3 (hi-hat) — shared by ISR and main-loop step functions.
// Applies accent pattern if active, handles retrigger guard.
void triggerD3() {
  uint32_t now = micros();
  bool skipNoteOff = (now - lastD3TriggerUs) < MIN_RETRIGGER_US;
  lastD3TriggerUs = now;

  float baseDecay = d3EffectiveBaseDecay();
  float applyDecay = baseDecay;

  if (d3AltMode != ALT_OFF && isAccent(d3AltMode, currentStep)) {
    applyDecay = baseDecay * d3AltFactor;
  }

  AudioNoInterrupts();
  d3AmpEnv.decay(applyDecay);
  d3606AmpEnv.decay(applyDecay);
  drum3.length(applyDecay);
  AudioInterrupts();

  if (!skipNoteOff) {
    d3AmpEnv.noteOff();
    d3606AmpEnv.noteOff();
  }
  d3AmpEnv.noteOn();
  d3606AmpEnv.noteOn();
  drum3.noteOn();
}

// buttons and LEDs

void updateOtherButtons() {
  static uint32_t lastDebounceTick[otherButtonsCount] = { 0 };
  static bool state[otherButtonsCount] = { false };
  static bool lastState[otherButtonsCount] = { false };

  // Button 7 long-hold state — shared between state-change and continuous-hold blocks
  static uint32_t btn7PressTick = 0;
  static bool btn7EnteredPpqn = false;

  uint32_t nowTick;
  noInterrupts();
  nowTick = sysTickMs;
  interrupts();

  for (int i = 0; i < otherButtonsCount; i++) {

    otherButtonsMux.channel(i);
    delayMicroseconds(5);

    bool rawPressed = !otherButtonsMux.read(i);

    if (rawPressed != lastState[i]) {
      lastDebounceTick[i] = nowTick;
    }

    // Play button (6) gets minimal debounce for sync; others get standard debounce
    uint16_t debounceTicks = (i == 6) ? 2 : 25;
    if ((uint32_t)(nowTick - lastDebounceTick[i]) > debounceTicks && (rawPressed != state[i])) {

      state[i] = rawPressed;

      // Button 7 (CHANGE MEMORY SLOT) — state transitions only.
      // 5s hold detection is in continuous check below.
      if (i == 7) {
        if (state[7]) {
          // Press down — record timestamp
          btn7PressTick = nowTick;
          btn7EnteredPpqn = false;
        } else {
          // Release
          if (btn7EnteredPpqn) {
            // Hold already entered PPQN mode — ignore release (don't cycle slot)
          } else if (ppqnModeActive) {
            // In PPQN mode — confirm and save
            ppqn = ppqnModeSelection;
            savePpqnToEEPROM(ppqn);
            if (tstate == RUN_EXT) {
              noInterrupts();
              extStepAcc = 0;
              interrupts();
            }
            ppqnModeActive = false;
            snprintf(displayParameter1, sizeof(displayParameter1), "PPQN %d", ppqn);
            snprintf(displayParameter2, sizeof(displayParameter2), "SAVED");
            parameterOverlayStartTick = nowTick;
            activeRail = RAIL_NONE;
            lastActiveKnob = 255;
          } else {
            // Short press — normal cycle memory slot
            activeSaveSlot = (activeSaveSlot + 1) % SAVE_SLOT_COUNT;
          }
          btn7EnteredPpqn = false;
        }
      }

      if (state[i] && i != 7) {
        switch (i) {

          case 0: selectSequence(0); break;
          case 1: selectSequence(1); break;
          case 2: selectSequence(2); break;

          case 3: break;
          case 4: break;
          case 5: break;

          case 6:
            if (!sequencePlaying) {
              // START — reset step position so first emitted step becomes 0
              noInterrupts();
              currentStep = numSteps - 1;
              pendingStepCount = 0;
              extStepAcc = 0;
              interrupts();

              setTransport(RUN_INT);
              sequencePlaying = true;
            } else {
              // STOP
              sequencePlaying = false;
              setTransport(STOPPED);
              applyMasterGainFromState();
              noInterrupts();
              extStepAcc = 0;
              interrupts();
            }
            break;

          case 8:
            sequencePlaying = false;
            setTransport(STOPPED);
            applyMasterGainFromState();

            loadStateFromEEPROM(activeSaveSlot);
            updateLEDs();

            // Ensure next START begins at step 0 again
            noInterrupts();
            currentStep = numSteps - 1;
            pendingStepCount = 0;
            interrupts();

            break;

          case 9:
            {
              uint32_t t;
              noInterrupts();
              t = sysTickMs;
              interrupts();

              activeRail = RAIL_NONE;
              lastActiveKnob = 255;

              if (sequencePlaying) {
                snprintf(displayParameter1, sizeof(displayParameter1), "STOP");
                snprintf(displayParameter2, sizeof(displayParameter2), "TO SAVE");
              } else {
                if (patternDirty) saveStateToEEPROM(activeSaveSlot);
                snprintf(displayParameter1, sizeof(displayParameter1), "PATTERN");
                snprintf(displayParameter2, sizeof(displayParameter2), "SAVED");
              }

              parameterOverlayStartTick = t;
              break;
            }

          default:
            break;
        }
      }
    }

    // Button 7 continuous hold check — enters PPQN mode after 5s while held.
    // Runs on every scan iteration (not just state change). Uses function-scope
    // btn7PressTick/btn7EnteredPpqn shared with the state-change block above.
    if (i == 7 && state[7] && !ppqnModeActive && !btn7EnteredPpqn) {
      if ((uint32_t)(nowTick - btn7PressTick) >= PPQN_LONG_PRESS_MS) {
        ppqnModeActive = true;
        ppqnModeLastActivityTick = nowTick;
        ppqnModeSelection = ppqn;
        btn7EnteredPpqn = true;  // Prevent release from cycling slot
      }
    }

    lastState[i] = rawPressed;
  }
}

void selectSequence(int index) {
  switch (index) {
    case 0: activeTrack = TRACK_D1; break;
    case 1: activeTrack = TRACK_D2; break;
    case 2: activeTrack = TRACK_D3; break;
    default: return;
  }
  updateLEDs();
}

static inline uint16_t accentMaskFromMode(uint8_t mode) {
  switch (mode) {
    case ALT_HALF: return 0b1000000010000000;     // steps 0 and 8
    case ALT_QUARTER: return 0b1000100010001000;  // steps 0,4,8,12
    case ALT_EIGHT: return 0b1010101010101010;
    case ALT_EIGHTUP: return 0b0101010101010101;  // steps 1,3,5,7,9,11,13,15
    case ALT_VARI1: return PAT_VARI1;
    case ALT_VARI2: return PAT_VARI2;
    case ALT_VARI3: return PAT_VARI3;
    case ALT_VARI4: return PAT_VARI4;
    case ALT_ALT5: return PAT_ALT5;
    case ALT_ALT6: return PAT_ALT6;
    case ALT_ALT7: return PAT_ALT7;
    case ALT_ALT8: return PAT_ALT8;
    case ALT_ALT9: return PAT_ALT9;
    default: return 0;
  }
}

void updateLEDs() {
  // Main-loop only — no ISR access to accentPreview*, no guards needed
  bool preview = accentPreviewActive;
  uint16_t mask = accentPreviewMask;

  // Accent preview mode (temporary display)
  if (preview) {
    for (int i = 0; i < numSteps; i++) {
      bool on = (mask >> (15 - i)) & 1;
      sr.set(i, on);
    }
    return;
  }

  // Normal pattern display with optional current step indicator
  byte* seq = seqByTrack(activeTrack);

  // Snapshot current step and play state atomically
  int currentStepSnap;
  bool playingSnap;
  noInterrupts();
  currentStepSnap = currentStep;
  playingSnap = sequencePlaying;
  interrupts();

  for (int i = 0; i < numSteps; i++) {
    bool ledState = seq[i];

    // Force current step ON when playing (creates moving bright spot)
    if (playingSnap && i == currentStepSnap) {
      ledState = true;
    }

    sr.set(i, ledState);
  }
}

void updateStepButtons() {
  static uint32_t lastDebounceTick[stepButtonCount] = { 0 };
  static bool state[stepButtonCount] = { false };
  static bool lastState[stepButtonCount] = { false };

  byte* seq = seqByTrack(activeTrack);

  // single stable timebase for this whole scan
  uint32_t nowTick;
  noInterrupts();
  nowTick = sysTickMs;
  interrupts();

  for (int i = 0; i < stepButtonCount; i++) {
    stepButtonsMux.channel(i);
    delayMicroseconds(5);
    bool rawPressed = !stepButtonsMux.read(i);

    if (rawPressed != lastState[i]) {
      lastDebounceTick[i] = nowTick;
    }

    if ((uint32_t)(nowTick - lastDebounceTick[i]) > debounceDelayTicks && (rawPressed != state[i])) {

      state[i] = rawPressed;
      if (state[i]) {
        seq[i] ^= 1;
        sr.set(i, seq[i]);
        patternDirty = true;
      }
    }

    lastState[i] = rawPressed;
  }
}

inline int readKnobRaw(byte idx) {
  int raw;
  if (idx < 16) {
    knobMux1.channel(idx);
    delayMicroseconds(5);
    raw = knobMux1.read(idx);
  } else {
    knobMux2.channel(idx - 16);
    delayMicroseconds(5);
    raw = knobMux2.read(idx - 16);
  }
  // Clamp ADC edges: pots rarely hit true 0/1023 — map the reliable
  // range to the full 0-1023 so both extremes are always reachable.
  raw = constrain(raw, 3, 1020);
  return map(raw, 3, 1020, 0, 1023);
}

inline void setOverlayTimer(byte idx) {
  lastActiveKnob = idx;
  uint32_t t;
  noInterrupts();
  t = sysTickMs;
  interrupts();
  parameterOverlayStartTick = t;
}

// parameter overlay text

inline void updateParameterDisplay(byte idx, int knobValue) {
  setOverlayTimer(idx);
  activeRail = RAIL_NONE;

  switch (idx) {

      // ========================================================================
      // D1 - KICK DRUM (Knobs 0-7)
      // ========================================================================

    case 0:  // D1 Drive/Distortion
      {
        int percent = (int)map(knobValue, 0, 1023, 1, 100);
        snprintf(displayParameter1, sizeof(displayParameter1), "D1 DISTORT");
        snprintf(displayParameter2, sizeof(displayParameter2), "%d%%", percent);
        break;
      }

    case 1:  // D1 Waveform Shape (uses rail display)
      {
        snprintf(displayParameter1, sizeof(displayParameter1), "");
        displayParameter2[0] = 0;
        activeRail = RAIL_D1_SHAPE;
        uiMixD1Shape = normKnob(knobValue);
        break;
      }

    case 2:  // D1 Decay
      {
        float norm = normKnob(knobValue);
        float decayMs;

        if (norm <= 0.25f) {
          float blend = norm / 0.25f;
          decayMs = 50.0f + (70.0f - 50.0f) * blend;
        } else {
          float blend = (norm - 0.25f) / 0.75f;
          decayMs = 71.0f + (1000.0f - 71.0f) * blend;
        }

        snprintf(displayParameter1, sizeof(displayParameter1), "D1 DECAY");
        snprintf(displayParameter2, sizeof(displayParameter2), "%.0f ms", decayMs);
        break;
      }

    case 3:  // D1 Pitch
      {
        float norm = normKnob(knobValue);
        float freqHz;

        if (norm <= 0.33f) {
          float blend = norm / 0.33f;
          freqHz = 60.0f + (105.0f - 60.0f) * blend;
        } else {
          float blend = (norm - 0.33f) / 0.67f;
          freqHz = 106.0f + (500.0f - 106.0f) * blend;
        }

        snprintf(displayParameter1, sizeof(displayParameter1), "D1 FREQ");
        snprintf(displayParameter2, sizeof(displayParameter2), "%d Hz", (int)freqHz);
        break;
      }

    case 4:  // D1 Volume
      {
        float norm = normKnob(knobValue);
        int percent = (int)(norm * 100.0f + 0.5f);
        snprintf(displayParameter1, sizeof(displayParameter1), "D1 VOLUME");
        snprintf(displayParameter2, sizeof(displayParameter2), "%d%%", percent);
        break;
      }

    case 5:  // D1 Attack/Snap
      {
        int percent = (int)map(knobValue, 0, 1023, 0, 100);
        snprintf(displayParameter1, sizeof(displayParameter1), "D1 SNAP");
        snprintf(displayParameter2, sizeof(displayParameter2), "%d%%", percent);
        break;
      }

    case 6:  // D1 EQ (Body)
      {
        int percent = (int)map(knobValue, 0, 1023, 0, 100);
        snprintf(displayParameter1, sizeof(displayParameter1), "D1 BODY");
        snprintf(displayParameter2, sizeof(displayParameter2), "%d%%", percent);
        break;
      }

    case 7:  // D1 Delay Send
      {
        int percent = (int)map(knobValue, 0, 1023, 0, 75);
        snprintf(displayParameter1, sizeof(displayParameter1), "D1 DLY SND");
        snprintf(displayParameter2, sizeof(displayParameter2), "%d%%", percent);
        break;
      }

      // ========================================================================
      // D2 - SNARE/CLAP (Knobs 8-15)
      // ========================================================================

    case 8:  // D2 Pitch
      {
        float freqHz = mapf(knobValue, 0, 1023, 100.0f, 300.0f);
        snprintf(displayParameter1, sizeof(displayParameter1), "D2 FREQ");
        snprintf(displayParameter2, sizeof(displayParameter2), "%d Hz", (int)freqHz);
        break;
      }

    case 9:  // D2 Decay
      {
        int percent = (int)map(knobValue, 0, 1023, 1, 100);
        snprintf(displayParameter1, sizeof(displayParameter1), "D2 DECAY");
        snprintf(displayParameter2, sizeof(displayParameter2), "%d%%", percent);
        break;
      }

    case 10:  // D2 Voice Mix - Snare/Clap (uses rail display)
      {
        snprintf(displayParameter1, sizeof(displayParameter1), "");
        displayParameter2[0] = 0;
        activeRail = RAIL_D2_VOICE;
        uiMixD2Voice = normKnob(knobValue);
        break;
      }

    case 11:  // D2 Wavefolder Drive
      {
        float norm = normKnob(knobValue);
        int percent = (int)(norm * 100.0f + 0.5f);
        snprintf(displayParameter1, sizeof(displayParameter1), "D2 DISTORT");
        snprintf(displayParameter2, sizeof(displayParameter2), "%d%%", percent);
        break;
      }

    case 12:  // D2 Delay Send
      {
        int percent = (int)map(knobValue, 0, 1023, 0, 75);
        snprintf(displayParameter1, sizeof(displayParameter1), "D2 DLY SND");
        snprintf(displayParameter2, sizeof(displayParameter2), "%d%%", percent);
        break;
      }

    case 13:  // D2 Reverb
      {
        float norm = normKnob(knobValue);
        int percent = (int)(norm * 100.0f);

        snprintf(displayParameter1, sizeof(displayParameter1), "D2 VERB");

        if (percent < 5) {
          snprintf(displayParameter2, sizeof(displayParameter2), "OFF");
        } else {
          snprintf(displayParameter2, sizeof(displayParameter2), "%d%%", percent);
        }
        break;
      }

    case 14:  // D2 Noise
      {
        snprintf(displayParameter1, sizeof(displayParameter1), "D2 NOISE");
        if (knobValue < 5) {
          snprintf(displayParameter2, sizeof(displayParameter2), "OFF");
        } else {
          int percent = (int)map(knobValue, 5, 1023, 1, 100);
          snprintf(displayParameter2, sizeof(displayParameter2), "%d%%", percent);
        }
        break;
      }

    case 15:  // D2 Volume
      {
        float norm = normKnob(knobValue);
        int percent = (int)(norm * 100.0f + 0.5f);
        snprintf(displayParameter1, sizeof(displayParameter1), "D2 VOLUME");
        snprintf(displayParameter2, sizeof(displayParameter2), "%d%%", percent);
        break;
      }

      // ========================================================================
      // D3 - HI-HAT (Knobs 16-23)
      // ========================================================================

    case 16:  // D3 Pitch (displayed as generic "tune")
      {
        int tune = (int)((float)knobValue * (100.0f / 1023.0f) + 0.5f);
        snprintf(displayParameter1, sizeof(displayParameter1), "D3 TUNE");
        snprintf(displayParameter2, sizeof(displayParameter2), "%3d", tune);
        break;
      }

    case 17:  // D3 Decay
      {
        int percent = (int)map(knobValue, 0, 1023, 1, 100);
        snprintf(displayParameter1, sizeof(displayParameter1), "D3 DECAY");
        snprintf(displayParameter2, sizeof(displayParameter2), "%d%%", percent);
        break;
      }

    case 18:  // D3 Voice Mix - 3-way (uses rail display)
      {
        snprintf(displayParameter1, sizeof(displayParameter1), "");
        displayParameter2[0] = 0;
        activeRail = RAIL_D3_VOICE;
        uiMixD3Voice = normKnob(knobValue);
        break;
      }

    case 19:  // D3 Wavefolder Drive
      {
        int percent = (int)map(knobValue, 0, 1023, 1, 100);
        snprintf(displayParameter1, sizeof(displayParameter1), "D3 DISTORT");
        snprintf(displayParameter2, sizeof(displayParameter2), "%d%%", percent);
        break;
      }

    case 20:  // D3 Delay Send
      {
        int percent = (int)map(knobValue, 0, 1023, 0, 100);
        snprintf(displayParameter1, sizeof(displayParameter1), "D3 DLY SND");
        snprintf(displayParameter2, sizeof(displayParameter2), "%d%%", percent);
        break;
      }

    case 21:  // D3 Filter
      {
        float norm = normKnob(knobValue);
        float normSq = norm * norm;
        float cutoffHz = 4500.0f + normSq * (8000.0f - 4500.0f);

        snprintf(displayParameter1, sizeof(displayParameter1), "D3 LPF");
        snprintf(displayParameter2, sizeof(displayParameter2), "%d Hz", (int)cutoffHz);
        break;
      }

    case 22:  // D3 Accent Pattern
      {
        const int ACCENT_DEADBAND = 24;
        uint8_t mode;
        const char* label;

        snprintf(displayParameter1, sizeof(displayParameter1), "D3 ACCENT");

        // Determine mode from knob position
        if (knobValue <= ACCENT_DEADBAND) {
          mode = ALT_OFF;
        } else {
          int zone = map(knobValue, ACCENT_DEADBAND + 1, 1023, 1, 12);
          switch (zone) {
            case 1: mode = ALT_HALF; break;
            case 2: mode = ALT_QUARTER; break;
            case 3: mode = ALT_EIGHT; break;
            case 4: mode = ALT_EIGHTUP; break;
            case 5: mode = ALT_VARI1; break;
            case 6: mode = ALT_VARI2; break;
            case 7: mode = ALT_VARI3; break;
            case 8: mode = ALT_VARI4; break;
            case 9: mode = ALT_ALT5; break;
            case 10: mode = ALT_ALT6; break;
            case 11: mode = ALT_ALT7; break;
            case 12: mode = ALT_ALT8; break;
            default: mode = ALT_ALT9; break;
          }
        }

        // Map mode to display label
        switch (mode) {
          case ALT_HALF: label = "HALF"; break;
          case ALT_QUARTER: label = "QUARTER"; break;
          case ALT_EIGHT: label = "EIGHT"; break;
          case ALT_EIGHTUP: label = "EIGHT UP"; break;
          case ALT_VARI1: label = "VAR1"; break;
          case ALT_VARI2: label = "VAR2"; break;
          case ALT_VARI3: label = "VAR3"; break;
          case ALT_VARI4: label = "VAR4"; break;
          case ALT_ALT5: label = "VAR5"; break;
          case ALT_ALT6: label = "VAR6"; break;
          case ALT_ALT7: label = "VAR7"; break;
          case ALT_ALT8: label = "VAR8"; break;
          case ALT_ALT9: label = "VAR9"; break;
          default: label = "OFF"; break;
        }

        snprintf(displayParameter2, sizeof(displayParameter2), "%s", label);
        break;
      }

    case 23:  // D3 Volume
      {
        float norm = normKnob(knobValue);
        int percent = (int)(norm * 100.0f + 0.5f);
        snprintf(displayParameter1, sizeof(displayParameter1), "D3 VOLUME");
        snprintf(displayParameter2, sizeof(displayParameter2), "%d%%", percent);
        break;
      }

      // ========================================================================
      // MASTER SECTION (Knobs 24-31)
      // ========================================================================

    case 24:  // Master Delay Time (quantized to tempo)
      {
        float msPerBeat = 60000.0f / bpm;
        float maxRatio = 1400.0f / msPerBeat;

        // Find maximum available ratio index within delay time limit
        int maxIdx = 0;
        for (int i = 0; i < numRatios; i++) {
          if (quantizeRatios[i] <= maxRatio) {
            maxIdx = i;
          } else {
            break;
          }
        }

        int ratioIdx = (int)((knobValue / 1023.0f) * maxIdx + 0.5f);
        ratioIdx = constrain(ratioIdx, 0, maxIdx);

        snprintf(displayParameter1, sizeof(displayParameter1), "DELAY TIME");
        snprintf(displayParameter2, sizeof(displayParameter2), "%s", ratioLabels[ratioIdx]);
        break;
      }

    case 25:  // Master Wavefolder Frequency
      {
        float norm = normKnob(knobValue);
        float freqHz = 40.0f + norm * (1000.0f - 40.0f);

        snprintf(displayParameter1, sizeof(displayParameter1), "WAVFLD FRQ");
        snprintf(displayParameter2, sizeof(displayParameter2), "%.0f Hz", freqHz);
        break;
      }

    case 26:  // Master Lowpass Filter
      {
        int freqHz = (int)map(knobValue, 0, 1023, 1000, 7500);
        snprintf(displayParameter1, sizeof(displayParameter1), "LPF");
        snprintf(displayParameter2, sizeof(displayParameter2), "%d Hz", freqHz);
        break;
      }

    case 27:  // Master Tempo (BPM) — inactive during external clock, PPQN selector in mode
      {
        if (ppqnModeActive) {
          // Map knob to discrete PPQN bins
          int bin = map(knobValue, 0, 1023, 0, PPQN_OPTION_COUNT - 1);
          bin = constrain(bin, 0, PPQN_OPTION_COUNT - 1);
          ppqnModeSelection = PPQN_OPTIONS[bin];
          uint32_t t;
          noInterrupts();
          t = sysTickMs;
          interrupts();
          ppqnModeLastActivityTick = t;
          snprintf(displayParameter1, sizeof(displayParameter1), "PPQN");
          snprintf(displayParameter2, sizeof(displayParameter2), "%d", ppqnModeSelection);
          break;
        }

        if (tstate == RUN_EXT) {
          snprintf(displayParameter1, sizeof(displayParameter1), "EXT MODE");
          snprintf(displayParameter2, sizeof(displayParameter2), " ");
          break;
        }

        float norm = normKnob(knobValue);
        float bpmValue;

        if (norm <= 0.60f) {
          float blend = norm / 0.60f;
          bpmValue = 60.0f + blend * (240.0f - 60.0f);
        } else {
          float blend = (norm - 0.60f) / 0.40f;
          bpmValue = 240.0f + blend * (1000.0f - 240.0f);
        }

        // Round to nearest 0.5
        bpmValue = floorf(bpmValue * 2.0f + 0.5f) * 0.5f;

        snprintf(displayParameter1, sizeof(displayParameter1), "BPM");
        snprintf(displayParameter2, sizeof(displayParameter2), "%.1f", bpmValue);
        break;
      }

    case 28:  // Master Volume
      {
        float norm = normKnob(knobValue);
        int percent = (int)(norm * 100.0f + 0.5f);
        snprintf(displayParameter1, sizeof(displayParameter1), "MASTER VOL");
        snprintf(displayParameter2, sizeof(displayParameter2), "%d%%", percent);
        break;
      }

    case 29:  // Master Choke Offset
      {
        float norm = normKnob(knobValue);
        int percent = (int)((norm - 0.5f) * 200.0f);  // -100 to +100

        snprintf(displayParameter1, sizeof(displayParameter1), "DCAY OFFST");
        snprintf(displayParameter2, sizeof(displayParameter2), "%+d%%", percent);
        break;
      }

    case 30:  // Master Wavefolder Drive + Waveform
      {
        float norm = normKnob(knobValue);
        const char* shapeName;
        int percent;

        // Mirror the engine's half-split + deadband + quadratic curve
        float drive;
        if (norm <= 0.5f) {
          shapeName = "SIN";
          drive = norm / 0.5f;
        } else {
          shapeName = "SAW";
          drive = (norm - 0.5f) / 0.5f;
        }

        const float DEADBAND = 0.05f;
        if (drive <= DEADBAND) {
          percent = 0;
        } else {
          float driveNorm = (drive - DEADBAND) / (1.0f - DEADBAND);
          percent = (int)(driveNorm * driveNorm * 100.0f + 0.5f);
        }

        snprintf(displayParameter1, sizeof(displayParameter1), "WF %s", shapeName);
        snprintf(displayParameter2, sizeof(displayParameter2), "%d%%", percent);
        break;
      }

    case 31:  // Master Delay Mix/Feedback
      {
        int percent = (int)map(knobValue, 0, 1023, 0, 100);
        snprintf(displayParameter1, sizeof(displayParameter1), "DLY AMOUNT");
        snprintf(displayParameter2, sizeof(displayParameter2), "%d%%", percent);
        break;
      }

      // ========================================================================
      // DEFAULT
      // ========================================================================

    default:
      {
        displayParameter1[0] = 0;
        displayParameter2[0] = 0;
        break;
      }
  }
}

inline void applyKnobToEngine(byte idx, int knobValue) {
  switch (idx) {

      // ========================================================================
      // D1 - KICK DRUM (Knobs 0-7)
      // ========================================================================

    case 0:  // D1 Drive/Distortion
      {
        float norm = normKnob(knobValue);
        float wavefolderAmp = 0.1f + knobValue * 0.001f;

        d1DCwf.amplitude(wavefolderAmp);
        d1DistPitchBoost = 0.03f * norm;
        applyD1Freq();
        break;
      }

    case 1:  // D1 Waveform Shape
      {
        float norm = normKnob(knobValue);
        uiMixD1Shape = norm;

        // Always keep a sine fundamental present
        const float SIN_FLOOR = 0.35f;
        const float SIN_CEIL = 0.85f;
        const float G_SAW_MAX = 0.55f;
        const float G_SQR_MAX = 0.55f;

        // Calculate voice gains
        float gainSine = SIN_CEIL - (SIN_CEIL - SIN_FLOOR) * norm;
        float gainSaw = 0.0f;
        float gainSquare = 0.0f;

        // Harmonic crossfade: sine → saw → square
        if (norm <= 0.50f) {
          float blend = norm / 0.50f;
          gainSaw = G_SAW_MAX * blend;
          gainSquare = 0.0f;
        } else {
          float blend = (norm - 0.50f) / 0.50f;
          gainSaw = G_SAW_MAX * (1.0f - blend);
          gainSquare = G_SQR_MAX * blend;
        }

        // Headroom management
        float sum = gainSine + gainSaw + gainSquare;
        float scale = 1.0f;
        const float SUM_TARGET = 0.95f;
        if (sum > SUM_TARGET) {
          scale = SUM_TARGET / sum;
        }

        AudioNoInterrupts();
        d1OscMixer.gain(0, gainSine * scale);
        d1OscMixer.gain(1, gainSaw * scale);
        d1OscMixer.gain(2, gainSquare * scale);
        d1OscMixer.gain(3, 0.0f);
        AudioInterrupts();
        break;
      }

    case 2:  // D1 Decay
      {
        float norm = normKnob(knobValue);
        float decayMs;

        if (norm <= 0.25f) {
          float blend = norm / 0.25f;
          decayMs = 50.0f + (70.0f - 50.0f) * blend;
        } else {
          float blend = (norm - 0.25f) / 0.75f;
          decayMs = 71.0f + (1000.0f - 71.0f) * blend;
        }

        d1DecayBase = decayMs;
        applyChokeToDecays();
        break;
      }

    case 3:  // D1 Pitch
      {
        float norm = normKnob(knobValue);
        float freqHz;

        if (norm <= 0.33f) {
          float blend = norm / 0.33f;
          freqHz = 60.0f + (105.0f - 60.0f) * blend;
        } else {
          float blend = (norm - 0.33f) / 0.67f;
          freqHz = 106.0f + (500.0f - 106.0f) * blend;
        }

        d1BaseFreq = freqHz;
        applyD1Freq();
        break;
      }

    case 4:  // D1 Volume
      {
        float norm = normKnob(knobValue);
        d1Vol = norm * 0.90f;  // Scaled down ~10% to balance with D2/D3
        drumMixer.gain(0, d1Vol);
        updateDrumDelayGains();
        break;
      }

    case 5:  // D1 Attack/Snap
      {
        float norm = normKnob(knobValue);

        // Pitch snap depth increases with knob
        float pitchDepth = 0.60f + (0.40f * norm);
        drum1.pitchMod(pitchDepth);

        // Transient emphasis, capped at unity
        float transientGain = 0.85f + (0.15f * norm);
        d1Mixer.gain(1, transientGain);

        // Store for envelope shaping in triggerD1()
        d1AttackAmt = norm;
        break;
      }

    case 6:  // D1 EQ (Body)
      {
        float norm = normKnob(knobValue);
        float eqAmount = 0.4f + 0.6f * norm;

        float blend;
        if (eqAmount <= 0.5f) {
          blend = eqAmount / 0.5f;
        } else {
          blend = 1.0f + (eqAmount - 0.5f) / 0.5f;
        }

        float notchFreq0 = 200.0f + 200.0f * blend;
        float notchFreq1 = notchFreq0 * (1.15f + 0.05f * norm);
        float shelfGain = 1.25f * blend;

        AudioNoInterrupts();
        d1EQ.setNotch(0, notchFreq0, 3);
        d1EQ.setNotch(1, notchFreq1, 1);
        d1EQ.setHighShelf(2, 3500.0f, shelfGain, 0.9f);
        AudioInterrupts();
        break;
      }

    case 7:  // D1 Delay Send
      {
        float delaySend = map(knobValue, 0, 1023, 0, 75) * 0.01f;
        d1DelaySend = delaySend;
        updateDrumDelayGains();
        break;
      }

      // ========================================================================
      // D2 - SNARE/CLAP (Knobs 8-15)
      // ========================================================================

    case 8:  // D2 Pitch
      {
        // d2b fixed at 1000Hz — set once in audioInit(), not here
        float freqHz = mapf(knobValue, 0, 1023, 100.0f, 300.0f);
        d2.frequency(freqHz);
        break;
      }

    case 9:  // D2 Decay
      {
        float decayMs = (knobValue < 512)
                          ? map(knobValue, 0, 511, 50, 300) * 0.1f
                          : map(knobValue, 512, 1023, 300, 1000) * 0.1f;
        d2DecayBase = decayMs * 10.0f;
        applyChokeToDecays();
        break;
      }

    case 10:  // D2 Voice Mix - Snare/Clap
      {
        float norm = normKnob(knobValue);
        uiMixD2Voice = constrain(norm, 0.0f, 1.0f);

        float gainSnare = norm;
        float gainClap = 1.0f - norm;

        snareClapMixer.gain(0, gainSnare * 0.95f);
        snareClapMixer.gain(1, gainClap * 1.05f);
        break;
      }

    case 11:  // D2 Wavefolder Drive
      {
        float norm = normKnob(knobValue);
        bool offZone = (knobValue < 10);

        // Default values (off state)
        float driveGain = 0.8f;
        float freqHz = 10.0f;
        float lpfFreqHz = 3500.0f;
        float gainDry = 1.0f;
        float gainWet = 0.0f;

        if (!offZone) {
          driveGain = 0.75f + 0.15f * norm;
          freqHz = 20.0f + norm * 800.0f;

          // Wet mix comes in gradually with quadratic curve
          const float WET_START = 0.10f;
          float wetNorm = 0.0f;
          if (norm > WET_START) {
            float blend = (norm - WET_START) / (1.0f - WET_START);
            wetNorm = blend * blend;
          }

          // LPF tracks inversely with drive
          if (norm <= 0.4f) {
            lpfFreqHz = 2500.0f;
          } else {
            float blend = (norm - 0.4f) / 0.6f;
            float blendSquared = blend * blend;
            lpfFreqHz = 2500.0f - blendSquared * (2500.0f - 1500.0f);
          }

          // Mix calculations — tamed to prevent excessive volume at high drive
          const float TOTAL_TARGET = 0.35f;
          const float WET_ATTEN = 0.45f;

          float wetMix = wetNorm;
          if (wetMix > 1.0f) wetMix = 1.0f;
          float dryMix = 1.0f - wetMix;

          gainDry = dryMix * TOTAL_TARGET;
          gainWet = wetMix * TOTAL_TARGET * WET_ATTEN;

          // Top-end trim to prevent excessive volume
          float topTrim = 1.0f - 0.40f * norm;
          gainDry *= topTrim;
          gainWet *= topTrim;
        }

        // Apply as one atomic update
        AudioNoInterrupts();
        d2WfAmp.gain(driveGain);
        d2WfSine.frequency(freqHz);
        d2WfLowpass.frequency(lpfFreqHz);
        d2MasterMixer.gain(0, gainDry);  // dry
        d2MasterMixer.gain(2, gainWet);  // wavefolder return
        AudioInterrupts();
        break;
      }

    case 12:  // D2 Delay Send
      {
        float delaySend = map(knobValue, 0, 1023, 0, 75) * 0.01f;
        d2DelaySend = delaySend;
        updateDrumDelayGains();
        break;
      }

    case 13:  // D2 Reverb
      {
        float norm = normKnob(knobValue);

        if (norm < 0.05f) {
          // Off
          AudioNoInterrupts();
          d2MasterMixer.gain(1, 0.0f);
          AudioInterrupts();
        } else {
          // Active reverb
          float blend = (norm - 0.05f) / 0.95f;
          float roomSize = 0.3f + blend * 0.6f;
          float damping = 1.0f;

          AudioNoInterrupts();
          d2Verb.roomsize(roomSize);
          d2Verb.damping(damping);
          d2MasterMixer.gain(1, blend);
          AudioInterrupts();
        }
        break;
      }

    case 14:  // D2 Noise
      {
        if (knobValue >= 5) {
          float decayMs = (knobValue < 512)
                            ? mapf(knobValue, 5, 511, 30.0f, 70.0f)
                            : mapf(knobValue, 512, 1023, 70.0f, 200.0f);

          d2NoiseEnvelope.hold(decayMs * 0.5f);
          d2NoiseEnvelope.decay(decayMs);

          float filterFreqHz = 3000.0f + 10.0f * decayMs;
          d2NoiseFilter.frequency(filterFreqHz);

          float norm = normKnob(knobValue);
          float noiseGain = 0.05f + 0.30f * norm;
          d2Mixer.gain(2, noiseGain);

          applyChokeToDecays();
        } else {
          // Off
          d2Mixer.gain(2, 0.0f);
        }
        break;
      }

    case 15:  // D2 Volume
      {
        float norm = normKnob(knobValue);
        float volume = norm * 3.5f;
        if (volume > 2.3f) volume = 2.3f;

        d2Vol = volume;
        drumMixer.gain(1, d2Vol);
        updateDrumDelayGains();
        break;
      }

      // ========================================================================
      // D3 - HI-HAT (Knobs 16-23)
      // ========================================================================

    case 16:  // D3 Pitch
      {
        float norm = normKnob(knobValue);

        // --- Voice 1 (606-ish hats) tuning ---
        const float BEND_START = 0.25f;
        float pitchBend;

        if (norm <= BEND_START) {
          float blend = norm / BEND_START;
          pitchBend = 0.60f * blend;
        } else {
          float blend = (norm - BEND_START) / (1.0f - BEND_START);
          pitchBend = 0.60f + 0.40f * (blend * blend);
        }

        const float hatMinBaseHz = 400.0f;
        const float hatMaxBaseHz = 6000.0f;
        const float hatCurve = pitchBend * pitchBend;
        const float hatBaseHz = hatMinBaseHz * powf(hatMaxBaseHz / hatMinBaseHz, hatCurve);

        // Gentle filter tracking (keeps volume steadier)
        const float hpfHz = 6800.0f * (1.0f + 0.06f * norm);
        const float bpfHz = 9800.0f * (1.0f + 0.05f * norm);

        // --- Voice 2 (FM hats) tuning ---
        float fmBend;
        if (norm <= BEND_START) {
          fmBend = 0.60f * (norm / BEND_START);
        } else {
          float blend = (norm - BEND_START) / (1.0f - BEND_START);
          fmBend = 0.60f + 0.40f * (blend * blend);
        }

        const float c1Min = 400.0f, c1Max = 2400.0f;
        const float c2Min = 600.0f, c2Max = 4200.0f;
        const float r1Min = 2.0f, r1Max = 4.0f;
        const float r2Min = 4.0f, r2Max = 6.0f;

        const float carrier1Hz = c1Min * powf(c1Max / c1Min, fmBend);
        const float carrier2Hz = c2Min * powf(c2Max / c2Min, fmBend);
        const float ratio1 = r1Min * powf(r1Max / r1Min, fmBend);
        const float ratio2 = r2Min * powf(r2Max / r2Min, fmBend);
        const float modulator1Hz = carrier1Hz * ratio1;
        const float modulator2Hz = carrier2Hz * ratio2;

        // FM depth: slightly more at the top end, but still capped
        const float depth1 = 0.06f + 0.52f * (fmBend * fmBend);
        const float depth2 = 0.04f + 0.44f * (fmBend * fmBend);

        AudioNoInterrupts();

        // Voice 2 FM hats
        d3W1.frequency(carrier1Hz);
        d3W3.frequency(carrier2Hz);
        d3W2.frequency(modulator1Hz);
        d3W4.frequency(modulator2Hz);
        d3W2.amplitude(depth1);
        d3W4.amplitude(depth2);

        // Voice 1 606 hat oscillator bank
        d3606W1.frequency(hatBaseHz * 1.00f);
        d3606W2.frequency(hatBaseHz * 1.08f);
        d3606W3.frequency(hatBaseHz * 1.17f);
        d3606W4.frequency(hatBaseHz * 1.26f);
        d3606W5.frequency(hatBaseHz * 1.36f);
        d3606W6.frequency(hatBaseHz * 1.48f);
        d3606HPF.frequency(hpfHz);
        d3606BPF.frequency(bpfHz);

        // Voice 3 noise-based hat
        drum3.frequency(carrier1Hz * 0.5f);

        AudioInterrupts();
        break;
      }

    case 17:  // D3 Decay
      {
        float decayMs = map(knobValue, 0, 1023, 100, 1200) * 0.1f;
        d3DecayBase = decayMs;
        applyChokeToDecays();
        break;
      }

    case 18:  // D3 Voice Mix - 3-way
      {
        const int value = constrain(knobValue, 0, 1023);

        // Zone boundaries
        const int PURE1_MAX = int(1023 * 0.06f);
        const int PURE2_MIN = int(1023 * 0.46f);
        const int PURE2_MAX = int(1023 * 0.65f);
        const int PURE3_MIN = int(1023 * 0.94f);

        const float MAX_GAIN = 0.9f;

        // Voice level trims
        const float VOICE1_BOOST = 3.8f;
        const float VOICE2_TRIM = 2.25f;
        const float VOICE3_TRIM = 0.65f;

        float gainVoice1 = 0.0f;
        float gainVoice2 = 0.0f;
        float gainVoice3 = 0.0f;

        // Calculate voice gains based on knob position
        if (value <= PURE1_MAX) {
          gainVoice1 = 1.0f;
        } else if (value < PURE2_MIN) {
          const float blend = float(value - PURE1_MAX) / float(PURE2_MIN - PURE1_MAX);
          gainVoice1 = 1.0f - blend;
          gainVoice2 = blend;
        } else if (value <= PURE2_MAX) {
          gainVoice2 = 1.0f;
        } else if (value < PURE3_MIN) {
          const float blend = float(value - PURE2_MAX) / float(PURE3_MIN - PURE2_MAX);
          gainVoice2 = 1.0f - blend;
          gainVoice3 = blend;
        } else {
          gainVoice3 = 1.0f;
        }

        AudioNoInterrupts();
        d3WfMixer.gain(0, gainVoice1 * MAX_GAIN * VOICE1_BOOST);
        d3WfMixer.gain(1, gainVoice2 * MAX_GAIN * VOICE2_TRIM);
        d3WfMixer.gain(2, gainVoice3 * MAX_GAIN * VOICE3_TRIM);
        AudioInterrupts();
        break;
      }

    case 19:  // D3 Wavefolder Drive
      {
        // True OFF in the first tiny region
        if (knobValue < 10) {
          d3DCwf.amplitude(0.0f);
          d3Mixer.gain(3, 0.0f);
          break;
        }

        float norm = normKnob(knobValue);
        norm = (norm - 0.10f) / 0.90f;
        if (norm < 0.0f) norm = 0.0f;
        if (norm > 1.0f) norm = 1.0f;

        // Drive goes 0..0.50 (linear)
        const float drive = 0.50f * norm;
        d3DCwf.amplitude(drive);

        // Wet return: comes in, but gets pulled down as drive rises
        const float wetGain = 0.10f * norm * (1.0f - 0.65f * norm);
        d3Mixer.gain(3, wetGain);
        break;
      }

    case 20:  // D3 Delay Send
      {
        float delaySend = map(knobValue, 0, 1023, 0, 33) * 0.1f;
        d3DelaySend = delaySend;
        updateDrumDelayGains();
        break;
      }

    case 21:  // D3 Filter
      {
        float norm = normKnob(knobValue);

        // Cutoff: quadratic curve so the last 20 percent is dramatic
        float normSquared = norm * norm;
        float cutoffHz = 4500.0f + normSquared * (8000.0f - 4500.0f);

        // Resonance: cubic curve so it stays tame, then goes wild
        float resonance = 0.2f + (norm * norm * norm) * (0.9f - 0.2f);

        AudioNoInterrupts();
        d3MasterFilter.frequency(cutoffHz);
        d3MasterFilter.resonance(resonance);
        AudioInterrupts();
        break;
      }

    case 22:  // D3 Accent Pattern
      {
        uint8_t prevMode = d3AltMode;
        const int ACCENT_DEADBAND = 24;

        // Determine mode from knob position
        if (knobValue <= ACCENT_DEADBAND) {
          d3AltMode = ALT_OFF;
        } else {
          int zone = map(knobValue, ACCENT_DEADBAND + 1, 1023, 1, 12);
          switch (zone) {
            case 1: d3AltMode = ALT_HALF; break;
            case 2: d3AltMode = ALT_QUARTER; break;
            case 3: d3AltMode = ALT_EIGHT; break;
            case 4: d3AltMode = ALT_EIGHTUP; break;
            case 5: d3AltMode = ALT_VARI1; break;
            case 6: d3AltMode = ALT_VARI2; break;
            case 7: d3AltMode = ALT_VARI3; break;
            case 8: d3AltMode = ALT_VARI4; break;
            case 9: d3AltMode = ALT_ALT5; break;
            case 10: d3AltMode = ALT_ALT6; break;
            case 11: d3AltMode = ALT_ALT7; break;
            case 12: d3AltMode = ALT_ALT8; break;
            default: d3AltMode = ALT_ALT9; break;
          }
        }

        // Update D3 base decay to be coherent with new mode
        {
          float baseDecay = d3EffectiveBaseDecay();
          AudioNoInterrupts();
          d3AmpEnv.decay(baseDecay);
          d3606AmpEnv.decay(baseDecay);
          drum3.length(baseDecay);
          AudioInterrupts();
        }

        // If mode changed, show the pattern briefly on step LEDs
        if (d3AltMode != prevMode) {
          accentPreviewMask = accentMaskFromMode(d3AltMode);

          uint32_t nowTick;
          noInterrupts();
          nowTick = sysTickMs;  // sysTickMs is ISR-written
          interrupts();

          accentPreviewUntilTick = nowTick + accentPreviewDurationTicks;
          accentPreviewActive = true;  // Main-loop only — no guard needed

          updateLEDs();
        }
        break;
      }

    case 23:  // D3 Volume
      {
        float norm = normKnob(knobValue);
        float volume = norm * 5.0f;

        d3Vol = volume;
        drumMixer.gain(2, d3Vol);
        updateDrumDelayGains();
        break;
      }

      // ========================================================================
      // MASTER EFFECTS (Knobs 24-31)
      // ========================================================================

    case 24:  // Master Delay Time (quantized to tempo)
      {
        float msPerBeat = 60000.0f / bpm;
        float maxRatio = 1400.0f / msPerBeat;

        // Find maximum available ratio index within delay time limit
        int maxIdx = 0;
        for (int i = 0; i < numRatios; i++) {
          if (quantizeRatios[i] <= maxRatio) {
            maxIdx = i;
          } else {
            break;
          }
        }

        int ratioIdx = (int)((knobValue / 1023.0f) * maxIdx + 0.5f);
        ratioIdx = constrain(ratioIdx, 0, maxIdx);

        float delayMs = msPerBeat * quantizeRatios[ratioIdx];
        if (delayMs > 1400.0f) delayMs = 1400.0f;
        if (delayMs < 0.0f) delayMs = 0.0f;

        masterDelay.delay(0, delayMs);
        break;
      }

    case 25:  // Master Wavefolder Frequency
      {
        float norm = normKnob(knobValue);
        float freqHz = 40.0f + norm * (1000.0f - 40.0f);
        masterWfWaveform.frequency(freqHz);
        break;
      }

    case 26:  // Master Lowpass Filter
      {
        int freqHz = map(knobValue, 0, 1023, 1000, 7500);
        masterLPFDisp = map(knobValue, 0, 1023, 70, 78);
        masterLowPass.frequency(freqHz);
        break;
      }

    case 27:  // Master Tempo (BPM) — inactive during external clock / PPQN mode
      {
        if (ppqnModeActive) break;     // Don't change BPM while selecting PPQN
        if (tstate == RUN_EXT) break;  // Ignore knob when ext clock drives tempo

        float norm = normKnob(knobValue);
        float bpmValue;

        if (norm <= 0.60f) {
          float blend = norm / 0.60f;
          bpmValue = 60.0f + blend * (240.0f - 60.0f);
        } else {
          float blend = (norm - 0.60f) / 0.40f;
          bpmValue = 240.0f + blend * (1000.0f - 240.0f);
        }

        // Round to nearest 0.5
        bpm = floorf(bpmValue * 2.0f + 0.5f) * 0.5f;

        // If internal clock is currently running, retime it
        if (tstate == RUN_INT) {
          rearmStepTimer();
        }
        break;
      }

    case 28:  // Master Volume
      {
        float norm = normKnob(knobValue);
        float volume = norm * 2.0f;

        masterVolumeDisp = volume * 50.0f;
        masterNominalGain = volume * 5.0f;
        applyMasterGainFromState();
        break;
      }

    case 29:  // Master Choke Offset
      {
        chokeOffsetMs = chokeOffsetFromKnob(knobValue);
        applyChokeToDecays();
        break;
      }

    case 30:  // Master Wavefolder Drive + Waveform
      {
        float norm = normKnob(knobValue);

        // Bottom half = sine, top half = saw
        short waveType;
        float drive;
        if (norm <= 0.5f) {
          waveType = WAVEFORM_SINE;
          drive = norm / 0.5f;
        } else {
          waveType = WAVEFORM_SAWTOOTH;
          drive = (norm - 0.5f) / 0.5f;
        }

        masterWfWaveform.begin(waveType);

        // Deadband at bottom of each half (~5% of half-range)
        const float DEADBAND = 0.05f;
        if (drive <= DEADBAND) {
          AudioNoInterrupts();
          masterWfWaveform.amplitude(0.0f);
          masterWfComp = 1.0f;
          masterMixer.gain(0, 1.0f);  // dry
          masterMixer.gain(1, 1.0f);  // wavefolder return
          AudioInterrupts();

          applyMasterGainFromState();
          break;
        }

        // Remap past deadband to 0-1
        float driveNorm = (drive - DEADBAND) / (1.0f - DEADBAND);
        if (driveNorm > 1.0f) driveNorm = 1.0f;

        // Softer feel at low end (quadratic curve)
        float driveSq = driveNorm * driveNorm;
        float wavefolderAmp = driveSq * 0.55f;
        masterWfWaveform.amplitude(wavefolderAmp);

        // Keep dry from inflating too much when folding rises
        float dryBoost = 1.0f + 0.08f * driveSq;
        if (dryBoost > 1.08f) dryBoost = 1.08f;

        // Pull the folding return down as drive rises
        float wfReturn = 1.0f - 0.65f * driveSq;
        if (wfReturn < 0.25f) wfReturn = 0.25f;

        // Global loudness compensation (stronger to tame volume past 50%)
        const float COMP_K = 1.80f;
        masterWfComp = 1.0f / (1.0f + (COMP_K * driveSq));

        AudioNoInterrupts();
        masterMixer.gain(0, dryBoost);
        masterMixer.gain(1, wfReturn);
        AudioInterrupts();

        applyMasterGainFromState();
        break;
      }

    case 31:  // Master Delay Mix/Feedback
      {
        float norm = normKnob(knobValue);

        // Dead band below 5% — delay fully OFF
        if (norm <= 0.05f) {
          delayAmp.gain(0.0f);
          masterMixer.gain(2, 0.0f);  // delay return
        } else {
          // Remap 5%-100% → 0-1, then use sqrt curve for more level at low knob
          float blend = (norm - 0.05f) / 0.95f;
          float shaped = sqrtf(blend);  // sqrt gives more audible range at low settings
          delayAmp.gain(shaped * 2.0f);
          masterMixer.gain(2, shaped);
        }

        // Feedback amount (squared curve for finer low-end control), capped at 0.4
        float feedback = norm * norm * 0.4f;
        delayMixer.gain(3, feedback);
        break;
      }

    default:
      break;
  }
}

// initialization from hardware

void initKnobsFromHardware() {
  // Read all knobs, settle filters, apply to engine, and store boot values.
  // Boot-lock prevents ADC drift from changing params until the user moves a knob.
  for (byte idx = 0; idx < knobCount; idx++) {
    int rawValue = readKnobRaw(idx);

    // Update filter multiple times to settle it
    for (int i = 0; i < 4; i++) {
      analog[idx]->update(rawValue);
    }

    knobBootValue[idx] = rawValue;
    knobUnlocked[idx] = false;

    applyKnobToEngine(idx, rawValue);
  }
}

// OPTIMIZATION: round-robin knob scanning — reads 8 knobs per loop iteration
// instead of all 32, reducing per-loop delayMicroseconds overhead by 4x
void updateKnobs() {
  static uint8_t knobScanGroup = 0;
  byte start = knobScanGroup * 8;
  byte end = start + 8;

  for (byte idx = start; idx < end; idx++) {
    int rawValue = readKnobRaw(idx);
    analog[idx]->update(rawValue);

    if (analog[idx]->hasChanged()) {
      int knobValue = analog[idx]->getValue();

      // Boot-lock: ignore small ADC drift until the user intentionally moves
      // the knob past the unlock threshold from its power-on position.
      if (!knobUnlocked[idx]) {
        int diff = knobValue - knobBootValue[idx];
        if (diff < 0) diff = -diff;
        if (diff < KNOB_UNLOCK_THRESHOLD) continue;
        knobUnlocked[idx] = true;
      }

      updateParameterDisplay(idx, knobValue);
      applyKnobToEngine(idx, knobValue);
    }
  }

  knobScanGroup = (knobScanGroup + 1) & 3;  // 0,1,2,3 → cycle through 4 groups
}

// Oscilloscope functions — AC waveform from AudioRecordQueue

void updateScopeData() {
  static uint8_t blockSkipCounter = 0;
  const uint8_t BLOCKS_TO_SKIP = 8;
  const int SAMPLE_DECIMATION = 16;

  while (scopeQueue.available() >= 1) {
    int16_t* samples = scopeQueue.readBuffer();

    if (++blockSkipCounter < BLOCKS_TO_SKIP) {
      scopeQueue.freeBuffer();
      continue;
    }
    blockSkipCounter = 0;

    for (int i = 0; i < 128 / SAMPLE_DECIMATION && scopeBufferWriteIndex < SCOPE_DISPLAY_WIDTH; i++) {
      scopeBuffer[scopeBufferWriteIndex] = samples[i * SAMPLE_DECIMATION] / 32768.0f;
      scopeBufferWriteIndex = (scopeBufferWriteIndex + 1) % SCOPE_DISPLAY_WIDTH;
    }

    scopeQueue.freeBuffer();
    break;
  }
}

// Draw text with a 1-pixel black outline for readability over the scope.
// Draws the string 8 times in black at surrounding offsets, then once in white.
void drawOutlinedText(int x, int y, const char* text) {
  static const int8_t dx[] = {-1, 0, 1, -1, 1, -1, 0, 1};
  static const int8_t dy[] = {-1, -1, -1, 0, 0, 1, 1, 1};
  for (uint8_t i = 0; i < 8; i++) {
    display.setTextColor(0);
    display.setCursor(x + dx[i], y + dy[i]);
    display.print(text);
  }
  display.setTextColor(1);
  display.setCursor(x, y);
  display.print(text);
}

void drawScopeWaveform(int x, int y, int w, int h) {
  // Find min/max for AC auto-scaling (signed signal)
  float minVal = 0.0f, maxVal = 0.0f;
  for (int i = 0; i < SCOPE_DISPLAY_WIDTH; i++) {
    if (scopeBuffer[i] < minVal) minVal = scopeBuffer[i];
    if (scopeBuffer[i] > maxVal) maxVal = scopeBuffer[i];
  }

  float range = maxVal - minVal;
  if (range < 0.001f) return;           // nothing to draw
  if (range < 0.01f) range = 0.01f;     // floor for very quiet signals

  float vScale = (h * 0.9f) / range;

  // Snapshot write index so it doesn't change mid-render
  uint8_t writeSnap = scopeBufferWriteIndex;

  // Draw waveform (scrolling, AC-coupled — shows peaks and valleys)
  for (int i = 0; i < SCOPE_DISPLAY_WIDTH - 1; i++) {
    int i1 = (writeSnap + i) % SCOPE_DISPLAY_WIDTH;
    int i2 = (writeSnap + i + 1) % SCOPE_DISPLAY_WIDTH;

    int y1 = y + h / 2 - (int)((scopeBuffer[i1] - minVal) * vScale - range * vScale / 2);
    int y2 = y + h / 2 - (int)((scopeBuffer[i2] - minVal) * vScale - range * vScale / 2);

    y1 = constrain(y1, y, y + h - 1);
    y2 = constrain(y2, y, y + h - 1);

    int x1 = x + (i * w) / SCOPE_DISPLAY_WIDTH;
    int x2 = x + ((i + 1) * w) / SCOPE_DISPLAY_WIDTH;

    display.drawLine(x1, y1, x2, y2, 1);
  }
}

// UI helpers

static inline void drawCaretRail(int x, int y, int w, int h, int caretX) {
  display.drawRect(x, y, w, h, 1);
  int cx = constrain(caretX, x + 1, x + w - 2);
  display.drawLine(cx, y - 2, cx, y + h + 2, 1);
}

static inline void labelAtCenter(const char* text, int centerX, int y) {
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  int x = centerX - (w / 2);
  display.setCursor(x, y);
  display.print(text);
}

void renderVoiceRails() {
  // Rail displayed alongside scope: label text at y=23, rail bar at y=44
  const int railX = 6;
  const int railY = 44;
  const int railW = 116;
  const int railH = 10;
  const int labelY = 23;

  display.setTextSize(1);

  if (activeRail == RAIL_D1_SHAPE) {
    drawCaretRail(railX, railY, railW, railH, railX + (int)(uiMixD1Shape * railW));
    int third = railW / 3;
    labelAtCenter("SIN", railX + third * 0 + third / 2, labelY);
    labelAtCenter("SAW", railX + third * 1 + third / 2, labelY);
    labelAtCenter("SQR", railX + third * 2 + third / 2, labelY);
    int zone = (uiMixD1Shape < 1.0f / 3.0f)   ? 0
               : (uiMixD1Shape < 2.0f / 3.0f) ? 1
                                              : 2;
    int zx = railX + zone * third + 1;
    int zw = third - 2;
    display.fillRect(zx, railY + 1, zw, railH - 2, 1);
  }

  if (activeRail == RAIL_D2_VOICE) {
    int caret = railX + (int)(uiMixD2Voice * railW);
    drawCaretRail(railX, railY, railW, railH, caret);
    labelAtCenter("CLP", railX + 10, labelY);
    labelAtCenter("SNR", railX + railW - 10, labelY);
    int fillW = max(1, caret - railX);
    display.fillRect(railX + 1, railY + 1, fillW - 2, railH - 2, 1);
  }

  if (activeRail == RAIL_D3_VOICE) {
    drawCaretRail(railX, railY, railW, railH, railX + (int)(uiMixD3Voice * railW));
    int third = railW / 3;
    labelAtCenter("1", railX + third * 0 + third / 2, labelY);
    labelAtCenter("2", railX + third * 1 + third / 2, labelY);
    labelAtCenter("3", railX + third * 2 + third / 2, labelY);
    int zone = (uiMixD3Voice < 1.0f / 3.0f)   ? 0
               : (uiMixD3Voice < 2.0f / 3.0f) ? 1
                                              : 2;
    int zx = railX + zone * third + 1;
    int zw = third - 2;
    display.fillRect(zx, railY + 1, zw, railH - 2, 1);
  }
}

// Track last drawn state
static int lastDrawnBpm10 = -1;
static uint8_t lastDrawnTrack = 255;
static int lastDrawnLPF = -1;
static uint8_t lastDrawnSlot = 255;
static bool lastDrawnPlaying = false;
static uint32_t lastDrawnOverlayStart = 0;
static uint8_t lastDrawnKnob = 255;
static uint8_t lastDrawnRail = 255;
static char lastDrawnParam1[32] = { 0 };
static char lastDrawnParam2[32] = { 0 };

// OLED drawing
void updateDisplay() {
  // Snapshot time first
  uint32_t nowMs;
  noInterrupts();
  nowMs = sysTickMs;
  interrupts();

  // PPQN selection mode — full-screen takeover, skip normal UI
  if (ppqnModeActive) {
    display.clearDisplay();
    display.setFont(NULL);
    display.setTextColor(1);
    display.setTextWrap(false);

    display.setTextSize(1);
    display.setCursor(20, 0);
    display.print("CLOCK MODE");

    display.setCursor(4, 16);
    display.print("TURN BPM TO CHANGE");

    display.setTextSize(2);
    display.setCursor(16, 38);
    display.print("PPQN:");
    display.print(ppqnModeSelection);

    display.display();

    // Update watchdog timestamp so OLED reinit doesn't fire during PPQN mode
    noInterrupts();
    lastFrameDrawTick = nowMs;
    interrupts();
    return;
  }

  // Snapshot all state we will render so a single frame is coherent
  float bpmSnap;
  uint8_t trackSnap;
  int lpfSnap;
  uint8_t slotSnap;
  bool playingSnap;
  uint32_t overlayStartSnap;
  uint8_t knobSnap;
  uint8_t railSnap;
  char param1Snap[32];
  char param2Snap[32];

  // Snapshot ISR-shared state with interrupts disabled
  noInterrupts();
  playingSnap = sequencePlaying;
  interrupts();

  // Main-loop only — no ISR access, safe without guards
  bpmSnap = bpm;
  trackSnap = activeTrack;
  lpfSnap = masterLPFDisp;
  slotSnap = activeSaveSlot;
  overlayStartSnap = parameterOverlayStartTick;
  knobSnap = lastActiveKnob;
  railSnap = activeRail;

  strncpy(param1Snap, displayParameter1, sizeof(param1Snap) - 1);
  param1Snap[sizeof(param1Snap) - 1] = 0;

  strncpy(param2Snap, displayParameter2, sizeof(param2Snap) - 1);
  param2Snap[sizeof(param2Snap) - 1] = 0;

  // Overlay state
  bool overlayActiveNow =
    ((uint32_t)(nowMs - overlayStartSnap) < parameterOverlayDurationTicks);

  bool overlayActiveLastDraw =
    ((uint32_t)(nowMs - lastDrawnOverlayStart) < parameterOverlayDurationTicks);

  // Compare BPM by what we actually display (1 decimal place)
  int bpm10 = (int)(bpmSnap * 10.0f + 0.5f);

  // Check if anything changed
  bool somethingChanged =
    (lastDrawnBpm10 != bpm10) || (lastDrawnTrack != trackSnap) || (lastDrawnLPF != lpfSnap) || (lastDrawnSlot != slotSnap) || (lastDrawnPlaying != playingSnap) || (overlayActiveNow != overlayActiveLastDraw) || (lastDrawnKnob != knobSnap) || (lastDrawnRail != railSnap) || (strcmp(lastDrawnParam1, param1Snap) != 0) || (strcmp(lastDrawnParam2, param2Snap) != 0);

  // Always redraw — scope is always visible and animating
  somethingChanged = true;

  // If nothing changed, skip the redraw
  if (!somethingChanged) {
    return;
  }

  display.clearDisplay();

  // Setup text defaults
  display.setFont(NULL);
  display.setTextColor(1);
  display.setTextSize(1);
  display.setTextWrap(false);

  // Draw border
  display.drawLine(1, 21, 1, 63, 1);
  display.drawLine(1, 20, 127, 20, 1);
  display.drawLine(0, 63, 0, 20, 1);
  display.drawLine(1, 19, 127, 19, 1);

  // BPM / EXT clock state — always show one decimal place
  display.setCursor(0, 0);
  if (tstate == RUN_EXT) {
    display.print("EXT");
    display.setCursor(0, 10);
    if (extBpmDisplay > 0) {
      display.print(extBpmDisplay, 1);
    }
  } else {
    display.print("BPM");
    display.setCursor(0, 10);
    display.print(bpmSnap, 1);
  }

  // Track
  display.setCursor(37, 0);
  display.print("TRK");
  display.setCursor(40, 10);
  if (trackSnap == TRACK_D1) {
    display.print("D1");
  } else if (trackSnap == TRACK_D2) {
    display.print("D2");
  } else {
    display.print("D3");
  }

  // LPF
  display.setCursor(62, 0);
  display.print("LPF");
  if (lpfSnap >= 78) {
    display.setCursor(62, 10);
    display.print("OFF");
  } else {
    int barEnd = lpfSnap;
    if (barEnd < 62) barEnd = 62;
    display.drawLine(62, 10, barEnd, 10, 1);
    display.drawLine(barEnd + 4, 16, barEnd + 1, 11, 1);
  }

  // Memory slot
  display.setCursor(90, 0);
  display.print("MEM");
  display.setCursor(92, 10);
  display.print(slotSnap);

  // Play/Stop icon
  if (playingSnap) {
    display.drawBitmap(116, 4, image_play_bits, 10, 10, 1);
  } else {
    display.drawBitmap(116, 4, image_stop_bits, 10, 10, 1);
  }

  // Oscilloscope — always drawn (not replaced by overlay)
  drawScopeWaveform(2, 22, SCOPE_DISPLAY_WIDTH, SCOPE_DISPLAY_HEIGHT);

  // Parameter overlay at bottom of screen (on top of scope)
  if (overlayActiveNow) {
    if (railSnap == RAIL_NONE) {
      // Normal parameter: name left-aligned, value right-aligned at bottom
      // Outlined text keeps scope waveform visible behind the overlay
      display.setTextSize(1);
      const int bottomY = 56;
      if (param1Snap[0]) {
        drawOutlinedText(5, bottomY, param1Snap);
      }
      if (param2Snap[0]) {
        int16_t x1, y1;
        uint16_t w2, h2;
        display.getTextBounds(param2Snap, 0, 0, &x1, &y1, &w2, &h2);
        drawOutlinedText(128 - w2 - 2, bottomY, param2Snap);
      }
    } else {
      // Voice rail only — rail graphic already contains labels, no extra text
      renderVoiceRails();
    }
  }

  display.display();

  // Remember what we just drew
  lastDrawnBpm10 = bpm10;
  lastDrawnTrack = trackSnap;
  lastDrawnLPF = lpfSnap;
  lastDrawnSlot = slotSnap;
  lastDrawnPlaying = playingSnap;
  lastDrawnOverlayStart = overlayStartSnap;
  lastDrawnKnob = knobSnap;
  lastDrawnRail = railSnap;

  strncpy(lastDrawnParam1, param1Snap, sizeof(lastDrawnParam1) - 1);
  lastDrawnParam1[sizeof(lastDrawnParam1) - 1] = 0;

  strncpy(lastDrawnParam2, param2Snap, sizeof(lastDrawnParam2) - 1);
  lastDrawnParam2[sizeof(lastDrawnParam2) - 1] = 0;

  // Update OLED watchdog timestamp only after a successful draw
  noInterrupts();
  lastFrameDrawTick = nowMs;
  interrupts();
}