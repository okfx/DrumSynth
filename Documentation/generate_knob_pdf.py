#!/usr/bin/env python3
"""Generate a landscape PDF of the DrumSynth knob layout."""

from fpdf import FPDF

class KnobPDF(FPDF):
    pass

pdf = KnobPDF(orientation='L', unit='mm', format='Letter')
pdf.set_auto_page_break(auto=False)
pdf.add_page()

# Page dimensions (Letter landscape)
pw = 279.4  # mm
ph = 215.9  # mm
margin = 12

# Title
pdf.set_font("Helvetica", "B", 20)
pdf.set_xy(margin, margin)
pdf.cell(0, 8, "DrumSynth v1.0.3 - Knob Layout", align="L")

pdf.set_font("Helvetica", "", 9)
pdf.set_xy(margin, margin + 9)
pdf.cell(0, 5, "4 columns x 8 rows  -  32 knobs mapped to 2x 16-channel analog mux", align="L")

# Layout: 4 columns (D1, D2, D3, Master), 8 rows
# Each column has 4 rows, each row is a pair (left knob, right knob)
# Format: (idx, name, desc) for each knob
columns = [
    ("D1  KICK", [
        (("3", "TUNE", "Base frequency (Hz)"),        ("4", "VOLUME", "Voice output level")),
        (("2", "DECAY", "Amplitude decay time"),       ("5", "SNAP", "Pitch env / Attack*")),
        (("1", "OSC", "Sine / Saw / Square"),          ("6", "BODY", "Tone EQ / Env Filter*")),
        (("0", "DISTORT", "Wavefolder drive"),         ("7", "DELAY SEND", "Pre-distortion send")),
    ]),
    ("D2  SNARE/CLAP", [
        (("8", "TUNE", "Oscillator frequency"),        ("15", "VOLUME", "Voice output level")),
        (("9", "DECAY", "Amplitude decay time"),       ("14", "SNR NOISE", "Noise mix level")),
        (("10", "VOICE", "Clap <--> Snare"),           ("13", "REVERB", "Reverb send level")),
        (("11", "DISTORT", "Wavefolder drive"),        ("12", "DELAY SEND", "Pre-distortion send")),
    ]),
    ("D3  HATS/PERC", [
        (("16", "TUNE", "Tune (606/FM/Perc)"),         ("23", "VOLUME", "Voice output level")),
        (("17", "DECAY", "Amplitude decay time"),      ("22", "ACCENT", "Accent pattern select")),
        (("18", "VOICE", "FM / 606 / Perc"),           ("21", "ENV FILTER", "Envelope filter depth")),
        (("19", "DISTORT", "Wavefolder drive"),        ("20", "DELAY SEND", "Pre-distortion send")),
    ]),
    ("MASTER", [
        (("27", "BPM", "60-300 / 800-999 hyper"),     ("28", "VOLUME", "Master output level")),
        (("26", "FILTER", "LP < noon | HP > noon"),    ("29", "CHOKE", "Global decay +/-")),
        (("25", "WF FREQ", "Wavefolder frequency"),    ("30", "WF DRIVE", "Wavefolder intensity")),
        (("24", "DELAY TIME", "Synced to BPM"),        ("31", "DELAY MIX", "Delay feedback + wet")),
    ]),
]

# Grid geometry
top_y = margin + 20
col_w = (pw - 2 * margin - 3 * 4) / 4  # 4 columns with 3 gaps of 4mm
knob_w = col_w / 2  # each row has 2 knobs side by side
row_h = 32
header_h = 10
gap = 4

# Colors
col_colors = [
    (45, 50, 80),     # D1 - dark blue
    (80, 45, 45),     # D2 - dark red
    (45, 75, 50),     # D3 - dark green
    (70, 60, 40),     # Master - dark gold
]
col_light = [
    (220, 225, 245),  # D1 light
    (245, 220, 220),  # D2 light
    (220, 240, 225),  # D3 light
    (240, 235, 215),  # Master light
]

for ci, (col_title, rows) in enumerate(columns):
    x = margin + ci * (col_w + gap)

    # Column header
    r, g, b = col_colors[ci]
    pdf.set_fill_color(r, g, b)
    pdf.set_text_color(255, 255, 255)
    pdf.set_font("Helvetica", "B", 12)
    pdf.set_xy(x, top_y)
    pdf.cell(col_w, header_h, col_title, border=0, fill=True, align="C")

    # Knob rows (each row has left + right knob)
    for ri, (left_knob, right_knob) in enumerate(rows):
        ry = top_y + header_h + ri * row_h

        # Alternating row background
        lr, lg, lb = col_light[ci]
        if ri % 2 == 0:
            pdf.set_fill_color(lr, lg, lb)
        else:
            pdf.set_fill_color(255, 255, 255)
        pdf.rect(x, ry, col_w, row_h, style="F")

        # Row border
        pdf.set_draw_color(180, 180, 180)
        pdf.rect(x, ry, col_w, row_h, style="D")

        # Vertical divider between left and right knob
        pdf.set_draw_color(200, 200, 200)
        pdf.line(x + knob_w, ry + 1, x + knob_w, ry + row_h - 1)

        # Draw each knob in its half
        for ki, (idx, name, desc) in enumerate([left_knob, right_knob]):
            kx = x + ki * knob_w

            # Knob index (small, top-left)
            pdf.set_font("Helvetica", "", 7)
            pdf.set_text_color(140, 140, 140)
            pdf.set_xy(kx + 1.5, ry + 1)
            pdf.cell(8, 4, idx, align="L")

            # Knob name (bold, centered)
            pdf.set_font("Helvetica", "B", 10)
            pdf.set_text_color(30, 30, 30)
            pdf.set_xy(kx, ry + 7)
            pdf.cell(knob_w, 7, name, align="C")

            # Description (smaller, below name)
            pdf.set_font("Helvetica", "", 7)
            pdf.set_text_color(80, 80, 80)
            pdf.set_xy(kx, ry + 17)
            pdf.cell(knob_w, 5, desc, align="C")

# Footer notes
footer_y = top_y + header_h + 4 * row_h + 4
pdf.set_font("Helvetica", "", 8)
pdf.set_text_color(100, 100, 100)
pdf.set_xy(margin, footer_y)
pdf.cell(0, 4, "* D1 Chroma mode:  Snap -> Attack (0.5-250ms)    Body -> Envelope Filter depth    |    D3 Env Filter: 0-5% deadband, 5-25% darken, 25-100% envelope sweep", align="L")
pdf.set_xy(margin, footer_y + 5)
pdf.cell(0, 4, "Swing: X + Step 16 to cycle (14 levels, 52-79%)    |    Chroma: X + D1/D2/D3    |    MONOBASS: hold X 4s    |    PPQN: X + LOAD hold 1.5s", align="L")

out_path = "/Users/j/Documents/Arduino/DrumSynth/Documentation/DrumSynth_Knob_Layout.pdf"
pdf.output(out_path)
print(f"Written to {out_path}")
