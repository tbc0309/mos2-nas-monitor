"""Generate native-hinted W7 16-level filesystem resources. Set FONT_PATH.

Native W7 covers all common Chinese at small sizes and every UI glyph at all sizes.
Regenerate the resource and matching firmware index together.
"""

from pathlib import Path
import json, math, os, re, struct, zlib
import freetype as ft

root = Path(__file__).resolve().parents[1]
index_path = root / "include/font_index.h"
pack_path = root / "data/fonts/ui04.bin"
old = index_path.read_text(encoding="utf8")
font_path = Path(os.environ.get("FONT_PATH", "TencentSans-W7.ttf"))
if not font_path.is_file():
    raise SystemExit("Set FONT_PATH to your licensed Tencent Sans W7 font.")


def table(name):
    return list(
        map(int, re.search(r"\b" + name + r"\[.*?=\{(.*?)\};", old, re.S)[1].split(","))
    )


# Broad coverage for city names and dynamic small labels; all UI glyphs at large sizes.
core = set(table("codes20")) | set(range(32, 127))
for folder in [root / "include", root / "src", root / "simulator"]:
    for path in folder.rglob("*"):
        if path.suffix not in (".h", ".cpp", ".json") or path.name == "font_index.h":
            continue
        core.update(ord(ch) for ch in path.read_text(encoding="utf8") if 0x4e00 <= ord(ch) <= 0x9fff or 0x3000 <= ord(ch) <= 0x303f or 0xff00 <= ord(ch) <= 0xffef)
codes = sorted(set(table("codes")) | core)
core = sorted(core)
payload = bytearray()
lines = ["#pragma once", "#include <Arduino.h>", "namespace Cjk {"]
for name, values in [("codes", codes), ("codes20", core)]:
    lines.append(
        f"const uint16_t {name}[] PROGMEM={{" + ",".join(map(str, values)) + "};"
    )
face = ft.Face(str(font_path))
flags = ft.FT_LOAD_RENDER | ft.FT_LOAD_TARGET_NORMAL
audit = []
locations = {}
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
    locations[f"resourceData{cell}"] = 24 + len(payload)
    payload.extend(data)
    lines.extend(
        [
            f"const uint32_t offsets{cell}[] PROGMEM={{"
            + ",".join(map(str, offsets))
            + "};",
            f"const uint8_t advances{cell}[] PROGMEM={{"
            + ",".join(map(str, advances))
            + "};",
        ]
    )
    audit.append(
        dict(
            cell=cell,
            pixels=pixels,
            baseline=ascent,
            bytes=len(data),
            intermediatePixels=middle,
        )
    )
crc = zlib.crc32(payload)
resource = struct.pack("<8s4I", b"MOSFONT2", 24 + len(payload), crc, 2, 16) + payload
lines.extend(f"const uint32_t {name}={value}u;" for name, value in locations.items())
lines.extend(
    [f"const uint32_t resourceBytes={len(resource)}u,resourceCrc={crc}u;", "}"]
)
assert len(resource) <= 1600 * 1024, "Reserve space for seven photos and user data"
pack_path.write_bytes(resource)
index_path.write_text("\n".join(lines), encoding="utf8")
(pack_path.parent / "manifest.json").write_text(
    json.dumps(
        dict(
            version=2,
            levels=16,
            bytes=len(resource),
            crc32=crc,
            primaryGlyphs=len(core),
            commonGlyphs=len(codes),
            fallbackGlyphs=0,
            sizes=audit,
        ),
        indent=2,
    )
    + "\n",
    encoding="utf8",
)
print("W7 resource:", len(resource), "bytes, CRC32", hex(crc), "glyphs", len(core))
