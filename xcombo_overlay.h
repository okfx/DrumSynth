// xcombo_overlay.h — X-combo overlay: large "X" over scope + chroma/swing status.
//
// Renders a large outlined "X" over the normal idle UI with D1/D2/D3 chroma
// labels below (inverted when active). Small swing % in the top-right corner
// when swing is on.
//
// Depends on: hw_setup.h (display), display_ui.h (drawOutlinedText),
//             isSafeToPushOled() (forward-declared in .ino above this include).

#pragma once

extern Adafruit_SH1106G display;
extern bool d1ChromaMode;
extern bool d2ChromaMode;
extern bool d3ChromaMode;
extern volatile SwingMode swingMode;
static void renderXComboOverlay(uint32_t nowMs, uint32_t pressTick) {
  // Flash phase — shared with LED flashing in updateLEDs().
  // Offset from pressTick so first phase is always "on".
  bool flashOn = ((nowMs - pressTick) / 400) % 2 == 0;

  // Large outlined "X" centered over scope area.
  display.setTextSize(2);
  drawOutlinedText(55, 22, "X");
  display.setTextSize(1);

  // Small swing indicator in top-right corner (immediate, no flash gating).
  if (swingMode != SWING_OFF) {
    display.setFont(NULL);
    display.setTextSize(1);
    char sBuf[10];
    snprintf(sBuf, sizeof(sBuf), "S=%u%%", (unsigned)kSwingPercent[swingMode]);
    int sw = (int)strlen(sBuf) * 6;
    drawOutlinedText(127 - sw, 21, sBuf);
  }

  // D1/D2/D3 chroma status — always shown, flashing.
  if (flashOn) {
    display.setFont(NULL);
    display.setTextSize(1);

    static const char* labels[] = { "D1", "D2", "D3" };
    static const uint8_t xPos[] = { 16, 56, 96 };
    const bool active[] = { d1ChromaMode, d2ChromaMode, d3ChromaMode };

    for (int i = 0; i < 3; i++) {
      int lx = xPos[i];
      int ly = 42;

      if (active[i]) {
        display.fillRect(lx - 2, ly - 1, 16, 10, SH110X_WHITE);
        display.setTextColor(SH110X_BLACK);
        display.setCursor(lx, ly);
        display.print(labels[i]);
        display.setTextColor(SH110X_WHITE);
      } else {
        drawOutlinedText(lx, ly, labels[i]);
      }
    }
  }
}
