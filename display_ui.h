// display_ui.h — OLED display rendering: top bar, scope area, chroma dots,
// parameter overlay, MONOBASS sweep animation, and main updateDisplay() loop.
//
// Depends on: hw_setup.h (display), oscilloscope.h (drawScopeWaveform),
// chroma.h (formatChromaNote, cancelChromaAnim, startChromaSettle),
// monobass.h (renderMonoBassScope), xcombo_overlay.h (renderXComboOverlay),
// buttons.h (comboMod), bitmaps.h (play/stop icons).
// All included before this header in DrumSynth.ino.

#pragma once

// --- Extern: display (from hw_setup.h, already in scope) ---
extern Adafruit_SH1106G display;

// --- Extern: state variables (defined in .ino above include point) ---
extern float uiMixD1Shape;
extern float uiMixD2Voice;
extern float uiMixD3Voice;
extern RailMode activeRail;
extern Track activeTrack;
extern char displayParameter1[];
extern char displayParameter2[];
extern uint32_t parameterOverlayStartTick;
extern volatile uint32_t lastFrameDrawTick;
extern int chokeDisplayPercent;
extern float extBpmDisplay;

extern volatile float bpm;
extern volatile uint32_t sysTickMs;
extern volatile TransportState transportState;
extern uint32_t shuffleOverlayStartTick;
extern char     shuffleOverlayText[];
extern bool     slotPending;
extern volatile bool sequencePlaying;
extern volatile bool armed;
extern volatile uint16_t armPulseCountdown;
extern volatile uint8_t ppqn;

extern bool ppqnModeActive;
extern uint8_t ppqnModeSelection;
extern uint8_t activeSaveSlot;

extern bool d1ChromaMode;
extern bool d2ChromaMode;
extern bool d3ChromaMode;
extern bool wfChromaMode;
extern int8_t d1ChromaHeldStep;
extern int8_t d2ChromaHeldStep;
extern int8_t d3ChromaHeldStep;
extern uint8_t d1ChromaNote[];
extern uint8_t d2ChromaNote[];
extern uint8_t d3ChromaNote[];
extern int8_t chromaAnimDot;
extern ChromaAnimPhase chromaAnimPhase;
extern uint32_t chromaAnimStart;
extern bool chromaAnimAppearing;

extern MonoBassState monoBass;
extern MonoAnimPhase monoAnimPhase;
extern uint32_t monoAnimStart;
extern uint8_t monoTextMask[];
extern bool monoMasksReady;

// --- Extern: functions (defined in .ino or earlier headers) ---
extern bool isSafeToPushOled(uint32_t nowMs);
extern void playSequence();

// --- UI Helpers ---

// Draw white text with a 2-pixel black outline for readability over the scope waveform.
// Stamps black text at 8 offsets (4 cardinal + 4 diagonal, 2px each) then white on top.
void drawOutlinedText(int x, int y, const char* text) {
  static const int8_t offsets[][2] = {
    {-2, 0}, {2, 0}, {0, -2}, {0, 2},   // cardinal 2px
    {-2,-2}, {2,-2}, {-2, 2}, {2, 2}     // diagonal 2px
  };
  display.setTextColor(0);
  for (auto& o : offsets) {
    display.setCursor(x + o[0], y + o[1]);
    display.print(text);
  }
  display.setTextColor(1);
  display.setCursor(x, y);
  display.print(text);
}

static inline void drawCaretRail(int x, int y, int w, int h, int caretX) {
  display.drawRect(x, y, w, h, 1);
  int cx = constrain(caretX, x + 1, x + w - 2);
  display.drawLine(cx, y - 2, cx, y + h + 2, 1);
}

static inline void labelAtCenter(const char* text, int centerX, int y) {
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  int x = centerX - (w / 2);
  drawOutlinedText(x, y, text);
}

