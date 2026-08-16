#!/usr/bin/env python3
"""
Public Domain Controller & Keyboard Button Icon Generator
Creates high-fidelity 32-bit RGBA PNG button glyphs and keycaps for in-game UI.
Uses pure Python standard library (zlib + struct) to guarantee zero external dependencies.
"""

import math
import os
import struct
import zlib

def write_png(filename, width, height, pixels):
    """
    Writes a 32-bit RGBA PNG file.
    pixels: list of (r, g, b, a) tuples or bytearray of size width * height * 4
    """
    raw_data = bytearray()
    for y in range(height):
        raw_data.append(0)  # filter type 0 (None)
        offset = y * width * 4
        raw_data.extend(pixels[offset:offset + width * 4])

    compressed = zlib.compress(bytes(raw_data), 9)

    png = bytearray(b'\x89PNG\r\n\x1a\n')

    def make_chunk(chunk_type, data):
        chunk = bytearray(chunk_type) + data
        crc = zlib.crc32(chunk) & 0xffffffff
        return struct.pack('>I', len(data)) + chunk + struct.pack('>I', crc)

    # IHDR
    ihdr = struct.pack('>IIBBBBB', width, height, 8, 6, 0, 0, 0)
    png.extend(make_chunk(b'IHDR', ihdr))

    # IDAT
    png.extend(make_chunk(b'IDAT', compressed))

    # IEND
    png.extend(make_chunk(b'IEND', b''))

    os.makedirs(os.path.dirname(os.path.abspath(filename)), exist_ok=True)
    with open(filename, 'wb') as f:
        f.write(png)
    print(f"Generated: {filename} ({width}x{height})")

