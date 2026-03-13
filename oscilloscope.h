#ifndef OSCILLOSCOPE_H
#define OSCILLOSCOPE_H
// ============================================================================
//  Oscilloscope — oscilloscope.h
//
//  Triggered waveform display from AudioRecordQueue.
//  Captures a contiguous buffer of audio samples, finds a rising zero-
//  crossing to lock the phase, then renders from that trigger point.
//  This makes sine/saw/square shapes visually distinct and stable.
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

// Trigger search window — how many samples to scan for a rising zero-crossing
// before falling back to free-running. 128 samples at 22 kHz ≈ 5.8ms, enough
// to find a crossing for any frequency above ~170 Hz within one period.
static constexpr int SCOPE_TRIGGER_SEARCH = 128;

// Raw capture buffer — display width plus trigger search headroom
static constexpr int SCOPE_CAPTURE_LEN = SCOPE_DISPLAY_WIDTH + SCOPE_TRIGGER_SEARCH;
static float scopeCapture[SCOPE_CAPTURE_LEN] = { 0 };

// Display-ready buffer — trigger-aligned samples for rendering
static float scopeBuffer[SCOPE_DISPLAY_WIDTH] = { 0 };
static bool  scopeBufferReady = false;

// Smoothed auto-scale — peak hold with slow release
static constexpr float SCOPE_NOISE_FLOOR = 0.05f;
static float scopePeakRange = SCOPE_NOISE_FLOOR;

// Refresh timing — matches OLED_FRAME_INTERVAL_MS (42ms) so a fresh triggered
// snapshot is ready every display frame (~24 FPS). Capture takes ~12ms minimum
// (4 audio blocks), so 35ms gives a small margin without idle time.
static constexpr uint32_t SCOPE_REFRESH_MS = 35;
static uint32_t scopeLastCaptureMs = 0;

// ============================================================================
//  findTrigger() — rising zero-crossing search
//
//  Scans the first `searchLen` samples for a rising zero-crossing
//  (negative→positive transition). Returns the index of the first
//  positive sample, or 0 if no crossing found (free-running fallback).
// ============================================================================

static int findTrigger(const float* buf, int searchLen) {
  for (int i = 1; i < searchLen; i++) {
    if (buf[i - 1] <= 0.0f && buf[i] > 0.0f) {
      return i;
    }
  }
  return 0;  // no crossing — show from start
}

// ============================================================================
//  updateScopeData() — call from main loop to consume audio samples
//
//  Captures contiguous samples into an oversized buffer, then finds a
//  trigger point and copies SCOPE_DISPLAY_WIDTH samples starting there
//  into the display buffer. Decimation by 2 gives 22 kHz effective
//  sample rate — good detail for all waveform shapes.
// ============================================================================

void updateScopeData() {
  static int fillIndex = 0;
  static bool capturing = false;

  while (scopeQueue.available() >= 1) {
    int16_t* samples = scopeQueue.readBuffer();

    if (!capturing) {
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
      // Decimate by 2: effective 22050 Hz — preserves sharp edges on square waves
      static constexpr int SCOPE_DECIMATE = 2;
      for (int i = 0; i < AUDIO_BLOCK_SIZE && fillIndex < SCOPE_CAPTURE_LEN; i += SCOPE_DECIMATE) {
        scopeCapture[fillIndex++] = samples[i] * INV_32768;
      }

      if (fillIndex >= SCOPE_CAPTURE_LEN) {
        capturing = false;
        scopeLastCaptureMs = sysTickMs;

        // Find rising zero-crossing in the trigger search window
        int trigIdx = findTrigger(scopeCapture, SCOPE_TRIGGER_SEARCH);

        // Copy trigger-aligned samples into display buffer
        memcpy(scopeBuffer, &scopeCapture[trigIdx],
               SCOPE_DISPLAY_WIDTH * sizeof(float));
        scopeBufferReady = true;
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

  if (range < 0.001f) return;  // nothing to draw

  // Smoothed auto-scale with attack/release
  if (range > scopePeakRange) {
    scopePeakRange += (range - scopePeakRange) * 0.5f;
  } else {
    scopePeakRange *= 0.97f;
    if (scopePeakRange < SCOPE_NOISE_FLOOR) scopePeakRange = SCOPE_NOISE_FLOOR;
  }

  float vScale = (h * 0.9f) / scopePeakRange;

  // AC-couple: center on this snapshot's midpoint
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

    // Fill vertical gaps on sharp transients (>3px jump).
    // Makes square waves and drum attacks look bold and solid.
    int gap = abs(ys[i + 1] - ys[i]);
    if (gap > 3) {
      int top = min(ys[i], ys[i + 1]);
      int bot = max(ys[i], ys[i + 1]);
      display.drawLine(px + 1, top, px + 1, bot, 1);
    }
  }
}

#endif // OSCILLOSCOPE_H
