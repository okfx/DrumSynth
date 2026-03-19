// ============================================================================
//  DrumSynth Hardware Diagnostics
//
//  Standalone test program for verifying all hardware:
//    - 32 pots (guided sweep to both poles)
//    - 16 step LEDs (chase + all-on/all-off)
//    - 26 buttons (guided press-to-advance)
//    - OLED display (fill, checkerboard, border, corner text)
//    - Audio output (sine sweep on I2S + USB)
//
//  Navigation: step buttons 15 (skip) and 0 (next) during pot test.
//  PLAY button advances between test modes on summary/non-pot screens.
// ============================================================================

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <ShiftRegister74HC595.h>
#include <Mux.h>
#include <Fonts/Picopixel.h>

// ---- Pin assignments (duplicated from hw_setup.h for independence) --------

static constexpr int OLED_MOSI = 4;
static constexpr int OLED_CLK  = 5;
static constexpr int OLED_DC   = 6;

static constexpr int LED_DATA  = 9;
static constexpr int LED_CLOCK = 10;
static constexpr int LED_LATCH = 11;

// ---- Hardware objects -----------------------------------------------------

Adafruit_SH1106G display(128, 64, OLED_MOSI, OLED_CLK, OLED_DC, -1, -1);
ShiftRegister74HC595<2> ledShiftReg(LED_DATA, LED_CLOCK, LED_LATCH);

admux::Mux knobMux1      = admux::Mux(admux::Pin(14, INPUT, admux::PinType::Analog),  admux::Pinset(0, 1, 2, 3));
admux::Mux knobMux2      = admux::Mux(admux::Pin(16, INPUT, admux::PinType::Analog),  admux::Pinset(0, 1, 2, 3));
admux::Mux stepButtonsMux  = admux::Mux(admux::Pin(17, INPUT, admux::PinType::Digital), admux::Pinset(0, 1, 2, 3));
admux::Mux otherButtonsMux = admux::Mux(admux::Pin(22, INPUT, admux::PinType::Digital), admux::Pinset(0, 1, 2, 3));

// ---- Audio graph (minimal: sine -> amp -> I2S + USB) ----------------------

AudioSynthWaveformSine testSine;
AudioAmplifier         testAmpL;
AudioAmplifier         testAmpR;
AudioOutputI2S         i2s1;
AudioControlSGTL5000   sgtl5000_1;

#ifdef USB_AUDIO
AudioAmplifier         testUsbAmpL;
AudioAmplifier         testUsbAmpR;
AudioOutputUSB         usb1;
AudioConnection pc5(testSine, 0, testUsbAmpL, 0);
AudioConnection pc6(testSine, 0, testUsbAmpR, 0);
AudioConnection pc7(testUsbAmpL, 0, usb1, 0);
AudioConnection pc8(testUsbAmpR, 0, usb1, 1);
#endif

AudioConnection pc1(testSine, 0, testAmpL, 0);
AudioConnection pc2(testSine, 0, testAmpR, 0);
AudioConnection pc3(testAmpL, 0, i2s1, 0);
AudioConnection pc4(testAmpR, 0, i2s1, 1);

// ---- Constants ------------------------------------------------------------

static constexpr int KNOB_COUNT         = 32;
static constexpr int STEP_BUTTON_COUNT  = 16;
static constexpr int CTRL_BUTTON_COUNT  = 10;
static constexpr int PLAY_CHANNEL       = 6;
static constexpr int COMBO_CHANNEL      = 7;

static constexpr int POT_MIN_THRESH = 20;
static constexpr int POT_MAX_THRESH = 1003;
static constexpr int POT_ACTIVATE_THRESH = 50;

// Knob names for guided test
static const char* const knobNames[KNOB_COUNT] = {
  "D1 DISTORT", "D1 SHAPE",    "D1 DECAY",     "D1 PITCH",
  "D1 VOLUME",  "D1 SNAP",     "D1 BODY",      "D1 DLY SEND",
  "D2 PITCH",   "D2 DECAY",    "D2 VOICE MIX", "D2 DISTORT",
  "D2 DLY SEND","D2 REVERB",   "D2 NOISE",     "D2 VOLUME",
  "D3 TUNE",    "D3 DECAY",    "D3 VOICE MIX", "D3 DISTORT",
  "D3 DLY SEND","D3 LOWPASS",  "D3 ACCENT",    "D3 VOLUME",
  "M DLY TIME", "M WF FREQ",   "M LOWPASS",    "M BPM",
  "M VOLUME",   "M CHOKE",     "M WF DRIVE",   "M DLY AMT"
};

