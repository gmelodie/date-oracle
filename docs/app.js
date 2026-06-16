import { ESPLoader, Transport } from "https://cdn.jsdelivr.net/npm/esptool-js@0.4.5/+esm";

const MAGIC   = new TextEncoder().encode("DOIDBLOB_V01");
const STORAGE = "dateOracle.state.v1";

const BINS = [
  { file: "firmware/bootloader.bin", addr: 0x1000 },
  { file: "firmware/partitions.bin", addr: 0x8000 },
  { file: "firmware/boot_app0.bin",  addr: 0xe000 },
  { file: "firmware/firmware.bin",   addr: 0x10000, patch: true },
];

const DEFAULT_STATE = {
  categories: [
    { name: "WHAT TO EAT", items: [
      "Sushi", "Pizza, obviously", "Tacos", "That burger place",
      "Cook together", "Acai + chill", "Whatever SHE wants", "Surprise delivery",
    ] },
    { name: "DATE IDEA", items: [
      "Movie on couch", "Stargazing drive", "Board game night", "Walk the dog",
      "Bake something", "Mini road trip", "Build LEGO", "Home karaoke",
    ] },
    { name: "LOSER...", items: [
      "...does dishes", "...walks the dog", "...picks the movie",
      "...pays dinner", "...gives a massage", "...makes coffee 1wk",
    ] },
  ],
};

let state = loadState();

function loadState() {
  try {
    const s = JSON.parse(localStorage.getItem(STORAGE));
    if (s && Array.isArray(s.categories)) return s;
  } catch {}
  return structuredClone(DEFAULT_STATE);
}

function saveState() {
  localStorage.setItem(STORAGE, JSON.stringify(state));
}

const $cats     = document.getElementById("categories");
const $addCat   = document.getElementById("add-cat");
const $flash       = document.getElementById("flash");
const $status      = document.getElementById("status");
const $progress    = document.getElementById("progress");
const $progressPct = document.getElementById("progress-pct");
const $progressWrap = document.getElementById("progress-wrap");

function render() {
  $cats.replaceChildren();
  state.categories.forEach((cat, ci) => $cats.appendChild(renderCategory(cat, ci)));
}

function renderCategory(cat, ci) {
  const card = document.createElement("article");
  card.className = "cat-card";

  const head = document.createElement("div");
  head.className = "cat-card-head";

  const name = document.createElement("input");
  name.className = "cat-name";
  name.value = cat.name;
  name.placeholder = "Category";
  name.addEventListener("input", () => { cat.name = name.value; saveState(); });
  head.appendChild(name);

  const delCat = document.createElement("button");
  delCat.className = "cat-del";
  delCat.title = "remove category";
  delCat.textContent = "×";
  delCat.addEventListener("click", () => {
    if (!confirm(`Remove "${cat.name}"?`)) return;
    state.categories.splice(ci, 1);
    saveState();
    render();
  });
  head.appendChild(delCat);
  card.appendChild(head);

  const ul = document.createElement("ul");
  cat.items.forEach((item, ii) => ul.appendChild(renderItem(cat, ci, item, ii)));
  card.appendChild(ul);

  const add = document.createElement("button");
  add.className = "add-item";
  add.textContent = "+ add idea";
  add.addEventListener("click", () => {
    cat.items.push("");
    saveState();
    render();
    const inputs = $cats.children[ci].querySelectorAll("ul input");
    inputs[inputs.length - 1].focus();
  });
  card.appendChild(add);

  return card;
}

function renderItem(cat, ci, item, ii) {
  const li = document.createElement("li");

  const inp = document.createElement("input");
  inp.value = item;
  inp.placeholder = "idea";
  inp.addEventListener("input", () => { cat.items[ii] = inp.value; saveState(); });
  li.appendChild(inp);

  const del = document.createElement("button");
  del.className = "item-del";
  del.title = "remove";
  del.textContent = "×";
  del.addEventListener("click", () => {
    cat.items.splice(ii, 1);
    saveState();
    render();
  });
  li.appendChild(del);

  return li;
}

$addCat.addEventListener("click", () => {
  state.categories.push({ name: "NEW CATEGORY", items: [""] });
  saveState();
  render();
});

render();

function stateToText(s) {
  const blocks = s.categories.map(cat => {
    const items = cat.items.map(i => i.trim()).filter(Boolean);
    return `[${cat.name.trim() || "UNNAMED"}]\n${items.join("\n")}`;
  });
  return blocks.join("\n\n") + "\n";
}

function findMagic(buf) {
  outer: for (let i = 0; i <= buf.length - MAGIC.length; i++) {
    for (let j = 0; j < MAGIC.length; j++) {
      if (buf[i + j] !== MAGIC[j]) continue outer;
    }
    return i;
  }
  return -1;
}

