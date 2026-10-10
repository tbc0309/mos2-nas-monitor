"""Export simulator drawing commands directly to PNG, without browser screenshots."""
import copy
import importlib.util
from pathlib import Path
from PIL import Image, ImageDraw

root = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("screen_sim", root / "simulator/server.py")
sim = importlib.util.module_from_spec(spec)
spec.loader.exec_module(sim)
cfg = copy.deepcopy(sim.config)
cfg.update(alarmEnabled=1, alarmTime="07:00")
data = copy.deepcopy(sim.state)
data.update(alarm=dict(state=0, remaining=0), disks=[], volumes=[], weather=copy.deepcopy(sim.weather))
data["weather"].update(valid=True, error="", city="天津", cityEnglish="Tianjin", condition="晴", conditionEnglish="Sunny", wind="南风2级", windEnglish="S 2", temperature=22, humidity=55, aqi=32, code=0)

def rgb(value):
    return ((value >> 11 & 31) * 255 // 31, (value >> 5 & 63) * 255 // 63, (value & 31) * 255 // 31)

for page, name in [(6, "desktop-clock"), (5, "weather-clock")]:
    image = Image.new("RGB", (240, 240))
    draw = ImageDraw.Draw(image)
    for op, *values in sim.render(page, data, cfg):
        if op == "text":
            continue  # Text is already rasterized into rectangles by the shared firmware font renderer.
        color = rgb(values[-1])
        if op in ("rect", "round"):
            x, y, w, h = values[:4]
            box = (x, y, x + w - 1, y + h - 1)
            if op == "round":
                draw.rounded_rectangle(box, radius=values[4], fill=color)
            else:
                draw.rectangle(box, fill=color)
        elif op == "circle":
            x, y, radius = values[:3]
            draw.ellipse((x-radius, y-radius, x+radius, y+radius), fill=color)
        elif op == "line":
            draw.line(values[:4], fill=color)
        elif op == "triangle":
            draw.polygon([(values[i], values[i+1]) for i in (0, 2, 4)], fill=color)
        else:
            raise ValueError("Unknown simulator primitive: " + op)
    # Keep the screen at its native 240 x 240 resolution.
    target = root / "docs/images" / (name + ".png")
    image.save(target)
    print(target.name)