// Control button names (channels 0-9)
static const char* const ctrlBtnNames[CTRL_BUTTON_COUNT] = {
  "D1", "D2", "D3", "n/a", "n/a", "n/a", "PLAY", "X", "LOAD", "SAVE"
};

// ---- Test mode state machine ----------------------------------------------

enum TestMode : uint8_t {
  MODE_POT = 0,
  MODE_LED,
  MODE_BUTTON,
  MODE_OLED,
  MODE_AUDIO,
  MODE_COUNT
};

static TestMode currentMode = MODE_POT;
static bool modeJustEntered = true;

// ---- Isolated read helpers ------------------------------------------------
//
// The four muxes share address lines 0-3. Switching between analog and
// digital muxes causes crosstalk. These helpers enforce long settle times
// and multiple confirming reads to guarantee clean values.

// Read one knob with heavy settling and oversampling.
// Call ONLY when no other mux has been touched recently, or accept the
// 500 us front-porch settle cost.
// Knob read with address pre-staging.  The shared mux address bus can
// latch when all 4 lines change simultaneously (e.g. 1111→0000 after
// reading step button 15).  Setting a nearby channel first reduces the
// number of simultaneous transitions on the final switch.
int readKnobIsolated(uint8_t idx) {
  admux::Mux& mux = (idx < 16) ? knobMux1 : knobMux2;
  uint8_t ch = idx < 16 ? idx : idx - 16;

  // Pre-stage: set address to ch^1 (differs by 1 bit), let it settle,
  // then switch to the real channel (only 1 bit changes).
  mux.channel(ch ^ 1);
  delayMicroseconds(50);
  mux.channel(ch);
  delayMicroseconds(50);

  int sum = 0;
  for (int i = 0; i < 4; i++) sum += mux.read();
  return sum / 4;
}

// Read a step button with settle. Step buttons are digital and on their
// own signal pin (17), so they're less sensitive to crosstalk, but we
// still settle after the address change.
bool readStepButtonIsolated(uint8_t ch) {
  stepButtonsMux.channel(ch);
  delayMicroseconds(200);
  bool r1 = !stepButtonsMux.read();
  delayMicroseconds(50);
  bool r2 = !stepButtonsMux.read();
  return r1 && r2;  // both reads must agree
}

// Read a control button with heavy settle (these share address lines with
// knob muxes and are the primary source of crosstalk).
bool readCtrlButtonIsolated(uint8_t ch) {
  otherButtonsMux.channel(ch);
  delayMicroseconds(500);
  bool r1 = !otherButtonsMux.read();
  delayMicroseconds(200);
  bool r2 = !otherButtonsMux.read();
  delayMicroseconds(200);
  bool r3 = !otherButtonsMux.read();
  return r1 && r2 && r3;  // all three must agree
}

// ---- Debounced button readers ---------------------------------------------

// Standard debounce: once raw state differs from stable state for 50ms
// continuously, accept the change.  Returns true on rising edge only.
bool stepBtnPressed(uint8_t ch) {
  static bool stableState[STEP_BUTTON_COUNT] = {};
  static bool lastRaw[STEP_BUTTON_COUNT] = {};
  static uint32_t changeStart[STEP_BUTTON_COUNT] = {};

  bool raw = readStepButtonIsolated(ch);
  uint32_t now = millis();

  if (raw != lastRaw[ch]) {
    changeStart[ch] = now;   // raw changed — restart debounce timer
    lastRaw[ch] = raw;
  }

  if (raw != stableState[ch] && (now - changeStart[ch]) > 50) {
    stableState[ch] = raw;
    if (stableState[ch]) return true;  // rising edge
  }
  return false;
}

