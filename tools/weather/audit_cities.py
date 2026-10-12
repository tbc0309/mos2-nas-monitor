"""Snapshot the configured Chinese cities and 100 overseas weather endpoints.

This is an offline review tool, never run by the ESP. Requests are paced and
results checkpointed so interrupted reviews do not repeat successful requests.
"""
import argparse, collections, concurrent.futures, datetime, html, json, re
import threading, time, urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
HEADERS = {"Referer": "https://www.weather.com.cn/", "User-Agent": "Mozilla/5.0"}
gate = threading.Lock()
next_request = 0.0


def fetch(url):
    global next_request
    with gate:
        delay = max(0, next_request - time.monotonic())
        if delay: time.sleep(delay)
        next_request = time.monotonic() + .35
    return urllib.request.urlopen(urllib.request.Request(url, headers=HEADERS), timeout=12).read(131072).decode("utf-8-sig")


def overseas():
    source = fetch("https://www.weather.com.cn/forecast/world.shtml")
    cities = {}
    for code, label in re.findall(r'<a[^>]+href=["\'][^"\']*weather(?:1d)?/(\d{9})\.shtml[^"\']*["\'][^>]*>(.*?)</a>', source, re.S):
        if code.startswith(("101", "102")): continue
        name = html.unescape(re.sub('<[^>]+>', '', label)).strip()
        if name and len(name) < 40: cities[code] = dict(code=code, name=name, region="overseas")
    # Official country catalogues supplement the landing-page links.
    for country in ("177", "161", "30", "49", "172", "173", "163"):
        if len(cities) >= 100: break
        raw = fetch(f"https://d1.weather.com.cn/gw/gj{country}.html")
        data = json.loads(raw[raw.index('{'):raw.rindex('}')+1])
        entries = [data.get("sd", {})] + [v for group in data.values() if isinstance(group, list) for v in group]
        # Sample spread across the catalogue, rather than only A-prefixed cities.
        for item in entries[::max(1,len(entries)//12)]:
            code = item.get("id", "")
            if re.fullmatch(r"\d{9}", code) and not code.startswith(("101", "102")):
                cities.setdefault(code, dict(code=code,name=item["name"],english=item.get("en",""),region="overseas"))
            if len(cities) == 100: break
    if len(cities) < 100: raise RuntimeError(f"Only {len(cities)} overseas cities discovered")
    return list(cities.values())[:100]


def probe(city):
    result = dict(city, sampled=datetime.datetime.now().astimezone().isoformat())
    try:
        raw = fetch(f"https://d1.weather.com.cn/weather_index/{city['code']}.html")
        result["bytes"] = len(raw.encode())
        current = re.search(r"var\s+dataSK\s*=\s*(\{.*?\})\s*;", raw, re.S)
        forecast = re.search(r"var\s+fc\s*=\s*(\{.*\})", raw, re.S)
        if current:
            fields = json.loads(current[1]); result.update(type="current",fields=fields)
            keys = ("cityname","nameen","weather","weathere","WD","WS","wde","wse","temp","SD","aqi","time","weathercode")
            result["missing"] = [key for key in keys if fields.get(key) in (None, "")]
            result["cityMatches"] = fields.get("city") == city["code"]
        elif forecast:
            decoder = json.JSONDecoder(); fields,_ = decoder.raw_decode(raw[forecast.start(1):])
            result.update(type="forecast",fields=fields)
        else: result.update(type="unsupported",preview=raw[:160])
    except Exception as error:
        result.update(type="error",error=f"{type(error).__name__}: {error}")
    return result


def main():
    parser=argparse.ArgumentParser();parser.add_argument("--output",type=Path,required=True)
    args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=True)
    target=args.output/'cities.json'
    if target.exists(): cities=json.loads(target.read_text(encoding='utf-8'))
    else:
        cities=[dict(region=p,name=n,code=c) for p,n,c in json.loads((ROOT/'tools/weather/china-cities.json').read_text(encoding='utf-8'))]+overseas()
        target.write_text(json.dumps(cities,ensure_ascii=False,indent=2),encoding='utf-8')
    target=args.output/'samples.json'
    rows=json.loads(target.read_text(encoding='utf-8')) if target.exists() else []
    done={r['code'] for r in rows}
    pending=[r for r in cities if r['code'] not in done]
    with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:
        for result in pool.map(probe,pending):
            rows.append(result)
            target.write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding='utf-8')
            if len(rows)%25==0: print(f"{len(rows)}/{len(cities)} {dict(collections.Counter(r['type'] for r in rows))}",flush=True)
    print(f"Completed {len(rows)}: {dict(collections.Counter(r['type'] for r in rows))}",flush=True)


if __name__=='__main__': main()
