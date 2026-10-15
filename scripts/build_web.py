"""Build the static installer from one verified release image."""

import argparse
import hashlib
import json
import re
import shutil
import struct
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--firmware-dir', type=Path, required=True)
parser.add_argument('--output', type=Path, default=Path('_site'))
args = parser.parse_args()
candidates = list(args.firmware_dir.glob('MOS2-NAS-*.bin'))
if len(candidates) != 1:
    raise SystemExit('Provide exactly one released MOS2-NAS image')
firmware = candidates[0]
match = re.fullmatch(r'MOS2-NAS-(\d+\.\d+\.\d+)\.bin', firmware.name)
if not match:
    raise SystemExit('Unexpected release asset name')
version = match.group(1)
image = firmware.read_bytes()
if len(image) != 0x3FA000 or image[0] != 0xE9:
    raise SystemExit('Unexpected full image size or ESP header')
magic, size, app_crc, fs_crc, schema = struct.unpack_from('<8sIIII', image, 0x100000)
if magic != b'MOS2ALL1' or schema != 1 or not 0 < size <= 0x100000:
    raise SystemExit('Invalid full image manifest')
# The full container includes both the application and every filesystem resource.
if zlib.crc32(image[:size]) != app_crc or zlib.crc32(image[0x200000:]) != fs_crc:
    raise SystemExit('Image CRC mismatch')
source = ROOT / 'web'
output = args.output.resolve()
if output == ROOT or output.name not in ('_site', 'web-preview'):
    raise SystemExit('Choose a dedicated _site or web-preview directory')
output.mkdir(parents=True, exist_ok=True)
html = (source / 'index.html').read_text('utf-8')
# Cache-busted assets avoid mixing old markup with new scripts on Pages.
for name in ('app.js', 'style.css'):
    data = (source / name).read_bytes()
    hashed = Path(name).stem + '-' + hashlib.sha256(data).hexdigest()[:12] + Path(name).suffix
    (output / hashed).write_bytes(data)
    html = html.replace(name, hashed)
(output / 'index.html').write_text(html, 'utf-8')
shutil.copytree(source / 'vendor', output / 'vendor', dirs_exist_ok=True)
shutil.copytree(source / 'assets', output / 'assets', dirs_exist_ok=True)
(output / 'licenses').mkdir(exist_ok=True)
shutil.copy2(source / 'node_modules/esp-web-tools/LICENSE', output / 'licenses/ESP-Web-Tools-LICENSE')
(output / 'firmware').mkdir(exist_ok=True)
(output / 'firmware' / firmware.name).write_bytes(image)
metadata = {'version': version, 'size': len(image), 'sha256': hashlib.sha256(image).hexdigest()}
manifest = {
    'name': 'MOS2-NAS', 'version': version, 'new_install_prompt_erase': True,
    'new_install_improv_wait_time': 0,
    'builds': [{'chipFamily': 'ESP8266', 'improv': False,
                'parts': [{'path': 'firmware/' + firmware.name, 'offset': 0}]}],
}
for name, value in [('firmware.json', metadata), ('manifest.json', manifest)]:
    (output / name).write_text(json.dumps(value, indent=2) + '\n', 'utf-8')
(output / '.nojekyll').touch()
print(f'Built verified MOS2-NAS {version} installer')
