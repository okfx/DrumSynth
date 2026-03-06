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

// Display dimensions for scope area (used by .ino when calling drawScopeWaveform)
static constexpr int SCOPE_DISPLAY_WIDTH  = 124;
static constexpr int SCOPE_DISPLAY_HEIGHT = 40;

// Audio constants
static constexpr int   AUDIO_BLOCK_SIZE   = 128;   // Teensy audio library block size
static constexpr float INV_32768          = 1.0f / 32768.0f;

// Circular buffer holding one screen-width of normalized samples (-1..+1)
float scopeBuffer[SCOPE_DISPLAY_WIDTH] = { 0 };
uint8_t scopeBufferWriteIndex = 0;

// Smoothed auto-scale state — peak hold with slow release
float scopePeakRange = 0.05f;  // starts at noise floor
float scopeMidpoint = 0.0f;    // smoothed vertical center

// ============================================================================
//  updateScopeData() — call from main loop to consume audio samples
// ============================================================================

void updateScopeData() {
  static uint8_t blockSkipCounter = 0;
  const uint8_t BLOCKS_TO_SKIP = 12;   // Process every 12th block (skip 11, keep 1)
  const int SAMPLE_DECIMATION = 16;
  const int SAMPLES_PER_BLOCK = AUDIO_BLOCK_SIZE / SAMPLE_DECIMATION;

  while (scopeQueue.available() >= 1) {
    int16_t* samples = scopeQueue.readBuffer();

    if (++blockSkipCounter < BLOCKS_TO_SKIP) {
      scopeQueue.freeBuffer();
      continue;
    }
    blockSkipCounter = 0;

    for (int i = 0; i < SAMPLES_PER_BLOCK; i++) {
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

void drawScopeWaveform(int x, int y, int h) {
  // Snapshot write index before any reads. Both updateScopeData() and this
  // function run sequentially in loop(), so the index can't change mid-call,
  // but snapshotting makes that assumption explicit and safe against refactoring.
  uint8_t writeSnap = scopeBufferWriteIndex;

  // Find min/max for AC auto-scaling (signed signal)
  float minVal = 0.0f, maxVal = 0.0f;
  for (int i = 0; i < SCOPE_DISPLAY_WIDTH; i++) {
    float v = scopeBuffer[i];
    if (v < minVal) minVal = v;
    if (v > maxVal) maxVal = v;
  }

  float range = maxVal - minVal;

  // Noise floor — keeps the scope visually quiet during silence
  // instead of amplifying background noise to fill the screen.
  const float NOISE_FLOOR = 0.05f;
  if (range < 0.001f) return;              // nothing to draw

  // Smoothed auto-scale with attack/release.
  // Fast (but not instant) attack keeps transients from jolting the scale;
  // slow release lets drum tails decay gracefully.
  if (range > scopePeakRange) {
    scopePeakRange += (range - scopePeakRange) * 0.5f;  // fast attack (50% per frame)
  } else {
    scopePeakRange *= 0.97f;               // slow release (~3% per frame)
    if (scopePeakRange < NOISE_FLOOR) scopePeakRange = NOISE_FLOOR;
  }

  float vScale = (h * 0.9f) / scopePeakRange;

  // Smooth vertical center with high inertia — prevents baseline jumping
  // as the circular buffer scrolls through different parts of the waveform.
  float rawMid = (minVal + maxVal) * 0.5f;
  float midDiff = rawMid - scopeMidpoint;
  if (midDiff > 0.0f) {
    scopeMidpoint += midDiff * 0.12f;  // slow attack (rise)
  } else {
    scopeMidpoint += midDiff * 0.06f;  // very slow release (fall)
  }

  // Precompute y-coordinates for all samples, centered on smoothed midpoint.
  // writeSnap+i wraps at most once, so a conditional subtract replaces modulo.
  int ys[SCOPE_DISPLAY_WIDTH];
  for (int i = 0; i < SCOPE_DISPLAY_WIDTH; i++) {
    int idx = writeSnap + i;
    if (idx >= SCOPE_DISPLAY_WIDTH) idx -= SCOPE_DISPLAY_WIDTH;
    int py = y + h / 2 - (int)((scopeBuffer[idx] - scopeMidpoint) * vScale);
    ys[i] = constrain(py, y, y + h - 1);
  }

  // Draw waveform (scrolling, AC-coupled — shows peaks and valleys)
  for (int i = 0; i < SCOPE_DISPLAY_WIDTH - 1; i++) {
    int px = x + i;

    display.drawLine(px, ys[i], px + 1, ys[i + 1], 1);

    // Fill vertical gaps on sharp transients (>5px jump).
    // Makes drum attacks look bold and punchy instead of thin diagonals.
    int gap = abs(ys[i + 1] - ys[i]);
    if (gap > 5) {
      int top = min(ys[i], ys[i + 1]);
      int bot = max(ys[i], ys[i + 1]);
      display.drawLine(px + 1, top, px + 1, bot, 1);
    }
  }
}

#endif // OSCILLOSCOPE_H
