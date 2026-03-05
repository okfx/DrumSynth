#!/usr/bin/env python3
"""
TR-606 Drum Machine — a Roland TR-606 inspired drum machine for macOS.

Usage:
  Double-click or run from Terminal:
    python3 TR606.py

Controls:
  Space           Start / Stop
  1-7             Solo instrument (BD SD LT HT CH OH CY)
  Click grid      Toggle steps on/off
  Escape          Quit

Requirements: Python 3 with tkinter (included with macOS).
"""

import tkinter as tk
import math
import struct
import wave
import os
import tempfile
import subprocess
import threading
import time
import random

# ─── Audio ────────────────────────────────────────────────────────────────────
SAMPLE_RATE = 44100

# ─── DSP Helpers ──────────────────────────────────────────────────────────────

def _clamp(v):
    return max(-1.0, min(1.0, v))


class OnePoleLP:
    """One-pole low-pass filter."""
    __slots__ = ("a", "z")

    def __init__(self, freq, sr=SAMPLE_RATE):
        rc = 1.0 / (2.0 * math.pi * freq)
        dt = 1.0 / sr
        self.a = dt / (rc + dt)
        self.z = 0.0

    def tick(self, x):
        self.z += self.a * (x - self.z)
        return self.z


class OnePoleHP:
    """One-pole high-pass filter."""
    __slots__ = ("a", "xi", "z")

    def __init__(self, freq, sr=SAMPLE_RATE):
        rc = 1.0 / (2.0 * math.pi * freq)
        dt = 1.0 / sr
        self.a = rc / (rc + dt)
        self.xi = 0.0
        self.z = 0.0

    def tick(self, x):
        self.z = self.a * (self.z + x - self.xi)
        self.xi = x
        return self.z


class BandpassSVF:
    """State-variable bandpass filter (12 dB/oct)."""
    __slots__ = ("f", "q", "lp", "bp", "hp")

    def __init__(self, freq, q=1.0, sr=SAMPLE_RATE):
        self.f = 2.0 * math.sin(math.pi * min(freq, sr * 0.45) / sr)
        self.q = max(0.5, q)
        self.lp = 0.0
        self.bp = 0.0
        self.hp = 0.0

    def tick(self, x):
        self.lp += self.f * self.bp
        self.hp = x - self.lp - self.bp / self.q
        self.bp += self.f * self.hp
        return self.bp


# ─── TR-606 Drum Synthesizers ────────────────────────────────────────────────
#
# Based on analysis of the original 606 circuits:
#   - BD: Twin-T resonator with two oscillators at ~60 Hz (Q=7) and ~130 Hz (Q=3)
#   - SD: Twin-T resonator body (~200 Hz) + white noise through HP filter
#   - Toms: Twin-T damped oscillators + low-passed noise tail
#   - HH/CY: 6 Schmitt-trigger square oscillators at inharmonic ratios,
#     split through two bridged-T bandpass filters at 3440 Hz and 7100 Hz
#
# Sources:
#   Roland TR-606 service notes; Baratatronix 606 cymbal/hi-hat analysis;
#   David Haillant 606BD clone documentation

def synth_bass_drum(tone=0.5, decay=0.5):
    """TR-606 BD: dual Twin-T resonators at 60 Hz and 130 Hz."""
    dur = 0.30 + decay * 0.50
    n = int(SAMPLE_RATE * dur)

    # OSC 1: ~60 Hz, Q=7 (longer ring, dominant body)
    osc1_hz = 55 + tone * 15          # 55-70 Hz range
    osc1_q = 7.0
    osc1_decay = osc1_q / (math.pi * osc1_hz)  # ring time from Q

    # OSC 2: ~130 Hz, Q=3 (short punch, adds click/attack)
    osc2_hz = 120 + tone * 25         # 120-145 Hz
    osc2_decay = 3.0 / (math.pi * osc2_hz)

    phase1, phase2 = 0.0, 0.0
    samples = []
    for i in range(n):
        t = i / SAMPLE_RATE

        # Both oscillators are damped sinusoids (Twin-T behavior)
        osc1 = math.sin(phase1) * math.exp(-t / osc1_decay)
        osc2 = math.sin(phase2) * math.exp(-t / osc2_decay) * 0.45

        phase1 += 2.0 * math.pi * osc1_hz / SAMPLE_RATE
        phase2 += 2.0 * math.pi * osc2_hz / SAMPLE_RATE

        # The trigger pulse creates a sharp initial transient
        trigger = math.exp(-t * 300) * 0.35

        # Overall VCA envelope
        vca = math.exp(-t * (3.0 + (1.0 - decay) * 8.0))

        out = (osc1 + osc2 + trigger) * vca
        samples.append(_clamp(out * 0.95))
    return samples