// Debounced PLAY button press (rising edge only).
bool playPressed() {
  static bool stableState = false;
  static bool lastRaw = false;
  static uint32_t changeStart = 0;

  bool raw = readCtrlButtonIsolated(PLAY_CHANNEL);
  uint32_t now = millis();

  if (raw != lastRaw) {
    changeStart = now;
    lastRaw = raw;
  }

  if (raw != stableState && (now - changeStart) > 50) {
    stableState = raw;
    if (stableState) return true;
  }
  return false;
}

// Debounced COMBO (X) button press (rising edge only).
bool comboPressed() {
  static bool stableState = false;
  static bool lastRaw = false;
  static uint32_t changeStart = 0;

  bool raw = readCtrlButtonIsolated(COMBO_CHANNEL);
  uint32_t now = millis();

  if (raw != lastRaw) {
    changeStart = now;
    lastRaw = raw;
  }

  if (raw != stableState && (now - changeStart) > 50) {
    stableState = raw;
    if (stableState) return true;
  }
  return false;
}

// ---- Utility --------------------------------------------------------------

void allLedsOff() {
  for (int i = 0; i < 16; i++) ledShiftReg.setNoUpdate(i, LOW);
  ledShiftReg.updateRegisters();
}

void drawCentered(const char* text, int y) {
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((128 - w) / 2, y);
  display.print(text);
}

// ---- POT TEST -------------------------------------------------------------
//
// During pot test, ONLY the knob mux and step button mux are read.
// The control button mux (otherButtonsMux) is NEVER touched to avoid
// address line crosstalk corrupting analog reads.
//
// Navigation: STEP 15 = skip, STEP 0 = next test (on summary screen)

static int potTestIdx = 0;
static int potMin[KNOB_COUNT];
static int potMax[KNOB_COUNT];
static uint8_t potResult[KNOB_COUNT];  // 0=pending, 1=pass, 2=skipped

// Pass requires both poles visited AND a wide sweep (prevents auto-pass
// from a pot sitting still at 0 or 1023 — noise alone can't span 900+).
static constexpr int POT_MIN_RANGE = 900;

void potTestInit() {
  potTestIdx = 0;
  for (int i = 0; i < KNOB_COUNT; i++) {
    potMin[i] = 1023;
    potMax[i] = 0;
    potResult[i] = 0;
  }
}

void potTestLoop() {
  if (potTestIdx >= KNOB_COUNT) {
    // Summary screen
    display.clearDisplay();
    display.setFont(nullptr);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("POT TEST COMPLETE");

    int passed = 0, skipped = 0;
    for (int i = 0; i < KNOB_COUNT; i++) {
      if (potResult[i] == 1) passed++;
      else if (potResult[i] == 2) skipped++;
    }
    display.setCursor(0, 12);
    display.print("PASS: "); display.print(passed);
    display.print("  SKIP: "); display.print(skipped);

    if (skipped > 0) {
      display.setCursor(0, 24);
      display.print("SKIPPED:");
      int col = 0;
      for (int i = 0; i < KNOB_COUNT; i++) {
        if (potResult[i] == 2) {
          display.setCursor(col * 18, 34);
          display.print(i);
          col++;
          if (col >= 7) { col = 0; }
        }
      }
    }
    display.setCursor(0, 58);
    display.print("STEP 0 = NEXT TEST");
    display.display();

    // Use step button 0 to advance (avoids control mux entirely)
    if (stepBtnPressed(0)) {
      currentMode = MODE_LED;
      modeJustEntered = true;
    }
    return;
  }

  // ---- Read phase: ONLY knob mux, no other mux touched ---
  int raw = readKnobIsolated(potTestIdx);

  // Always track min/max
  if (raw < potMin[potTestIdx]) potMin[potTestIdx] = raw;
  if (raw > potMax[potTestIdx]) potMax[potTestIdx] = raw;

  bool minOk = potMin[potTestIdx] <= POT_MIN_THRESH;
  bool maxOk = potMax[potTestIdx] >= POT_MAX_THRESH;
  bool rangeOk = (potMax[potTestIdx] - potMin[potTestIdx]) >= POT_MIN_RANGE;

  // ---- Display phase ---
  display.clearDisplay();
  display.setFont(nullptr);
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.print("POT TEST ");
  display.print(potTestIdx + 1);
  display.print("/");
  display.print(KNOB_COUNT);

  display.setCursor(0, 12);
  display.print("KNOB ");
  display.print(potTestIdx);
  display.print(" - ");
  display.print(knobNames[potTestIdx]);

  display.setCursor(0, 24);
  display.print("SWEEP MIN TO MAX");

  // Live value bar
  int barW = map(raw, 0, 1023, 0, 120);
  display.drawRect(4, 36, 120, 8, SH110X_WHITE);
  display.fillRect(4, 36, barW, 8, SH110X_WHITE);

  // Min/Max indicators
  display.setCursor(0, 50);
  display.print("MIN:");
  display.print(minOk ? "OK" : "--");
  display.setCursor(64, 50);
  display.print("MAX:");
  display.print(maxOk ? "OK" : "--");

  // ADC value
  display.setCursor(100, 24);
  display.print(raw);

  display.setCursor(0, 58);
  display.print("STEP15=SKIP");

  display.display();

  // Auto-advance on pass (both poles hit AND wide sweep confirmed)
  if (minOk && maxOk && rangeOk) {
    potResult[potTestIdx] = 1;
    potTestIdx++;
    delay(300);
  }

  // ---- Button phase: ONLY step button mux, after display ---
  // Step button 15 = skip (step mux is digital, safe to read here)
  if (stepBtnPressed(15)) {
    potResult[potTestIdx] = 2;
    potTestIdx++;
  }
}

