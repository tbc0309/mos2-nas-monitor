async function upgradeFirmware() {
  if (firmwareBusy) return;
  const input = document.getElementById("firmwareFile"),
    keep = document.getElementById("firmwareKeep"),
    button = document.getElementById("firmwareUpgrade"),
    progress = document.getElementById("firmwareProgress"),
    message = document.getElementById("firmwareMessage"),
    file = input.files[0];
  const feedback = (key, color = "#147e87") => {
    message.dataset.i18n = key;
    message.textContent = msg(key);
    message.style.color = color;
  };
  if (
    !file ||
    !file.name.toLowerCase().endsWith(".bin") ||
    file.size < 4096 ||
    file.size > 0x3fa000
  ) {
    feedback("Choose a valid program .bin (up to 1 MiB).", "#b84b42");
    return;
  }
  if (
    !confirm(
      keep.checked
        ? msg("Upgrade firmware and keep settings and photos?")
        : msg("Upgrade firmware and delete all settings and photos?"),
    )
  )
    return;
  firmwareBusy = true;
  input.disabled = keep.disabled = button.disabled = true;
  progress.value = 0;
  progress.classList.remove("hidden");
  feedback("Uploading… Do not disconnect power.");
  try {
    const bytes = new Uint8Array(await file.arrayBuffer()),
      body = new FormData();
    const program = FirmwareBundle.program(bytes);
    body.append("firmware", new Blob([program]), file.name);
    if (bytes[0] !== 0xe9) throw new Error("invalid");
    const result = await new Promise((resolve, reject) => {
      const request = new XMLHttpRequest();
      request.open(
        "POST",
        "/api/firmware?" +
          new URLSearchParams({
            size: program.length,
            crc: firmwareCRC(program),
            keep: keep.checked ? "1" : "0",
          }),
      );
      request.setRequestHeader("X-CSRF-Token", form.elements.token.value);
      request.timeout = 180000;
      request.upload.onprogress = (e) => {
        if (e.lengthComputable)
          progress.value = Math.round((e.loaded / e.total) * 100);
      };
      request.upload.onload = () => {
        feedback("Checking firmware… Do not disconnect power.");
      };
      request.onload = () => {
        let d;
        try {
          d = JSON.parse(request.responseText);
        } catch (e) {
          return reject(new Error("write"));
        }
        if (request.status === 200 && d.updated) resolve(d);
        else reject(new Error(d.error || "write"));
      };
      request.onerror = request.ontimeout = () => reject(new Error("network"));
      request.send(body);
    });
    progress.value = 100;
    feedback(
      result.simulated
        ? "Simulation passed. No firmware was flashed."
        : result.preserved
          ? "Upgrade complete. 1. Wait for restart. 2. Reopen management at the same address."
          : "Upgrade complete. 1. Wait for restart. 2. Connect to the device hotspot. 3. Set up Wi-Fi again.",
    );
    if (!result.simulated) return;
  } catch (error) {
    feedback(
      error.message === "network"
        ? "Connection interrupted. Check the device before retrying."
        : ["login_required", "csrf"].includes(error.message)
          ? "Session expired. Reload and sign in again."
          : error.message === "size"
            ? "Firmware is too large for the program partition."
            : "Upgrade failed: invalid, incompatible or incomplete firmware. Data was not cleared.",
      "#b84b42",
    );
  }
  firmwareBusy = false;
  input.disabled = keep.disabled = button.disabled = false;
}

function snmpOptions() {
  const v = form.elements.snmpVersion.value;
  document
    .querySelectorAll("[data-snmp]")
    .forEach((e) => e.classList.toggle("hidden", e.dataset.snmp !== v));
  form.elements.snmpUser.required = v === "3";
  form.elements.snmpPassword.minLength = v === "3" ? 8 : 0;
  const encrypted = v === "3" && form.elements.snmpPrivacy.value !== "none";
  document
    .getElementById("privacyPasswordField")
    .classList.toggle("hidden", !encrypted);
  form.elements.snmpPrivacyPassword.minLength = encrypted ? 8 : 0;
}
form.elements.snmpPrivacy.addEventListener("change", snmpOptions);
form.elements.snmpVersion.addEventListener("change", snmpOptions);
snmpOptions();

async function signOut(event) {
  event.preventDefault();
  try {
    const response = await post("/logout");
    if (response.ok || response.status === 401) location.replace("/manage");
    else throw Error();
  } catch (_) {
    document.getElementById("message").textContent = msg("Failed");
  }
}

if (location.hash === "#settings")
  document.querySelector('[data-tab="settings"]').click();

document.getElementById("rediscoverStorage").onclick = async (e) => {
  const button = e.currentTarget,
    feedback = document.getElementById("storageFeedback");
  button.disabled = true;
  try {
    const r = await post("/api/storage/rediscover", {});
    feedback.dataset.i18n = r.ok
      ? "Storage discovery queued"
      : "Discovery already requested";
    feedback.textContent = msg(feedback.dataset.i18n);
  } catch (error) {
    feedback.dataset.i18n = "Failed";
    feedback.textContent = msg("Failed");
  }
  setTimeout(() => (button.disabled = false), 60000);
};

// Monitoring selection is explicit and independent of the configuration editor.
document.getElementById("switchMonitor").onclick = async (event) => {
  const button = event.currentTarget;
  button.disabled = true;
  try {
    const response = await post("/api/machine", {action: "activate"});
    if (!response.ok) throw new Error("Switch failed");
    const result = await response.json();
    config.activeMachine = result.activeMachine;
    await tick();
  } catch (_) { document.getElementById("storageFeedback").textContent = msg("Failed"); }
  finally { button.disabled = false; }
};

// The city catalog loads only when settings are opened; selected codes remain editable.
var weatherCitiesLoading = false;
function loadWeatherCities() {
  if (weatherCitiesLoading) return;
  weatherCitiesLoading = true;
  const script = document.createElement("script");
  script.src = "/weather-cities.js?v=china-city-20261011";
  script.onload = () => {
    const province = document.getElementById("weatherProvince"), city = document.getElementById("weatherCity");
    const rows = window.weatherCities;
    province.replaceChildren(new Option("—", ""));
    for (const name of new Set(rows.map(r => r[0]))) province.add(new Option(name, name));
    function cities(code) {
      city.replaceChildren(new Option("—", ""));
      for (const row of rows.filter(r => r[0] === province.value)) city.add(new Option(row[1], row[2]));
      city.value = code || "";
    }
    const current = rows.find(r => r[2] === form.elements.cityCode.value);
    if (current) province.value = current[0];
    cities(current && current[2]);
    province.disabled = city.disabled = false;
    province.onchange = () => cities();
    city.onchange = () => { if (city.value) { form.elements.cityCode.value = city.value; form.elements.cityCode.dispatchEvent(new Event("input", {bubbles:true})); } };
    form.elements.cityCode.addEventListener("input", () => {
      const row = rows.find(r => r[2] === form.elements.cityCode.value);
      province.value = row ? row[0] : ""; cities(row && row[2]);
    });
  };
  script.onerror = () => { weatherCitiesLoading = false; document.getElementById("state").textContent = msg("Failed"); };
  document.body.append(script);
}
