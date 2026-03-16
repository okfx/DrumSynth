# A/B Master Audio Settings Test

## What This Is

The **L (LOAD) button** has been temporarily remapped to toggle between two master audio profiles so we can compare them in real time while playing.

- **Profile A** — the original settings (boots into this by default)
- **Profile B** — the proposed optimised settings

Press **L** to switch. The OLED shows `MASTER PROFILE A` or `MASTER PROFILE B`.

> **Combo + L (PPQN mode) still works** — only the standalone L press is remapped.
> **LOAD is disabled** while this code is in place. Do not rely on pattern loading via L.

---

## What Changed — Profile B vs Profile A

### End-Stage Master Filters

| Filter | Parameter | A (Old) | B (New) | Rationale |
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

### SGTL5000 Codec EQ

| Band | Frequency | A (Old) | B (New) | Rationale |
|---|---|---|---|---|
| Band 1 | 115 Hz | +0.25 (~+3 dB) | **+0.50** (~+6 dB) | Double the bass foundation |
| Band 2 | 330 Hz | -0.20 (~-2.4 dB) | **-0.08** (~-1 dB) | Preserve punch/body |
| Band 3 | 990 Hz | -0.10 (~-1.2 dB) | **-0.05** (~-0.6 dB) | Lighter touch, keep mid presence |
| Band 4 | 3 kHz | +0.05 (~+0.6 dB) | +0.05 | No change |
| Band 5 | 9.9 kHz | +0.05 (~+0.6 dB) | **+0.03** (~+0.4 dB) | Slight reduction to balance bass lift |

### SGTL5000 Other Settings

| Setting | A (Old) | B (New) | Rationale |
|---|---|---|---|
| `volume()` | 0.75 | **0.80** | Slight bump for fuller output |
| `dacVolume()` | 0.98 | **0.95** | More headroom with hotter bass |
| `autoVolumeControl` | disabled | **enabled** | Safety limiter for boosted lows |

### SGTL5000 Auto Volume Control (Profile B only)

| Parameter | Value | Rationale |
|---|---|---|
| `maxGain` | 0 (0 dB) | No upward expansion — limiter only |
| `response` | 1 (25 ms) | Fast integration catches bass peaks |
| `hardLimit` | 1 (limiter) | Hard limit for drum transients |
| `threshold` | -3.0 dBFS | Catches boosted bass before clipping |
| `attack` | 150.0 dB/s | Fast attack for percussive material |
| `decay` | 500.0 dB/s | Fast release — won't duck the groove |

---

## Files Modified

| File | Change |
|---|---|
| `audio_init.h` | Added `masterProfileB` flag, `applyMasterSettingsA()`, `applyMasterSettingsB()`, and `toggleMasterProfile()` at the top of the file |
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

Delete the entire A/B block (everything between the `A/B MASTER SETTINGS TOGGLE` comment block and the `audioInit()` function).

### 3. If keeping Profile B

Update the values inside `audioInit()` to match `applyMasterSettingsB()`.

### 4. Delete this file

Remove `AB_TEST.md` from the repo.
