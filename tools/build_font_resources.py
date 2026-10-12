"""Generate native-hinted W7 16-level filesystem resources. Set FONT_PATH.

Native W7 covers screen strings plus an explicit geography/weather extension.
Regenerate the resource and matching firmware index together.
"""

from pathlib import Path
import math, os
import freetype as ft

root = Path(__file__).resolve().parents[1]
from font_charset import required_codes
font_path = Path(os.environ.get("FONT_PATH", "TencentSans-W7.ttf"))
if not font_path.is_file():
    raise SystemExit("Set FONT_PATH to your licensed Tencent Sans W7 font.")
# One compact alphabet at every size; no legacy bitmap/font bank is retained.
codes = required_codes()
core = codes
rasters = {}
face = ft.Face(str(font_path))
flags = ft.FT_LOAD_RENDER | ft.FT_LOAD_TARGET_NORMAL
audit = []
for cell, request in [(12, 11), (16, 14), (20, 20), (26, 24)]:
    glyphs = codes if cell < 20 else core
    for pixels in range(request, 6, -1):
        face.set_pixel_sizes(0, pixels)
        ascent = descent = wide = 0
        for code in glyphs:
            assert face.get_char_index(code), f"Missing U+{code:04X}"
            face.load_char(chr(code), flags)
            g = face.glyph
            b = g.bitmap
            ascent = max(ascent, g.bitmap_top)
            descent = max(descent, b.rows - g.bitmap_top)
            wide = max(wide, b.width)
        if ascent + descent <= cell and wide <= cell:
            break
    else:
        raise ValueError("No native raster fits the UI cell")
    data = []
    offsets = [0]
    advances = []
    middle = 0
    for code in glyphs:
        face.load_char(chr(code), flags)
        g = face.glyph
        b = g.bitmap
        buffer = b.buffer
        shift = max(0, -g.bitmap_left)
        left = g.bitmap_left + shift
        top = ascent - g.bitmap_top
        assert (
            0 <= left and left + b.width <= cell and 0 <= top and top + b.rows <= cell
        )
        advance = (
            cell
            if code >= 128
            else max(2, math.ceil(g.advance.x / 64) + shift, left + b.width)
        )
        assert advance <= cell
        advances.append(advance)
        values = [0] * (cell * cell)
        for y in range(b.rows):
            row = y if b.pitch >= 0 else b.rows - 1 - y
            for x in range(b.width):
                alpha = round(buffer[row * abs(b.pitch) + x] * 15 / 255)
                values[(top + y) * cell + left + x] = alpha
                middle += 0 < alpha < 15
        # Zero runs and packed literal nibbles retain all sixteen coverage levels.
        i = 0
        while i < len(values):
            n = 1
            if values[i] == 0:
                while n < 128 and i + n < len(values) and values[i + n] == 0:
                    n += 1
                data.append(n - 1)
            else:
                while n < 128 and i + n < len(values):
                    if values[i + n : i + n + 3] == [0, 0, 0]:
                        break
                    n += 1
                data.append(128 | (n - 1))
                for j in range(0, n, 2):
                    data.append(
                        values[i + j] | ((values[i + j + 1] if j + 1 < n else 0) << 4)
                    )
            i += n
        offsets.append(len(data))
    rasters[cell] = (bytes(data), offsets, advances)
    audit.append(
        dict(
            cell=cell,
            pixels=pixels,
            baseline=ascent,
            bytes=len(data),
            intermediatePixels=middle,
        )
    )
from font_banks import write_banks
write_banks(root, codes, rasters, audit)
