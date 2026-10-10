from pathlib import Path
import json,re,html
root=Path(__file__).resolve().parents[1]
locales={p.stem:json.loads(p.read_text(encoding='utf8')) for p in sorted((root/'locales').glob('*.json'))}
# Fail generation when a visible management label has no translation.
keys=set(html.unescape(k) for k in re.findall(r'data-i18n="([^"]+)"',(root/'web/management.html').read_text(encoding='utf8')))
keys.update(re.findall(r'msg\("([^"]+)"\)',(root/'web/management.js').read_text(encoding='utf8')))
for language,messages in locales.items():
 missing=keys-messages.keys()
 if missing:raise ValueError(f"Missing {language} translations: {sorted(missing)}")
js='const I18n={locales:'+json.dumps(locales,ensure_ascii=False,separators=(',',':'))+" ,text(key,language){return this.locales[language]?.[key]??this.locales.en[key]??key;},apply(parent,language){parent.querySelectorAll('[data-i18n]').forEach(el=>el.textContent=this.text(el.dataset.i18n,language));}};\n"
(root/'web/i18n.js').write_text(js,encoding='utf8')
