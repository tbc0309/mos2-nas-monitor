let firmwareBusy = false;
let settingsBusy = false;
const form = document.getElementById("settings");
for (const [k, v] of Object.entries(config)) {
  if (form.elements[k]) form.elements[k].value = v;
}
if (config.iface) {
  form.elements.iface.add(new Option(config.iface, config.iface));
  form.elements.iface.value = config.iface;
}
let lang = config.adminLanguage || "zh";
const msg = (key) => I18n.text(key, lang);
// Group device fields without changing their submitted names.
const snmpSection = form.elements.nas.closest("section");
snmpSection.classList.add("snmpSettings");
const snmpFields = snmpSection.querySelector(".fields");
let deviceDetails;
for (const [title, names] of [
  [
    "Device information",
    ["machineName", "nas"],
  ],
  ["Display colors", ["nasBackground", "cardBackground"]],
  ["Connection", ["snmpVersion", "port", "iface", "interval", "community"]],
  [
    "Security",
    [
      "snmpUser",
      "snmpAuth",
      "snmpPassword",
      "snmpPrivacy",
      "snmpPrivacyPassword",
    ],
  ],
]) {
  const group = document.createElement("fieldset");
  group.className = "snmpGroup";
  if (title !== "Security") group.classList.add("compactGroup");
  if (title === "Security") group.dataset.snmp = "3";
  const heading = document.createElement(title === "Device information" ? "h3" : "h4");
  heading.dataset.i18n = title;
  heading.textContent = title;
  group.append(heading);
  for (const name of names) {
    const field = form.elements[name];
    if (field) {
      const wrapper =
        field.closest("#privacyPasswordField") || field.closest("label");
      if (name === "community") wrapper.classList.add("wideField");
      group.append(wrapper);
    }
  }
  if (title === "Device information") {
    deviceDetails = group; group.classList.add("deviceDetails"); snmpFields.append(group);
  } else {
    group.classList.add("subGroup"); deviceDetails.append(group);
  }
}
// Keep general display options and clock colors in separate peer cards.
const desktopFields = form.elements.clockColor.closest(".fields");
const displayCards = document.createElement("div");
displayCards.className = "displayCards";
for (const [title, names] of [
  ["Display options", ["language", "home", "showSeconds", "brightness", "rotation", "sound"]],
  ["Clock palettes", ["clockColor", "minuteColor"]],
]) {
  const card = document.createElement("div"); card.className = "settings-group";
  if (title === "Clock palettes") card.classList.add("clockPalette");
  const heading = document.createElement("h3"); heading.dataset.i18n = title; heading.textContent = msg(title);
  const fields = document.createElement("div"); fields.className = "fields";
  for (const name of names) if (form.elements[name]) fields.append(form.elements[name].closest("label"));
  card.append(heading, fields); displayCards.append(card);
}
desktopFields.insertBefore(displayCards, desktopFields.querySelector(".desktop-groups"));
// Presets only edit the form; saving remains an explicit user action.
function colorPresets(anchor, headingKey, fields, choices) {
  const row = document.createElement("div");
  row.className = "colorPresets";
  const heading = document.createElement("strong");
  heading.dataset.i18n = headingKey;
  heading.textContent = msg(headingKey);
  const reset = form.elements[fields[0]].closest("label").querySelector("button");
  if (reset) reset.remove();
  row.append(heading);
  for (const [key, colors] of choices) {
    const button = document.createElement("button");
    button.type = "button";
    button.className = "secondary";
    const swatch = document.createElement("span");
    swatch.className = "colorSwatch";
    swatch.style.background = `linear-gradient(90deg, ${colors[0]} 50%, ${colors[1]} 50%)`;
    const title = document.createElement("span");
    title.dataset.i18n = key;
    title.textContent = msg(key);
    button.append(swatch, title);
    button.addEventListener("click", () => {
      fields.forEach((name, index) => {
        const input = form.elements[name];
        input.value = colors[index];

      });
      dirty(); // One local form update; selecting a palette never sends a save request.
    });
    row.append(button);
  }
  const groups = anchor.querySelector(":scope > .desktop-groups");
  anchor.insertBefore(row, groups || null);
}
colorPresets(form.elements.minuteColor.closest(".fields"), "Clock palettes", ["clockColor", "minuteColor"], [
  ["Default palette", ["#ffffff", "#ffa500"]],
  ["White & blue", ["#ffffff", "#58baff"]],
  ["Cyan & white", ["#42d9ed", "#ffffff"]],
  ["Green & gold", ["#92ed56", "#ffcb55"]],
  ["White & pink", ["#ffffff", "#ff80b5"]],
  ["Gold & white", ["#ffcb55", "#ffffff"]],
  ["Violet & cyan", ["#c5a3ff", "#42d9ed"]],
  ["Lime & white", ["#bbef66", "#ffffff"]],
  ["White & coral", ["#ffffff", "#ff927c"]],
]);
colorPresets(form.elements.cardBackground.closest("fieldset"), "Device palettes", ["nasBackground", "cardBackground"], [
  ["Default palette", ["#21455c", "#103040"]],
  ["Ocean", ["#68acc4", "#32738a"]],
  ["Forest", ["#74b08a", "#3c7b5e"]],
  ["Amber", ["#d6b24c", "#856737"]],
  ["Violet", ["#a58bc9", "#69477d"]],
  ["Rose", ["#d781a2", "#854761"]],
]);
let editedMachine = config.selectedMachine ?? config.activeMachine ?? 0;
const machineSelect = document.getElementById("machineSelect");
(config.machines || [{ name: config.nas }]).forEach((m, i) =>
  machineSelect.add(new Option(m.name || m.address || config.nas, i)),
);
machineSelect.value = editedMachine;
machineSelect.classList.add("hidden");
machineSelect.setAttribute("aria-label", msg("Device list"));
const machineList = document.createElement("div");
machineList.className = "machineList";
machineSelect
  .closest("section")
  .insertBefore(machineList, machineSelect.closest(".actions"));