void renderVoiceRails() {
  // Rail displayed alongside scope: label text at y=23, rail bar at y=44
  const int railX = 6;
  const int railY = 44;
  const int railW = 116;
  const int railH = 10;
  const int labelY = 23;

  display.setTextSize(1);

  // Black rectangle behind labels + rail so they're visible over the scope waveform
  display.fillRect(railX - 2, labelY - 2, railW + 4, (railY + railH) - labelY + 5, 0);

  if (activeRail == RAIL_D1_SHAPE) {
    drawCaretRail(railX, railY, railW, railH, railX + (int)(uiMixD1Shape * railW));
    int third = railW / 3;
    labelAtCenter("SIN", railX + third * 0 + third / 2, labelY);
    labelAtCenter("SAW", railX + third * 1 + third / 2, labelY);
    labelAtCenter("SQR", railX + third * 2 + third / 2, labelY);
    // Zone thresholds match engine crossfade (case 1):
    // sine+saw blend for first half, saw+square blend for second half
    int zone = (uiMixD1Shape < 0.25f) ? 0    // SIN dominant
             : (uiMixD1Shape < 0.75f) ? 1    // SAW dominant (peaks at 50%)
                                      : 2;   // SQR dominant
    int zx = railX + zone * third + 1;
    int zw = third - 2;
    display.fillRect(zx, railY + 1, zw, railH - 2, 1);

  } else if (activeRail == RAIL_D2_VOICE) {
    int caret = railX + (int)(uiMixD2Voice * railW);
    drawCaretRail(railX, railY, railW, railH, caret);
    labelAtCenter("CLAP", railX + 14, labelY);
    labelAtCenter("SNARE", railX + railW - 14, labelY);
    int fillW = max(1, caret - railX);
    if (fillW > 2) display.fillRect(railX + 1, railY + 1, fillW - 2, railH - 2, 1);

  } else if (activeRail == RAIL_D3_VOICE) {
    drawCaretRail(railX, railY, railW, railH, railX + (int)(uiMixD3Voice * railW));
    int third = railW / 3;
    labelAtCenter("FM", railX + third * 0 + third / 2, labelY);
    labelAtCenter("606", railX + third * 1 + third / 2, labelY);
    labelAtCenter("PERC", railX + third * 2 + third / 2, labelY);
    // Zone thresholds match engine crossfade midpoints (case 18):
    // FM solo < 6%, crossfade 6–46%, 606 solo 46–65%, crossfade 65–94%, PERC solo > 94%
    int zone = (uiMixD3Voice < 0.27f)  ? 0    // FM side (solo + crossfade into 606)
             : (uiMixD3Voice < 0.80f)  ? 1    // 606 zone (crossfade + solo + crossfade)
                                       : 2;   // PERC side (crossfade from 606 + solo)
    int zx = railX + zone * third + 1;
    int zw = third - 2;
    display.fillRect(zx, railY + 1, zw, railH - 2, 1);
  }
}

// --- OLED Drawing — sub-renderers ---

// Top bar: BPM/EXT, track digit, choke, MEM slot, transport icon.
void renderTopBar(float bpmSnap, uint8_t trackSnap, int chokeSnap,
                  uint8_t slotSnap, bool playingSnap) {
  // MONOBASS mode: replace entire toolbar with centered "MONOBASS" text.
  // During exit sweep, draw the text as base — the sweep overwrites it R→L.
  if (monoBass.active) {
    display.setTextSize(2);
    display.setCursor(kMonoTextX, kMonoTextY);
    display.print("MONOBASS");
    display.setTextSize(1);
    return;
  }

  // BPM / EXT clock state
  display.setCursor(0, 0);
  if (extBpmDisplay > 0.0f) {
    display.print("EXT");
    display.setCursor(0, 10);
    float snapped = floorf(extBpmDisplay * 2.0f + 0.5f) * 0.5f;
    display.print(snapped, 1);
  } else {
    display.print("BPM");
    display.setCursor(0, 10);
    display.print(bpmSnap, 1);
  }

  // Size-2 track digit
  display.setTextSize(2);
  display.setCursor(38, 2);
  display.print(trackSnap + 1);
  display.setTextSize(1);

  // Choke
  display.setCursor(58, 0);
  display.print("CHOKE");
  {
    char chokeBuf[8];
    if (chokeSnap == 0) {
      snprintf(chokeBuf, sizeof(chokeBuf), "OFF");
    } else {
      snprintf(chokeBuf, sizeof(chokeBuf), "%+d%%", chokeSnap);
    }
    display.setCursor(73 - (int)strlen(chokeBuf) * 3, 10);
    display.print(chokeBuf);
  }

  // Memory slot — blink number when pending (selected but not yet loaded)
  display.setCursor(95, 0);
  display.print("MEM");
  {
    bool showNum = !slotPending || ((sysTickMs / 300) & 1);  // ~1.7 Hz blink
    if (showNum) {
      char memBuf[4];
      snprintf(memBuf, sizeof(memBuf), "%d", slotSnap + 1);
      display.setCursor(104 - (int)strlen(memBuf) * 3, 10);
      display.print(memBuf);
    }
  }

  // Transport icon
  if (playingSnap) {
    display.drawBitmap(118, 4, image_play_bits, 10, 10, 1);
  } else {
    display.drawBitmap(118, 4, image_stop_bits, 10, 10, 1);
  }
}

