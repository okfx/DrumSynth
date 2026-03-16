// xcombo_overlay.h — Full-screen X-combo help overlay renderer.
//
// Three groups mirroring the physical unit layout:
//   1. Header:  "HOLD X AND PRESS..." in 5×7 font, bold X, hand-drawn dots
//   2. Circles: D1/D2/D3 voice buttons + PLAY, with chroma labels below
//   3. Steps:   step button diagram with MEM and SHUFFLE labels + arrow
//
// Depends on: hw_setup.h (display), Picopixel font,
//             isSafeToPushOled() (forward-declared in .ino above this include).

#pragma once

extern Adafruit_SH1106G display;

// Step-button X positions mirroring the physical unit layout.
static constexpr uint8_t kStepX[16] = {
  1, 9, 17, 25, 33, 41, 49, 57, 65, 73, 82, 90, 98, 106, 113, 120
};

static void renderXComboOverlay(uint32_t nowMs) {
  display.clearDisplay();

  // --- GROUP 1: Header ---
  // "HOLD X AND PRESS" in default 5×7 font, centered at x=11.
  // "X" redrawn 1px right for bold effect. "..." as three 2×2 pixel squares.
  display.setFont(NULL);
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(11, 2);
  display.print("HOLD X AND PRESS");
  display.setCursor(42, 2);   // x=41+1 — bold X overprint
  display.print("X");
  for (int d = 0; d < 3; d++) {
    int dx = 108 + d * 3;
    display.drawPixel(dx,     7, SH110X_WHITE);
    display.drawPixel(dx + 1, 7, SH110X_WHITE);
    display.drawPixel(dx,     8, SH110X_WHITE);
    display.drawPixel(dx + 1, 8, SH110X_WHITE);
  }

  // --- GROUP 2: Voice circles + PLAY ---
  // Circles mirroring D1/D2/D3 and PLAY positions, centered at y=23.
  display.drawCircle(13,  23, 5, SH110X_WHITE);  // D1
  display.drawCircle(33,  23, 5, SH110X_WHITE);  // D2
  display.drawCircle(53,  23, 5, SH110X_WHITE);  // D3
  display.drawCircle(114, 23, 5, SH110X_WHITE);  // PLAY

  display.setFont(&Picopixel);
  display.setCursor(12, 25);  display.print("1");
  display.setCursor(32, 25);  display.print("2");
  display.setCursor(52, 25);  display.print("3");

  // Play triangle — 7 rows, widths {1,2,3,4,3,2,1}, top-left at (113, 20)
  static const uint8_t triW[] = {1, 2, 3, 4, 3, 2, 1};
  for (int r = 0; r < 7; r++)
    for (int c = 0; c < triW[r]; c++)
      display.drawPixel(113 + c, 20 + r, SH110X_WHITE);

  // Labels below circles
  display.setCursor(1,   37);  display.print("TO TOGGLE CHROMA");
  display.setCursor(107, 37);  display.print("(WF)");

  // --- GROUP 3: Step buttons ---

  // Steps 0–9: full-size 7×9 outline with Picopixel digit inside
  for (int i = 0; i < 10; i++)
    display.drawRect(kStepX[i], 45, 7, 9, SH110X_WHITE);
  for (int i = 0; i < 10; i++) {
    char ch[2] = { (char)('0' + i), 0 };
    display.setCursor(kStepX[i] + 2, 51);
    display.print(ch);
  }

  // Steps 10–14: 5×7 dotted outline — no combo function, visually de-prioritized.
  // Corners always solid; edges dotted with relative parity so all five boxes match.
  for (int i = 10; i <= 14; i++) {
    uint8_t bx = kStepX[i], by = 46, bw = 5, bh = 7;
    display.drawPixel(bx,        by,        SH110X_WHITE);
    display.drawPixel(bx+bw-1,   by,        SH110X_WHITE);
    display.drawPixel(bx,        by+bh-1,   SH110X_WHITE);
    display.drawPixel(bx+bw-1,   by+bh-1,   SH110X_WHITE);
    for (int dx = bx + 1; dx < bx + bw - 1; dx++) {
      if ((dx - bx) % 2 == 0) {
        display.drawPixel(dx, by,      SH110X_WHITE);
        display.drawPixel(dx, by+bh-1, SH110X_WHITE);
      }
    }
    for (int dy = by + 1; dy < by + bh - 1; dy++) {
      if ((dy - by) % 2 == 0) {
        display.drawPixel(bx,      dy, SH110X_WHITE);
        display.drawPixel(bx+bw-1, dy, SH110X_WHITE);
      }
    }
  }

  // Step 15: full 7×9 + centered 3×3 filled marker
  display.drawRect(120, 45, 7, 9, SH110X_WHITE);
  display.fillRect(122, 48, 3, 3, SH110X_WHITE);

  // Labels below step buttons
  display.setCursor(1,  61);  display.print("TO SELECT MEM 0-9");
  display.setCursor(91, 62);  display.print("SHUFFLE");

  // ⤴ arrow — horizontal bar from SHUFFLE text to stem, vertical stem up to step 15
  for (int ax = 119; ax <= 123; ax++)
    display.drawPixel(ax, 60, SH110X_WHITE);
  for (int ay = 56; ay < 60; ay++)
    display.drawPixel(123, ay, SH110X_WHITE);
  display.drawPixel(123, 56, SH110X_WHITE);  // tip
  display.drawPixel(122, 57, SH110X_WHITE);  // left wing
  display.drawPixel(124, 57, SH110X_WHITE);  // right wing

  display.setFont(NULL);  // restore default for next caller

  if (isSafeToPushOled(nowMs)) {
    display.display();
  }
}
