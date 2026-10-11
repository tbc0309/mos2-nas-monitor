"""Create one raw USB image which management also accepts as an OTA container."""
import argparse
import struct
import zlib
from pathlib import Path

root = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser()
p.add_argument('--program', type=Path, default=root / '.pio/build/mos2/firmware.bin')
p.add_argument('--filesystem', type=Path, default=root / '.pio/build/mos2/littlefs.bin')
p.add_argument('--output', type=Path, required=True)
a = p.parse_args()
program, filesystem = a.program.read_bytes(), a.filesystem.read_bytes()
if not 4096 <= len(program) <= 0x100000 or program[0] != 0xe9:
    p.error('Invalid application image')
if len(filesystem) != 0x1fa000:
    p.error('Expected the 4m2m filesystem layout')
image = bytearray(b'\xff' * 0x3fa000)
image[:len(program)] = program
image[0x200000:] = filesystem
# This metadata is in the unused OTA staging area, not SDK/RF sectors.
image[0x100000:0x100018] = struct.pack('<8sIIII', b'MOS2ALL1', len(program), zlib.crc32(program), zlib.crc32(filesystem), 1)
a.output.parent.mkdir(parents=True, exist_ok=True)
a.output.write_bytes(image)
print(f'{a.output.name}: {len(image)} bytes; application {len(program)} bytes')
