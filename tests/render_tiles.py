"""Compare tiled LCD rendering with full-frame pixels, including both fallback tile sizes."""

import copy, importlib.util, subprocess
from pathlib import Path
from PIL import Image, ImageDraw

root = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("sim", root / "simulator/server.py")
sim = importlib.util.module_from_spec(spec)
spec.loader.exec_module(sim)
original_run = subprocess.run
payload = None


def capture(*args, **kwargs):
    global payload
    payload = kwargs["input"]
    return original_run(*args, **kwargs)


def pixels(commands):
    result = Image.new("RGB", (240, 240))
    layer = Image.new("RGB", (240, 240))
    draw = ImageDraw.Draw(layer)
    top, bottom = 0, 240
    for op, *v in commands + [["clip", 240, 240]]:
        if op == "clip":
            result.paste(layer.crop((0, top, 240, bottom)), (0, top))
            top, bottom = v
            continue
        if op == "text":
            continue
        color = v[-1]
        color = (
            (color >> 11 & 31) * 255 // 31,
            (color >> 5 & 63) * 255 // 63,
            (color & 31) * 255 // 31,
        )
        if op in ("rect", "round"):
            x, y, w, h = v[:4]
            box = (x, y, x + w - 1, y + h - 1)
            if op == "round":
                draw.rounded_rectangle(box, radius=v[4], fill=color)
            else:
                draw.rectangle(box, fill=color)
        elif op == "circle":
            x, y, r = v[:3]
            draw.ellipse((x - r, y - r, x + r, y + r), fill=color)
        elif op == "line":
            draw.line(v[:4], fill=color)
        elif op == "triangle":
            draw.polygon([(v[i], v[i + 1]) for i in (0, 2, 4)], fill=color)
        else:
            raise AssertionError(op)
    return result.tobytes()


cfg = copy.deepcopy(sim.config)
data = copy.deepcopy(sim.state)
data.update(
    cpu=42,
    memory=27.4,
    temperature=80,
    rx=126000,
    tx=23000,
    online=True,
    model="Synology",
    interface="eth0",
    memoryTotal=16 * 1024**3,
    memoryAvail=12 * 1024**3,
    history=[[i * 1000, i * 500] for i in range(30)],
    weather=copy.deepcopy(sim.weather),
    alarm=dict(state=0, remaining=0),
    disks=[
        dict(name=f"Disk {i}", model="HDD", temperature=41 + i, status=1, health=1)
        for i in range(1, 5)
    ],
    volumes=[
        dict(name=f"/volume{i}", total=1000000, used=200000 * i) for i in range(1, 4)
    ],
)
for language in ("zh", "en"):
    cfg["language"] = language
    for page in (0, 1, 2, 8, 9, 4):
        sim.subprocess.run = capture
        full = sim.render(page, data, cfg)
        sim.subprocess.run = original_run
        expected = pixels(full)
        for tile in (15, 5):
            result = original_run(
                [
                    str(
                        root
                        / "simulator"
                        / (
                            "renderer.exe"
                            if sim.sys.platform == "win32"
                            else "renderer"
                        )
                    ),
                    str(tile),
                ],
                input=payload,
                capture_output=True,
                check=True,
            )
            commands = []
            for line in result.stdout.decode("utf8").splitlines():
                parts = line.split("\t")
                if parts[0] != "text":
                    commands.append([parts[0]] + list(map(int, parts[1:])))
            actual = pixels(commands)
            if actual != expected:
                from PIL import ImageChops

                before = Image.frombytes("RGB", (240, 240), expected)
                after = Image.frombytes("RGB", (240, 240), actual)
                print(
                    "Difference bounds:", ImageChops.difference(before, after).getbbox()
                )
            assert actual == expected, (language, page, tile)
print(
    "PASS: all six NAS pages, Chinese/English, 15px/5px tiles match full-frame pixels"
)