def synth_snare(tone=0.5, snappy=0.5):
    """TR-606 SD: Twin-T resonator body (~200 Hz) + HP-filtered white noise."""
    dur = 0.30
    n = int(SAMPLE_RATE * dur)

    # Body: Twin-T resonator at ~200 Hz (damped sine)
    body_hz = 190 + tone * 40
    body_q = 5.0
    body_decay = body_q / (math.pi * body_hz)

    # Noise path: white noise through high-pass (~1 kHz) characteristic of 606
    hp1 = OnePoleHP(1000)
    hp2 = OnePoleHP(1200)  # Second stage for steeper rolloff

    phase = 0.0
    samples = []
    for i in range(n):
        t = i / SAMPLE_RATE

        # Body oscillator (damped Twin-T sine)
        body = math.sin(phase) * math.exp(-t / body_decay) * (0.7 - snappy * 0.25)
        phase += 2.0 * math.pi * body_hz / SAMPLE_RATE

        # White noise through HP filter chain
        noise = random.random() * 2.0 - 1.0
        noise = hp1.tick(noise)
        noise = hp2.tick(noise)
        noise *= math.exp(-t * 16) * (0.5 + snappy * 0.4)

        # Sharp trigger transient
        trigger = math.exp(-t * 400) * 0.3

        out = body + noise + trigger
        samples.append(_clamp(out * 0.85))
    return samples


def synth_low_tom(tune=0.5):
    """TR-606 LT: Twin-T damped oscillator + low-passed noise reverb tail."""
    dur = 0.35
    n = int(SAMPLE_RATE * dur)

    # Tom body: damped sine, slight pitch drop (not as extreme as 808)
    base_hz = 100 + tune * 30
    start_hz = base_hz * 1.6  # Subtle pitch drop
    tom_q = 6.0

    # Low-passed noise for body/reverb character
    lp_noise = OnePoleLP(800)

    phase = 0.0
    samples = []
    for i in range(n):
        t = i / SAMPLE_RATE

        # Pitch drops quickly then settles
        freq = base_hz + (start_hz - base_hz) * math.exp(-t * 50)
        phase += 2.0 * math.pi * freq / SAMPLE_RATE

        body_decay = tom_q / (math.pi * base_hz)
        body = math.sin(phase) * math.exp(-t / body_decay)

        # Noise tail (simulates room/body resonance)
        noise = (random.random() * 2.0 - 1.0)
        noise = lp_noise.tick(noise) * math.exp(-t * 12) * 0.12

        trigger = math.exp(-t * 250) * 0.2

        out = body + noise + trigger
        samples.append(_clamp(out * 0.85))
    return samples


def synth_high_tom(tune=0.5):
    """TR-606 HT: same topology as LT, higher pitch."""
    dur = 0.28
    n = int(SAMPLE_RATE * dur)

    base_hz = 160 + tune * 40
    start_hz = base_hz * 1.5
    tom_q = 5.0

    lp_noise = OnePoleLP(1200)

    phase = 0.0
    samples = []
    for i in range(n):
        t = i / SAMPLE_RATE
        freq = base_hz + (start_hz - base_hz) * math.exp(-t * 55)
        phase += 2.0 * math.pi * freq / SAMPLE_RATE
        body_decay = tom_q / (math.pi * base_hz)
        body = math.sin(phase) * math.exp(-t / body_decay)
        noise = (random.random() * 2.0 - 1.0)
        noise = lp_noise.tick(noise) * math.exp(-t * 14) * 0.10
        trigger = math.exp(-t * 250) * 0.2
        out = body + noise + trigger
        samples.append(_clamp(out * 0.82))
    return samples