// ---- LED TEST -------------------------------------------------------------

static uint8_t ledTestPhase = 0;
static uint8_t ledChaseIdx = 0;
static uint32_t ledLastTick = 0;

void ledTestInit() {
  ledTestPhase = 0;
  ledChaseIdx = 0;
  ledLastTick = millis();
  allLedsOff();
}

void ledTestLoop() {
  uint32_t now = millis();

  display.clearDisplay();
  display.setFont(nullptr);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("LED TEST");

  if (ledTestPhase == 0) {
    display.setCursor(0, 12);
    display.print("CHASE - LED ");
    display.print(ledChaseIdx);

    if ((now - ledLastTick) > 150) {
      ledLastTick = now;
      for (int i = 0; i < 16; i++) ledShiftReg.setNoUpdate(i, i == ledChaseIdx ? HIGH : LOW);
      ledShiftReg.updateRegisters();
      ledChaseIdx++;
      if (ledChaseIdx >= 16) {
        ledChaseIdx = 0;
        ledTestPhase = 1;
      }
    }
  } else if (ledTestPhase == 1) {
    display.setCursor(0, 12);
    display.print("ALL ON");
    for (int i = 0; i < 16; i++) ledShiftReg.setNoUpdate(i, HIGH);
    ledShiftReg.updateRegisters();

    if ((now - ledLastTick) > 1000) {
      ledLastTick = now;
      ledTestPhase = 2;
    }
  } else if (ledTestPhase == 2) {
    display.setCursor(0, 12);
    display.print("ALL OFF");
    allLedsOff();

    if ((now - ledLastTick) > 1000) {
      ledLastTick = now;
      ledTestPhase = 0;
    }
  }

  display.setCursor(0, 50);
  display.print("STEP0=NEXT TEST");
  display.display();

  // Poll button multiple times per frame to catch short presses
  for (int i = 0; i < 10; i++) {
    if (stepBtnPressed(0)) {
      allLedsOff();
      currentMode = MODE_BUTTON;
      modeJustEntered = true;
      return;
    }
    delay(5);
  }
}

// ---- BUTTON TEST ----------------------------------------------------------

static uint8_t btnTestIdx = 0;       // 0-15 = step, 16-25 = ctrl
static uint8_t btnResult[26];
static bool btnWaitRelease = false;

void btnTestInit() {
  btnTestIdx = 0;
  btnWaitRelease = false;
  for (int i = 0; i < 26; i++) btnResult[i] = 0;
  allLedsOff();
}

