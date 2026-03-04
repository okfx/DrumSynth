#ifndef EEPROM_H_DRUMACHINE
#define EEPROM_H_DRUMACHINE
// ============================================================================
//  EEPROM Persistence — eeprom.h
//
//  Pattern save/load across 10 slots, plus PPQN persistence.
//  Each slot stores a 3-track + bass line × 16-step pattern with magic number validation.
//  PPQN is stored separately after the pattern slots.
//
//  Include AFTER: hw_setup.h (numSteps), EEPROM.h, sequence arrays,
//  ppqn, PPQN_DEFAULT, PPQN_OPTIONS, UI overlay variables.
// ============================================================================

#include <Arduino.h>
#include <EEPROM.h>

// Simple CRC8 (polynomial 0x07) over a byte array.
// Used to detect EEPROM corruption beyond the magic number.
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
extern byte drum1Sequence[];
extern byte drum2Sequence[];
extern byte drum3Sequence[];
extern uint8_t bassLineNote[];

// PPQN
extern volatile uint8_t ppqn;
extern const uint8_t PPQN_OPTIONS[];
// PPQN_OPTION_COUNT is constexpr in .ino — visible before this header is included

// UI overlay feedback (for "PATTERN LOADED" / "SAVED" messages)
extern RailMode activeRail;
extern char displayParameter1[24];
extern char displayParameter2[24];
extern uint32_t parameterOverlayStartTick;
extern volatile uint32_t sysTickMs;

// LEDs
extern void updateLEDs();

// ============================================================================
//  Data structures
// ============================================================================

struct PatternStore {
  byte drum1[numSteps];
  byte drum2[numSteps];
  byte drum3[numSteps];
  byte bassLine[numSteps];   // Per-step MIDI note for bass line mode
};

struct EepromSlot {
  uint16_t magic;
  uint16_t seq;
  PatternStore patterns;
  uint8_t crc;  // CRC8 over PatternStore bytes — detects partial writes / corruption
};

// ============================================================================
//  Constants
// ============================================================================

static constexpr uint16_t EEPROM_MAGIC      = 0x4244;  // Identifies valid slot (CRC8 format)
static constexpr uint8_t  SAVE_SLOT_COUNT   = 10;
// PPQN stored after all save slots
static constexpr int      EEPROM_PPQN_ADDR  = SAVE_SLOT_COUNT * (int)sizeof(EepromSlot);
static constexpr uint8_t  EEPROM_PPQN_MAGIC = 0xAA;

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

  // Verify magic number
  if (slot.magic != EEPROM_MAGIC) return false;

  // Verify CRC8 over pattern data — catches partial writes and bit rot
  uint8_t expected = crc8((const uint8_t*)&slot.patterns, sizeof(PatternStore));
  if (slot.crc != expected) return false;

  // Load pattern data with interrupts disabled so a step trigger
  // mid-copy can't read a half-loaded pattern (~1us on Cortex-M7).
  noInterrupts();
  for (int step = 0; step < numSteps; step++) {
    drum1Sequence[step] = slot.patterns.drum1[step] ? 1 : 0;  // sanitize to boolean
    drum2Sequence[step] = slot.patterns.drum2[step] ? 1 : 0;
    drum3Sequence[step] = slot.patterns.drum3[step] ? 1 : 0;
    bassLineNote[step] = constrain(slot.patterns.bassLine[step], 33, 69);  // A1–A4
  }
  interrupts();

  // Refresh step LEDs so they reflect the loaded pattern
  updateLEDs();

  // Update sequence number (keep monotonic for debugging)
  if (slot.seq > eepromSeq) {
    eepromSeq = slot.seq;
  }

  patternDirty = false;

  // Show "PATTERN LOADED" overlay message
  activeRail = RAIL_NONE;
  snprintf(displayParameter1, sizeof(displayParameter1), "PATTERN");
  snprintf(displayParameter2, sizeof(displayParameter2), "LOADED");

  // Start overlay timer atomically
  noInterrupts();
  parameterOverlayStartTick = sysTickMs;
  interrupts();

  return true;
}

// NOTE: Caller handles the "SAVED" overlay text (unlike loadStateFromEEPROM which sets its own).
void saveStateToEEPROM(uint8_t slotIndex) {
  if (slotIndex >= SAVE_SLOT_COUNT) return;

  size_t addr = (size_t)slotIndex * sizeof(EepromSlot);
  if (addr + sizeof(EepromSlot) > EEPROM.length()) return;

  EepromSlot slot = {};
  eepromSeq++;
  if (eepromSeq == 0 || eepromSeq == 0xFFFF) eepromSeq = 1;  // Skip 0 and 0xFFFF (erased-EEPROM sentinels)

  slot.magic = EEPROM_MAGIC;
  slot.seq = eepromSeq;

  // Copy arrays with interrupts disabled so the snapshot is consistent —
  // a step trigger mid-copy could see a mix of old and new step values (~1µs on Cortex-M7).
  noInterrupts();
  for (int step = 0; step < numSteps; step++) {
    slot.patterns.drum1[step] = drum1Sequence[step];
    slot.patterns.drum2[step] = drum2Sequence[step];
    slot.patterns.drum3[step] = drum3Sequence[step];
    slot.patterns.bassLine[step] = bassLineNote[step];
  }
  interrupts();

  // CRC8 over pattern data — detects partial writes on load
  slot.crc = crc8((const uint8_t*)&slot.patterns, sizeof(PatternStore));

  EEPROM.put((int)addr, slot);  // Cast to int here for EEPROM.put()

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
  ppqn = PPQN_DEFAULT;  // Defined in main .ino — shared single source of truth
}

void savePpqnToEEPROM(uint8_t val) {
  EEPROM.update(EEPROM_PPQN_ADDR, EEPROM_PPQN_MAGIC);
  EEPROM.update(EEPROM_PPQN_ADDR + 1, val);
}

#endif // EEPROM_H_DRUMACHINE