(config.machines || []).forEach((m, i) => {
  const row = document.createElement("div");
  row.className = "machineItem" + (i == editedMachine ? " active" : "");
  const choose = document.createElement("button");
  choose.type = "button";
  choose.className = "choose";
  choose.textContent = i + 1 + ". " + (m.name || m.address || "SNMP");
  const address = document.createElement("small");
  address.textContent = m.address || "";
  choose.append(address);
  choose.onclick = () => {
    if (i != editedMachine) machineAction("select", i);
  };
  row.append(choose);
  for (const [label, offset, key] of [
    ["↑", -1, "Move up"],
    ["↓", 1, "Move down"],
  ]) {
    const move = document.createElement("button");
    move.type = "button";
    move.className = "move";
    move.textContent = label;
    move.dataset.i18nTitle = key;
    move.title = msg(key);
    move.setAttribute("aria-label", msg(key));
    move.disabled = i + offset < 0 || i + offset >= config.machines.length;
    move.onclick = () => machineAction("move", i + ":" + (i + offset));
    row.append(move);
  }
  machineList.append(row);
});
form.elements.machineIndex.value = editedMachine;
document.getElementById("addMachine").disabled =
  (config.machines || []).length >= 6;
document.getElementById("deleteMachine").disabled =
  (config.machines || []).length <= 1;
let machineBusy = false;
async function machineAction(action, index) {
  if (machineBusy) return;
  if (
    (action === "select" ? snmpSection.classList.contains("dirty") : [...form.querySelectorAll("section.dirty")].length) &&
    !confirm(msg("Discard unsaved changes?"))
  ) {
    machineSelect.value = editedMachine;
    form.elements.machineIndex.value = editedMachine;
    return;
  }
  if (action === "delete" && !confirm(msg("Delete this device?"))) return;
  machineBusy = true;
  try {
    const response = await post("/api/machine", { action, index: index ?? "" });
    if (response.ok) {
      const result = await response.json();
      if (action === "select") {
        editedMachine = result.selectedMachine;
        Object.assign(config, result);
        for (const name of ["machineName", "nas", "nasBackground", "cardBackground", "snmpVersion", "port", "iface", "interval", "snmpUser", "snmpAuth", "snmpPrivacy"]) {
          const field = form.elements[name];
          if (name === "iface") field.replaceChildren(new Option(msg("Automatic"), ""), ...(result.iface ? [new Option(result.iface, result.iface)] : []));
          field.value = result[name] ?? "";
          originals.set(field, field.value);
        }
        for (const name of ["community", "snmpPassword", "snmpPrivacyPassword"]) {
          form.elements[name].value = "";
          originals.set(form.elements[name], "");
        }
        form.elements.machineIndex.value = editedMachine;
        machineSelect.value = editedMachine;
        [...machineList.children].forEach((row, i) => row.classList.toggle("active", i === editedMachine));
        snmpOptions(); dirty();
        history.replaceState(null, "", "/manage?device=" + editedMachine + "#settings");
        return;
      }
      location.href = "/manage?device=" + (result.selectedMachine ?? result.activeMachine) + "#settings";
    } else
      document.getElementById("message").textContent = msg(
        (await response.text()).trim() || "Failed",
      );
  } catch (_) {
    document.getElementById("message").textContent = msg("Failed");
  } finally {
    machineBusy = false;
  }
}
machineSelect.onchange = () => machineAction("select", machineSelect.value);
document.getElementById("addMachine").onclick = () => machineAction("add");
document.getElementById("deleteMachine").onclick = () =>
  machineAction("delete", editedMachine);
