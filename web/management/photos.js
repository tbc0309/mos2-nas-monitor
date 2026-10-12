const crop = document.getElementById("crop"),
  cropCtx = crop.getContext("2d");
let photo = null,
  angle = 0,
  zoom = 1,
  offsetX = 0,
  offsetY = 0,
  drag = null;
let albumEntries = [];
function drawCrop() {
  cropCtx.fillStyle = "#0b1421";
  cropCtx.fillRect(0, 0, 240, 240);
  if (!photo) return;
  let w = angle % 180 ? photo.height : photo.width,
    h = angle % 180 ? photo.width : photo.height,
    scale = Math.max(240 / w, 240 / h) * zoom;
  offsetX = Math.max(
    -(w * scale - 240) / 2,
    Math.min((w * scale - 240) / 2, offsetX),
  );
  offsetY = Math.max(
    -(h * scale - 240) / 2,
    Math.min((h * scale - 240) / 2, offsetY),
  );
  cropCtx.save();
  cropCtx.translate(120 + offsetX, 120 + offsetY);
  cropCtx.rotate((angle * Math.PI) / 180);
  cropCtx.scale(scale, scale);
  cropCtx.drawImage(photo, -photo.width / 2, -photo.height / 2);
  cropCtx.restore();
}
function resetCrop() {
  zoom = 1;
  offsetX = offsetY = 0;
  document.getElementById("zoom").value = 1;
  drawCrop();
}
function rotatePhoto() {
  angle = (angle + 90) % 360;
  resetCrop();
}
document.getElementById("zoom").oninput = (e) => {
  zoom = +e.target.value;
  drawCrop();
};
document.getElementById("photoFile").onchange = (e) => {
  const file = e.target.files[0];
  if (!file) return;
  const url = URL.createObjectURL(file),
    img = new Image();
  img.onload = () => {
    photo = img;
    angle = 0;
    resetCrop();
    URL.revokeObjectURL(url);
    document.getElementById("uploadPhoto").disabled =
      !document.getElementById("photoSlot").value;
  };
  img.onerror = () => {
    URL.revokeObjectURL(url);
    document.getElementById("photoMessage").textContent =
      msg("Cannot open photo");
  };
  img.src = url;
};
crop.onpointerdown = (e) => {
  if (!photo) return;
  drag = [e.clientX, e.clientY, offsetX, offsetY];
  crop.setPointerCapture(e.pointerId);
};
crop.onpointermove = (e) => {
  if (!drag) return;
  offsetX = drag[2] + ((e.clientX - drag[0]) * 240) / crop.clientWidth;
  offsetY = drag[3] + ((e.clientY - drag[1]) * 240) / crop.clientHeight;
  drawCrop();
};
crop.onpointerup = crop.onpointercancel = () => (drag = null);
let albumRefreshBusy = false, albumLoaded = false;
function updateAlbumLabels() {
  document.getElementById("photoCount").textContent = msg("Photos") + " · " + albumEntries.length + " / 7";
  document.querySelectorAll("#album small[data-slot]").forEach(el => {
    el.textContent = msg("Photo ") + (Number(el.dataset.slot) + 1);
  });
  for (const option of document.getElementById("photoSlot").options) {
    option.textContent = option.value === "add" ? msg("Add new photo (automatic slot)") :
      msg("Replace photo ") + (Number(option.value) + 1) + msg(" (occupied)");
  }
}
async function refreshAlbum() {
  if (albumRefreshBusy) return;
  albumRefreshBusy = true;
  try {
    const entries = await (await fetch("/api/album")).json();
    albumEntries = entries;
    const slots = document.getElementById("photoSlot"),
      previous = slots.value;
    slots.replaceChildren();
    document.getElementById("photoCount").textContent =
      msg("Photos") + " · " + entries.length + " / 7";
    if (entries.length < 7) {
      let o = document.createElement("option");
      o.value = "add";
      o.textContent = msg("Add new photo (automatic slot)");
      slots.append(o);
    }
    for (const entry of entries) {
      let o = document.createElement("option");
      o.value = entry.slot;
      o.textContent =
        msg("Replace photo ") + (entry.slot + 1) + msg(" (occupied)");
      slots.append(o);
    }
    if (previous === "add" && entries.length < 7) slots.value = "add";
    document.getElementById("uploadPhoto").disabled = !photo || !slots.value;
    const container = document.getElementById("album");
    container.replaceChildren();
    for (const entry of entries) {
      let card = document.createElement("div"),
        img = document.createElement("img"),
        caption = document.createElement("small"),
        button = document.createElement("button");
      img.decoding = "async";
      caption.dataset.slot = entry.slot;
      caption.textContent = msg("Photo ") + (entry.slot + 1);
      button.textContent = msg("Delete");
      button.dataset.i18n = "Delete";
      button.onclick = async () => {
        const r = await fetch("/api/album/delete", {
          method: "POST",
          body: new URLSearchParams({
            slot: entry.slot,
            token: form.elements.token.value,
          }),
        });
        if (r.ok) refreshAlbum();
      };
      card.append(img, caption, button);
      container.append(card);
      // Only load the visible album, one image at a time. ESP8266 cannot afford
      // a browser's six parallel TCP image downloads alongside settings saves.
      await new Promise(resolve => {
        img.onload = img.onerror = resolve;
        img.src = entry.url + "&v=" + Date.now();
      });
    }
    albumLoaded = true;
  } catch (e) {
    document.getElementById("photoMessage").textContent =
      msg("Cannot load photos");
  } finally {
    albumRefreshBusy = false;
  }
}
async function uploadPhoto() {
  if (!photo) return;
  const destination = document.getElementById("photoSlot").value;
  if (!destination) return;
  if (destination !== "add" && !confirm(msg("Replace this photo?"))) return;
  let button = document.getElementById("uploadPhoto");
  button.disabled = true;
  try {
    let blob;
    for (let q = 0.9; q >= 0.3; q -= 0.1) {
      blob = await new Promise((resolve) =>
        crop.toBlob(resolve, "image/jpeg", q),
      );
      if (blob && blob.size <= 49152) break;
    }
    if (!blob || blob.size > 49152) throw Error(msg("Photo exceeds 48KB"));
    let data = new FormData();
    data.append("file", blob, "photo.jpg");
    let r = await fetch(
      "/api/album/upload?" +
        (destination === "add" ? "mode=add" : "slot=" + destination),
      {
        method: "POST",
        headers: { "X-CSRF-Token": form.elements.token.value },
        body: data,
      },
    );
    if (!r.ok) throw Error(await r.text());
    document.getElementById("photoMessage").textContent = msg("Photo saved");
    refreshAlbum();
  } catch (e) {
    document.getElementById("photoMessage").textContent = e.message;
  } finally {
    button.disabled = !photo || !document.getElementById("photoSlot").value;
  }
}
document.getElementById("photoSlot").onchange = () =>
  (document.getElementById("uploadPhoto").disabled = !photo);
drawCrop();

