// xcombo_overlay.h — Full-screen X-combo help overlay renderer.
//
// Shows all available X+button combos as a spatial diagram mirroring
// the physical unit layout, with a scrolling marquee at the bottom.
//
// Depends on: hw_setup.h (display), Picopixel font, xComboOverlayStartMs,
// isSafeToPushOled() (forward-declared in .ino above this include).

#pragma once

extern Adafruit_SH1106G display;
extern uint32_t xComboOverlayStartMs;

// --- Scroll buffer (private to overlay) ---

// Pre-rendered scroll buffer (Picopixel glyphs, 400px wide × 6px tall).
// 6 rows: 5px cap height + 1px descender (Q, g, p, etc.).
// Byte-per-pixel (not bit-packed) for simple blit loop; 2.4KB is negligible on T4.
static constexpr uint16_t SCROLL_BUF_W = 400;
static constexpr uint8_t  SCROLL_BUF_H = 6;
static uint8_t  scrollBuf[SCROLL_BUF_W * SCROLL_BUF_H];
static uint16_t scrollBufWidth = 0;  // actual rendered width in pixels

// Step-button X positions for overlay diagram
static constexpr uint8_t kStepX[16] = {
  1, 9, 17, 25, 33, 41, 49, 57, 65, 73, 82, 90, 98, 106, 113, 120
};

// --- Scroll text pre-rendering ---

// Pre-renders scroll text into scrollBuf[] using Picopixel font glyphs.
// Called once per X-press; walks each character through the PROGMEM glyph
// table and blits columns into the flat pixel buffer.
static void prerenderScrollText() {
  static const char scrollStr[] =
    "          "  // 10 leading spaces ≈ 20px — text appears within ~1s
    "KEEP HOLDING X FOR MONOBASS MODE  ...  "
    "HOLD X + L TO SET PPQN  ...  ";

  memset(scrollBuf, 0, sizeof(scrollBuf));

  const GFXfont* f = &Picopixel;
  uint16_t cursorX = 0;

  for (const char* p = scrollStr; *p; ++p) {
    char c = *p;
    if (c < f->first || c > f->last) continue;
    uint8_t glyphIdx = c - f->first;

    GFXglyph glyph;
    memcpy_P(&glyph, &f->glyph[glyphIdx], sizeof(GFXglyph));

    // Glyph bitmap: packed bits, MSB first
    uint16_t bitmapOff = glyph.bitmapOffset;
    uint8_t  gw = glyph.width;
    uint8_t  gh = glyph.height;
    int8_t   xo = glyph.xOffset;
    int8_t   yo = glyph.yOffset;

    // Picopixel yAdvance=7, baseline at row 5 (0-indexed).
    // yo is relative to baseline (negative = above).
    // Map glyph rows into scrollBuf rows 0..5 (6 rows for descenders).
    // Baseline row in buf = 4, so glyph row r maps to bufY = 4 + yo + r.
    for (uint8_t r = 0; r < gh; ++r) {
      int bufY = 4 + yo + r;
      if (bufY < 0 || bufY >= SCROLL_BUF_H) continue;
      for (uint8_t col = 0; col < gw; ++col) {
        uint16_t bitIdx = (uint16_t)r * gw + col;
        uint8_t  byteVal = pgm_read_byte(&f->bitmap[bitmapOff + (bitIdx >> 3)]);
        if (byteVal & (0x80 >> (bitIdx & 7))) {
          int bufX = (int)cursorX + xo + col;
          if (bufX >= 0 && bufX < SCROLL_BUF_W) {
            scrollBuf[bufY * SCROLL_BUF_W + bufX] = 1;
          }
        }
      }
    }
    cursorX += glyph.xAdvance;
    if (cursorX >= SCROLL_BUF_W) break;
  }
  scrollBufWidth = (cursorX < SCROLL_BUF_W) ? cursorX : SCROLL_BUF_W;
}

// --- Full overlay renderer ---

