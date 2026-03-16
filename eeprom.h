#ifndef EEPROM_H_DRUMACHINE
#define EEPROM_H_DRUMACHINE
// ============================================================================
//  EEPROM Persistence — eeprom.h
//
//  Pattern save/load across 10 slots, plus PPQN and MONOBASS persistence.
//  Each slot stores a 3-track + D1/D2/D3 chroma × 16-step pattern with
//  magic number validation and CRC-8 integrity checking.
//  PPQN and MONOBASS are stored separately after the pattern slots.
//
//  Memory map (Teensy 4.0: 1080 bytes available):
//    Slots 0–9:  0–1019   (10 × 102 = 1020 bytes)
//    PPQN:       1020–1021 (magic + value)
//    MONOBASS:   1022–1023 (magic + bool)
//    Total:      1024 of 1080 bytes used
//
//  Include AFTER: hw_setup.h (numSteps), EEPROM.h, sequence arrays,
//  ppqn, PPQN_DEFAULT, PPQN_OPTIONS, UI overlay variables.
// ============================================================================

#include <Arduino.h>
#include <EEPROM.h>

// CRC-8 (init=0xFF, poly=0x07, unreflected) over a byte array.
// Not a standard named variant — values will not match generic CRC-8 tools.
// Used solely to detect EEPROM corruption beyond the magic number.
static uint8_t crc8(const uint8_t* data, size_t len) {
  uint8_t crc = 0xFF;
  for (size_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t bit = 0; bit < 8; bit++) {
      crc = (crc & 0x80) ? ((crc << 1) ^ 0x07) : (crc << 1);
    }
  }
  return crc;
}

// ============================================================================
//  External dependencies — defined in main .ino
// ============================================================================

// Sequence data
extern uint8_t d1Sequence[];
extern uint8_t d2Sequence[];
extern uint8_t d3Sequence[];
extern uint8_t d1ChromaNote[];
extern uint8_t d2ChromaNote[];
extern uint8_t d3ChromaNote[];

// Chroma mode flags
extern bool d1ChromaMode;
extern bool d2ChromaMode;
extern bool d3ChromaMode;
extern bool wfChromaMode;
extern float d1BaseFreq;
extern float d1FreqBeforeChroma;
extern AudioFilterStateVariable d1HighPass;

// PPQN (ppqn is extern; PPQN_OPTIONS, PPQN_OPTION_COUNT, PPQN_DEFAULT are
// constexpr in .ino — visible because this header is included after them)
extern volatile uint8_t ppqn;

// UI overlay feedback (for "PATTERN LOADED" / "SAVED" messages)
extern RailMode activeRail;
extern char displayParameter1[24];
extern char displayParameter2[24];
extern uint32_t parameterOverlayStartTick;
extern volatile uint32_t sysTickMs;

// LEDs
extern void updateLEDs();

// Knob dispatch (for re-applying Body knob on chroma transitions)
extern void applyKnobToEngine(uint8_t idx, int knobValue);
extern ResponsiveAnalogRead* analog[];

// ============================================================================
//  Data structures
// ============================================================================

struct PatternStore {
  uint8_t drum1[numSteps];
  uint8_t drum2[numSteps];
  uint8_t drum3[numSteps];
  uint8_t d1Chroma[numSteps];     // Per-step MIDI note for D1 chroma mode
  uint8_t d2Chroma[numSteps];     // Per-step MIDI note for D2 chroma mode
  uint8_t d3Chroma[numSteps];     // Per-step MIDI note for D3 chroma mode
  uint8_t flags;                  // bits 0-3: d1/d2/d3/wfChromaMode, bits 4-6: shuffleMode, bit 7: reserved
};

struct EepromSlot {
  uint16_t magic;
  uint16_t seq;
  PatternStore patterns;
  uint8_t crc;  // CRC8 over PatternStore bytes — detects partial writes / corruption
};

static_assert(sizeof(PatternStore) == 97, "PatternStore layout changed — bump EEPROM_MAGIC");
static_assert(sizeof(EepromSlot)   == 102, "EepromSlot layout changed — bump EEPROM_MAGIC");

// ============================================================================
//  Constants
// ============================================================================