void btnTestLoop() {
  if (btnTestIdx >= 26) {
    // Summary
    display.clearDisplay();
    display.setFont(nullptr);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("BUTTON TEST COMPLETE");

    int passed = 0;
    for (int i = 0; i < 26; i++) {
      if (btnResult[i] == 1) passed++;
    }
    display.setCursor(0, 12);
    display.print("PASS: ");
    display.print(passed);
    display.print("/26");

    display.setCursor(0, 58);
    display.print("STEP0=NEXT TEST");
    display.display();
    allLedsOff();

    if (stepBtnPressed(0)) {
      currentMode = MODE_OLED;
      modeJustEntered = true;
    }
    return;
  }

  bool isStep = (btnTestIdx < STEP_BUTTON_COUNT);
  bool pressed;
  char buf[24];

  if (isStep) {
    pressed = readStepButtonIsolated(btnTestIdx);
    snprintf(buf, sizeof(buf), "STEP %d", btnTestIdx);
  } else {
    uint8_t ctrlCh = btnTestIdx - STEP_BUTTON_COUNT;
    // Skip n/a channels 3, 4, 5
    if (ctrlCh >= 3 && ctrlCh <= 5) {
      btnResult[btnTestIdx] = 1;
      btnTestIdx++;
      return;
    }
    pressed = readCtrlButtonIsolated(ctrlCh);
    snprintf(buf, sizeof(buf), "%s (CH %d)", ctrlBtnNames[ctrlCh], ctrlCh);
  }

  // Wait for release after detecting press
  if (btnWaitRelease) {
    bool stillPressed = isStep
      ? readStepButtonIsolated(btnTestIdx)
      : readCtrlButtonIsolated(btnTestIdx - STEP_BUTTON_COUNT);
    if (!stillPressed) {
      btnWaitRelease = false;
      btnResult[btnTestIdx] = 1;
      allLedsOff();
      btnTestIdx++;
      delay(150);
    }
    return;
  }

  display.clearDisplay();
  display.setFont(nullptr);
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.print("BUTTON TEST ");
  display.print(btnTestIdx + 1);
  display.print("/26");

  display.setCursor(0, 16);
  display.print("PRESS: ");
  display.print(buf);

  if (pressed) {
    display.setCursor(0, 36);
    display.print("OK!");
    if (isStep) {
      ledShiftReg.setNoUpdate(btnTestIdx, HIGH);
      ledShiftReg.updateRegisters();
    }
    btnWaitRelease = true;
  }

  display.display();
}

// ---- OLED TEST ------------------------------------------------------------

static uint8_t oledTestPhase = 0;
static uint32_t oledLastTick = 0;

void oledTestInit() {
  oledTestPhase = 0;
  oledLastTick = 0;

  display.clearDisplay();
  display.setFont(nullptr);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("OLED TEST");
  display.setCursor(0, 12);
  display.print("AUTO-CYCLING PATTERNS");
  display.setCursor(0, 50);
  display.print("STEP0=NEXT TEST");
  display.display();
  delay(1500);
}

void oledTestLoop() {
  uint32_t now = millis();

  if ((now - oledLastTick) < 1500) {
    if (stepBtnPressed(0)) {
      currentMode = MODE_AUDIO;
      modeJustEntered = true;
    }
    return;
  }

  oledLastTick = now;
  display.clearDisplay();

  switch (oledTestPhase) {
    case 0:
      display.fillRect(0, 0, 128, 64, SH110X_WHITE);
      break;
    case 1:
      break;
    case 2:
      for (int y = 0; y < 64; y += 4) {
        for (int x = 0; x < 128; x += 4) {
          if (((x / 4) + (y / 4)) % 2 == 0) {
            display.fillRect(x, y, 4, 4, SH110X_WHITE);
          }
        }
      }
      break;
    case 3:
      display.drawRect(0, 0, 128, 64, SH110X_WHITE);
      display.drawRect(2, 2, 124, 60, SH110X_WHITE);
      break;
    case 4: {
      display.setFont(nullptr);
      display.setTextSize(1);
      display.setTextColor(SH110X_WHITE);
      display.setCursor(0, 0);     display.print("TL");
      display.setCursor(116, 0);   display.print("TR");
      display.setCursor(0, 56);    display.print("BL");
      display.setCursor(116, 56);  display.print("BR");
      drawCentered("OLED OK", 28);
      break;
    }
    default:
      oledTestPhase = 0;
      return;
  }

  display.display();
  oledTestPhase++;
  if (oledTestPhase > 4) oledTestPhase = 0;

  if (stepBtnPressed(0)) {
    currentMode = MODE_AUDIO;
    modeJustEntered = true;
  }
}