// Renders the full-screen X-combo help overlay.
// Layout: header tab, circle buttons, step button diagram, scrolling marquee.
static void renderXComboOverlay(uint32_t nowMs) {
  display.clearDisplay();
  display.setFont(&Picopixel);
  display.setTextSize(1);
  display.setTextWrap(false);

  // ── Header tab (y=0–6, compact left-aligned) ──
  display.fillRect(0, 0, 68, 7, SH110X_WHITE);
  display.setTextColor(SH110X_BLACK, SH110X_WHITE);
  display.setCursor(2, 5);
  display.print("HOLD X AND PRESS...");
  display.setTextColor(SH110X_WHITE);
  for (int ux = 0; ux < 67; ++ux) display.drawPixel(ux, 7, SH110X_WHITE);

  // ── Circle buttons: D1(1) D2(2) D3(3) and PLAY ──
  // Button circles at y=19 with radius 5
  static constexpr uint8_t circCx[] = { 8, 28, 48, 119 };
  for (int i = 0; i < 4; ++i) {
    display.drawCircle(circCx[i], 19, 5, SH110X_WHITE);
  }
  // Labels "1", "2", "3" centered in circles
  display.setCursor(7, 21);  display.print("1");
  display.setCursor(27, 21); display.print("2");
  display.setCursor(47, 21); display.print("3");

  // PLAY triangle inside circle at (119, 19)
  // Hand-drawn: 7 rows centered, widths 1,2,3,4,3,2,1 → solid right-arrow
  static const uint8_t triW[] = {1, 2, 3, 4, 3, 2, 1};
  for (int r = 0; r < 7; ++r)
    for (int c = 0; c < triW[r]; ++c)
      display.drawPixel(118 + c, 16 + r, SH110X_WHITE);

  // "TOGGLE CHROMA" label between (3) and (PLAY)
  display.setCursor(58, 21);
  display.print("TOGGLE CHROMA");

  // ── Labels above step buttons (y=33–38) ──
  display.setCursor(1, 37);
  display.print("SELECT MEM 0-9");

  display.setCursor(91, 37);
  display.print("SHUFFLE");

  // Hand-drawn ⤵ arrow: horizontal bar → vertical stem → arrowhead tip
  display.drawPixel(120, 34, SH110X_WHITE);  // horizontal bar
  display.drawPixel(121, 34, SH110X_WHITE);
  display.drawPixel(122, 34, SH110X_WHITE);
  display.drawPixel(123, 34, SH110X_WHITE);
  display.drawPixel(123, 35, SH110X_WHITE);  // vertical stem
  display.drawPixel(123, 36, SH110X_WHITE);
  display.drawPixel(123, 37, SH110X_WHITE);
  display.drawPixel(122, 37, SH110X_WHITE);  // arrowhead left
  display.drawPixel(124, 37, SH110X_WHITE);  // arrowhead right
  display.drawPixel(123, 38, SH110X_WHITE);  // arrowhead tip

  // ── Step buttons (y=40–48) ──
  char numBuf[3];
  for (int i = 0; i < 16; ++i) {
    uint8_t x = kStepX[i];
    if (i <= 9) {
      // Full-size buttons (7×9) with digit label
      display.drawRect(x, 40, 7, 9, SH110X_WHITE);
      snprintf(numBuf, sizeof(numBuf), "%d", i);
      display.setCursor(x + 2, 46);
      display.print(numBuf);
    } else if (i <= 14) {
      // Smaller buttons (5×7), dotted outline — no combo function
      uint8_t bx = x, by = 41, bw = 5, bh = 7;
      // Four corners always solid
      display.drawPixel(bx, by, SH110X_WHITE);
      display.drawPixel(bx + bw - 1, by, SH110X_WHITE);
      display.drawPixel(bx, by + bh - 1, SH110X_WHITE);
      display.drawPixel(bx + bw - 1, by + bh - 1, SH110X_WHITE);
      // Top/bottom edges — relative parity so all boxes match
      for (int dx = bx + 1; dx < bx + bw - 1; dx++) {
        if ((dx - bx) % 2 == 0) {
          display.drawPixel(dx, by, SH110X_WHITE);
          display.drawPixel(dx, by + bh - 1, SH110X_WHITE);
        }
      }
      // Left/right edges — relative parity
      for (int dy = by + 1; dy < by + bh - 1; dy++) {
        if ((dy - by) % 2 == 0) {
          display.drawPixel(bx, dy, SH110X_WHITE);
          display.drawPixel(bx + bw - 1, dy, SH110X_WHITE);
        }
      }
    } else {
      // Step 15: full-size with centered 3×3 marker
      display.drawRect(x, 40, 7, 9, SH110X_WHITE);
      display.fillRect(x + 2, 43, 3, 3, SH110X_WHITE);
    }
  }

  // ── Scrolling marquee (y=56–63, 8px tall) ──
  // White bar with manually rounded corners (skip 4 corner pixels)
  display.fillRect(1, 56, 126, 8, SH110X_WHITE);
  for (int ry = 57; ry <= 62; ++ry) {
    display.drawPixel(0, ry, SH110X_WHITE);
    display.drawPixel(127, ry, SH110X_WHITE);
  }

  if (scrollBufWidth > 0) {
    uint32_t elapsed = nowMs - xComboOverlayStartMs;
    uint16_t scrollOff = (uint16_t)((elapsed * 2) / 75) % scrollBufWidth;

    // Blit 126px window from scroll buffer (6 rows at y=57–62)
    for (int sx = 0; sx < 126; ++sx) {
      uint16_t srcX = (scrollOff + sx) % scrollBufWidth;
      for (int sy = 0; sy < SCROLL_BUF_H; ++sy) {
        if (scrollBuf[sy * SCROLL_BUF_W + srcX]) {
          display.drawPixel(sx + 1, 57 + sy, SH110X_BLACK);
        }
      }
    }
  }

  // Push to OLED if safe
  if (isSafeToPushOled(nowMs)) {
    display.display();
  }

  // Restore default font for next caller
  display.setFont(NULL);
}
