"""Reject broken resource packs before attempting to decode glyph data."""
from pathlib import Path
import os, subprocess, sys, tempfile
root = Path(__file__).resolve().parents[1]
data = (root / "data/fonts/ui04.bin").read_bytes()
with tempfile.TemporaryDirectory() as tmp:
    tmp = Path(tmp)
    for name,content in [("truncated",data[:-1]),("header",b"INVALID!"+data[8:]),
                         ("payload",data[:100]+bytes([data[100]^1])+data[101:])]:
        p=tmp/name;p.write_bytes(content)
        subprocess.run([sys.argv[1],"invalid"],env={**os.environ,"MOS2_FONT_RESOURCE":str(p)},check=True)
    subprocess.run([sys.argv[1],"invalid"],env={**os.environ,"MOS2_FONT_RESOURCE":str(tmp/"missing")},check=True)
