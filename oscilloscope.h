#ifndef OSCILLOSCOPE_H
#define OSCILLOSCOPE_H
// ============================================================================
//  Oscilloscope — oscilloscope.h
//
//  Phase-coherent waveform display from AudioRecordQueue.
//  Captures a contiguous snapshot of audio samples, then holds it until
//  the next refresh interval. This preserves waveshape (sine looks like
//  sine, square looks like square) unlike a scrolling decimated view.
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

// Snapshot buffer — filled with contiguous samples, then frozen for display
float scopeBuffer[SCOPE_DISPLAY_WIDTH] = { 0 };
bool  scopeBufferReady = false;

// Smoothed auto-scale state — peak hold with slow release
float scopePeakRange = 0.05f;  // starts at noise floor

// Refresh timing — hold each snapshot for ~80ms before capturing a new one
static constexpr uint32_t SCOPE_REFRESH_MS = 80;
uint32_t scopeLastCaptureMs = 0;

// ============================================================================
//  updateScopeData() — call from main loop to consume audio samples
//
//  Strategy: accumulate samples from consecutive audio blocks into a
//  contiguous snapshot. Decimate by 4 (not 16) so waveform shape is
//  preserved. Each 128-sample block yields 32 display samples; we need
//  4 consecutive blocks (4×32=128 > 124) to fill the display width.
//  After filling, freeze the buffer until the refresh interval elapses.
// ============================================================================

void updateScopeData() {
  static int fillIndex = 0;        // how many samples written so far
  static bool capturing = false;   // true while accumulating blocks

  // Drain the queue — always free buffers to prevent backup
  while (scopeQueue.available() >= 1) {
    int16_t* samples = scopeQueue.readBuffer();

    if (!capturing) {
      // Check if it's time for a new snapshot
      uint32_t now = sysTickMs;
      if ((uint32_t)(now - scopeLastCaptureMs) >= SCOPE_REFRESH_MS) {
        capturing = true;
        fillIndex = 0;
      } else {
        scopeQueue.freeBuffer();
        continue;
      }
    }

    if (capturing) {
      // Decimate by 4: take every 4th sample from the 128-sample block
      // Effective sample rate: 44100/4 = 11025 Hz — plenty for bass waveforms
      const int DECIMATE = 4;
      for (int i = 0; i < AUDIO_BLOCK_SIZE && fillIndex < SCOPE_DISPLAY_WIDTH; i += DECIMATE) {
        scopeBuffer[fillIndex++] = samples[i] * INV_32768;
      }

      if (fillIndex >= SCOPE_DISPLAY_WIDTH) {
        // Snapshot complete — freeze for display
        capturing = false;
        scopeBufferReady = true;
        scopeLastCaptureMs = sysTickMs;
      }
    }

    scopeQueue.freeBuffer();
  }
}

// ============================================================================
//  drawScopeWaveform() — render waveform into display buffer
// ============================================================================

void drawScopeWaveform(int x, int y, int h) {
  if (!scopeBufferReady) return;

  // Find min/max for AC auto-scaling (signed signal)
  float minVal = 0.0f, maxVal = 0.0f;
  for (int i = 0; i < SCOPE_DISPLAY_WIDTH; i++) {
    float v = scopeBuffer[i];
    if (v < minVal) minVal = v;
    if (v > maxVal) maxVal = v;
  }

  float range = maxVal - minVal;

  // Noise floor — keeps the scope visually quiet during silence
  const float NOISE_FLOOR = 0.05f;
  if (range < 0.001f) return;              // nothing to draw

  // Smoothed auto-scale with attack/release.
  // Fast attack keeps transients from jolting the scale;
  // slow release lets drum tails decay gracefully.
  if (range > scopePeakRange) {
    scopePeakRange += (range - scopePeakRange) * 0.5f;  // fast attack
  } else {
    scopePeakRange *= 0.97f;               // slow release
    if (scopePeakRange < NOISE_FLOOR) scopePeakRange = NOISE_FLOOR;
  }

  float vScale = (h * 0.9f) / scopePeakRange;

  // AC-couple: center on this snapshot's midpoint (no smoothing needed
  // since the buffer is a contiguous capture, not a scrolling mix)
  float mid = (minVal + maxVal) * 0.5f;

  // Precompute y-coordinates for all samples
  int ys[SCOPE_DISPLAY_WIDTH];
  for (int i = 0; i < SCOPE_DISPLAY_WIDTH; i++) {
    int py = y + h / 2 - (int)((scopeBuffer[i] - mid) * vScale);
    ys[i] = constrain(py, y, y + h - 1);
  }

  // Draw waveform with line segments
  for (int i = 0; i < SCOPE_DISPLAY_WIDTH - 1; i++) {
    int px = x + i;
    display.drawLine(px, ys[i], px + 1, ys[i + 1], 1);

    // Fill vertical gaps on sharp transients (>5px jump).
    // Makes square waves and drum attacks look bold.
    int gap = abs(ys[i + 1] - ys[i]);
    if (gap > 5) {
      int top = min(ys[i], ys[i + 1]);
      int bot = max(ys[i], ys[i + 1]);
      display.drawLine(px + 1, top, px + 1, bot, 1);
    }
  }
}

#endif // OSCILLOSCOPE_H
