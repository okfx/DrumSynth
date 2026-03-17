# USB Audio on macOS — Clock Drift Fix

## The Problem

DrumSynth outputs USB audio at the Teensy 4.0's native sample rate, which is
**~44,117.647 Hz** (derived from the 600 MHz CPU PLL), not exactly 44,100 Hz.
The USB audio descriptor reports 44100, but the actual clock is ~17.6 Hz faster.

macOS CoreAudio slaves its sample rate to the USB device descriptor, not to the
actual hardware clock. Over time, the fractional offset accumulates in Logic's
(or any DAW's) record buffer:

- **~800 Hz crackling** — continuous resampling noise from the fractional
  rate mismatch; audible after the unit has been on for a few minutes
- **~1-second click at ~8 kHz** — periodic buffer slip when the accumulated
  drift exceeds one sample period

Neither artifact appears on the headphone output. The headphone path is driven
directly by the Teensy's audio DMA — it plays at the true hardware rate with no
resampling in between.

## The Fix: macOS Aggregate Device

Create an Aggregate Device that uses the **Teensy as the clock source**. This
forces CoreAudio to slave to the Teensy's actual hardware clock instead of the
advertised 44100 Hz, eliminating drift.

### Steps

1. Open **Audio MIDI Setup** (`/Applications/Utilities/Audio MIDI Setup.app`).

2. Click **+** in the bottom-left corner → **Create Aggregate Device**.

3. In the right panel, tick the checkbox next to **Teensy Audio** (or whatever
   the Teensy appears as in your USB device list).

4. Tick any other devices you want in the aggregate (built-in output, etc.).

5. In the **Clock Source** column, select **Teensy Audio** (or the Teensy device).
   This is the critical step — it makes CoreAudio lock to the Teensy's actual
   hardware clock.

6. Rename the aggregate device if desired (e.g. "DrumSynth In").

7. In Logic (or your DAW), set the **Audio Device** to the new aggregate device.

### Result

- No crackling, no periodic clicks, regardless of session length.
- The aggregate device automatically compensates for the ~17.6 Hz offset by
  running CoreAudio at the Teensy's true rate.

## Why Not Fix It in Firmware?

The PJRC USB audio implementation does not expose an isochronous feedback
endpoint. There is no way to signal the actual hardware sample rate to the host
over USB from firmware. The aggregate device approach is the correct system-level
fix and requires no firmware changes.

## Notes

- This is a one-time setup per Mac. The aggregate device persists across reboots.
- If the Teensy is disconnected and reconnected, CoreAudio may need a moment to
  re-lock. Stop and restart recording if you notice drift resuming after a
  reconnect.
- The headphone output is unaffected and always clean — use it as a reference
  if you suspect drift has returned.
