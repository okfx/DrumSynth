#!/usr/bin/env python3
"""
Space Invaders — a classic arcade game for macOS.

Usage:
  Double-click or run from Terminal:
    python3 SpaceInvaders.py

Controls:
  ← →  or  A D    Move ship
  Space             Fire
  P                 Pause
  R                 Restart (when game over)
  Escape            Quit

Requirements: Python 3 with tkinter (included with macOS).
"""

import tkinter as tk
import random
import math
import time

# ─── Configuration ────────────────────────────────────────────────────────────
CANVAS_W       = 640
CANVAS_H       = 720
FPS            = 60
FRAME_MS       = 1000 // FPS

# Colours
BG             = "#0A0A1A"
STAR_DIM       = "#334"
STAR_BRIGHT    = "#99A"
HUD_COL        = "#00FF88"
SHIP_COL       = "#00DDFF"
SHIP_THRUST    = "#FF6600"
BULLET_COL     = "#FFFF00"
ENEMY_BULLET   = "#FF4466"
SHIELD_COL     = "#00FF88"
EXPLOSION_COL  = ["#FFFFFF", "#FFFF00", "#FF8800", "#FF4400", "#FF0000", "#880000"]

# Player
SHIP_W         = 36
SHIP_H         = 24
SHIP_SPEED     = 5
BULLET_SPEED   = 9
FIRE_COOLDOWN  = 12  # frames

# Enemies
ENEMY_COLS     = 11
ENEMY_ROWS     = 5
ENEMY_W        = 30
ENEMY_H        = 22
ENEMY_PAD_X    = 14
ENEMY_PAD_Y    = 12
ENEMY_START_Y  = 80
ENEMY_DROP     = 18
ENEMY_SHOOT_CHANCE = 0.012
ENEMY_BULLET_SPEED = 4

# Shields
SHIELD_COUNT   = 4
SHIELD_W       = 52
SHIELD_H       = 36
SHIELD_Y       = CANVAS_H - 140

# UFO
UFO_W          = 40
UFO_H          = 16
UFO_SPEED      = 2.5
UFO_CHANCE     = 0.002  # per frame

# ─── Pixel Art Helpers ────────────────────────────────────────────────────────

INVADER_SHAPES = [
    # Type 0: Squid (top rows)
    [
        "    ##    ",
        "   ####   ",
        "  ######  ",
        " ## ## ## ",
        " ######## ",
        "  #    #  ",
        " #  ##  # ",
        "#  #  #  #",
    ],
    # Type 1: Crab (middle rows)
    [
        "  #    #  ",
        "   #  #   ",
        "  ######  ",
        " ## ## ## ",
        "##########",
        "# ###### #",
        "# #    # #",
        "   ##  ## ",
    ],
    # Type 2: Octopus (bottom rows)
    [
        "   ####   ",
        " ######## ",
        "##########",
        "###  # ###",
        "##########",
        "  ##  ##  ",
        " ##  ## # ",
        "# #    # #",
    ],
]

INVADER_COLOURS = ["#FF4488", "#AA44FF", "#4488FF", "#44DDFF", "#44FF88"]


# ─── Particle ────────────────────────────────────────────────────────────────
class Particle:
    __slots__ = ("x", "y", "vx", "vy", "life", "max_life", "size")

    def __init__(self, x, y, vx, vy, life, size=2):
        self.x = x
        self.y = y
        self.vx = vx
        self.vy = vy
        self.life = life
        self.max_life = life
        self.size = size


# ─── Shield Block ─────────────────────────────────────────────────────────────
class ShieldBlock:
    __slots__ = ("x", "y", "w", "h", "pixels")

    def __init__(self, cx, cy):
        self.x = cx - SHIELD_W // 2
        self.y = cy - SHIELD_H // 2
        self.w = SHIELD_W
        self.h = SHIELD_H
        # Each pixel is 4x4; grid of booleans
        cols = SHIELD_W // 4
        rows = SHIELD_H // 4
        self.pixels = [[True] * cols for _ in range(rows)]
        # Carve arch at bottom centre
        mid = cols // 2
        for r in range(rows - 3, rows):
            for c in range(mid - 2, mid + 2):
                if 0 <= c < cols:
                    self.pixels[r][c] = False

    def draw(self, canvas):
        cols = len(self.pixels[0])
        rows = len(self.pixels)
        for r in range(rows):
            for c in range(cols):
                if self.pixels[r][c]:
                    px = self.x + c * 4
                    py = self.y + r * 4
                    canvas.create_rectangle(px, py, px + 4, py + 4,
                                            fill=SHIELD_COL, outline="")

    def hit(self, bx, by, radius=6):
        cols = len(self.pixels[0])
        rows = len(self.pixels)
        hit = False
        for r in range(rows):
            for c in range(cols):
                if self.pixels[r][c]:
                    px = self.x + c * 4 + 2
                    py = self.y + r * 4 + 2
                    if abs(px - bx) < radius and abs(py - by) < radius:
                        self.pixels[r][c] = False
                        hit = True
        return hit


