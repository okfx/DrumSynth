#ifndef OSCILLOSCOPE_H
#define OSCILLOSCOPE_H
// ============================================================================
//  Oscilloscope — oscilloscope.h
//
//  AC-coupled scrolling waveform display from AudioRecordQueue.
//  Decimates and block-skips to show a readable waveform on the 128x64 OLED.
//  Auto-scales to signal amplitude.
//
//  Include AFTER: audiotool.h (scopeQueue), hw_setup.h (display)
// ============================================================================

#include <Arduino.h>

// Display dimension for scope area
static constexpr int SCOPE_DISPLAY_WIDTH = 124;

// Circular buffer holding one screen-width of normalized samples (-1..+1)
float scopeBuffer[SCOPE_DISPLAY_WIDTH] = { 0 };
uint8_t scopeBufferWriteIndex = 0;

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

    for (int i = 0; i < 128 / SAMPLE_DECIMATION && scopeBufferWriteIndex < SCOPE_DISPLAY_WIDTH; i++) {
      scopeBuffer[scopeBufferWriteIndex] = samples[i * SAMPLE_DECIMATION] / 32768.0f;
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
  if (range < 0.001f) return;           // nothing to draw
  if (range < 0.01f) range = 0.01f;     // floor for very quiet signals

  float vScale = (h * 0.9f) / range;

  // Snapshot write index so it doesn't change mid-render
  uint8_t writeSnap = scopeBufferWriteIndex;

  // Draw waveform (scrolling, AC-coupled — shows peaks and valleys)
  for (int i = 0; i < SCOPE_DISPLAY_WIDTH - 1; i++) {
    int i1 = (writeSnap + i) % SCOPE_DISPLAY_WIDTH;
    int i2 = (writeSnap + i + 1) % SCOPE_DISPLAY_WIDTH;

    int y1 = y + h / 2 - (int)((scopeBuffer[i1] - minVal) * vScale - range * vScale / 2);
    int y2 = y + h / 2 - (int)((scopeBuffer[i2] - minVal) * vScale - range * vScale / 2);

    y1 = constrain(y1, y, y + h - 1);
    y2 = constrain(y2, y, y + h - 1);

    int x1 = x + (i * w) / SCOPE_DISPLAY_WIDTH;
    int x2 = x + ((i + 1) * w) / SCOPE_DISPLAY_WIDTH;

    display.drawLine(x1, y1, x2, y2, 1);
  }
}

#endif // OSCILLOSCOPE_H
