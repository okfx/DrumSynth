// xcombo_overlay.h — X-combo overlay: large "X" over scope + chroma status.
//
// Instead of a full-screen takeover, this renders a large outlined "X" over
// the normal idle UI, with D1/D2/D3/WF chroma status below it. The labels
// flash in sync with the combo-active step LEDs (~1.7 Hz).
//
// Depends on: hw_setup.h (display), display_ui.h (drawOutlinedText),
//             isSafeToPushOled() (forward-declared in .ino above this include).

#pragma once

extern Adafruit_SH1106G display;
extern bool d1ChromaMode;
extern bool d2ChromaMode;
extern bool d3ChromaMode;
static void renderXComboOverlay(uint32_t nowMs, uint32_t pressTick) {
  // Flash phase — shared with LED flashing in updateLEDs().
  // Offset from pressTick so first phase is always "on".
  bool flashOn = ((nowMs - pressTick) / 400) % 2 == 0;

  // Large outlined "X" centered over scope area (same size as "MONOBASS").
  // Scope area is roughly y=18..54, x=0..127.
  display.setTextSize(2);
  drawOutlinedText(55, 22, "X");
  display.setTextSize(1);

  // Chroma status labels below the X, evenly spaced across the display.
  // WF is always chromatic so not shown here — only D1/D2/D3 toggle.
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
