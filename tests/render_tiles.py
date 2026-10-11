"""Compare tiled LCD rendering with full-frame pixels, including both fallback tile sizes."""

import copy, importlib.util, subprocess, os
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
cfg.update(snmpVersion="3", snmpPrivacy="DES")
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
        dict(
            name=f"Disk {i}",
            model="" if i == 1 else "HDD",
            temperature=41 + i,
            status=1,
            health=1,
        )
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
        if page == 0:
            assert any(
                op[0] == "round"
                and op[1] == 128
                and op[2] == 204
                and op[3] == 9
                and op[-1] == 0x87E8
                for op in full
            ), "Encrypted lock missing with empty disk model"
        if page == 0:
            assert any(
                op[0] == "circle" and op[1] == 143 and op[2] == 205 for op in full
            ), "Status dot missing"
        expected = pixels(full)
        for tile in (15, 5):
            result = original_run(
                [
                    os.environ.get(
                        "MOS2_RENDERER",
                        str(
                            root
                            / "simulator"
                            / (
                                "renderer.exe"
                                if sim.sys.platform == "win32"
                                else "renderer"
                            )
                        ),
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
asymmetric = copy.deepcopy(data)
asymmetric["history"] = [[3_000_000 + i * 100_000, 10_000 + i * 2000] for i in range(10)]
for page, top, bottom in [(0, 38, 112), (1, 112, 182)]:
    commands = sim.render(page, asymmetric, cfg)
    heights = [op[2] for op in commands if op[0] == "circle" and op[-1] == 0xa9e7 and top <= op[2] < bottom]
    middle = (top + bottom) // 2
    assert all(middle + 2 <= y < bottom - 2 for y in heights), heights
    green = [op[2] for op in commands if op[0] == "circle" and op[-1] == 0x5d85 and top <= op[2] < bottom]
    assert len(green) == 10 and all(top + 2 <= y <= middle - 2 for y in green), green
    assert len(heights) == 10 and max(heights) - min(heights) >= 12, heights
print("PASS: upload trend stays visible alongside downloads 100 times faster")

# Each filled column reaches the curve without a pale gap; all dots cover lines.
for page, top, bottom in [(0, 38, 112), (1, 112, 182)]:
    commands = sim.render(page, asymmetric, cfg)
    middle = (top + bottom) // 2
    for color, low, high, base in [(0x5d85, top, middle, middle-4), (0xa9e7, middle, bottom, bottom-6)]:
        dots = [(i, op) for i, op in enumerate(commands) if op[0] == "circle" and op[-1] == color and low <= op[2] < high]
        lines = [(i, op) for i, op in enumerate(commands) if op[0] == "line" and op[-1] == color and low <= op[2] < high]
        assert lines and max(i for i, _ in lines) < min(i for i, _ in dots)
        points = [op[1:3] for _, op in dots]
        for (xa, ya), (xb, yb) in zip(points, points[1:]):
            for x in range(xa, xb+1):
                curve = ya + int((yb-ya)*(x-xa)/(xb-xa))
                blocks = [op for op in commands if op[0] == "rect" and op[1] == x and op[3] == 1 and low <= op[2] < high]
                covered = {y for op in blocks for y in range(op[2], op[2]+op[4])}
                assert all(y in covered for y in range(curve, base+1)), (page, x, curve)
                fill_color = 0x87e8 if color == 0x5d85 else 0xfacb
                background = 0xef9d
                alpha = 255
                mixed = (((((fill_color >> 11) * alpha + (background >> 11) * (255-alpha)) // 255) << 11)
                         | (((((fill_color >> 5 & 63) * alpha + (background >> 5 & 63) * (255-alpha)) // 255) << 5))
                         | (((fill_color & 31) * alpha + (background & 31) * (255-alpha)) // 255))
                assert any(op[2] == curve and op[-1] == mixed for op in blocks), (page, x)
print("PASS: solid dots, matching lines, continuous gradient peaks match arrow colors")

one = copy.deepcopy(data)
one["history"] = [[4096, 2048]]
for page, top, bottom in [(0, 38, 112), (1, 112, 182)]:
    commands = sim.render(page, one, cfg)
    dots = [op for op in commands if op[0] == "circle" and op[-1] in (0x5d85, 0xa9e7) and top <= op[2] < bottom]
    assert len(dots) == 2
    assert not any(op[0] == "line" and op[-1] in (0x5d85, 0xa9e7) and top <= op[2] < bottom for op in commands)
print("PASS: first measured rate displays dots without fabricated history")