static constexpr uint16_t EEPROM_MAGIC_V1    = 0x4247;  // V1 magic (D1 chroma 33–69)
static constexpr uint16_t EEPROM_MAGIC_V2   = 0x4248;  // V2 magic (D1 chroma 33–75)
static constexpr uint16_t EEPROM_MAGIC      = 0x4249;  // Current magic (+ wfChromaMode in flags)
static constexpr uint8_t  SAVE_SLOT_COUNT   = 10;
// PPQN stored after all save slots (2 bytes: magic + value)
static constexpr int      EEPROM_PPQN_ADDR  = SAVE_SLOT_COUNT * (int)sizeof(EepromSlot);
static constexpr uint8_t  EEPROM_PPQN_MAGIC = 0xAA;
// MONOBASS global state stored after PPQN (2 bytes: magic + bool)
static constexpr int      EEPROM_MONOBASS_ADDR  = EEPROM_PPQN_ADDR + 2;
static constexpr uint8_t  EEPROM_MONOBASS_MAGIC = 0xBB;
// Total EEPROM footprint: 10×102 + 2 + 2 = 1024 bytes (Teensy 4.0 has 1080)
static constexpr int      EEPROM_TOTAL_BYTES = EEPROM_MONOBASS_ADDR + 2;
static_assert(EEPROM_TOTAL_BYTES <= 1080, "EEPROM layout exceeds Teensy 4.0 capacity (1080 bytes)");

// ============================================================================
//  State
// ============================================================================

uint16_t eepromSeq = 0;
uint8_t activeSaveSlot = 0;
bool patternDirty = false;

// ============================================================================
//  Functions
// ============================================================================

bool loadStateFromEEPROM(uint8_t slotIndex) {
  if (slotIndex >= SAVE_SLOT_COUNT) return false;

  // Calculate and verify address (type-safe)
  size_t addr = (size_t)slotIndex * sizeof(EepromSlot);
  if (addr + sizeof(EepromSlot) > EEPROM.length()) return false;

  // Read slot from EEPROM
  EepromSlot slot;
  EEPROM.get((int)addr, slot);

  // Verify magic number — accept current and two previous versions
  uint8_t version = 0;  // 1=V1, 2=V2, 3=current
  if (slot.magic == EEPROM_MAGIC)       version = 3;
  else if (slot.magic == EEPROM_MAGIC_V2) version = 2;
  else if (slot.magic == EEPROM_MAGIC_V1) version = 1;
  else return false;

  // Verify CRC8 over pattern data — catches partial writes and bit rot
  uint8_t expected = crc8((const uint8_t*)&slot.patterns, sizeof(PatternStore));
  if (slot.crc != expected) return false;

  // D1 chroma upper bound depends on which version wrote the slot.
  // V1 slots used 33–69; V2+ uses 33–75 (D1_CHROMA_NOTE_MAX).
  uint8_t d1ChromaMax = (version <= 1) ? 69 : 75;

  // Sequences are main-loop only (see concurrency contract in ext_sync.h).
  // If loading during playback, one step may fire from a mix of old/new data
  // before this copy finishes — a brief glitch, not a crash.
  for (int step = 0; step < numSteps; step++) {
    d1Sequence[step] = slot.patterns.drum1[step] ? 1 : 0;  // sanitize to boolean
    d2Sequence[step] = slot.patterns.drum2[step] ? 1 : 0;
    d3Sequence[step] = slot.patterns.drum3[step] ? 1 : 0;
    d1ChromaNote[step] = constrain(slot.patterns.d1Chroma[step], 33, d1ChromaMax);
    d2ChromaNote[step] = constrain(slot.patterns.d2Chroma[step], 45, 69);  // A2–A4
    d3ChromaNote[step] = constrain(slot.patterns.d3Chroma[step], 48, 84);  // C3–C6
  }

  // Restore chroma mode flags
  bool wasD1Chroma = d1ChromaMode;
  d1ChromaMode = (slot.patterns.flags & 0x01) != 0;
  d2ChromaMode = (slot.patterns.flags & 0x02) != 0;
  d3ChromaMode = (slot.patterns.flags & 0x04) != 0;
  wfChromaMode = (version >= 3) ? ((slot.patterns.flags & 0x08) != 0) : false;

  // Restore shuffle mode from bits 4-6 (0 = SHUFFLE_OFF for old patterns)
  uint8_t rawShuffle = (slot.patterns.flags >> 4) & 0x07;
  shuffleMode = (rawShuffle <= SHUFFLE_7) ? (ShuffleMode)rawShuffle : SHUFFLE_OFF;

  // If D1 chroma was just enabled by load, save current freq for restore on exit
  if (d1ChromaMode && !wasD1Chroma) {
    d1FreqBeforeChroma = d1BaseFreq;
    // Lower HPF to pass bass fundamentals (C2 = 65 Hz, normal 85 Hz kills it)
    AudioNoInterrupts();
    d1HighPass.frequency(30.0f);
    d1HighPass.resonance(0.7f);
    AudioInterrupts();
    applyKnobToEngine(6, analog[6]->getValue());  // switch Body to filter mode
  } else if (!d1ChromaMode && wasD1Chroma) {
    // Restore drum HPF on chroma exit via pattern load
    AudioNoInterrupts();
    d1HighPass.frequency(85.0f);
    d1HighPass.resonance(2.0f);
    AudioInterrupts();
    applyKnobToEngine(6, analog[6]->getValue());  // restore normal EQ
  }

  // Refresh step LEDs so they reflect the loaded pattern
  updateLEDs();

  // Update sequence number (keep monotonic for debugging)
  if (slot.seq > eepromSeq) {
    eepromSeq = slot.seq;
  }

  patternDirty = false;

  // Live-update step timer if shuffle mode changed during playback
  if (transportState == RUN_INT && sequencePlaying) {
    stepTimer.update(shuffledStepPeriodUs(currentStep));
  }

  // Show "PATTERN LOADED" overlay message
  activeRail = RAIL_NONE;
  snprintf(displayParameter1, sizeof(displayParameter1), "PATTERN");
  snprintf(displayParameter2, sizeof(displayParameter2), "LOADED");

  parameterOverlayStartTick = sysTickMs;

  return true;
}

