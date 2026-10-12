let wifiNetworks = [],
  wifiScanning = false,
  wifiScanned = false,
  wifiScanMessage = "",
  currentWifi = null;
function updateWifiLabels() {
  const select = document.getElementById("manageNetworks"),
    selected = select.value;
  select.replaceChildren(
    new Option(msg("Choose a network"), ""),
    ...wifiNetworks.map(
      (n) => new Option(n.ssid + " · " + n.rssi + " dBm", n.ssid),
    ),
  );
  select.value = selected;
  document.getElementById("manageScanStatus").textContent = wifiScanMessage
    ? msg(wifiScanMessage)
    : "";
  if (currentWifi)
    document.getElementById("wifiCurrent").textContent =
      currentWifi.wifiConnected
        ? msg("Current Wi-Fi") + " · " + currentWifi.wifiSsid
        : msg("Not connected");
}
function updateCurrentWifi(d) {
  currentWifi = d;
  updateWifiLabels();
}
async function scanManagementWifi(refresh = false) {
  if (wifiScanning || firmwareBusy) return;
  wifiScanning = true;
  wifiScanned = true;
  const button = document.getElementById("manageRescan");
  button.disabled = true;
  wifiScanMessage = "Scanning…";
  updateWifiLabels();
  try {
    const r = await fetch("/api/wifi/scan" + (refresh ? "?refresh=1" : ""), {
      cache: "no-store",
    });
    if (r.status === 401) {
      location.replace("/manage");
      return;
    }
    if (!r.ok) throw Error();
    const d = await r.json();
    if (d.scanning) {
      setTimeout(() => scanManagementWifi(), 1000);
      return;
    }
    wifiNetworks = d.networks || [];
    wifiScanMessage = wifiNetworks.length ? "" : "No networks found";
  } catch (_) {
    wifiScanMessage = "Wi-Fi scan failed";
  } finally {
    wifiScanning = false;
    button.disabled = false;
    updateWifiLabels();
  }
}
document.getElementById("manageNetworks").onchange = (e) => {
  if (e.target.value) {
    form.elements.ssid.value = e.target.value;
    dirty();
  }
};
// Discover only when the network selector is used; opening unrelated settings
// must not start an SDK scan during a configuration save or page transfer.
document.getElementById("manageNetworks").onfocus = () => {
  if (!wifiScanned) scanManagementWifi();
};
document.getElementById("manageRescan").onclick = () =>
  scanManagementWifi(true);

translate();
setInterval(tick, 3000);

function resetColors(group) {
  const defaults =
    group === "clock"
      ? { clockColor: "#ffffff", minuteColor: "#ffa500" }
      : { nasBackground: "#21455c" };
  for (const [name, value] of Object.entries(defaults))
    form.elements[name].value = value;
  dirty();
}

function updateInterfaces(d) {
  if (editedMachine !== config.activeMachine) d = {}; // Do not offer another device's ports.
  const select = form.elements.iface,
    selected = select.value;
  const options = [
    {
      index: "",
      name: msg("Automatic") + (d.interface ? " · " + d.interface : ""),
    },
  ];
  for (const item of d.interfaces || [])
    options.push({
      index: String(item.index),
      name: item.name + " · " + msg(item.up ? "Connected" : "Not connected"),
    });
  if (selected && !options.some((x) => x.index === selected))
    options.push({ index: selected, name: selected });
  const key = JSON.stringify(options);
  if (select.dataset.options !== key) {
    select.replaceChildren(...options.map((x) => new Option(x.name, x.index)));
    select.value = selected;
    select.dataset.options = key;
  }
}

