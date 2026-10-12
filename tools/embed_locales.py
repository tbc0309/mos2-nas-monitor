from pathlib import Path
import json, re, html

root = Path(__file__).resolve().parents[1]
locales = {
    p.stem: json.loads(p.read_text(encoding="utf8"))
    for p in sorted((root / "locales").glob("*.json"))
}
# Fail generation when a visible management label has no translation.
keys = set(
    html.unescape(k)
    for k in re.findall(
        r'data-i18n="([^"]+)"',
        (root / "web/management.html").read_text(encoding="utf8"),
    )
)
keys.update(
    re.findall(
        r"msg\(['\"]([^'\"]+)['\"]\)",
        (root / "web/management.js").read_text(encoding="utf8"),
    )
)
# Dynamic group headings and accessible reorder labels also need both languages.
management_js = (root / "web/management.js").read_text(encoding="utf8")
keys.update(re.findall(r"\['([^']+)',\[", management_js))
keys.update(re.findall(r"\['[↑↓]',-?1,'([^']+)'\]", management_js))
for language, messages in locales.items():
    if set(messages) != set(locales["en"]):
        raise ValueError(f"Different {language} language keys")
    missing = keys - messages.keys()
    if missing:
        raise ValueError(f"Missing {language} translations: {sorted(missing)}")
js = (
    "const I18n={locales:"
    + json.dumps(locales, ensure_ascii=False, separators=(",", ":"))
    + " ,text(key,language){return this.locales[language]?.[key]??this.locales.en[key]??key;},apply(parent,language){parent.querySelectorAll('[data-i18n]').forEach(el=>el.textContent=this.text(el.dataset.i18n,language));}};\n"
)
(root / "web/i18n.js").write_text(js, encoding="utf8")

# Access-page text is public; keep it separate from authenticated management scripts.
access = {
    p.stem: json.loads(p.read_text(encoding="utf8"))
    for p in sorted((root / "locales/access").glob("*.json"))
}
for language, messages in access.items():
    if set(messages) != set(access["en"]):
        raise ValueError(f"Different access/{language} keys")
access_keys = set(
    re.findall(
        r'data-text="([^"]+)"', (root / "web/access.html").read_text(encoding="utf8")
    )
)
access_keys.update(
    re.findall(
        r"accessText\(['\"]([^'\"]+)['\"]\)",
        (root / "web/access.js").read_text(encoding="utf8")
        + (root / "web/setup.js").read_text(encoding="utf8"),
    )
)
access_keys -= {"title", "submit"}
if access_keys - access["en"].keys():
    raise ValueError(f'Missing access keys: {access_keys-access["en"].keys()}')
(root / "web/access-i18n.js").write_text(
    "// Generated from locales/access/*.json.\nconst AccessI18n="
    + json.dumps(access, ensure_ascii=False, separators=(",", ":"))
    + ";\n",
    encoding="utf8",
)

backend = {
    p.stem: json.loads(p.read_text(encoding="utf8"))
    for p in sorted((root / "locales/backend").glob("*.json"))
}
if set(backend["zh"]) != set(backend["en"]):
    raise ValueError("Different backend language keys")
header = "#pragma once\n// Generated from locales/backend/*.json.\nnamespace WebMessages {\ninline String text(const String& key,bool english){if(english)return key;\n"
for key, value in backend["zh"].items():
    header += (
        " if(key=="
        + json.dumps(key, ensure_ascii=False)
        + ")return "
        + json.dumps(value, ensure_ascii=False)
        + ";\n"
    )
header += " return key;\n}\n}\n"
(root / "include/locales/web_messages.h").write_text(header, encoding="utf8")
print(
    f'Language packs verified: {len(locales["en"])} management, {len(access["en"])} access, {len(backend["en"])} backend labels.'
)