function translate() {
  document.documentElement.lang = lang === "en" ? "en" : "zh-CN";
  document.getElementById("language").value = lang;
  I18n.apply(document, lang);
  document.querySelectorAll("[data-i18n-title]").forEach((el) => {
    el.title = msg(el.dataset.i18nTitle);
    el.setAttribute("aria-label", el.title);
  });
  machineSelect.setAttribute("aria-label", msg("Device list"));
  updateWifiLabels();
  updateSaveAction();
  document.getElementById("cityCodeLookup").title = msg(
    "Search for a city on China Weather. The nine digits in its weather page URL are the city code.",
  );
  document
    .getElementById("crop")
    .setAttribute("aria-label", msg("Photo crop preview"));
  if (typeof albumEntries !== "undefined") updateAlbumLabels();
  tick();
}
document.getElementById("language").onchange = async (e) => {
  if (settingsBusy) { e.target.value = lang; return; }
  settingsBusy = true;
  const previous = lang;
  lang = e.target.value;
  translate();
  try {
    const r = await post("/api/language", { language: lang });
    if (!r.ok) throw new Error("save");
    config.adminLanguage = lang;
  } catch (error) {
    lang = previous;
    translate();
  } finally { settingsBusy = false; }
};
document.querySelectorAll(".day").forEach((c) => {
  c.checked = !!((config.alarmDays ?? 31) & +c.value);
  c.onchange = () => {
    form.elements.alarmDays.value = [
      ...document.querySelectorAll(".day:checked"),
    ].reduce((n, c) => n | +c.value, 0);
  };
});
const originals = new Map(
  [...form.querySelectorAll("input,select")].map((el) => [
    el,
    el.type === "checkbox" ? el.checked : el.value,
  ]),
);
function updateSaveAction() {
  const key = "Save & restart";
  const label = document.getElementById("saveActionLabel");
  label.dataset.i18n = key;
  label.textContent = msg(key);
}
function dirty() {
  updateSaveAction();
  for (const card of form.querySelectorAll("section"))
    card.classList.toggle(
      "dirty",
      [
        ...card.querySelectorAll(
          "input[name]:not([type=hidden]),select[name],input.day",
        ),
      ].some(
        (el) =>
          (el.type === "checkbox" ? el.checked : el.value) !==
          originals.get(el),
      ),
    );
}
form.addEventListener("input", dirty);
form.addEventListener("change", dirty);
form.onsubmit = async (e) => {
  e.preventDefault();
  if (settingsBusy) return;
  const message = document.getElementById("message"),
    button = document.querySelector("#settingsActions button");
  if (!+form.elements.alarmDays.value) {
    message.textContent = msg("Select at least one weekday");
    return;
  }
  button.disabled = true;
  settingsBusy = true;
  let validatingCity = true;
  message.textContent = msg("Checking city code…");
  try {
    const cityCode = form.elements.cityCode.value.trim();
    form.elements.cityCode.value = cityCode;
    const deadline = Date.now() + 12000;
    let status = "pending";
    while (Date.now() < deadline) {
      const checkAbort = new AbortController();
      const checkTimer = setTimeout(() => checkAbort.abort(), 4000);
      const checked = await fetch("/api/weather/check", {
        signal: checkAbort.signal,
        method: "POST", body: new URLSearchParams({code:cityCode,token:form.elements.token.value}),
      }).finally(() => clearTimeout(checkTimer));
      if (checked.status === 401 || checked.status === 403) throw new Error(msg("Reload page"));
      const result = await checked.json(); status = result.status;
      if (status !== "pending" && status !== "busy") break;
      await new Promise(resolve => setTimeout(resolve, 500));
    }
    if (status !== "valid") {
      message.textContent = status === "forecast" ? msg("This city provides forecasts only; live weather is unavailable.") :
        status === "invalid" ? msg("City code not found or unsupported.") : msg("Weather service unavailable. Try again.");
      message.style.color = "#b84b42"; button.disabled = false; return;
    }
    validatingCity = false;
    message.textContent = msg("Saving…");
    const response = await fetch(form.action, {
      method: "POST",
      signal: AbortSignal.timeout(12000),
      body: new URLSearchParams(new FormData(form)),
    });
    const content = await response.text();
    message.textContent = (response.headers.get("Content-Type") || "").includes(
      "text/html",
    )
      ? response.ok
        ? msg("Saved.")
        : msg("Failed")
      : msg(content);
    message.style.color = response.ok ? "#147e87" : "#b84b42";
    if (response.ok) {
      for (const name of ["pass", "admin", "community", "snmpPassword", "snmpPrivacyPassword"])
        form.elements[name].value = "";
      // A successful settings save always restarts the device; do not submit
      // another restart request or allow a second save during the boot window.
      setTimeout(() => { location.href = "/manage"; }, 8000);
    } else button.disabled = false;
  } catch (error) {
    message.textContent = validatingCity ? msg("Weather service unavailable. Try again.") : msg("Failed");
    message.style.color = "#b84b42";
    button.disabled = false;
  } finally { settingsBusy = false; }
};
document.querySelectorAll("[data-tab]").forEach(
  (b) =>
    (b.onclick = () => {
      document
        .querySelectorAll("[data-tab]")
        .forEach((x) => x.classList.toggle("active", x === b));
      ["screen", "photos", "maintenance"].forEach((n) =>
        document
          .getElementById("tab-" + n)
          .classList.toggle("hidden", b.dataset.tab !== n),
      );
      form.classList.toggle("hidden", b.dataset.tab !== "settings");
      document
        .getElementById("settingsActions")
        .classList.toggle("hidden", b.dataset.tab !== "settings");
      if (b.dataset.tab === "settings") loadWeatherCities();
      if (b.dataset.tab === "photos" && !albumLoaded) refreshAlbum();
    }),
);
const format = (v, u) => (v < 0 || v == null ? "--" : v.toFixed(1) + u);
const rate = (v) =>
  v < 0
    ? "--"
    : v >= 1048576
      ? (v / 1048576).toFixed(1) + " MB/s"
      : (v / 1024).toFixed(1) + " KB/s";