class Canvas:
    def __init__(self, width, height):
        self.w = width
        self.h = height
        self.data = bytearray(width * height * 4)

    def set_pixel(self, x, y, r, g, b, a=255):
        if 0 <= x < self.w and 0 <= y < self.h:
            idx = (y * self.w + x) * 4
            src_a = a / 255.0
            dst_a = self.data[idx + 3] / 255.0
            out_a = src_a + dst_a * (1.0 - src_a)
            if out_a > 0.001:
                out_r = (r * src_a + self.data[idx] * dst_a * (1.0 - src_a)) / out_a
                out_g = (g * src_a + self.data[idx + 1] * dst_a * (1.0 - src_a)) / out_a
                out_b = (b * src_a + self.data[idx + 2] * dst_a * (1.0 - src_a)) / out_a
                self.data[idx] = int(min(255, max(0, out_r)))
                self.data[idx + 1] = int(min(255, max(0, out_g)))
                self.data[idx + 2] = int(min(255, max(0, out_b)))
                self.data[idx + 3] = int(min(255, max(0, out_a * 255)))

    def draw_circle(self, cx, cy, radius, r, g, b, a=255, fill=True, border_width=1, border_color=(0,0,0,255)):
        r_inner = radius - border_width if not fill else 0
        for y in range(int(cy - radius - 1), int(cy + radius + 2)):
            for x in range(int(cx - radius - 1), int(cx + radius + 2)):
                dist = math.hypot(x - cx, y - cy)
                if dist <= radius + 0.5:
                    alpha_factor = max(0.0, min(1.0, radius + 0.5 - dist))
                    if fill:
                        self.set_pixel(x, y, r, g, b, int(a * alpha_factor))
                    elif dist >= r_inner - 0.5:
                        self.set_pixel(x, y, r, g, b, int(a * alpha_factor))

    def draw_rounded_rect(self, x0, y0, w, h, radius, r, g, b, a=255):
        for y in range(y0, y0 + h):
            for x in range(x0, x0 + w):
                # Check corners
                dx = 0
                dy = 0
                if x < x0 + radius:
                    dx = (x0 + radius) - x
                elif x > x0 + w - 1 - radius:
                    dx = x - (x0 + w - 1 - radius)
                if y < y0 + radius:
                    dy = (y0 + radius) - y
                elif y > y0 + h - 1 - radius:
                    dy = y - (y0 + h - 1 - radius)

                if dx > 0 and dy > 0:
                    dist = math.hypot(dx, dy)
                    if dist <= radius + 0.5:
                        alpha_factor = max(0.0, min(1.0, radius + 0.5 - dist))
                        self.set_pixel(x, y, r, g, b, int(a * alpha_factor))
                else:
                    self.set_pixel(x, y, r, g, b, a)

    def draw_letter(self, letter, cx, cy, size, r=255, g=255, b=255, a=255):
        # 5x7 bitmap font definitions scaled up
        bitmaps = {
            'A': [
                " 111 ",
                "1   1",
                "1   1",
                "11111",
                "1   1",
                "1   1",
                "1   1"
            ],
            'B': [
                "1111 ",
                "1   1",
                "1   1",
                "1111 ",
                "1   1",
                "1   1",
                "1111 "
            ],
            'X': [
                "1   1",
                "1   1",
                " 1 1 ",
                "  1  ",
                " 1 1 ",
                "1   1",
                "1   1"
            ],
            'Y': [
                "1   1",
                "1   1",
                " 1 1 ",
                "  1  ",
                "  1  ",
                "  1  ",
                "  1  "
            ],
            'L': [
                "1    ",
                "1    ",
                "1    ",
                "1    ",
                "1    ",
                "1    ",
                "11111"
            ],
            'R': [
                "1111 ",
                "1   1",
                "1   1",
                "1111 ",
                "1 1  ",
                "1  1 ",
                "1   1"
            ],
            'T': [
                "11111",
                "  1  ",
                "  1  ",
                "  1  ",
                "  1  ",
                "  1  ",
                "  1  "
            ],
            'W': [
                "1   1",
                "1   1",
                "1   1",
                "1 1 1",
                "1 1 1",
                "11 11",
                "1   1"
            ],
            'S': [
                " 1111",
                "1    ",
                "1    ",
                " 111 ",
                "    1",
                "    1",
                "1111 "
            ],
            'D': [
                "1111 ",
                "1   1",
                "1   1",
                "1   1",
                "1   1",
                "1   1",
                "1111 "
            ],
            'E': [
                "11111",
                "1    ",
                "1    ",
                "1111 ",
                "1    ",
                "1    ",
                "11111"
            ],
            'Q': [
                " 111 ",
                "1   1",
                "1   1",
                "1   1",
                "1 1 1",
                "1  1 ",
                " 11 1"
            ],
            'C': [
                " 1111",
                "1    ",
                "1    ",
                "1    ",
                "1    ",
                "1    ",
                " 1111"
            ],
            'F': [
                "11111",
                "1    ",
                "1    ",
                "1111 ",
                "1    ",
                "1    ",
                "1    "
            ],
            '1': [
                "  1  ",
                " 11  ",
                "  1  ",
                "  1  ",
                "  1  ",
                "  1  ",
                "11111"
            ],
            '2': [
                " 111 ",
                "1   1",
                "    1",
                "  11 ",
                " 1   ",
                "1    ",
                "11111"
            ],
            'UP': [
                "  1  ",
                " 111 ",
                "11111",
                "  1  ",
                "  1  ",
                "  1  ",
                "  1  "
            ],
            'DOWN': [
                "  1  ",
                "  1  ",
                "  1  ",
                "  1  ",
                "11111",
                " 111 ",
                "  1  "
            ],
            'LEFT': [
                "   1  ",
                "  1   ",
                " 1    ",
                "111111",
                " 1    ",
                "  1   ",
                "   1  "
            ],
            'RIGHT': [
                "  1   ",
                "   1  ",
                "    1 ",
                "111111",
                "    1 ",
                "   1  ",
                "  1   "
            ],
            'ESC': [
                "111 111 111",
                "1   1   1  ",
                "11  11  1  ",
                "1     1 1  ",
                "111 111 111"
            ],
            'TAB': [
                "111 111 111",
                " 1  1 1 1 1",
                " 1  111 111",
                " 1  1 1 1 1",
                " 1  1 1 111"
            ],
            'RET': [
                "    1  ",
                "    1  ",
                "1   1  ",
                "1111111",
                "1   1  ",
                "    1  ",
                "    1  "
            ]
        }

        bmp = bitmaps.get(letter)
        if not bmp:
            return

        rows = len(bmp)
        cols = len(bmp[0])
        scale = size
        start_x = int(cx - (cols * scale) / 2)
        start_y = int(cy - (rows * scale) / 2)

        for r_idx, row in enumerate(bmp):
            for c_idx, ch in enumerate(row):
                if ch == '1':
                    for sy in range(scale):
                        for sx in range(scale):
                            self.set_pixel(start_x + c_idx * scale + sx, start_y + r_idx * scale + sy, r, g, b, a)

