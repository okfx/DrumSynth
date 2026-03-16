// splash.h — Boot splash: version display with Bayer dissolve into idle UI.
//
// Phase 1 (0–500ms):  White screen with FIRMWARE_VERSION centered in black.
// Phase 2 (500–1500ms): Bayer 4×4 dissolve reveals normal UI underneath.
//
// Depends on: hw_setup.h (display), FIRMWARE_VERSION (DrumSynth.ino),
//             splashDissolveActive/splashDissolveStartMs/splashCapture (DrumSynth.ino).

#pragma once

extern Adafruit_SH1106G display;
extern bool splashDissolveActive;
extern uint32_t splashDissolveStartMs;
extern uint8_t splashCapture[1024];

// Bayer dissolve overlay — called from updateDisplay() before OLED push.
// ORs surviving splash pixels on top of the normal UI framebuffer.
static void applySplashDissolve(float progress) {
  static const uint8_t bayer4[4][4] = {
    {  0,  8,  2, 10 },
    { 12,  4, 14,  6 },
    {  3, 11,  1,  9 },
    { 15,  7, 13,  5 }
  };

  float threshold16 = progress * 16.0f;
  uint8_t* buf = display.getBuffer();
  for (int page = 0; page < 8; page++) {
    for (int col = 0; col < 128; col++) {
      uint8_t splashByte = splashCapture[page * 128 + col];
      if (splashByte == 0) continue;  // no splash pixels in this column-page
      uint8_t mask = 0;
      for (int bit = 0; bit < 8; bit++) {
        if (splashByte & (1 << bit)) {
          int y = page * 8 + bit;
          if (bayer4[y & 3][col & 3] >= threshold16) {
            mask |= (1 << bit);
          }
        }
      }
      buf[page * 128 + col] |= mask;
    }
  }
}

static void splashAnimation() {
  // Phase 1: white screen with centered version number
  display.fillRect(0, 0, 128, 64, SH110X_WHITE);
  display.setFont(NULL);
  display.setTextSize(2);
  display.setTextColor(SH110X_BLACK, SH110X_WHITE);

  uint8_t len = strlen(FIRMWARE_VERSION);
  int16_t textW = len * 12;   // size 2: 6px base × 2
  int16_t textH = 16;         // size 2: 8px base × 2
  display.setCursor((128 - textW) / 2, (64 - textH) / 2);
  display.print(FIRMWARE_VERSION);
  display.display();
  delay(500);

  // Capture splash framebuffer for dissolve overlay
  memcpy(splashCapture, display.getBuffer(), 1024);
  splashDissolveStartMs = millis();
  splashDissolveActive = true;
  // Return — main loop begins, dissolve happens in updateDisplay()
}
