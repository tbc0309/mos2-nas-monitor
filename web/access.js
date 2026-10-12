const accessMode = document.querySelector("main").dataset.mode;
const setupMode = accessMode === "setup";
const accessForm = document.getElementById("accessForm");
let accessLanguage;
try {
  accessLanguage = localStorage.getItem("mos2-access-language") || "zh";
} catch (_) {
  accessLanguage = "zh";
}
if (accessLanguage !== "en") accessLanguage = "zh";
const accessMessages = AccessI18n;
for (const language of ["zh", "en"]) {
  accessMessages[language].title =
    accessMessages[language][setupMode ? "setupTitle" : "loginTitle"];
  accessMessages[language].submit =
    accessMessages[language][setupMode ? "setupSubmit" : "loginSubmit"];
}
const accessText = (key) => accessMessages[accessLanguage][key] || key;
function translateAccess() {
  document.documentElement.lang = accessLanguage;
  document.getElementById("accessLanguage").value = accessLanguage;
  document.getElementById("accessLang").value = accessLanguage;
  document.getElementById("title").textContent = accessText("title");
  document
    .querySelectorAll("[data-text]")
    .forEach((el) => (el.textContent = accessText(el.dataset.text)));
  const feedback = document.getElementById("feedback");
  feedback.textContent = feedback.dataset.error
    ? accessText(feedback.dataset.error)
    : "";
  document.getElementById("hint").textContent = setupMode
    ? accessText("hint")
    : "";
  if (typeof updateNetworkLabels === "function") updateNetworkLabels();
  const savedNetwork = document.getElementById("savedNetwork");
  if (savedNetwork.dataset.ssid)
    savedNetwork.textContent =
      accessText("networkSaved") + savedNetwork.dataset.ssid;
}
document.getElementById("wifiFields").classList.toggle("hidden", !setupMode);
document.getElementById("ssid").disabled = !setupMode;
document.getElementById("ssid").required = setupMode;
document.getElementById("accessLanguage").onchange = (event) => {
  accessLanguage = event.target.value;
  try {
    localStorage.setItem("mos2-access-language", accessLanguage);
  } catch (_) {}
  translateAccess();
};
translateAccess();

const passwordDialog = document.getElementById("passwordDialog");
passwordDialog.addEventListener("cancel", (event) => event.preventDefault());
if (accessMode === "change") {
  accessForm.classList.add("hidden");
  passwordDialog.showModal();
}
if (new URLSearchParams(location.search).get("password") === "changed") {
  const feedback = document.getElementById("feedback");
  feedback.dataset.error = "passwordChanged";
  feedback.className = "success";
}
translateAccess();
document
  .getElementById("passwordForm")
  .addEventListener("submit", async (event) => {
    event.preventDefault();
    const form = event.target,
      button = document.getElementById("passwordSubmit"),
      feedback = document.getElementById("passwordFeedback");
    if (
      form.newPassword.value === "12345678" ||
      form.newPassword.value !== form.confirmPassword.value
    ) {
      feedback.textContent = accessText("passwordInvalid");
      return;
    }
    button.disabled = true;
    feedback.textContent = "";
    try {
      const response = await fetch("/password/change", {
        method: "POST",
        body: new URLSearchParams(new FormData(form)),
      });
      const result = await response.json();
      if (!response.ok || !result.changed)
        throw new Error(result.error || "saveFailed");
      location.replace("/manage?password=changed");
    } catch (error) {
      feedback.textContent = accessText(error.message);
      button.disabled = false;
    }
  });