def generate_xbox_button(filename, letter, base_color, text_color=(255, 255, 255)):
    size = 128
    c = Canvas(size, size)
    cx, cy = size // 2, size // 2
    r_outer = 56

    # Drop shadow
    c.draw_circle(cx, cy + 4, r_outer, 0, 0, 0, 100)

    # Outer metallic rim
    c.draw_circle(cx, cy, r_outer, 45, 48, 52, 255)
    c.draw_circle(cx, cy, r_outer - 2, 20, 22, 24, 255)

    # Button face with subtle top-to-bottom gradient
    for rad in range(r_outer - 4, 0, -1):
        factor = rad / (r_outer - 4)
        br = int(base_color[0] * (0.7 + 0.4 * factor))
        bg = int(base_color[1] * (0.7 + 0.4 * factor))
        bb = int(base_color[2] * (0.7 + 0.4 * factor))
        c.draw_circle(cx, cy, rad, br, bg, bb, 255)

    # Top highlight / gloss curve
    c.draw_circle(cx, cy - 14, r_outer // 2, 255, 255, 255, 60)

    # Letter in center
    c.draw_letter(letter, cx, cy, 6, text_color[0], text_color[1], text_color[2], 255)

    write_png(filename, size, size, c.data)

def generate_capsule_button(filename, text, base_color=(45, 48, 55)):
    w, h = 160, 96
    c = Canvas(w, h)
    # Shadow
    c.draw_rounded_rect(8, 12, w - 16, h - 20, 24, 0, 0, 0, 90)
    # Rim
    c.draw_rounded_rect(8, 8, w - 16, h - 20, 24, 80, 85, 95, 255)
    # Face
    c.draw_rounded_rect(12, 12, w - 24, h - 28, 20, base_color[0], base_color[1], base_color[2], 255)
    # Text
    if len(text) == 2:
        c.draw_letter(text[0], w // 2 - 20, h // 2 - 2, 5, 240, 245, 255, 255)
        c.draw_letter(text[1], w // 2 + 20, h // 2 - 2, 5, 240, 245, 255, 255)
    else:
        c.draw_letter(text, w // 2, h // 2 - 2, 5, 240, 245, 255, 255)

    write_png(filename, w, h, c.data)

def generate_keycap(filename, text, width=110, height=110):
    c = Canvas(width, height)
    # Shadow
    c.draw_rounded_rect(6, 10, width - 12, height - 16, 12, 0, 0, 0, 110)
    # Keycap base rim (Dark slate)
    c.draw_rounded_rect(6, 6, width - 12, height - 16, 12, 50, 54, 62, 255)
    # Keycap top surface (Slightly raised & beveled)
    c.draw_rounded_rect(10, 8, width - 20, height - 24, 10, 32, 35, 42, 255)
    # Top highlight line
    c.draw_rounded_rect(14, 11, width - 28, 4, 2, 80, 85, 100, 180)

    # Key text/glyph
    if text in ('UP', 'DOWN', 'LEFT', 'RIGHT', 'ESC', 'TAB', 'RET'):
        c.draw_letter(text, width // 2, height // 2 - 2, 4, 235, 240, 250, 255)
    elif text == 'SPACE':
        # Draw spacebar symbol
        c.draw_rounded_rect(width // 4, height // 2 - 3, width // 2, 8, 3, 235, 240, 250, 255)
    else:
        c.draw_letter(text, width // 2, height // 2 - 2, 5, 235, 240, 250, 255)

    write_png(filename, width, height, c.data)

def generate_dpad_button(filename, highlighted_dir=None):
    size = 128
    c = Canvas(size, size)
    cx, cy = size // 2, size // 2

    # Outer cross shadow
    c.draw_rounded_rect(cx - 18, 14, 36, 104, 8, 0, 0, 0, 90)
    c.draw_rounded_rect(14, cy - 18, 104, 36, 8, 0, 0, 0, 90)

    # Outer cross body
    c.draw_rounded_rect(cx - 18, 10, 36, 104, 8, 35, 38, 45, 255)
    c.draw_rounded_rect(10, cy - 18, 104, 36, 8, 35, 38, 45, 255)

    # Central depression circle
    c.draw_circle(cx, cy, 20, 22, 24, 28, 255)

    # Direction highlights
    hl_color = (0, 220, 120, 255) if highlighted_dir else (180, 190, 200, 180)

    # Arrows
    c.draw_letter('UP', cx, cy - 32, 3, *(hl_color if highlighted_dir == 'UP' else (160, 170, 185, 200)))
    c.draw_letter('DOWN', cx, cy + 32, 3, *(hl_color if highlighted_dir == 'DOWN' else (160, 170, 185, 200)))
    c.draw_letter('LEFT', cx - 32, cy, 3, *(hl_color if highlighted_dir == 'LEFT' else (160, 170, 185, 200)))
    c.draw_letter('RIGHT', cx + 32, cy, 3, *(hl_color if highlighted_dir == 'RIGHT' else (160, 170, 185, 200)))

    write_png(filename, size, size, c.data)

def main():
    buttons_dir = "data/media/menu/buttons"
    os.makedirs(buttons_dir, exist_ok=True)

    print("=== Generating Public Domain Xbox Controller & Keyboard Icons ===")

    # Xbox Face Buttons (Authentic Colors: A=Green, B=Red, X=Blue, Y=Yellow)
    generate_xbox_button(os.path.join(buttons_dir, "xbox_a.png"), 'A', (16, 170, 72))
    generate_xbox_button(os.path.join(buttons_dir, "xbox_b.png"), 'B', (225, 32, 38))
    generate_xbox_button(os.path.join(buttons_dir, "xbox_x.png"), 'X', (20, 115, 230))
    generate_xbox_button(os.path.join(buttons_dir, "xbox_y.png"), 'Y', (240, 180, 15))

    # Shoulder Bumpers & Triggers
    generate_capsule_button(os.path.join(buttons_dir, "xbox_lb.png"), "LB", (48, 52, 60))
    generate_capsule_button(os.path.join(buttons_dir, "xbox_rb.png"), "RB", (48, 52, 60))
    generate_capsule_button(os.path.join(buttons_dir, "xbox_lt.png"), "LT", (38, 42, 50))
    generate_capsule_button(os.path.join(buttons_dir, "xbox_rt.png"), "RT", (38, 42, 50))

    # Thumbsticks & Center buttons
    generate_capsule_button(os.path.join(buttons_dir, "xbox_ls.png"), "LS", (42, 46, 54))
    generate_capsule_button(os.path.join(buttons_dir, "xbox_rs.png"), "RS", (42, 46, 54))
    generate_capsule_button(os.path.join(buttons_dir, "xbox_start.png"), "START", (42, 46, 54))
    generate_capsule_button(os.path.join(buttons_dir, "xbox_back.png"), "BACK", (42, 46, 54))

    # D-Pad Cross and directional variants
    generate_dpad_button(os.path.join(buttons_dir, "xbox_dpad.png"), None)
    generate_dpad_button(os.path.join(buttons_dir, "xbox_dpad_up.png"), 'UP')
    generate_dpad_button(os.path.join(buttons_dir, "xbox_dpad_down.png"), 'DOWN')
    generate_dpad_button(os.path.join(buttons_dir, "xbox_dpad_left.png"), 'LEFT')
    generate_dpad_button(os.path.join(buttons_dir, "xbox_dpad_right.png"), 'RIGHT')

    # Keyboard Keycaps
    generate_keycap(os.path.join(buttons_dir, "key_w.png"), 'W')
    generate_keycap(os.path.join(buttons_dir, "key_a.png"), 'A')
    generate_keycap(os.path.join(buttons_dir, "key_s.png"), 'S')
    generate_keycap(os.path.join(buttons_dir, "key_d.png"), 'D')
    generate_keycap(os.path.join(buttons_dir, "key_q.png"), 'Q')
    generate_keycap(os.path.join(buttons_dir, "key_e.png"), 'E')
    generate_keycap(os.path.join(buttons_dir, "key_c.png"), 'C')
    generate_keycap(os.path.join(buttons_dir, "key_f.png"), 'F')

    generate_keycap(os.path.join(buttons_dir, "key_up.png"), 'UP')
    generate_keycap(os.path.join(buttons_dir, "key_down.png"), 'DOWN')
    generate_keycap(os.path.join(buttons_dir, "key_left.png"), 'LEFT')
    generate_keycap(os.path.join(buttons_dir, "key_right.png"), 'RIGHT')

    generate_keycap(os.path.join(buttons_dir, "key_space.png"), 'SPACE', width=220, height=100)
    generate_keycap(os.path.join(buttons_dir, "key_enter.png"), 'RET', width=140, height=100)
    generate_keycap(os.path.join(buttons_dir, "key_esc.png"), 'ESC', width=110, height=100)
    generate_keycap(os.path.join(buttons_dir, "key_tab.png"), 'TAB', width=130, height=100)

    print("All button and keycap PNG icons successfully created in data/media/menu/buttons/!")

if __name__ == "__main__":
    main()
