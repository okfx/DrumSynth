#ifndef OSCILLOSCOPE_H
#define OSCILLOSCOPE_H
// ============================================================================
//  Oscilloscope — oscilloscope.h
//
//  AC-coupled scrolling waveform display from AudioRecordQueue.
//  Decimates and block-skips to show a readable waveform on the 128x64 OLED.
//  Auto-scales to signal amplitude with smoothed peak hold.
//
//  Include AFTER: audiotool.h (scopeQueue), hw_setup.h (display)
// ============================================================================

#include <Arduino.h>

// Display dimension for scope area
static constexpr int SCOPE_DISPLAY_WIDTH  = 124;
static constexpr int SCOPE_DISPLAY_HEIGHT = 40;
static constexpr float INV_32768 = 1.0f / 32768.0f;

// Circular buffer holding one screen-width of normalized samples (-1..+1)
float scopeBuffer[SCOPE_DISPLAY_WIDTH] = { 0 };
uint8_t scopeBufferWriteIndex = 0;

// Smoothed auto-scale state — peak hold with slow release
float scopePeakRange = 0.05f;  // starts at noise floor

// External dependencies
extern AudioRecordQueue scopeQueue;            // from audiotool.h
extern Adafruit_SH1106G display;               // from hw_setup.h

// ============================================================================
//  updateScopeData() — call from main loop to consume audio samples
// ============================================================================

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

    for (int i = 0; i < 128 / SAMPLE_DECIMATION; i++) {
      scopeBuffer[scopeBufferWriteIndex] = samples[i * SAMPLE_DECIMATION] * INV_32768;
      scopeBufferWriteIndex = (scopeBufferWriteIndex + 1) % SCOPE_DISPLAY_WIDTH;
    }

    scopeQueue.freeBuffer();
    break;
  }
}

// ============================================================================
//  drawScopeWaveform() — render waveform into display buffer
// ============================================================================

void drawScopeWaveform(int x, int y, int w, int h) {
  // Find min/max for AC auto-scaling (signed signal)
  float minVal = 0.0f, maxVal = 0.0f;
  for (int i = 0; i < SCOPE_DISPLAY_WIDTH; i++) {
    if (scopeBuffer[i] < minVal) minVal = scopeBuffer[i];
    if (scopeBuffer[i] > maxVal) maxVal = scopeBuffer[i];
  }

  float range = maxVal - minVal;

  // #3: Higher noise floor — keeps the scope visually quiet during silence
  // instead of amplifying background noise to fill the screen.
  const float NOISE_FLOOR = 0.05f;
  if (range < 0.001f) return;              // nothing to draw

  // #2: Smoothed auto-scale with attack/release.
  // Jumps instantly to louder signals (attack), but releases slowly (5% per frame)
  // so the waveform doesn't jitter between frames as drum tails decay.
  if (range > scopePeakRange) {
    scopePeakRange = range;                 // instant attack
  } else {
    scopePeakRange *= 0.95f;               // slow release (~5% per frame)
    if (scopePeakRange < NOISE_FLOOR) scopePeakRange = NOISE_FLOOR;
  }

  float vScale = (h * 0.9f) / scopePeakRange;

  // Snapshot write index so it doesn't change mid-render
  uint8_t writeSnap = scopeBufferWriteIndex;

  // Precompute y-coordinates for all samples
  int ys[SCOPE_DISPLAY_WIDTH];
  float midOffset = scopePeakRange * vScale * 0.5f;
  for (int i = 0; i < SCOPE_DISPLAY_WIDTH; i++) {
    int idx = (writeSnap + i) % SCOPE_DISPLAY_WIDTH;
    int py = y + h / 2 - (int)((scopeBuffer[idx] - minVal) * vScale - midOffset);
    ys[i] = constrain(py, y, y + h - 1);
  }

  // Draw waveform (scrolling, AC-coupled — shows peaks and valleys)
  for (int i = 0; i < SCOPE_DISPLAY_WIDTH - 1; i++) {
    int x1 = x + (i * w) / SCOPE_DISPLAY_WIDTH;
    int x2 = x + ((i + 1) * w) / SCOPE_DISPLAY_WIDTH;

    display.drawLine(x1, ys[i], x2, ys[i + 1], 1);

    // #4: Fill vertical gaps on sharp transients (>5px jump).
    // Makes drum attacks look bold and punchy instead of thin diagonals.
    int gap = ys[i + 1] - ys[i];
    if (gap < 0) gap = -gap;
    if (gap > 5) {
      int top = (ys[i] < ys[i + 1]) ? ys[i] : ys[i + 1];
      int bot = (ys[i] > ys[i + 1]) ? ys[i] : ys[i + 1];
      display.drawLine(x2, top, x2, bot, 1);
    }
  }
}

#endif // OSCILLOSCOPE_H