def _metallic_osc_606(n):
    """TR-606 metallic oscillator: 6 Schmitt-trigger square oscillators
    at inharmonic frequencies, split through two bridged-T bandpass filters
    centred at 3440 Hz and 7100 Hz (same centres as the TR-808)."""

    # 606 Schmitt-trigger oscillator frequencies (inharmonic ratios)
    # These create the characteristic metallic shimmer
    freqs = [206.0, 245.0, 304.0, 370.0, 522.0, 640.0]

    phases = [random.random() * 2.0 * math.pi for _ in range(6)]  # random start phase

    # Two bridged-T bandpass filters at the documented centre frequencies
    bp_lo = BandpassSVF(3440, q=2.5)   # Lower metallic body
    bp_hi = BandpassSVF(7100, q=3.0)   # Upper shimmer

    buf = []
    for i in range(n):
        mix = 0.0
        for j in range(6):
            phases[j] += 2.0 * math.pi * freqs[j] / SAMPLE_RATE
            # Square wave via sign of sine (Schmitt trigger)
            mix += (1.0 if math.sin(phases[j]) >= 0 else -1.0)
        mix /= 6.0

        # Split through both bandpass filters and recombine
        lo = bp_lo.tick(mix) * 0.6
        hi = bp_hi.tick(mix) * 0.4
        buf.append(lo + hi)
    return buf


def synth_closed_hat(tone=0.5):
    """TR-606 CH: metallic oscillators, very short VCA decay (~30-80 ms)."""
    dur = 0.06 + tone * 0.05
    n = int(SAMPLE_RATE * dur)
    raw = _metallic_osc_606(n)
    samples = []
    for i, s in enumerate(raw):
        t = i / SAMPLE_RATE
        # Very fast exponential decay (characteristic 606 CH snap)
        amp = math.exp(-t * (50 + (1 - tone) * 30))
        samples.append(_clamp(s * amp * 0.8))
    return samples


def synth_open_hat(tone=0.5, decay=0.5):
    """TR-606 OH: metallic oscillators, longer VCA decay (~150-400 ms)."""
    dur = 0.20 + decay * 0.30
    n = int(SAMPLE_RATE * dur)
    raw = _metallic_osc_606(n)
    samples = []
    for i, s in enumerate(raw):
        t = i / SAMPLE_RATE
        amp = math.exp(-t * (5 + (1 - decay) * 10))
        samples.append(_clamp(s * amp * 0.75))
    return samples


def synth_cymbal(tone=0.5, decay=0.5):
    """TR-606 CY: metallic oscillators (same source as HH),
    through lower bandpass emphasis, long VCA decay."""
    dur = 0.6 + decay * 1.2
    n = int(SAMPLE_RATE * dur)
    raw = _metallic_osc_606(n)
    # Extra shimmer: add filtered noise for body
    lp = OnePoleLP(8000)
    samples = []
    for i, s in enumerate(raw):
        t = i / SAMPLE_RATE
        noise = (random.random() * 2.0 - 1.0) * 0.12
        noise = lp.tick(noise)
        amp = math.exp(-t * (1.8 + (1 - decay) * 3))
        samples.append(_clamp((s * 0.75 + noise) * amp * 0.6))
    return samples


# ─── WAV Utility ──────────────────────────────────────────────────────────────

def write_wav(path, samples):
    """Write float samples [-1,1] as 16-bit mono WAV."""
    with wave.open(path, "w") as wf:
        wf.setnchannels(1)
        wf.setsampwidth(2)
        wf.setframerate(SAMPLE_RATE)
        data = b"".join(struct.pack("<h", int(s * 32767)) for s in samples)
        wf.writeframes(data)


# ─── Audio Player ─────────────────────────────────────────────────────────────

