from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
html = (root / "web/management.html").read_text(encoding="utf8")
assert ')MOSWEB"' not in html
raw = html.encode("utf8")
segments = []
pos = 0
for m in re.finditer(rb"\{\{(token|config|credentials|environment)\}\}", raw):
    segments.append((pos, m.start() - pos, m[1].decode()))
    pos = m.end()
segments.append((pos, len(raw) - pos, ""))
source = (
    '#pragma once\nconst char managementHTML[] PROGMEM = R"MOSWEB('
    + html
    + ')MOSWEB";\n'
)
source += "struct WebSegment {uint32_t offset,length;uint8_t key;};\nconst WebSegment webSegments[] PROGMEM={\n"
source += (
    "\n".join(
        "{%d,%d,%d},"
        % (
            start,
            length,
            {"": 0, "token": 1, "config": 2, "credentials": 3, "environment": 4}[key],
        )
        for start, length, key in segments
    )
    + "\n};\n"
)
(root / "include/webui.h").write_text(source, encoding="utf8")
for filename, symbol in [
    ("access-i18n.js", "accessLocaleJS"),
    ("i18n.js", "localeJS"),
    ("management.js", "managementJS"),
    ("firmware-bundle.js", "firmwareBundleJS"),
    ("setup.js", "setupJS"),
    ("access.js", "accessJS"),
]:
    js = (root / "web" / filename).read_text(encoding="utf8")
    source += "const char " + symbol + '[] PROGMEM = R"MOSJS(' + js + ')MOSJS";\n'
(root / "include/webui.h").write_text(source, encoding="utf8")

source += (
    'const char accessHTML[] PROGMEM = R"MOSWEB('
    + (root / "web/access.html").read_text(encoding="utf8")
    + ')MOSWEB";\n'
)
(root / "include/webui.h").write_text(source, encoding="utf8")
