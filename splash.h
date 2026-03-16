// splash.h — Boot splash animation (wavefolding sine → version info).
//
// Depends on: hw_setup.h (display), FIRMWARE_VERSION/FIRMWARE_DATE_FMT
// (defined in DrumSynth.ino above all includes).

#pragma once

extern Adafruit_SH1106G display;

// Compile-time date/time parsing from __DATE__ / __TIME__.
// Format: "Mar  4 2026" → fwMonth=3, fwDay=4, fwYear=26
static constexpr int fwMonth =
    (__DATE__[0] == 'J' && __DATE__[1] == 'a') ? 1  :
    (__DATE__[0] == 'F')                        ? 2  :
    (__DATE__[0] == 'M' && __DATE__[2] == 'r') ? 3  :
    (__DATE__[0] == 'A' && __DATE__[1] == 'p') ? 4  :
    (__DATE__[0] == 'M' && __DATE__[2] == 'y') ? 5  :
    (__DATE__[0] == 'J' && __DATE__[2] == 'n') ? 6  :
    (__DATE__[0] == 'J' && __DATE__[2] == 'l') ? 7  :
    (__DATE__[0] == 'A' && __DATE__[1] == 'u') ? 8  :
    (__DATE__[0] == 'S')                        ? 9  :
    (__DATE__[0] == 'O')                        ? 10 :
    (__DATE__[0] == 'N')                        ? 11 :
    (__DATE__[0] == 'D')                        ? 12 : 0;
static constexpr int fwDay =
    (__DATE__[4] == ' ') ? (__DATE__[5] - '0')
                         : (__DATE__[4] - '0') * 10 + (__DATE__[5] - '0');
static constexpr int fwYear = (__DATE__[9] - '0') * 10 + (__DATE__[10] - '0');
static constexpr int fwHour = (__TIME__[0] - '0') * 10 + (__TIME__[1] - '0');
static constexpr int fwMin  = (__TIME__[3] - '0') * 10 + (__TIME__[4] - '0');

static constexpr const char* FIRMWARE_DATE_FMT  = "%02d.%02d.%02d - %02d:%02d";
#define FIRMWARE_DATE_ARGS fwMonth, fwDay, fwYear, fwHour, fwMin

// Boot splash: sine wave → wavefold → version info (36 frames @ 28ms).
static void splashAnimation() {
  static constexpr int FRAME_MS    = 28;   // ~36 fps
  static constexpr int TOTAL_FRAMES = 36;
  static constexpr int X_MIN       = 4;
  static constexpr int X_MAX       = 124;
  static constexpr int X_CENTER    = 64;
  static constexpr int Y_CENTER    = 32;
  static constexpr float PI2       = 6.2831853f;

  for (int frame = 0; frame < TOTAL_FRAMES; frame++) {
    unsigned long t0 = millis();
    display.clearDisplay();

    if (frame <= 3) {
      // PHASE 1 — flat line grows from center
      float progress = (float)frame / 3.0f;
      int half = (int)(progress * (X_MAX - X_CENTER));
      int xL = X_CENTER - half;
      int xR = X_CENTER + half;
      if (xL < X_MIN) xL = X_MIN;
      if (xR > X_MAX) xR = X_MAX;
      display.drawLine(xL, Y_CENTER, xR, Y_CENTER, 1);

    } else if (frame <= 12) {
      // PHASE 2 — sine wave emerges
      float progress = (float)(frame - 4) / 8.0f;
      float amplitude = progress * 18.0f;
      float freq = 1.0f + progress * 0.5f;

      int prevY = Y_CENTER;
      for (int x = X_MIN; x <= X_MAX; x++) {
        float t = (float)(x - X_MIN) / (float)(X_MAX - X_MIN);
        float sine = sinf(t * PI2 * freq);
        int y = Y_CENTER - (int)(sine * amplitude);
        if (y < 1) y = 1;
        if (y > 62) y = 62;
        if (x == X_MIN) {
          display.drawPixel(x, y, 1);
        } else {
          display.drawLine(x - 1, prevY, x, y, 1);
        }
        prevY = y;
      }

    } else if (frame <= 28) {
      // PHASE 3 — wavefolding intensifies
      float progress = (float)(frame - 13) / 15.0f;
      float foldAmount = 1.0f + progress * 5.0f;
      float amplitude = 18.0f + progress * 6.0f;
      float freq = 1.5f + progress * 0.5f;
      float drift = sinf(frame * 0.3f) * 0.2f;

      int prevY = Y_CENTER;
      for (int x = X_MIN; x <= X_MAX; x++) {
        float t = (float)(x - X_MIN) / (float)(X_MAX - X_MIN);
        float sine = sinf(t * PI2 * freq + drift);
        // Remap [-1,1] to [0,1]
        float value = (sine + 1.0f) * 0.5f;
        // Wavefold transfer function
        float v = value * foldAmount;
        v = v - floorf(v);
        if ((int)floorf(value * foldAmount) % 2 == 1) v = 1.0f - v;
        float folded = v * 2.0f - 1.0f;

        int y = Y_CENTER - (int)(folded * amplitude);
        if (y < 1) y = 1;
        if (y > 62) y = 62;
        if (x == X_MIN) {
          display.drawPixel(x, y, 1);
        } else {
          display.drawLine(x - 1, prevY, x, y, 1);
        }
        prevY = y;
      }

    } else {
      // PHASE 4 — version info
      display.setTextColor(1);
      display.setTextWrap(false);
      display.setFont(NULL);
      display.setTextSize(1);
      display.setCursor(43, 16);
      display.print("VERSION");
      display.setCursor(55, 28);
      display.print(FIRMWARE_VERSION);
      display.setCursor(10, 40);
      char dateBuf[20];
      snprintf(dateBuf, sizeof(dateBuf), FIRMWARE_DATE_FMT, FIRMWARE_DATE_ARGS);
      display.print(dateBuf);
    }

    display.display();

    // Maintain frame rate
    unsigned long elapsed = millis() - t0;
    if (elapsed < (unsigned long)FRAME_MS) {
      delay(FRAME_MS - elapsed);
    }
  }

  // Hold version text
  delay(800);
  display.clearDisplay();
  display.display();
}