# ─── Enemy ────────────────────────────────────────────────────────────────────
class Enemy:
    __slots__ = ("x", "y", "kind", "alive", "anim_frame")

    def __init__(self, x, y, kind):
        self.x = x
        self.y = y
        self.kind = kind
        self.alive = True
        self.anim_frame = 0


# ─── Game ─────────────────────────────────────────────────────────────────────
class SpaceInvadersGame(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("Space Invaders")
        self.configure(bg="#000")
        self.resizable(False, False)

        self.canvas = tk.Canvas(self, width=CANVAS_W, height=CANVAS_H,
                                bg=BG, highlightthickness=0)
        self.canvas.pack()

        # Stars
        self.stars = [(random.randint(0, CANVAS_W),
                       random.randint(0, CANVAS_H),
                       random.uniform(0.3, 1.5))
                      for _ in range(80)]

        # Input
        self.keys = set()
        self.bind("<KeyPress>",   lambda e: self.keys.add(e.keysym.lower()))
        self.bind("<KeyRelease>", lambda e: self.keys.discard(e.keysym.lower()))
        self.focus_force()

        self._init_game()
        self._loop()

        # Centre window
        self.update_idletasks()
        w, h = self.winfo_width(), self.winfo_height()
        x = (self.winfo_screenwidth() - w) // 2
        y = (self.winfo_screenheight() - h) // 3
        self.geometry(f"+{x}+{y}")

    # ── Init ──────────────────────────────────────────────────────────────
    def _init_game(self):
        self.score = 0
        self.lives = 3
        self.level = 1
        self.game_over = False
        self.paused = False
        self.win = False
        self.frame_count = 0

        # Player
        self.ship_x = CANVAS_W / 2
        self.ship_y = CANVAS_H - 50
        self.bullets = []       # [x, y]
        self.fire_cd = 0

        # Enemies
        self.enemies = []
        self.enemy_dir = 1      # 1 = right, -1 = left
        self.enemy_speed = 1.0
        self.enemy_move_timer = 0
        self.enemy_move_interval = 45  # frames between moves
        self.enemy_bullets = []  # [x, y]

        # UFO
        self.ufo_x = -UFO_W
        self.ufo_active = False
        self.ufo_dir = 1

        # Shields
        spacing = CANVAS_W / (SHIELD_COUNT + 1)
        self.shields = [ShieldBlock(int(spacing * (i + 1)), SHIELD_Y)
                        for i in range(SHIELD_COUNT)]

        # Particles
        self.particles = []

        # Populate enemies
        self._spawn_enemies()

    def _spawn_enemies(self):
        self.enemies.clear()
        total_w = ENEMY_COLS * (ENEMY_W + ENEMY_PAD_X) - ENEMY_PAD_X
        start_x = (CANVAS_W - total_w) / 2 + ENEMY_W / 2
        for row in range(ENEMY_ROWS):
            kind = min(row, 2)  # 0=squid, 1=crab, 2=octopus
            for col in range(ENEMY_COLS):
                ex = start_x + col * (ENEMY_W + ENEMY_PAD_X)
                ey = ENEMY_START_Y + row * (ENEMY_H + ENEMY_PAD_Y)
                self.enemies.append(Enemy(ex, ey, kind))

        # Speed scales with level
        self.enemy_move_interval = max(10, 45 - (self.level - 1) * 5)
        self.enemy_speed = 1.0 + (self.level - 1) * 0.3

    # ── Main Loop ─────────────────────────────────────────────────────────
    def _loop(self):
        t0 = time.perf_counter()
        self._handle_input()
        if not self.paused and not self.game_over:
            self._update()
        self._draw()
        elapsed_ms = (time.perf_counter() - t0) * 1000
        delay = max(1, FRAME_MS - int(elapsed_ms))
        self.after(delay, self._loop)

    # ── Input ─────────────────────────────────────────────────────────────
    def _handle_input(self):
        if "escape" in self.keys:
            self.destroy()
            return

        if "p" in self.keys:
            self.keys.discard("p")
            self.paused = not self.paused

        if self.game_over and "r" in self.keys:
            self.keys.discard("r")
            self._init_game()

    # ── Update ────────────────────────────────────────────────────────────
    def _update(self):
        self.frame_count += 1

        # Ship movement
        if "left" in self.keys or "a" in self.keys:
            self.ship_x = max(SHIP_W / 2, self.ship_x - SHIP_SPEED)
        if "right" in self.keys or "d" in self.keys:
            self.ship_x = min(CANVAS_W - SHIP_W / 2, self.ship_x + SHIP_SPEED)

        # Fire
        if self.fire_cd > 0:
            self.fire_cd -= 1
        if "space" in self.keys and self.fire_cd <= 0:
            self.bullets.append([self.ship_x, self.ship_y - SHIP_H / 2])
            self.fire_cd = FIRE_COOLDOWN

        # Move bullets
        self.bullets = [[bx, by - BULLET_SPEED] for bx, by in self.bullets if by > -10]

        # Move enemy bullets
        self.enemy_bullets = [[bx, by + ENEMY_BULLET_SPEED]
                              for bx, by in self.enemy_bullets if by < CANVAS_H + 10]

        # Enemy movement (step-based like original)
        self.enemy_move_timer += 1
        alive = [e for e in self.enemies if e.alive]

        # Speed up as fewer enemies remain
        alive_count = len(alive)
        if alive_count > 0:
            speed_factor = max(1, 55 - alive_count)
            effective_interval = max(3, self.enemy_move_interval - speed_factor // 3)
        else:
            effective_interval = self.enemy_move_interval

        if self.enemy_move_timer >= effective_interval and alive:
            self.enemy_move_timer = 0

            # Check edges
            drop = False
            for e in alive:
                if self.enemy_dir > 0 and e.x + ENEMY_W / 2 >= CANVAS_W - 20:
                    drop = True
                    break
                if self.enemy_dir < 0 and e.x - ENEMY_W / 2 <= 20:
                    drop = True
                    break

            if drop:
                self.enemy_dir *= -1
                for e in alive:
                    e.y += ENEMY_DROP
                    e.anim_frame = 1 - e.anim_frame
            else:
                for e in alive:
                    e.x += self.enemy_speed * self.enemy_dir * 8
                    e.anim_frame = 1 - e.anim_frame

        # Enemy shooting
        if alive:
            # Only bottom-most enemy in each column can shoot
            columns = {}
            for e in alive:
                col_key = round(e.x / (ENEMY_W + ENEMY_PAD_X))
                if col_key not in columns or e.y > columns[col_key].y:
                    columns[col_key] = e
            for e in columns.values():
                if random.random() < ENEMY_SHOOT_CHANCE:
                    self.enemy_bullets.append([e.x, e.y + ENEMY_H / 2])

        # UFO
        if not self.ufo_active and random.random() < UFO_CHANCE:
            self.ufo_active = True
            self.ufo_dir = random.choice([-1, 1])
            self.ufo_x = -UFO_W if self.ufo_dir > 0 else CANVAS_W + UFO_W

        if self.ufo_active:
            self.ufo_x += UFO_SPEED * self.ufo_dir
            if self.ufo_x < -UFO_W * 2 or self.ufo_x > CANVAS_W + UFO_W * 2:
                self.ufo_active = False

        # Collisions: player bullets vs enemies
        for bullet in self.bullets[:]:
            bx, by = bullet
            for e in self.enemies:
                if not e.alive:
                    continue
                if (abs(bx - e.x) < ENEMY_W / 2 and abs(by - e.y) < ENEMY_H / 2):
                    e.alive = False
                    if bullet in self.bullets:
                        self.bullets.remove(bullet)
                    points = (30, 20, 10)[e.kind]
                    self.score += points
                    self._spawn_explosion(e.x, e.y, INVADER_COLOURS[e.kind % len(INVADER_COLOURS)])
                    break

        # Collisions: player bullets vs UFO
        if self.ufo_active:
            for bullet in self.bullets[:]:
                bx, by = bullet
                if abs(bx - self.ufo_x) < UFO_W / 2 and abs(by - 50) < UFO_H:
                    self.ufo_active = False
                    self.bullets.remove(bullet)
                    ufo_points = random.choice([50, 100, 150, 300])
                    self.score += ufo_points
                    self._spawn_explosion(self.ufo_x, 50, "#FF0000")
                    break

        # Collisions: bullets vs shields
        for bullet in self.bullets[:]:
            bx, by = bullet
            for shield in self.shields:
                if shield.hit(bx, by, 5):
                    if bullet in self.bullets:
                        self.bullets.remove(bullet)
                    break

        for bullet in self.enemy_bullets[:]:
            bx, by = bullet
            for shield in self.shields:
                if shield.hit(bx, by, 5):
                    if bullet in self.enemy_bullets:
                        self.enemy_bullets.remove(bullet)
                    break

        # Collisions: enemy bullets vs player
        for bullet in self.enemy_bullets[:]:
            bx, by = bullet
            if (abs(bx - self.ship_x) < SHIP_W / 2 and
                    abs(by - self.ship_y) < SHIP_H / 2):
                self.enemy_bullets.remove(bullet)
                self.lives -= 1
                self._spawn_explosion(self.ship_x, self.ship_y, SHIP_COL)
                if self.lives <= 0:
                    self.game_over = True
                else:
                    self.ship_x = CANVAS_W / 2
                break

        # Enemies reaching bottom
        for e in alive:
            if e.y + ENEMY_H / 2 >= self.ship_y - SHIP_H:
                self.game_over = True
                break

        # Wave cleared
        if alive_count == 0 and not self.game_over:
            self.level += 1
            self.enemy_bullets.clear()
            self.bullets.clear()
            self._spawn_enemies()

        # Particles
        for p in self.particles:
            p.x += p.vx
            p.y += p.vy
            p.vy += 0.05  # gravity
            p.life -= 1
        self.particles = [p for p in self.particles if p.life > 0]

    # ── Explosions ────────────────────────────────────────────────────────
    def _spawn_explosion(self, x, y, colour):
        for _ in range(16):
            angle = random.uniform(0, 2 * math.pi)
            speed = random.uniform(1, 4)
            vx = math.cos(angle) * speed
            vy = math.sin(angle) * speed
            life = random.randint(10, 25)
            self.particles.append(Particle(x, y, vx, vy, life, random.randint(2, 4)))

    # ── Draw ──────────────────────────────────────────────────────────────
    def _draw(self):
        c = self.canvas
        c.delete("all")

        # Stars
        for sx, sy, sb in self.stars:
            brightness = 0.3 + 0.7 * (0.5 + 0.5 * math.sin(self.frame_count * 0.02 * sb))
            grey = int(40 + 60 * brightness)
            col = f"#{grey:02x}{grey:02x}{int(grey * 1.1):02x}"
            size = 1 if sb < 1.0 else 2
            c.create_rectangle(sx, sy, sx + size, sy + size, fill=col, outline="")

        # Shields
        for shield in self.shields:
            shield.draw(c)

        # Enemies
        for e in self.enemies:
            if e.alive:
                self._draw_invader(c, e)

        # UFO
        if self.ufo_active:
            self._draw_ufo(c, self.ufo_x, 50)

        # Player ship
        if not self.game_over:
            self._draw_ship(c, self.ship_x, self.ship_y)

        # Player bullets
        for bx, by in self.bullets:
            c.create_rectangle(bx - 2, by - 6, bx + 2, by + 2,
                               fill=BULLET_COL, outline="")

        # Enemy bullets
        for bx, by in self.enemy_bullets:
            c.create_rectangle(bx - 2, by - 2, bx + 2, by + 6,
                               fill=ENEMY_BULLET, outline="")

        # Particles
        for p in self.particles:
            frac = p.life / p.max_life
            idx = min(len(EXPLOSION_COL) - 1, int((1 - frac) * len(EXPLOSION_COL)))
            col = EXPLOSION_COL[idx]
            s = int(p.size * frac)
            if s > 0:
                c.create_rectangle(p.x - s, p.y - s, p.x + s, p.y + s,
                                   fill=col, outline="")

        # HUD
        c.create_text(16, 16, text=f"SCORE  {self.score:06d}",
                       font=("Courier", 16, "bold"), fill=HUD_COL, anchor="nw")
        c.create_text(CANVAS_W - 16, 16, text=f"LEVEL {self.level}",
                       font=("Courier", 16, "bold"), fill=HUD_COL, anchor="ne")

        # Lives
        for i in range(self.lives):
            lx = 16 + i * 30
            ly = 42
            c.create_polygon(lx, ly - 6, lx - 8, ly + 6, lx + 8, ly + 6,
                             fill=SHIP_COL, outline="")

        # Bottom line
        c.create_line(0, CANVAS_H - 20, CANVAS_W, CANVAS_H - 20,
                      fill="#1A3A1A", width=1)

        # Paused overlay
        if self.paused:
            c.create_rectangle(0, 0, CANVAS_W, CANVAS_H,
                               fill="", outline="")
            c.create_text(CANVAS_W / 2, CANVAS_H / 2, text="PAUSED",
                          font=("Courier", 40, "bold"), fill="#FFFFFF")
            c.create_text(CANVAS_W / 2, CANVAS_H / 2 + 50,
                          text="Press P to resume",
                          font=("Courier", 16), fill="#888888")

        # Game over overlay
        if self.game_over:
            c.create_rectangle(0, CANVAS_H // 2 - 60, CANVAS_W, CANVAS_H // 2 + 60,
                               fill="#000000", stipple="gray50", outline="")
            c.create_text(CANVAS_W / 2, CANVAS_H / 2 - 20, text="GAME OVER",
                          font=("Courier", 40, "bold"), fill="#FF4444")
            c.create_text(CANVAS_W / 2, CANVAS_H / 2 + 25,
                          text=f"Final Score: {self.score}",
                          font=("Courier", 18), fill="#FFFFFF")
            c.create_text(CANVAS_W / 2, CANVAS_H / 2 + 55,
                          text="Press R to restart",
                          font=("Courier", 14), fill="#888888")

    # ── Sprite Drawing ────────────────────────────────────────────────────
    def _draw_invader(self, canvas, enemy):
        shape = INVADER_SHAPES[enemy.kind]
        colour = INVADER_COLOURS[enemy.kind % len(INVADER_COLOURS)]
        px_size = 3
        rows = len(shape)
        cols = len(shape[0])
        ox = enemy.x - (cols * px_size) / 2
        oy = enemy.y - (rows * px_size) / 2

        # Simple animation: shift odd rows on alt frames
        for r, row_str in enumerate(shape):
            for ch_c, ch in enumerate(row_str):
                if ch == "#":
                    # Slight animation wiggle
                    anim_off = (1 if enemy.anim_frame and r % 2 else 0)
                    px = ox + (ch_c + anim_off) * px_size
                    py = oy + r * px_size
                    canvas.create_rectangle(
                        px, py, px + px_size, py + px_size,
                        fill=colour, outline=""
                    )

    def _draw_ship(self, canvas, x, y):
        # Main body — pointed triangle
        canvas.create_polygon(
            x, y - SHIP_H / 2,
            x - SHIP_W / 2, y + SHIP_H / 2,
            x + SHIP_W / 2, y + SHIP_H / 2,
            fill=SHIP_COL, outline="#FFFFFF", width=1
        )
        # Cockpit
        canvas.create_oval(
            x - 4, y - 2, x + 4, y + 6,
            fill="#003344", outline=SHIP_COL
        )
        # Thrust glow (animated)
        if self.frame_count % 4 < 2:
            canvas.create_polygon(
                x - 6, y + SHIP_H / 2,
                x, y + SHIP_H / 2 + 8,
                x + 6, y + SHIP_H / 2,
                fill=SHIP_THRUST, outline=""
            )

    def _draw_ufo(self, canvas, x, y):
        # Body
        canvas.create_oval(x - UFO_W / 2, y - UFO_H / 2,
                           x + UFO_W / 2, y + UFO_H / 2,
                           fill="#FF2244", outline="#FF6688", width=1)
        # Dome
        canvas.create_oval(x - 10, y - UFO_H, x + 10, y,
                           fill="#FF4466", outline="#FFAACC", width=1)
        # Blinking light
        if self.frame_count % 10 < 5:
            canvas.create_oval(x - 2, y - UFO_H - 2, x + 2, y - UFO_H + 2,
                               fill="#FFFFFF", outline="")


# ─── Entry Point ──────────────────────────────────────────────────────────────
if __name__ == "__main__":
    game = SpaceInvadersGame()
    game.mainloop()
