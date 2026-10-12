"""Build a standalone Windows simulator matching the current firmware sources."""

import argparse, hashlib, json, shutil, subprocess, sys, tempfile
from pathlib import Path

root = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser()
p.add_argument("--cxx", default="g++")
p.add_argument("--output", type=Path, default=root / "dist")
p.add_argument("--firmware", type=Path, default=root / ".pio/build/mos2/firmware.bin")
args = p.parse_args()
if sys.platform != "win32":
    p.error(
        "The downloadable simulator is a Windows build; source simulator also supports Linux."
    )
if not args.firmware.is_file():
    p.error("Build firmware first or supply --firmware.")
out = args.output.resolve()
out.mkdir(parents=True, exist_ok=True)
# Windows antivirus may briefly retain a handle to the generated executable.
# Cleanup failure must not discard an otherwise successful build.
with tempfile.TemporaryDirectory(ignore_cleanup_errors=True) as tmp:
    tmp = Path(tmp)
    renderer = tmp / "renderer.exe"
    subprocess.run(
        [
            args.cxx,
            "-std=c++17",
            "-O2",
            "-static",
            "-I" + str(root / "include"),
            "-I" + str(root / "simulator/native"),
            str(root / "simulator/native/renderer.cpp"),
            "-o",
            str(renderer),
        ],
        check=True,
    )
    subprocess.run(
        [
            sys.executable,
            "-m",
            "PyInstaller",
            "--noconfirm",
            "--clean",
            "--onedir",
            "--name",
            "MOS2-NAS-Simulator",
            "--distpath",
            str(out),
            "--workpath",
            str(tmp / "build"),
            "--specpath",
            str(tmp),
            "--paths",
            str(root / "tools"),
            "--hidden-import",
            "qrcode",
            "--collect-all",
            "Crypto",
            "--add-data",
            str(root / "web") + ";web",
            "--add-data",
            str(root / "data") + ";data",
            "--add-data",
            str(root / "simulator/index.html") + ";simulator",
            "--add-data",
            str(root / "simulator/qr-locales.json") + ";simulator",
            "--add-data",
            str(root / "simulator/assets") + ";simulator/assets",
            "--add-binary",
            str(renderer) + ";simulator",
            str(root / "simulator/server.py"),
        ],
        check=True,
    )
folder = out / "MOS2-NAS-Simulator"
shutil.copy2(args.firmware, folder / "MOS2-NAS-1.2.0.bin")
shutil.copy2(root / "simulator/README.md", folder / "README.md")
shutil.copy2(root / "NOTICE.md", folder / "NOTICE.md")
manifest = dict(
    version="1.2.0",
    firmwareSha256=hashlib.sha256(args.firmware.read_bytes()).hexdigest(),
    commit=subprocess.check_output(
        ["git", "rev-parse", "HEAD"], cwd=root, text=True
    ).strip(),
    mode="shared-source-renderer",
)
(folder / "build-info.json").write_text(
    json.dumps(manifest, indent=2) + "\n", encoding="utf8"
)
archive = shutil.make_archive(
    str(out / "MOS2-NAS-Simulator-1.2.0-Windows"), "zip", out, folder.name
)
print(archive)