void saveStateToEEPROM(uint8_t slotIndex) {
  if (slotIndex >= SAVE_SLOT_COUNT) return;

  size_t addr = (size_t)slotIndex * sizeof(EepromSlot);
  if (addr + sizeof(EepromSlot) > EEPROM.length()) return;

  EepromSlot slot = {};
  eepromSeq++;
  if (eepromSeq == 0 || eepromSeq == 0xFFFF) eepromSeq = 1;  // Skip 0 and 0xFFFF (erased-EEPROM sentinels)

  slot.magic = EEPROM_MAGIC;
  slot.seq = eepromSeq;

  // Sequences are main-loop only (see concurrency contract in ext_sync.h)
  for (int step = 0; step < numSteps; step++) {
    slot.patterns.drum1[step] = d1Sequence[step];
    slot.patterns.drum2[step] = d2Sequence[step];
    slot.patterns.drum3[step] = d3Sequence[step];
    slot.patterns.d1Chroma[step] = d1ChromaNote[step];
    slot.patterns.d2Chroma[step] = d2ChromaNote[step];
    slot.patterns.d3Chroma[step] = d3ChromaNote[step];
  }

  // Pack chroma mode flags + shuffle mode (bits 4-6)
  slot.patterns.flags = (d1ChromaMode ? 0x01 : 0)
                      | (d2ChromaMode ? 0x02 : 0)
                      | (d3ChromaMode ? 0x04 : 0)
                      | (wfChromaMode ? 0x08 : 0)
                      | (((uint8_t)shuffleMode & 0x07) << 4);

  // CRC8 over pattern data — detects partial writes on load
  slot.crc = crc8((const uint8_t*)&slot.patterns, sizeof(PatternStore));

  EEPROM.put((int)addr, slot);  // Cast to int here for EEPROM.put()

  patternDirty = false;

  // Show "PATTERN SAVED" overlay message
  activeRail = RAIL_NONE;
  snprintf(displayParameter1, sizeof(displayParameter1), "PATTERN");
  snprintf(displayParameter2, sizeof(displayParameter2), "SAVED");
  parameterOverlayStartTick = sysTickMs;
}

void loadPpqnFromEEPROM() {
  if (EEPROM.read(EEPROM_PPQN_ADDR) == EEPROM_PPQN_MAGIC) {
    uint8_t val = EEPROM.read(EEPROM_PPQN_ADDR + 1);
    for (int i = 0; i < PPQN_OPTION_COUNT; i++) {
      if (PPQN_OPTIONS[i] == val) { ppqn = val; return; }
    }
  }
  ppqn = PPQN_DEFAULT;
}

void savePpqnToEEPROM(uint8_t val) {
  // Only save values from the valid PPQN set
  bool valid = false;
  for (int i = 0; i < PPQN_OPTION_COUNT; i++) {
    if (PPQN_OPTIONS[i] == val) { valid = true; break; }
  }
  if (!valid) return;
  EEPROM.update(EEPROM_PPQN_ADDR, EEPROM_PPQN_MAGIC);
  EEPROM.update(EEPROM_PPQN_ADDR + 1, val);
}

// ============================================================================
//  MONOBASS global persistence — survives power cycle, independent of patterns
// ============================================================================

bool loadMonoBassStatusFromEEPROM() {
  if (EEPROM.read(EEPROM_MONOBASS_ADDR) == EEPROM_MONOBASS_MAGIC) {
    return EEPROM.read(EEPROM_MONOBASS_ADDR + 1) != 0;
  }
  return false;
}

void saveMonoBassStatusToEEPROM(bool active) {
  EEPROM.update(EEPROM_MONOBASS_ADDR, EEPROM_MONOBASS_MAGIC);
  EEPROM.update(EEPROM_MONOBASS_ADDR + 1, active ? 1 : 0);
}

#endif // EEPROM_H_DRUMACHINE