// renderMonoBassScope() is now in monobass.h

// Chroma note select: large note name when a chroma step is held.
// Returns true if it drew content (suppresses oscilloscope).
bool renderChromaNoteSelect() {
  bool noteSelectActive = (d1ChromaHeldStep >= 0) || (d2ChromaHeldStep >= 0) || (d3ChromaHeldStep >= 0);
  if (!noteSelectActive) return false;

  int stepNum;
  uint8_t midiNote;
  if (d1ChromaHeldStep >= 0) {
    stepNum = d1ChromaHeldStep;
    midiNote = d1ChromaNote[d1ChromaHeldStep];
  } else if (d2ChromaHeldStep >= 0) {
    stepNum = d2ChromaHeldStep;
    midiNote = d2ChromaNote[d2ChromaHeldStep];
  } else {
    stepNum = d3ChromaHeldStep;
    midiNote = d3ChromaNote[d3ChromaHeldStep];
  }

  char noteName[8];
  formatChromaNote(midiNote, noteName);

  // "STEP X" label — centered at top of scope area
  display.setTextSize(1);
  char stepLabel[10];
  snprintf(stepLabel, sizeof(stepLabel), "STEP %d", stepNum + 1);
  int16_t sx1, sy1;
  uint16_t sw, sh;
  display.getTextBounds(stepLabel, 0, 0, &sx1, &sy1, &sw, &sh);
  display.setCursor((128 - sw) / 2, 24);
  display.print(stepLabel);

  // Large note name
  display.setTextSize(3);
  int16_t nx1, ny1;
  uint16_t nw, nh;
  display.getTextBounds(noteName, 0, 0, &nx1, &ny1, &nw, &nh);
  display.setCursor((128 - nw) / 2, 35);
  display.print(noteName);
  display.setTextSize(1);
  return true;
}

