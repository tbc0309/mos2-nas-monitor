"""Every LCD translation must use native W7 at all four sizes."""
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
index = (root / "include/font_index.h").read_text(encoding="utf8")
def alphabet(name):
    return set(map(int, re.search(r"\b" + name + r"\[.*?=\{(.*?)\};", index)[1].split(",")))
common, large = alphabet("codes"), alphabet("codes20")
assert large <= common
for directory in ("include/locales", "src/app", "simulator"):
    for path in (root / directory).rglob("*"):
        if path.suffix not in (".h", ".cpp", ".json"):
            continue
        text = path.read_text(encoding="utf8")
        missing = {ch for ch in text if 0x4e00 <= ord(ch) <= 0x9fff and ord(ch) not in large}
        assert not missing, (path, sorted(missing))
assert all(ord(ch) in large for ch in "默认密码已修改周一二三四五六日")
print(f"PASS: native W7 coverage: {len(common)} common glyphs, {len(large)} UI glyphs at all sizes")
