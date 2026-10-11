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
run([sys.executable, root / "tools/embed_web.py"])
run(["node", root / "tests/firmware_bundle.js"])
run([sys.executable, root / "tests/font_coverage.py"])

with tempfile.TemporaryDirectory() as folder:
    des_objects = []
    for cipher in (
        "des_ct",
        "des_ct_cbcenc",
        "des_ct_cbcdec",
        "des_support",
        "sha1",
        "aes_ct",
        "aes_ct_enc",
        "aes_ct_cbcenc",
    ):
        obj = Path(folder) / (cipher + ".o")
        run(
            [
                str(Path(args.cxx).with_name("gcc.exe" if os.name == "nt" else "gcc")),
                "-O2",
                "-I" + str(root / "tests/vendor/bearssl"),
                "-c",
                root / "tests/vendor/bearssl" / (cipher + ".c"),
                "-o",
                obj,
            ]
        )
        des_objects.append(obj)
    for name, source, includes in [
        ("firmware_image", "tests/firmware_image.cpp", ["include"]),
        ("touch", "tests/touch.cpp", ["include"]),
        ("alarm", "tests/alarm.cpp", ["include"]),
        ("storage", "tests/storage.cpp", ["include"]),
        ("resources", "tests/resources.cpp", ["include"]),
        ("poll_schedule", "tests/poll_schedule.cpp", ["include"]),
        ("startup_plan", "tests/startup_plan.cpp", ["include"]),
        ("font", "tests/font.cpp", ["include", "simulator/native"]),
        ("font_resource", "tests/font_resource.cpp", ["include", "simulator/native"]),
        (
            "snmp_v3",
            "tests/host/usm.cpp",
            ["include", "tests/host", "tests/vendor/bearssl"],
        ),
        (
            "snmp",
            "tests/host/test.cpp",
            ["include", "tests/host", "tests/vendor/bearssl"],
        ),
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
                *(des_objects if name in ("snmp", "snmp_v3") else []),
                "-o",
                target,
            ]
        )
        run([target])
        if name == "font_resource":
            run([sys.executable, root / "tests/font_resource.py", target])
    # Use an isolated binary: a running simulator may lock its renderer on Windows.
    renderer = Path(folder) / ("renderer.exe" if os.name == "nt" else "renderer")
    os.environ["MOS2_RENDERER"] = str(renderer)
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
    run([sys.executable, root / "tests/snmp_v3.py"])
    run([sys.executable, root / "tests/simulator.py"])
    run([sys.executable, root / "tests/render_tiles.py"])
print("All component and simulator checks passed.")