// Chroma dot indicator — bottom bar dots for active chroma channels.
// Animation: 3-phase Bayer-dithered 8×8 dot (outline → fill → hollow center).
void renderChromaDots(uint32_t nowMs) {
  if (monoBass.active) return;

  const bool chromaActive[4] = {
    d1ChromaMode, d2ChromaMode, d3ChromaMode, wfChromaMode
  };
  const int dotCenters[4] = { 16, 48, 80, 112 };
  static constexpr int kSteadySize = 8;
  static constexpr int kClearSize = 12;
  static constexpr int kCenterY = 60;
  static constexpr int kHalf = 4;

  bool animActive = (chromaAnimDot >= 0 && chromaAnimPhase != CHROMA_ANIM_NONE);
  uint32_t animElapsed = animActive ? (nowMs - chromaAnimStart) : 0;

  if (!chromaActive[0] && !chromaActive[1] && !chromaActive[2]
      && !chromaActive[3] && !animActive) return;

  for (int i = 0; i < 4; i++) {
    int cx = dotCenters[i];

    if (animActive && i == chromaAnimDot) {
      display.fillRect(cx - kClearSize / 2, kCenterY - kClearSize / 2,
                       kClearSize, kClearSize, SH110X_BLACK);

      if (chromaAnimPhase == CHROMA_ANIM_RAMPING) {
        float progress = (float)animElapsed / (float)CHROMA_COMBO_RAMP_MS;
        if (progress > 1.0f) progress = 1.0f;

        if (chromaAnimAppearing) {
          // 3-phase Bayer dither: outline → fill → hollow center
          if (progress <= 0.4f) {
            // Phase 1: outline dithers in
            float t = progress / 0.4f;
            for (int py = kCenterY - kHalf; py < kCenterY + kHalf; py++) {
              for (int px = cx - kHalf; px < cx + kHalf; px++) {
                bool isEdge = (py == kCenterY - kHalf || py == kCenterY + kHalf - 1 ||
                               px == cx - kHalf || px == cx + kHalf - 1);
                if (isEdge && bayerDither(px, py, t))
                  display.drawPixel(px, py, SH110X_WHITE);
              }
            }
          } else if (progress <= 0.8f) {
            // Phase 2: outline solid, interior dithers in
            float t = (progress - 0.4f) / 0.4f;
            for (int py = kCenterY - kHalf; py < kCenterY + kHalf; py++) {
              for (int px = cx - kHalf; px < cx + kHalf; px++) {
                bool isEdge = (py == kCenterY - kHalf || py == kCenterY + kHalf - 1 ||
                               px == cx - kHalf || px == cx + kHalf - 1);
                if (isEdge) {
                  display.drawPixel(px, py, SH110X_WHITE);
                } else if (bayerDither(px, py, t)) {
                  display.drawPixel(px, py, SH110X_WHITE);
                }
              }
            }
          } else {
            // Phase 3: full square, center hollows out
            float t = (progress - 0.8f) / 0.2f;
            display.fillRect(cx - kHalf, kCenterY - kHalf, kHalf * 2, kHalf * 2, SH110X_WHITE);
            for (int py = kCenterY - 1; py <= kCenterY; py++) {
              for (int px = cx - 1; px <= cx; px++) {
                if (bayerDither(px, py, t))
                  display.drawPixel(px, py, SH110X_BLACK);
              }
            }
          }
        } else {
          // Disappearing: steady dot dissolves via dither
          display.fillRect(cx - kHalf, kCenterY - kHalf, 8, 8, SH110X_WHITE);
          display.fillRect(cx - 1, kCenterY - 1, 2, 2, SH110X_BLACK);
          for (int py = kCenterY - kHalf; py < kCenterY + kHalf; py++) {
            for (int px = cx - kHalf; px < cx + kHalf; px++) {
              if (bayerDither(px, py, progress))
                display.drawPixel(px, py, SH110X_BLACK);
            }
          }
        }
      } else {
        // SETTLING: snap to steady state
        if (chromaAnimAppearing) {
          display.fillRect(cx - kHalf, kCenterY - kHalf, 8, 8, SH110X_WHITE);
          display.fillRect(cx - 1, kCenterY - 1, 2, 2, SH110X_BLACK);
        }
      }
    } else if (chromaActive[i]) {
      // Steady state: hollow 8×8 (white border, 2×2 black center)
      display.fillRect(cx - kHalf - 1, kCenterY - kHalf - 1,
                       kSteadySize + 2, kSteadySize + 2, SH110X_BLACK);
      display.fillRect(cx - kHalf, kCenterY - kHalf, kSteadySize, kSteadySize, SH110X_WHITE);
      display.fillRect(cx - 1, kCenterY - 1, 2, 2, SH110X_BLACK);
    }
  }
}

