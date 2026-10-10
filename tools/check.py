"""Run firmware component tests and isolated simulator integration tests."""

import argparse, os, subprocess, sys, tempfile
from pathlib import Path

root = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser()
p.add_argument("--cxx", default=os.environ.get("CXX", "g++"))
args = p.parse_args()


def run(argv):
    subprocess.run([str(x) for x in argv], cwd=root, check=True)


run([sys.executable, root / "tools/embed_locales.py"])

with tempfile.TemporaryDirectory() as folder:
    for name, source, includes in [
        ("touch", "tests/touch.cpp", ["include"]),
        ("alarm", "tests/alarm.cpp", ["include"]),
        ("storage", "tests/storage.cpp", ["include"]),
        ("resources", "tests/resources.cpp", ["include"]),
        ("font", "tests/font.cpp", ["include", "simulator/native"]),
        ("snmp", "tests/host/test.cpp", ["include", "tests/host"]),
        (
            "weather",
            "tests/weather/test.cpp",
            ["include", "tests/weather", ".pio/libdeps/mos2/ArduinoJson/src"],
        ),
    ]:
        target = Path(folder) / (name + ".exe" if os.name == "nt" else name)
        run(
            [
                args.cxx,
                "-std=c++17",
                "-O2",
                *["-I" + str(root / i) for i in includes],
                root / source,
                "-o",
                target,
            ]
        )
        run([target])
    renderer = root / "simulator" / ("renderer.exe" if os.name == "nt" else "renderer")
    run(
        [
            args.cxx,
            "-std=c++17",
            "-O2",
            "-I" + str(root / "include"),
            "-I" + str(root / "simulator/native"),
            root / "simulator/native/renderer.cpp",
            "-o",
            renderer,
        ]
    )
    run([sys.executable, root / "tests/simulator.py"])
    run([sys.executable, root / "tests/render_tiles.py"])
print("All component and simulator checks passed.")
