"""Write independent semantic/size banks and their firmware routing metadata."""
from pathlib import Path
import json, struct, zlib
from font_charset import glyph_categories

GROUPS = ("latin", "interface", "weather", "geography")
SIZES = (12, 16, 20, 26)

def write_banks(root, codes, rasters, audit):
    directory = root / "data/fonts"
    directory.mkdir(parents=True, exist_ok=True)
    groups = glyph_categories(codes)
    lines = ["#pragma once", "#include <Arduino.h>", "namespace Cjk {"]
    def array(kind, name, values):
        lines.append(f"const {kind} {name}[] PROGMEM={{" + ",".join(map(str, values)) + "};")
    array("uint16_t", "codes", codes)
    array("uint16_t", "codes20", codes)
    array("uint8_t", "glyphGroups", groups)
    banks = []
    for cell in SIZES:
        data, offsets, advances = rasters[cell]
        firsts, lengths = [0]*len(codes), [0]*len(codes)
        for group, name in enumerate(GROUPS):
            payload = bytearray()
            for i in range(len(codes)):
                if groups[i] != group: continue
                firsts[i] = len(payload)
                glyph = data[offsets[i]:offsets[i+1]]
                lengths[i] = len(glyph)
                payload.extend(glyph)
            crc = zlib.crc32(payload)
            bank_id = len(banks)
            filename = f"{name}-{cell}.bin"
            content = struct.pack("<8s4I", b"MOSFBK03", 24+len(payload), crc, bank_id, 16)+payload
            (directory / filename).write_bytes(content)
            banks.append(dict(file=filename, group=name, cell=cell, bytes=len(content), crc32=crc, glyphs=groups.count(group)))
        array("uint32_t", f"offsets{cell}", firsts)
        array("uint16_t", f"lengths{cell}", lengths)
        array("uint8_t", f"advances{cell}", advances)
    total = 24 + sum(b["bytes"] for b in banks)
    identity = b"".join(b["file"].encode()+b"\0"+(directory/b["file"]).read_bytes() for b in banks)
    crc = zlib.crc32(identity)
    (directory / "catalog.bin").write_bytes(struct.pack("<8s4I", b"MOSFSET3", total, crc, len(banks), 16))
    array("uint32_t", "bankBytes", [b["bytes"] for b in banks])
    array("uint32_t", "bankCrc", [b["crc32"] for b in banks])
    lines += [f"const uint32_t resourceBytes={total}u,resourceCrc={crc}u;", "}"]
    (root/"include/font_index.h").write_text("\n".join(lines)+"\n", encoding="utf8")
    (directory/"manifest.json").write_text(json.dumps(dict(version=3,levels=16,bytes=total,crc32=crc,primaryGlyphs=len(codes),commonGlyphs=len(codes),fallbackGlyphs=0,sizes=audit,banks=banks),indent=2)+"\n",encoding="utf8")
    assert total <= 1600*1024, "Reserve space for seven photos and user data"
    # Only this exact obsolete generated resource is removed; no user data lives here.
    (directory/"ui04.bin").unlink(missing_ok=True)
    print(f"Font banks: {len(banks)}, {len(codes)} glyphs, {total} bytes")
    from embed_font_hot import generate
    generate(root)