// MONOBASS white-fill sweep animation — a solid white rectangle sweeps across
// the top-bar region with MONOBASS text knocked out in black. The sweep position
// directly communicates hold progress. Uses precomputed text mask.
void renderMonoTextAnim(uint32_t nowMs) {
  if (monoAnimPhase == MONO_ANIM_NONE || !monoMasksReady) return;

  uint32_t elapsed = nowMs - monoAnimStart;
  if (elapsed >= MONO_ANIM_DURATION_MS) elapsed = MONO_ANIM_DURATION_MS;

  float progress = (float)elapsed / (float)MONO_ANIM_DURATION_MS;
  bool entering = (monoAnimPhase == MONO_ANIM_ENTERING);

  // sweepPos: 0 to (kSweepW + kSweepTransitionW)
  // At progress=0: nothing filled. At progress=1: entire region filled.
  float sweepPos = progress * (float)(kSweepW + kSweepTransitionW);

  for (int sy = 0; sy < kSweepRegionH; sy++) {
    for (int sx = kSweepLeftX; sx < kSweepRightX; sx++) {
      int dx = entering ? (sx - kSweepLeftX) : (kSweepRightX - 1 - sx);
      float behind = sweepPos - (float)dx;
      if (behind <= 0.0f) continue;  // ahead of sweep — keep base (top bar or text)

      // Is this a text pixel?
      bool isText = false;
      int my = sy - kMonoTextY;
      int mx = sx - kMonoTextX;
      if (mx >= 0 && mx < kMonoTextW && my >= 0 && my < kMonoTextH) {
        int bit = my * kMonoTextW + mx;
        isText = monoTextMask[bit >> 3] & (1 << (bit & 7));
      }

      if (behind >= (float)kSweepTransitionW) {
        // Fully behind sweep: white fill, text knocked out in black
        display.drawPixel(sx, sy, isText ? SH110X_BLACK : SH110X_WHITE);
      } else {
        // Transition zone: clear to black, then dither white fill in
        display.drawPixel(sx, sy, SH110X_BLACK);
        if (!isText && bayerDither(sx, sy, behind / (float)kSweepTransitionW)) {
          display.drawPixel(sx, sy, SH110X_WHITE);
        }
      }
    }
  }
}

// Parameter overlay — bottom text or voice rails.
void renderParameterOverlay(uint8_t railSnap, const char* param1Snap,
                            const char* param2Snap) {
  if (railSnap == RAIL_NONE) {
    display.setTextSize(1);
    const int bottomY = 56;
    if (param1Snap[0]) {
      drawOutlinedText(5, bottomY, param1Snap);
    }
    if (param2Snap[0]) {
      int16_t x1, y1;
      uint16_t w2, h2;
      display.getTextBounds(param2Snap, 0, 0, &x1, &y1, &w2, &h2);
      drawOutlinedText(128 - w2 - 2, bottomY, param2Snap);
    }
  } else {
    renderVoiceRails();
  }
}

// --- OLED Drawing — main dispatcher ---