class AudioPlayer:
    """Plays WAV files via afplay subprocess pool.

    Pre-spawns nothing — each play() fires a short-lived afplay process
    on a daemon thread.  On macOS this gives ~20-40 ms latency which is
    acceptable for an interactive drum machine demo.
    """

    def __init__(self):
        self._sounds = {}        # name -> filepath
        self._procs = {}         # name -> most-recent Popen (for choke/kill)

    def load(self, name, path):
        self._sounds[name] = path

    def play(self, name):
        path = self._sounds.get(name)
        if not path:
            return
        threading.Thread(target=self._play_af, args=(name, path),
                         daemon=True).start()

    def stop(self, name):
        """Kill the most recent playback of *name* (used for hi-hat choke)."""
        proc = self._procs.get(name)
        if proc and proc.poll() is None:
            try:
                proc.terminate()
            except OSError:
                pass

    def _play_af(self, name, path):
        proc = subprocess.Popen(
            ["afplay", path],
            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        self._procs[name] = proc
        proc.wait()


# ─── Sound Bank ───────────────────────────────────────────────────────────────

INSTRUMENTS = [
    ("BD", "Bass Drum"),
    ("SD", "Snare"),
    ("LT", "Low Tom"),
    ("HT", "High Tom"),
    ("CH", "Closed Hat"),
    ("OH", "Open Hat"),
    ("CY", "Cymbal"),
]

INST_COLOURS = {
    "BD": "#FF6B35",
    "SD": "#F7C948",
    "LT": "#45B7D1",
    "HT": "#96E6A1",
    "CH": "#DDA0DD",
    "OH": "#C77DFF",
    "CY": "#FF69B4",
    "AC": "#FF4444",
}


def build_sound_bank(tmp_dir):
    """Synthesize all drum sounds and write WAV files. Returns {name: path}."""
    bank = {}
    synths = {
        "BD":  lambda: synth_bass_drum(0.5, 0.5),
        "BD+": lambda: [s * 1.3 for s in synth_bass_drum(0.6, 0.6)],
        "SD":  lambda: synth_snare(0.5, 0.5),
        "SD+": lambda: [s * 1.25 for s in synth_snare(0.5, 0.7)],
        "LT":  lambda: synth_low_tom(0.5),
        "LT+": lambda: [s * 1.2 for s in synth_low_tom(0.5)],
        "HT":  lambda: synth_high_tom(0.5),
        "HT+": lambda: [s * 1.2 for s in synth_high_tom(0.5)],
        "CH":  lambda: synth_closed_hat(0.5),
        "CH+": lambda: [s * 1.15 for s in synth_closed_hat(0.5)],
        "OH":  lambda: synth_open_hat(0.5, 0.5),
        "OH+": lambda: [s * 1.2 for s in synth_open_hat(0.5, 0.6)],
        "CY":  lambda: synth_cymbal(0.5, 0.5),
        "CY+": lambda: [s * 1.15 for s in synth_cymbal(0.5, 0.6)],
    }
    for name, fn in synths.items():
        path = os.path.join(tmp_dir, f"{name}.wav")
        samples = [_clamp(s) for s in fn()]
        write_wav(path, samples)
        bank[name] = path
    return bank


# ─── 606 Hardware Colour Palette ──────────────────────────────────────────────
SILVER         = "#C2BAB0"      # Main body
SILVER_LIGHT   = "#D4CCC2"      # Highlights
SILVER_DARK    = "#A89E91"      # Edges / grooves
DARK_STRIP     = "#2D2D2C"      # Header / footer panels
DARK_MID       = "#3A3A38"      # Slightly lighter dark
CREAM          = "#D6CFC6"      # Light inset areas
BTN_OFF        = "#444444"      # Step button off
BTN_ON         = "#E53935"      # Step button on (Roland red)
BTN_ACCENT     = "#FF8F00"      # Accent on (amber)
KNOB_BODY      = "#1A1A1A"      # Knob face
KNOB_RING      = "#333333"      # Knob outer ring
KNOB_IND       = "#FFFFFF"      # Knob indicator
LED_ON         = "#FF2200"      # Active LED
LED_OFF        = "#3A1515"      # Inactive LED
LED_GREEN      = "#22DD44"      # Playback LED
TEXT_ON_SILVER = "#2D2D2C"      # Dark text on silver
TEXT_ON_DARK   = "#D8D4D0"      # Light text on dark
ROLAND_RED     = "#E53935"      # Brand colour
GROOVE_COL     = "#B0A899"      # Panel grooves / lines

# Layout
KNOB_R         = 16             # Knob radius
CELL_W         = 36             # Step cell width
CELL_H         = 28             # Step cell height
CELL_PAD       = 4              # Gap between cells
GRID_COLS      = 16
GRID_ROWS      = 8              # 7 instruments + accent

# Canvas dimensions
W = 780
H = 590


# ─── Main Application ────────────────────────────────────────────────────────

class TR606(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("TR-606 DRUMATIX")
        self.configure(bg=SILVER_DARK)
        self.resizable(False, False)

        # ── State ──
        self.bpm = 130
        self.playing = False
        self.current_step = 0
        self.swing = 0

        self.pattern = [[0] * 16 for _ in range(8)]
        self.levels = [0.8] * 7  # per instrument

        self.next_tick = 0.0
        self._after_id = None

        # Knob interaction
        self._knobs = []          # list of knob dicts
        self._dragging_knob = None
        self._drag_start_y = 0
        self._drag_start_val = 0.0
        self._hover_cell = (-1, -1)

        # ── Synthesize sounds ──
        self._tmp = tempfile.mkdtemp(prefix="tr606_")
        self._bank = build_sound_bank(self._tmp)
        self._player = AudioPlayer()
        for name, path in self._bank.items():
            self._player.load(name, path)

        # ── Canvas ──
        self.canvas = tk.Canvas(self, width=W, height=H,
                                bg=SILVER, highlightthickness=0)
        self.canvas.pack(padx=4, pady=4)

        self.canvas.bind("<Button-1>", self._on_press)
        self.canvas.bind("<B1-Motion>", self._on_drag)
        self.canvas.bind("<ButtonRelease-1>", self._on_release)
        self.canvas.bind("<Motion>", self._on_hover)

        self._bind_keys()
        self._setup_knobs()
        self._draw_all()

        # Centre on screen
        self.update_idletasks()
        sw, sh = self.winfo_screenwidth(), self.winfo_screenheight()
        ww, wh = self.winfo_width(), self.winfo_height()
        self.geometry(f"+{(sw-ww)//2}+{(sh-wh)//3}")

    # ── Knob Setup ────────────────────────────────────────────────────────
    def _setup_knobs(self):
        self._knobs.clear()
        # Tempo knob
        self._knobs.append({
            "cx": 105, "cy": 60, "r": KNOB_R + 4,
            "value": (self.bpm - 60) / 180.0,
            "label": "TEMPO", "fmt": lambda v: f"{int(60 + v * 180)}",
            "callback": self._knob_tempo,
        })
        # Swing knob
        self._knobs.append({
            "cx": 185, "cy": 60, "r": KNOB_R,
            "value": 0.0,
            "label": "SWING", "fmt": lambda v: f"{int(v * 50)}%",
            "callback": self._knob_swing,
        })
        # Instrument level knobs
        x_start = 310
        for i, (abbr, name) in enumerate(INSTRUMENTS):
            self._knobs.append({
                "cx": x_start + i * 62, "cy": 60, "r": KNOB_R,
                "value": self.levels[i],
                "label": abbr, "fmt": lambda v: f"{int(v * 100)}",
                "callback": lambda v, idx=i: self._knob_level(idx, v),
            })

    def _knob_tempo(self, v):
        self.bpm = int(60 + v * 180)

    def _knob_swing(self, v):
        self.swing = int(v * 50)

    def _knob_level(self, idx, v):
        self.levels[idx] = v

    # ── Full Draw ─────────────────────────────────────────────────────────
    def _draw_all(self):
        c = self.canvas
        c.delete("all")

        # ── Silver body background ──
        c.create_rectangle(0, 0, W, H, fill=SILVER, outline="")

        # Subtle edge bevel
        c.create_line(0, 0, W, 0, fill=SILVER_LIGHT, width=2)
        c.create_line(0, 0, 0, H, fill=SILVER_LIGHT, width=2)
        c.create_line(W, 0, W, H, fill=SILVER_DARK, width=2)
        c.create_line(0, H, W, H, fill=SILVER_DARK, width=2)

        # ── Dark header strip ──
        c.create_rectangle(8, 8, W - 8, 36, fill=DARK_STRIP, outline=DARK_MID)

        # Roland branding
        c.create_text(20, 22, text="R O L A N D", anchor="w",
                      font=("Helvetica", 11, "bold"), fill=TEXT_ON_DARK)

        c.create_text(170, 22, text="COMPUTER CONTROLLED",
                      anchor="w", font=("Helvetica", 7), fill="#888884")

        c.create_text(W // 2 + 40, 22, text="TR-606",
                      anchor="w", font=("Helvetica", 14, "bold"), fill=ROLAND_RED)

        c.create_text(W // 2 + 148, 22, text="DRUMATIX",
                      anchor="w", font=("Helvetica", 10), fill=TEXT_ON_DARK)

        # ── Decorative screws ──
        for sx, sy in [(18, 100), (W - 18, 100), (18, H - 18), (W - 18, H - 18)]:
            c.create_oval(sx - 5, sy - 5, sx + 5, sy + 5,
                          fill=SILVER_DARK, outline="#AAA49A")
            c.create_line(sx - 3, sy, sx + 3, sy, fill="#999490", width=1)

        # ── Groove lines on silver body ──
        c.create_line(8, 96, W - 8, 96, fill=GROOVE_COL, width=1)
        c.create_line(8, 98, W - 8, 98, fill=SILVER_LIGHT, width=1)

        # ── Knob section (below header) ──
        c.create_rectangle(12, 38, W - 12, 95, fill=SILVER, outline="")

        # Section labels
        c.create_text(105, 42, text="TEMPO / SWING",
                      font=("Helvetica", 7), fill=TEXT_ON_SILVER)
        c.create_text(500, 42, text="INSTRUMENT MIX",
                      font=("Helvetica", 7), fill=TEXT_ON_SILVER)

        # Draw all knobs
        for knob in self._knobs:
            self._draw_knob(c, knob)

        # ── Transport button ──
        tx, ty = 240, 60
        playing = self.playing
        fill = "#882222" if playing else "#226622"
        txt = "STOP" if playing else "RUN"
        c.create_oval(tx - 18, ty - 18, tx + 18, ty + 18,
                      fill=fill, outline="#111111", width=2)
        c.create_text(tx, ty, text=txt, font=("Helvetica", 8, "bold"),
                      fill="#FFFFFF")
        # Store transport button location
        self._transport_btn = (tx, ty, 18)

        # Run indicator LED
        led_col = LED_GREEN if playing else LED_OFF
        c.create_oval(tx - 3, ty - 28, tx + 3, ty - 22,
                      fill=led_col, outline="#111")

        # ── Step grid ──
        self._draw_grid(c)

        # ── Dark footer strip ──
        fy = H - 38
        c.create_rectangle(8, fy, W - 8, H - 8, fill=DARK_STRIP, outline=DARK_MID)

        # Footer text
        c.create_text(20, fy + 15, text="SPACE: Run/Stop    1-7: Preview    Click: Program    R: Random    C: Clear",
                      anchor="w", font=("Helvetica", 9), fill="#888884")

        # BPM display
        bpm_txt = f"{self.bpm} BPM"
        if self.swing > 0:
            bpm_txt += f"  Swing {self.swing}%"
        c.create_text(W - 20, fy + 15, text=bpm_txt,
                      anchor="e", font=("Helvetica", 10, "bold"),
                      fill=ROLAND_RED if playing else "#888884")

    # ── Draw a single knob ────────────────────────────────────────────────
    def _draw_knob(self, c, knob):
        cx, cy, r = knob["cx"], knob["cy"], knob["r"]
        val = knob["value"]

        # Outer ring (shadow)
        c.create_oval(cx - r - 2, cy - r - 2, cx + r + 2, cy + r + 2,
                      fill=SILVER_DARK, outline="")
        # Knob body
        c.create_oval(cx - r, cy - r, cx + r, cy + r,
                      fill=KNOB_BODY, outline=KNOB_RING, width=2)
        # Inner ring (metallic sheen)
        ir = r - 4
        c.create_oval(cx - ir, cy - ir, cx + ir, cy + ir,
                      fill="#222222", outline="#2A2A2A", width=1)

        # Indicator line: angle from 225° (min) sweeping 270° clockwise
        angle_deg = 225 - val * 270
        angle_rad = math.radians(angle_deg)
        lx = cx + (r - 4) * math.cos(angle_rad)
        ly = cy - (r - 4) * math.sin(angle_rad)
        c.create_line(cx, cy, lx, ly, fill=KNOB_IND, width=2, capstyle="round")

        # Label below
        c.create_text(cx, cy + r + 12, text=knob["label"],
                      font=("Helvetica", 8, "bold"), fill=TEXT_ON_SILVER)
        # Value below label
        c.create_text(cx, cy + r + 22, text=knob["fmt"](val),
                      font=("Helvetica", 7), fill=SILVER_DARK)

    # ── Draw step grid ────────────────────────────────────────────────────
    def _draw_grid(self, c):
        grid_x0 = 100
        grid_y0 = 112

        rows = list(INSTRUMENTS) + [("AC", "Accent")]

        for row_i, (abbr, full_name) in enumerate(rows):
            y = grid_y0 + row_i * (CELL_H + CELL_PAD)
            is_accent = (row_i == 7)

            # Instrument label
            label_col = "#AA4400" if is_accent else TEXT_ON_SILVER
            c.create_text(grid_x0 - 8, y + CELL_H / 2,
                          text=abbr, anchor="e",
                          font=("Helvetica", 10, "bold"), fill=label_col)

            # Full name (smaller, lighter)
            if not is_accent:
                c.create_text(grid_x0 - 50, y + CELL_H / 2,
                              text=full_name, anchor="e",
                              font=("Helvetica", 7), fill=SILVER_DARK)

            for col in range(16):
                x = grid_x0 + col * (CELL_W + CELL_PAD)
                on = self.pattern[row_i][col]

                # Group background (every 4 steps)
                if col % 4 == 0 and row_i == 0:
                    gx = x - 1
                    gw = 4 * (CELL_W + CELL_PAD) - CELL_PAD + 2
                    gy = grid_y0 - 4
                    gh = GRID_ROWS * (CELL_H + CELL_PAD) + 2
                    shade = CREAM if (col // 4) % 2 == 0 else SILVER_LIGHT
                    c.create_rectangle(gx, gy, gx + gw, gy + gh,
                                       fill=shade, outline="")

                # Step button
                if on:
                    fill = BTN_ACCENT if is_accent else BTN_ON
                    outline = "#FF6644" if is_accent else "#FF5555"
                else:
                    is_hover = (row_i, col) == self._hover_cell
                    fill = "#555555" if is_hover else BTN_OFF
                    outline = "#555555"

                # Button shape (raised 3D look)
                c.create_rectangle(x, y, x + CELL_W, y + CELL_H,
                                   fill=fill, outline=outline, width=1)
                # Top/left highlight (3D bevel)
                if on:
                    c.create_line(x + 1, y + 1, x + CELL_W - 1, y + 1,
                                  fill="#FF8866" if is_accent else "#FF7777", width=1)
                    c.create_line(x + 1, y + 1, x + 1, y + CELL_H - 1,
                                  fill="#FF8866" if is_accent else "#FF7777", width=1)
                else:
                    c.create_line(x + 1, y + 1, x + CELL_W - 1, y + 1,
                                  fill="#666666", width=1)
                    c.create_line(x + 1, y + 1, x + 1, y + CELL_H - 1,
                                  fill="#666666", width=1)

        # ── Step position LEDs ──
        led_y = grid_y0 - 12
        for col in range(16):
            x = grid_x0 + col * (CELL_W + CELL_PAD) + CELL_W / 2
            if col == self.current_step and self.playing:
                c.create_oval(x - 4, led_y - 4, x + 4, led_y + 4,
                              fill=LED_ON, outline="#FF4422")
                # Glow effect
                c.create_oval(x - 7, led_y - 7, x + 7, led_y + 7,
                              fill="", outline="#662200", width=1)
            else:
                c.create_oval(x - 3, led_y - 3, x + 3, led_y + 3,
                              fill=LED_OFF, outline="#2A1111")

        # Step numbers below grid
        num_y = grid_y0 + GRID_ROWS * (CELL_H + CELL_PAD) + 6
        for col in range(16):
            x = grid_x0 + col * (CELL_W + CELL_PAD) + CELL_W / 2
            c.create_text(x, num_y, text=str(col + 1),
                          font=("Helvetica", 8),
                          fill=ROLAND_RED if col % 4 == 0 else TEXT_ON_SILVER)

        # Separator line above grid
        c.create_line(grid_x0 - 2, grid_y0 - 20, grid_x0 + 16 * (CELL_W + CELL_PAD),
                      grid_y0 - 20, fill=GROOVE_COL, width=1)

    # ── Hit Testing ───────────────────────────────────────────────────────
    def _grid_cell_at(self, mx, my):
        grid_x0 = 100
        grid_y0 = 112
        col = int((mx - grid_x0) / (CELL_W + CELL_PAD))
        row = int((my - grid_y0) / (CELL_H + CELL_PAD))
        if 0 <= col < 16 and 0 <= row < GRID_ROWS:
            cx = grid_x0 + col * (CELL_W + CELL_PAD)
            cy = grid_y0 + row * (CELL_H + CELL_PAD)
            if cx <= mx <= cx + CELL_W and cy <= my <= cy + CELL_H:
                return row, col
        return -1, -1

    def _knob_at(self, mx, my):
        for knob in self._knobs:
            dx = mx - knob["cx"]
            dy = my - knob["cy"]
            if dx * dx + dy * dy <= (knob["r"] + 4) ** 2:
                return knob
        return None

    # ── Mouse Events ──────────────────────────────────────────────────────
    def _on_press(self, event):
        mx, my = event.x, event.y

        # Check knobs
        knob = self._knob_at(mx, my)
        if knob:
            self._dragging_knob = knob
            self._drag_start_y = my
            self._drag_start_val = knob["value"]
            return

        # Check transport button
        tx, ty, tr = self._transport_btn
        if (mx - tx) ** 2 + (my - ty) ** 2 <= tr ** 2:
            self._toggle_play()
            return

        # Check grid
        row, col = self._grid_cell_at(mx, my)
        if row >= 0:
            self.pattern[row][col] = 1 - self.pattern[row][col]
            self._draw_all()

    def _on_drag(self, event):
        if self._dragging_knob:
            dy = self._drag_start_y - event.y
            new_val = max(0.0, min(1.0, self._drag_start_val + dy * 0.006))
            self._dragging_knob["value"] = new_val
            if self._dragging_knob["callback"]:
                self._dragging_knob["callback"](new_val)
            self._draw_all()

    def _on_release(self, event):
        self._dragging_knob = None

    def _on_hover(self, event):
        cell = self._grid_cell_at(event.x, event.y)
        if cell != self._hover_cell:
            self._hover_cell = cell
            if not self._dragging_knob:
                self._draw_all()

    # ── Transport ─────────────────────────────────────────────────────────
    def _toggle_play(self):
        if self.playing:
            self._stop()
        else:
            self._start()

    def _start(self):
        self.playing = True
        self.current_step = 0
        self.next_tick = time.perf_counter()
        self._tick()

    def _stop(self):
        self.playing = False
        if self._after_id:
            self.after_cancel(self._after_id)
            self._after_id = None
        self._draw_all()

    def _tick(self):
        if not self.playing:
            return

        self._trigger_step(self.current_step)
        self._draw_all()

        step_dur = 60.0 / self.bpm / 4.0
        if self.current_step % 2 == 1 and self.swing > 0:
            swing_amt = (self.swing / 100.0) * step_dur
            step_dur += swing_amt

        self.current_step = (self.current_step + 1) % 16
        self.next_tick += step_dur

        now = time.perf_counter()
        delay_ms = max(1, int((self.next_tick - now) * 1000))
        self._after_id = self.after(delay_ms, self._tick)

    def _trigger_step(self, step):
        accent = self.pattern[7][step]
        for i, (abbr, _) in enumerate(INSTRUMENTS):
            if self.pattern[i][step]:
                level = self.levels[i]
                if level < 0.05:
                    continue
                name = f"{abbr}+" if accent else abbr
                self._player.play(name)

    # ── Keyboard ──────────────────────────────────────────────────────────
    def _bind_keys(self):
        self.bind("<space>", lambda e: self._toggle_play())
        self.bind("<Escape>", lambda e: self.destroy())
        self.bind("r", lambda e: self._randomize_pattern())
        self.bind("c", lambda e: self._clear_pattern())
        for i in range(1, 8):
            self.bind(str(i), self._make_solo_handler(i - 1))
        self.focus_force()

    def _make_solo_handler(self, idx):
        def handler(event):
            abbr = INSTRUMENTS[idx][0]
            self._player.play(abbr)
        return handler

    def _clear_pattern(self):
        self.pattern = [[0] * 16 for _ in range(8)]
        self._draw_all()

    def _randomize_pattern(self):
        densities = [0.30, 0.25, 0.15, 0.15, 0.40, 0.12, 0.08, 0.20]
        for row in range(8):
            d = densities[row]
            for col in range(16):
                beat_boost = 0.15 if col % 4 == 0 else (0.05 if col % 2 == 0 else 0)
                self.pattern[row][col] = 1 if random.random() < (d + beat_boost) else 0
        self.pattern[0][0] = 1
        self.pattern[1][4] = 1
        self.pattern[1][12] = 1
        self._draw_all()

    # ── Cleanup ───────────────────────────────────────────────────────────
    def destroy(self):
        self.playing = False
        for path in self._bank.values():
            try:
                os.unlink(path)
            except OSError:
                pass
        try:
            os.rmdir(self._tmp)
        except OSError:
            pass
        super().destroy()


# ─── Entry Point ──────────────────────────────────────────────────────────────
if __name__ == "__main__":
    app = TR606()
    app.mainloop()