async function patchBlob(buf, text) {
  const off = findMagic(buf);
  if (off < 0) throw new Error("magic header not found in firmware.bin (mismatched build?)");
  const dv = new DataView(buf.buffer, buf.byteOffset, buf.byteLength);
  const capacity = dv.getUint16(off + 18, true);
  const data = new TextEncoder().encode(text);
  if (data.length >= capacity) {
    throw new Error(`ideas too large: ${data.length} of ${capacity} bytes. Trim some lines.`);
  }

  const lo = off + 16;
  const hi = off + 20 + capacity;
  const before = buf.slice(lo, hi);

  dv.setUint16(off + 16, data.length, true);
  buf.set(data, off + 20);
  buf.fill(0, off + 20 + data.length, off + 20 + capacity);

  // Keep the ESP32 image trailers valid: 1-byte segment XOR checksum, then
  // 32-byte SHA256 if header byte 23 says so. Bootloader rejects on mismatch.
  let xorDelta = 0;
  for (let i = 0; i < before.length; i++) xorDelta ^= before[i] ^ buf[lo + i];
  const hashAppended = buf[23] === 1;
  buf[hashAppended ? buf.length - 33 : buf.length - 1] ^= xorDelta;

  if (hashAppended) {
    const digest = await crypto.subtle.digest("SHA-256", buf.subarray(0, buf.length - 32));
    buf.set(new Uint8Array(digest), buf.length - 32);
  }
}

function u8ToBinStr(u8) {
  let s = "";
  const chunk = 0x8000;
  for (let i = 0; i < u8.length; i += chunk) {
    s += String.fromCharCode.apply(null, u8.subarray(i, i + chunk));
  }
  return s;
}

function setStatus(msg, cls = "") {
  $status.textContent = msg;
  $status.className = cls;
}

function setProgress(pct) {
  const v = Math.max(0, Math.min(100, pct));
  $progress.value = v;
  $progressPct.textContent = `${v.toFixed(0)}%`;
}

async function fetchBin(path) {
  const r = await fetch(path, { cache: "no-cache" });
  if (!r.ok) throw new Error(`fetch ${path}: ${r.status}`);
  return new Uint8Array(await r.arrayBuffer());
}

async function resetBoard(transport) {
  await transport.setDTR(false);
  await transport.setRTS(true);
  await new Promise(r => setTimeout(r, 200));
  await transport.setRTS(false);
}

$flash.addEventListener("click", async () => {
  if (!("serial" in navigator)) {
    setStatus("Web Serial not supported. Use Chrome, Edge, Brave, or Opera.", "err");
    return;
  }

  $flash.disabled = true;
  $progressWrap.hidden = false;
  setProgress(0);
  setStatus("Loading firmware…");

  let transport;
  try {
    const files = await Promise.all(BINS.map(async b => ({
      ...b,
      data: await fetchBin(b.file),
    })));
    const text = stateToText(state);
    for (const f of files) {
      if (f.patch) await patchBlob(f.data, text);
    }
    const totalBytes = files.reduce((s, f) => s + f.data.length, 0);
    const offsets    = files.reduce((acc, f, i) => {
      acc.push((acc[i - 1] || 0) + (files[i - 1]?.data.length || 0));
      return acc;
    }, []);

    setStatus("Pick the Oracle's USB port…");
    const port = await navigator.serial.requestPort();
    transport = new Transport(port, true);
    const loader = new ESPLoader({ transport, baudrate: 921600, romBaudrate: 115200 });
    setStatus("Connecting…");
    // Fall back to no_reset for CP210x + early-rev ESP32, where the redundant
    // DTR/RTS toggle can leave SYNC unanswered.
    try { await loader.main("default_reset"); }
    catch { await loader.main("no_reset"); }

    setStatus("Flashing… keep the cable steady ♥");
    await loader.writeFlash({
      fileArray: files.map(f => ({ data: u8ToBinStr(f.data), address: f.addr })),
      flashSize: "keep",
      flashMode: "keep",
      flashFreq: "keep",
      eraseAll: false,
      compress: true,
      reportProgress: (idx, written, _total) => {
        setProgress(((offsets[idx] + written) / totalBytes) * 100);
      },
      calculateMD5Hash: null,
    });

    setStatus("Resetting…");
    try { if (typeof loader.hardReset === "function") await loader.hardReset(); } catch {}
    try { await resetBoard(transport); } catch {}
    await transport.disconnect();

    setProgress(100);
    setStatus("Flashed ♥ Oracle should be alive in a moment.", "ok");
  } catch (err) {
    console.error(err);
    setStatus(`Error: ${err.message || err}`, "err");
    if (transport) { try { await transport.disconnect(); } catch {} }
  } finally {
    $flash.disabled = false;
  }
});