void updateDisplay() {
  // Snapshot time first
  uint32_t nowMs;
  noInterrupts();
  nowMs = sysTickMs;
  interrupts();

  // Count-in display — shows "4" "3" "2" "1" countdown while armed
  if (armed && transportState == RUN_EXT) {
    display.clearDisplay();
    display.setFont(NULL);
    display.setTextColor(1);
    display.setTextWrap(false);

    // Calculate beat countdown: 4, 3, 2, 1
    // pulsesPerCycle = numSteps * ppqn / STEPS_PER_BEAT
    // pulsesPerBeat = ppqn
    // pulsesCounted = pulsesPerCycle - armPulseCountdown
    // Snapshot both ISR-written volatiles atomically before arithmetic.
    // armPulseCountdown is ARM-atomic (uint16_t LDRH), but an ISR decrement
    // between a separate ppqn read and the countdown comparison would make
    // pulsesPerCycle - armPulseCountdown inconsistent (off by one beat).
    uint8_t p;
    uint16_t armSnap;
    noInterrupts();
    p       = ppqn;
    armSnap = armPulseCountdown;
    interrupts();
    if (p == 0) return;  // defensive — ppqn should never be 0
    uint16_t pulsesPerCycle = (uint16_t)numSteps * p / STEPS_PER_BEAT;
    uint8_t beatsPerCycle = numSteps / STEPS_PER_BEAT;  // 4

    uint8_t currentBeat = 0;
    if (armSnap < pulsesPerCycle) {
      uint16_t pulsesCounted = pulsesPerCycle - armSnap;
      currentBeat = (pulsesCounted - 1) / p + 1;  // 1-based beat number
    }
    // Countdown: show beatsRemaining = beatsPerCycle - currentBeat + 1
    // But before first pulse (currentBeat=0), show beatsPerCycle
    uint8_t countdown = (currentBeat == 0)
                        ? beatsPerCycle
                        : beatsPerCycle - currentBeat + 1;

    // Large centered countdown number
    display.setTextSize(4);  // ~24px wide per char
    display.setCursor(52, 16);
    display.print(countdown);

    // Chunked push — no timing guard needed during count-in (no steps firing)
    oledStartPushAndStamp(nowMs);
    return;
  }

  // PPQN selection mode — full-screen takeover, skip normal UI
  if (ppqnModeActive) {
    display.clearDisplay();
    display.setFont(NULL);
    display.setTextColor(1);
    display.setTextWrap(false);

    display.setTextSize(2);
    display.setCursor(0, 0);
    display.print("PPQN SELECT");

    display.setTextSize(1);
    display.setCursor(28, 20);
    display.print("TAP TO CYCLE");
    display.setCursor(28, 32);
    display.print("HOLD TO SAVE");

    display.setTextSize(2);
    {
      char ppqnBuf[4];
      snprintf(ppqnBuf, sizeof(ppqnBuf), "%d", ppqnModeSelection);
      display.setCursor(65 - (int)strlen(ppqnBuf) * 6, 48);
      display.print(ppqnBuf);
    }

    if (isSafeToPushOled(nowMs)) {
      oledStartPushAndStamp(nowMs);
    }
    return;
  }

  // Shuffle overlay active within 800 ms of last cycle.
  // Computed early so X-combo overlay can yield to a fresh shuffle toggle.
  // Once the 800 ms timer expires, clear the start tick so a stale shuffle
  // overlay doesn't re-activate every time X is held in the future.
  bool shuffleOverlayActive = false;
  if (shuffleOverlayStartTick) {
    if ((uint32_t)(nowMs - shuffleOverlayStartTick) < SHUFFLE_OVERLAY_DURATION_MS) {
      shuffleOverlayActive = true;
    } else {
      shuffleOverlayStartTick = 0;
    }
  }

  // X-combo overlay flag — rendered after normal UI, before OLED push.
  bool xComboOverlayActive = comboMod.held && !comboMod.comboFired && !shuffleOverlayActive
      && (nowMs - comboMod.pressTick) >= COMBO_OVERLAY_DELAY_MS
      && !ppqnModeActive && !monoBass.active && monoAnimPhase == MONO_ANIM_NONE;

  // Snapshot all state we will render so a single frame is coherent
  float bpmSnap;
  uint8_t trackSnap;
  int chokeSnap;
  uint8_t slotSnap;
  bool playingSnap;
  uint32_t overlayStartSnap;
  uint8_t railSnap;
  char param1Snap[24];
  char param2Snap[24];

  // Snapshot ISR-shared state with interrupts disabled
  noInterrupts();
  playingSnap = sequencePlaying;
  interrupts();

  // bpm is volatile but 32-bit aligned — ARM-atomic read, no guard needed
  bpmSnap = bpm;
  trackSnap = activeTrack;
  chokeSnap = chokeDisplayPercent;
  slotSnap = activeSaveSlot;
  overlayStartSnap = parameterOverlayStartTick;
  railSnap = activeRail;

  strlcpy(param1Snap, displayParameter1, sizeof(param1Snap));
  strlcpy(param2Snap, displayParameter2, sizeof(param2Snap));

  // Overlay state
  bool overlayActiveNow =
    ((uint32_t)(nowMs - overlayStartSnap) < PARAMETER_OVERLAY_DURATION_MS);

  display.clearDisplay();

  // Setup text defaults
  display.setFont(NULL);
  display.setTextColor(1);
  display.setTextSize(1);
  display.setTextWrap(false);

  // Double-thick horizontal divider (y=19..20) and left vertical border (x=0..1)
  display.drawLine(1, 21, 1, 63, 1);
  display.drawLine(1, 20, 127, 20, 1);
  display.drawLine(0, 63, 0, 20, 1);
  display.drawLine(1, 19, 127, 19, 1);

  renderTopBar(bpmSnap, trackSnap, chokeSnap, slotSnap, playingSnap);

  // Scope area: chroma note select > oscilloscope, then MONOBASS overlay on top
  bool noteSelectActive = renderChromaNoteSelect();
  if (!noteSelectActive && !shuffleOverlayActive) {
    drawScopeWaveform(2, 22, SCOPE_DISPLAY_HEIGHT);
  }
  renderMonoBassScope(nowMs);  // outlined overlay on top of scope

  // Advance chroma animation state machine — runs every frame regardless of
  // whether renderChromaDots() is called, so overlays don't stall transitions.
  if (chromaAnimDot >= 0 && chromaAnimPhase != CHROMA_ANIM_NONE) {
    uint32_t animAge = nowMs - chromaAnimStart;
    if (chromaAnimPhase == CHROMA_ANIM_SETTLING && animAge >= CHROMA_SETTLE_MS) {
      cancelChromaAnim();
    } else if (chromaAnimPhase == CHROMA_ANIM_RAMPING && animAge >= CHROMA_COMBO_RAMP_MS) {
      startChromaSettle(nowMs);
    }
  }

  // Big shuffle overlay — centered text size 2.
  if (shuffleOverlayActive) {
    display.setTextSize(2);
    display.setTextColor(1);
    // "SHUFFLE" header — centered
    {
      int16_t x1, y1; uint16_t w, h;
      display.getTextBounds("SHUFFLE", 0, 0, &x1, &y1, &w, &h);
      display.setCursor(64 - w / 2, 24);
      display.print("SHUFFLE");
    }
    // Value line — centered below header
    {
      int16_t x1, y1; uint16_t w, h;
      display.getTextBounds(shuffleOverlayText, 0, 0, &x1, &y1, &w, &h);
      display.setCursor(64 - w / 2, 44);
      display.print(shuffleOverlayText);
    }
    display.setTextSize(1);
  }

  // Suppress small parameter overlay when scope area owns the display.
  // In MONOBASS, allow D1 voice rail through (D1 shape knob is still active).
  if (noteSelectActive) overlayActiveNow = false;
  if (monoBass.active && railSnap != RAIL_D1_SHAPE) overlayActiveNow = false;
  if (shuffleOverlayActive) overlayActiveNow = false;
  if (overlayActiveNow) {
    renderParameterOverlay(railSnap, param1Snap, param2Snap);
  } else if (!shuffleOverlayActive) {
    // Chroma dots share the bottom strip with the parameter overlay —
    // only draw them when the overlay is not visible.
    renderChromaDots(nowMs);
  }

  // MONOBASS white-fill sweep overlay (during combo hold, 1.75s-4s)
  renderMonoTextAnim(nowMs);

  // Fire any steps queued during framebuffer drawing, before the
  // chunked SPI transfer begins.
  if (playingSnap) {
    playSequence();
  }

  // Splash dissolve overlay — runs during first ~1s after boot.
  // ORs surviving splash pixels on top of the normal UI before push.
  // Uses millis() — not sysTickMs — because splashDissolveStartMs is set
  // from millis() in splashAnimation(), which runs before sysTickTimer starts.
  if (splashDissolveActive) {
    uint32_t elapsed = millis() - splashDissolveStartMs;
    if (elapsed >= 1000) {
      splashDissolveActive = false;
    } else {
      applySplashDissolve((float)elapsed / 1000.0f);
    }
  }

  // X-combo overlay — rendered on top of normal UI (not a full-screen takeover).
  // Large outlined "X" over scope + flashing D1/D2/D3/WF chroma status.
  if (xComboOverlayActive) {
    renderXComboOverlay(nowMs);
  }

  if (isSafeToPushOled(nowMs)) {
    // Stamp watchdog only on successful push — stale timestamp causes an
    // immediate retry next iteration, by which time the step has fired.
    oledStartPushAndStamp(nowMs);
  }
}