function weatherOptions() {
  const enabled = form.elements.weatherEnabled.value === "1";
  form.elements.home.querySelector('option[value="5"]').disabled = !enabled;
  document.getElementById("weatherPage").disabled = !enabled;
  if (!enabled && form.elements.home.value === "5")
    form.elements.home.value = "6";
}
form.elements.weatherEnabled.onchange = weatherOptions;
weatherOptions();
let statusBusy = false;
async function tick() {
  if (firmwareBusy || settingsBusy || statusBusy || document.hidden) return;
  statusBusy = true;
  try {
    const response = await fetch("/api/status?management=1", {
      cache: "no-store",
      signal: AbortSignal.timeout(6000),
    });
    if (response.status === 401) {
      location.href = "/manage";
      return;
    }
    const d = await response.json();
    updateInterfaces(d);
    updateCurrentWifi(d);
    document.getElementById("state").textContent = d.online
      ? msg("Online")
      : msg("Offline");
    for (const k of ["cpu", "memory"])
      document.getElementById(k).textContent = format(d[k], "%");
    document.getElementById("temperature").textContent = format(
      d.temperature,
      "°C",
    );
    document.getElementById("traffic").textContent =
      rate(d.rx) + " / " + rate(d.tx);
    document.getElementById("traffic").style.fontSize = "15px";
    document.getElementById("details").textContent =
      (d.model || "SNMP") +
      " · " +
      (d.interface || "--") +
      " · " +
      (d.disks || []).length +
      msg(" disks / ") +
      (d.volumes || []).length +
      msg(" volumes");
    const w = d.weather;
    document.getElementById("weatherDetails").textContent =
      w && w.valid
        ? (lang === "en"
            ? (w.cityEnglish || "Tianjin") + " · " + (w.conditionEnglish || "")
            : (w.city || "") + " · " + (w.condition || "")) +
          " · " +
          w.temperature +
          "°C · " +
          w.humidity +
          "%"
        : msg("Waiting for weather");
    const a = d.alarm;
    if (a)
      document.getElementById("alarmDetails").textContent =
        [msg("Idle"), msg("Ringing"), msg("Snoozed"), msg("Dismissed today")][
          a.state
        ] + (a.state === 2 ? " · " + a.remaining + "s" : "");
  } catch (e) {
    document.getElementById("state").textContent = msg("Disconnected");
  } finally {
    statusBusy = false;
  }
}
async function post(path, data = {}) {
  return fetch(path, {
    signal: AbortSignal.timeout(12000),
    method: "POST",
    body: new URLSearchParams({ ...data, token: form.elements.token.value }),
  });
}
async function setPage(page) {
  await post("/api/page", { page });
  tick();
}
async function testBeep() {
  const r = await post("/api/beep");
  document.getElementById("message").textContent = r.ok
    ? msg("Sound sent")
    : msg("Failed");
}
async function alarmAction(a) {
  const r = await post("/api/alarm/" + a, {
    tune: form.elements.alarmTune.value,
  });
  if (!r.ok)
    document.getElementById("alarmDetails").textContent =
      msg("No active alarm");
  else tick();
}
async function resetSettings() {
  const r = await post("/api/reset/request");
  if (!r.ok) return;
  const d = await r.json();
  if (!confirm(msg("Clear all settings and photos? This cannot be undone.")))
    return;
  const done = await post("/api/reset/confirm", { nonce: d.nonce });
  if (done.ok) {
    alert(msg("Reset complete. Reconnect to the device Wi-Fi."));
    location.reload();
  }
}
