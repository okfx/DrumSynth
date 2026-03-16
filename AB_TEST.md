# A/B/C Master Audio Settings Test

## What This Is

The **L (LOAD) button** has been temporarily remapped to cycle between three master audio profiles so we can compare them in real time while playing.

- **Profile A** — the original settings (boots into this by default)
- **Profile B** — proposed optimised filters/EQ + fast AVC limiter
- **Profile C** — same filters/EQ as B + alternate AVC tuning (slower, deeper threshold)

Press **L** to cycle: A → B → C → A. The OLED shows `MASTER PROFILE A`, `B`, or `C`.

> **Combo + L (PPQN mode) still works** — only the standalone L press is remapped.
> **LOAD is disabled** while this code is in place. Do not rely on pattern loading via L.

---

## What Changed — Profiles B & C vs Profile A

### End-Stage Master Filters (B & C identical)

| Filter | Parameter | A (Old) | B / C (New) | Rationale |
|---|---|---|---|---|
| `masterHighPass` | frequency | 60 Hz | **30 Hz** | Preserve sub-bass energy from kicks |
| `masterHighPass` | resonance | 1.5 | **0.707** | Flat Butterworth — no resonant peak |
| `masterLowPass` | frequency | 9000 Hz | **12000 Hz** | More headroom up top |
| `masterLowPass` | resonance | 0.25 | **0.3** | Marginal — keeps it smooth |
| `masterBandPass` | frequency | 1000 Hz | 1000 Hz | Unchanged |
| `masterBandPass` | resonance | 1.0 | **0.01** | Effectively bypassed (ultra-wide Q) |
| `finalFilter` stage 0 | Low Shelf 150 Hz | +2.0 dB, Q 0.7 | **+3.5 dB, Q 0.6** | Stronger warmth, wider shelf |
| `finalFilter` stage 1 | Notch | 350 Hz, Q 1.7 | **400 Hz, Q 1.0** | Gentler mud cut |
| `finalFilter` stage 2 | Notch | 2000 Hz, Q 3.0 | **2500 Hz, Q 2.0** | Gentler cut, less thinning |
| `finalFilter` stage 3 | High Shelf 4500 Hz | +3.5 dB, Q 0.7 | **+2.5 dB, Q 0.7** | Less bright/thin tilt |
| `finalAmp` | gain | 3.5 | **2.8** | Less needed with fuller spectrum |

### SGTL5000 Codec EQ (B & C identical)

| Band | Frequency | A (Old) | B / C (New) | Rationale |
|---|---|---|---|---|
| Band 1 | 115 Hz | +0.25 (~+3 dB) | **+0.50** (~+6 dB) | Double the bass foundation |
| Band 2 | 330 Hz | -0.20 (~-2.4 dB) | **-0.08** (~-1 dB) | Preserve punch/body |
| Band 3 | 990 Hz | -0.10 (~-1.2 dB) | **-0.05** (~-0.6 dB) | Lighter touch, keep mid presence |
| Band 4 | 3 kHz | +0.05 (~+0.6 dB) | +0.05 | No change |
| Band 5 | 9.9 kHz | +0.05 (~+0.6 dB) | **+0.03** (~+0.4 dB) | Slight reduction to balance bass lift |

### SGTL5000 Other Settings (B & C identical)

| Setting | A (Old) | B / C (New) | Rationale |
|---|---|---|---|
| `volume()` | 0.75 | **0.80** | Slight bump for fuller output |
| `dacVolume()` | 0.98 | **0.95** | More headroom with hotter bass |
| `autoVolumeControl` | disabled | **enabled** | Safety limiter for boosted lows |

### SGTL5000 Auto Volume Control — B vs C

| Parameter | A | B | C | Notes |
|---|---|---|---|---|
| `maxGain` | — | 0 (0 dB) | 0 (0 dB) | Both: no upward expansion |
| `response` | — | 1 (25 ms) | 2 (50 ms) | C slower integration |
| `hardLimit` | — | 1 (limiter) | 1 (limiter) | Both: hard limit |
| `threshold` | — | -3.0 dBFS | **-4.5 dBFS** | C catches earlier |
| `attack` | — | 150.0 dB/s | **30.0 dB/s** | C much slower attack |
| `decay` | — | 500.0 dB/s | **2.0 dB/s** | C much slower release |

**B** = fast, transparent limiter (quick catch-and-release for transients).
**C** = slow, heavy-handed limiter (deep threshold, sluggish attack/decay — more audible compression character).

---

## Files Modified

| File | Change |
|---|---|
| `audio_init.h` | Added `masterProfile` counter, `applyMasterSettingsA/B/C()`, and `toggleMasterProfile()` at the top of the file |
| `buttons.h` | Replaced `btnLoadPress` body — standalone L press now calls `toggleMasterProfile()` instead of loading a pattern. Combo+L (PPQN) is untouched |

---

## How to Revert

Once you've decided on a profile, revert the L button and bake the winner into `audioInit()`:

### 1. Revert `buttons.h`

Replace the current `btnLoadPress` with the original:

```cpp
static void btnLoadPress(ButtonHandler& self, uint32_t nowTick) {
  self.pressTick = nowTick;
  if (comboMod.held) {
    if (monoBass.active) { showMonoBassDisabled(nowTick); }
    else { dispatchCombo(8, nowTick); }
    return;
  }
  if (monoBass.active) { showMonoBassDisabled(nowTick); return; }
  slotPending = false;
  if (!loadStateFromEEPROM(activeSaveSlot)) {
    clearPatternState();
    updateLEDs();
    patternDirty = false;
    activeRail = RAIL_NONE;
    snprintf(displayParameter1, sizeof(displayParameter1), "SLOT %d",
             activeSaveSlot + 1);
    snprintf(displayParameter2, sizeof(displayParameter2), "EMPTY");
    parameterOverlayStartTick = nowTick;
  }
  flashStepLed(activeSaveSlot, 2);
}
```

### 2. Revert `audio_init.h`

Delete the entire A/B/C block (everything between the `A/B/C MASTER SETTINGS TOGGLE` comment block and the `audioInit()` function).

### 3. If keeping Profile B or C

Update the values inside `audioInit()` to match `applyMasterSettingsB()` or `applyMasterSettingsC()`.

### 4. Delete this file

Remove `AB_TEST.md` from the repo.