// ---- AUDIO TEST -----------------------------------------------------------

static float audioFreq = 100.0f;
static bool audioSweepUp = true;
static uint32_t audioLastTick = 0;

void audioTestInit() {
  audioFreq = 100.0f;
  audioSweepUp = true;
  audioLastTick = millis();

  AudioNoInterrupts();
  testSine.frequency(audioFreq);
  testSine.amplitude(0.4f);
  testAmpL.gain(1.0f);
  testAmpR.gain(1.0f);
#ifdef USB_AUDIO
  testUsbAmpL.gain(1.0f);
  testUsbAmpR.gain(1.0f);
#endif
  AudioInterrupts();
}

void audioTestLoop() {
  uint32_t now = millis();

  // Sweep frequency
  if ((now - audioLastTick) > 20) {
    audioLastTick = now;
    if (audioSweepUp) {
      audioFreq *= 1.01f;
      if (audioFreq > 2000.0f) { audioFreq = 2000.0f; audioSweepUp = false; }
    } else {
      audioFreq *= 0.99f;
      if (audioFreq < 100.0f) { audioFreq = 100.0f; audioSweepUp = true; }
    }
    testSine.frequency(audioFreq);
  }

  display.clearDisplay();
  display.setFont(nullptr);
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.print("AUDIO TEST");

  display.setCursor(0, 12);
  display.print("SINE SWEEP 100-2000Hz");

  display.setCursor(0, 24);
  display.print("FREQ: ");
  display.print((int)audioFreq);
  display.print(" Hz");

  display.setCursor(0, 36);
#ifdef USB_AUDIO
  display.print("I2S + USB OUTPUT");
#else
  display.print("I2S OUTPUT");
#endif

  // Visual frequency bar
  int barW = map((int)audioFreq, 100, 2000, 0, 120);
  display.drawRect(4, 48, 120, 8, SH110X_WHITE);
  display.fillRect(4, 48, barW, 8, SH110X_WHITE);

  display.setCursor(0, 58);
  display.print("X=RETRIG  STEP0=REDO");
  display.display();

  // X (COMBO) retriggers the sweep from 100Hz
  if (comboPressed()) {
    audioFreq = 100.0f;
    audioSweepUp = true;
    testSine.frequency(audioFreq);
    testSine.amplitude(0.4f);
  }

  if (stepBtnPressed(0)) {
    testSine.amplitude(0);
    currentMode = MODE_POT;
    modeJustEntered = true;
  }
}

// ---- SETUP ----------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("[DIAG] DrumSynth Hardware Diagnostics");

  // ADC — high averaging for clean reads
  analogReadResolution(10);
  analogReadAveraging(8);

  // OLED
  display.begin(0, true);
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setFont(nullptr);
  display.setTextSize(1);
  drawCentered("DRUMSYNTH", 16);
  drawCentered("HARDWARE DIAGNOSTICS", 28);
  drawCentered("v1.0", 40);
  drawCentered("PRESS STEP 0 TO START", 54);
  display.display();

  // LEDs
  allLedsOff();

  // Audio
  AudioMemory(20);
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.6f);
  sgtl5000_1.lineOutLevel(13);

  // Wait for PLAY to begin
  while (!stepBtnPressed(0)) {
    delay(10);
  }

  potTestInit();
}

// ---- LOOP -----------------------------------------------------------------

void loop() {
  if (modeJustEntered) {
    modeJustEntered = false;
    allLedsOff();
    switch (currentMode) {
      case MODE_POT:    potTestInit();   break;
      case MODE_LED:    ledTestInit();   break;
      case MODE_BUTTON: btnTestInit();   break;
      case MODE_OLED:   oledTestInit();  break;
      case MODE_AUDIO:  audioTestInit(); break;
      default: break;
    }
  }

  switch (currentMode) {
    case MODE_POT:    potTestLoop();   break;
    case MODE_LED:    ledTestLoop();   break;
    case MODE_BUTTON: btnTestLoop();   break;
    case MODE_OLED:   oledTestLoop();  break;
    case MODE_AUDIO:  audioTestLoop(); break;
    default: break;
  }
}
