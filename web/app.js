const english = {
  disclaimer:
    "An independent open-source modification, not affiliated with MOS2 or hardware vendors. No technical support is provided; consult the documentation or AI.",
  projectLink: "Project",
  title: "Install MOS2-NAS online",
  intro:
    "A desktop clock, weather display and multi-device SNMP monitor for MOS2.",
  install: "1. Install firmware",
  browser:
    "Use desktop Chrome or Edge. Connect MOS2 with a USB data cable and close other serial applications.",
  warning:
    "Fresh installation overwrites settings, photos and fonts. To keep your data, use Maintenance → Firmware update in the device management page.",
  ack: "I understand that fresh installation clears settings and photos.",
  prepare: "Prepare installation",
  native:
    "Select the serial port belonging to MOS2. The installer dialogs are in English. Keep the device powered throughout installation.",
  setup: "2. Wi-Fi and setup",
  step1: "On first boot, join MOS2-NAS-xxxxxx. Password: 12345678.",
  step2: "Open http://192.168.4.1/ and connect to 2.4GHz Wi-Fi.",
  step3:
    "After connection, tap the screen five times and scan the management QR code. Default password: 12345678; change it on first login.",
  step4: "Add devices under SNMP settings, then save and restart.",
  guide: "Read the user guide",
  download: "Download firmware",
};
const nodes = [...document.querySelectorAll("[data-i]")];
const chinese = Object.fromEntries(
  nodes.map((n) => [n.dataset.i, n.textContent]),
);
// The first browser preference determines the initial UI language.
function browserLanguage(
  language = navigator.languages?.[0] || navigator.language || "en",
) {
  return /^zh(?:[-_]|$)/i.test(language) ? "zh" : "en";
}
let lang = browserLanguage(),
  ready = false;
const status = document.querySelector("#status"),
  prepare = document.querySelector("#prepare");
function applyLanguage() {
  document.querySelector("#language").value = lang;
  document.documentElement.lang = lang === "zh" ? "zh-CN" : "en";
  nodes.forEach(
    (n) => (n.textContent = (lang === "en" ? english : chinese)[n.dataset.i]),
  );
  document.title =
    lang === "zh" ? "MOS2-NAS · 在线刷写" : "MOS2-NAS · Web installer";
  document.querySelector('[data-i="guide"]').href =
    `https://github.com/tbc0309/mos2-nas-monitor/blob/main/README${lang === "en" ? ".en" : ""}.md`;
  const trigger = document.querySelector("#installer button");
  if (trigger)
    trigger.textContent =
      lang === "zh" ? "连接设备并刷写" : "Connect and install";
}
document.querySelector("#language").addEventListener("change", (e) => {
  lang = e.target.value;
  applyLanguage();
});
applyLanguage();
document
  .querySelector("#ack")
  .addEventListener(
    "change",
    (e) => (prepare.disabled = !e.target.checked || !ready),
  );
try {
  const response = await fetch("./firmware.json", {
    signal: AbortSignal.timeout(10000),
    cache: "no-cache",
  });
  if (!response.ok) throw Error();
  const meta = await response.json();
  document.querySelector("#release").textContent =
    `MOS2-NAS v${meta.version} · ${(meta.size / 1048576).toFixed(2)} MB`;
  ready = true;
  prepare.disabled = !document.querySelector("#ack").checked;
} catch {
  status.textContent =
    "固件信息加载失败，请刷新重试。 / Cannot load firmware information. Reload to retry.";
}
prepare.addEventListener("click", async () => {
  prepare.disabled = true;
  try {
    if (!navigator.serial || !window.isSecureContext)
      throw Error("请使用电脑端 Chrome / Edge。 / Use desktop Chrome or Edge.");
    status.textContent =
      lang === "zh" ? "正在加载刷写工具…" : "Loading installer…";
    await Promise.race([
      import("./vendor/install-button.js"),
      new Promise((_, reject) =>
        setTimeout(
          () => reject(Error("加载超时，请重试。 / Loading timed out. Retry.")),
          20000,
        ),
      ),
    ]);
    const button = document.createElement("esp-web-install-button");
    button.setAttribute("manifest", "./manifest.json");
    const trigger = document.createElement("button");
    trigger.slot = "activate";
    trigger.textContent =
      lang === "zh" ? "连接设备并刷写" : "Connect and install";
    button.append(trigger);
    document.querySelector("#installer").replaceChildren(button);
    status.textContent = "";
    prepare.hidden = true;
  } catch (error) {
    status.textContent = error.message;
    prepare.disabled = false;
  }
});
