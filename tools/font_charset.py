"""Deterministic LCD alphabet; never union an older generated font index."""
from pathlib import Path
import json, re
ROOT = Path(__file__).resolve().parents[1]

def required_codes():
    text = "°℃·…？：％–—□"
    sources = [ROOT / "include/locales/lcd_zh.h", ROOT / "include/dashboard.h"]
    sources += list((ROOT / "src/app").glob("*.h"))
    for path in sources:
        source = path.read_text(encoding="utf8")
        # Only string literals: comments do not determine the firmware alphabet.
        source = re.sub(r"//[^\n]*|/\*.*?\*/", "", source, flags=re.S)
        text += "".join(re.findall(r'"((?:[^"\\]|\\.)*)"', source))
    text += (ROOT / "simulator/qr-locales.json").read_text(encoding="utf8")
    text += (ROOT / "tools/font-extra.txt").read_text(encoding="utf8")
    for filename in ("china-cities.json", "conditions.json", "wind.json"):
        text += (ROOT / "tools/weather" / filename).read_text(encoding="utf8")
    return sorted(set(range(32, 127)) | {ord(c) for c in text if ord(c) > 127})


def glyph_categories(codes):
    """Exclusive semantic banks; shared glyphs belong to the first matching group."""
    ui = "°℃·…？：％–—□"
    for path in [ROOT / "include/locales/lcd_zh.h", ROOT / "include/dashboard.h", *sorted((ROOT / "src/app").rglob("*.h"))]:
        source = re.sub(r"//[^\n]*|/\*.*?\*/", "", path.read_text(encoding="utf8"), flags=re.S)
        ui += "".join(re.findall(r'"((?:[^"\\]|\\.)*)"', source))
    ui += (ROOT / "simulator/qr-locales.json").read_text(encoding="utf8")
    weather = "".join((ROOT / "tools/weather" / n).read_text(encoding="utf8") for n in ("conditions.json", "wind.json"))
    sets = [set(range(32,127)), {ord(c) for c in ui if ord(c)>127}, {ord(c) for c in weather if ord(c)>127}]
    return [next((i for i, group in enumerate(sets) if c in group), 3) for c in codes]
