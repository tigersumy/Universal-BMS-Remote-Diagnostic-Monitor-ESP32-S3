#pragma once

#include <Arduino.h>

const char FAVICON_SVG[] PROGMEM = R"rawliteral(
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 64 64">
  <rect x="8" y="16" width="48" height="40" rx="6" fill="#1e293b" stroke="#38bdf8" stroke-width="4"/>
  <rect x="24" y="8" width="16" height="8" rx="2" fill="#38bdf8"/>
  <path d="M34 24 L24 38 L32 38 L30 48 L40 34 L32 34 Z" fill="#fbbf24"/>
</svg>
)rawliteral";

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="uk">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Universal BMS Diagnostic Monitor</title>
<link rel="icon" type="image/svg+xml" href="/favicon.svg">
<style>
:root {
  --bg: #0f172a;
  --card: #1e293b;
  --border: #334155;
  --text: #f8fafc;
  --muted: #94a3b8;
  --accent: #38bdf8;
  --accent-hover: #0284c7;
  --green: #22c55e;
  --yellow: #eab308;
  --red: #ef4444;
  --cyan: #06b6d4;
}
* { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; }
body { background: var(--bg); color: var(--text); padding: 12px; min-height: 100vh; }
.container { max-width: 960px; margin: 0 auto; display: flex; flex-direction: column; gap: 14px; }
.header { display: flex; justify-content: space-between; align-items: center; padding: 12px 16px; background: var(--card); border-radius: 12px; border: 1px solid var(--border); }
.header h1 { font-size: 1.15rem; display: flex; align-items: center; gap: 8px; }
.badge { font-size: 0.75rem; padding: 4px 8px; border-radius: 9999px; font-weight: 600; }
.badge-online { background: rgba(34, 197, 94, 0.2); color: var(--green); border: 1px solid var(--green); }
.badge-offline { background: rgba(239, 68, 68, 0.2); color: var(--red); border: 1px solid var(--red); }
.badge-ts { background: rgba(56, 189, 248, 0.2); color: var(--accent); border: 1px solid var(--accent); }

.tabs { display: flex; gap: 8px; border-bottom: 1px solid var(--border); padding-bottom: 8px; }
.tab-btn { background: transparent; border: 1px solid var(--border); color: var(--muted); padding: 8px 16px; border-radius: 8px; cursor: pointer; font-weight: 600; font-size: 0.85rem; }
.tab-btn.active { background: var(--accent); color: #000; border-color: var(--accent); }

.grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(130px, 1fr)); gap: 10px; }
.card { background: var(--card); border: 1px solid var(--border); border-radius: 12px; padding: 12px; text-align: center; }
.card .label { font-size: 0.75rem; color: var(--muted); margin-bottom: 4px; text-transform: uppercase; }
.card .value { font-size: 1.35rem; font-weight: 700; color: var(--text); }
.card .unit { font-size: 0.8rem; color: var(--muted); font-weight: 400; margin-left: 2px; }

.section { background: var(--card); border: 1px solid var(--border); border-radius: 12px; padding: 14px; }
.section h2 { font-size: 0.95rem; margin-bottom: 12px; color: var(--accent); display: flex; justify-content: space-between; align-items: center; }

.cells-grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(90px, 1fr)); gap: 8px; }
.cell-box { background: rgba(15, 23, 42, 0.6); border: 1px solid var(--border); border-radius: 8px; padding: 6px; text-align: center; }
.cell-box .c-num { font-size: 0.7rem; color: var(--muted); }
.cell-box .c-val { font-size: 1rem; font-weight: 700; margin: 2px 0; }
.cell-box.min-cell { border-color: var(--yellow); }
.cell-box.max-cell { border-color: var(--green); }

.switches { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 10px; }
.switch-item { display: flex; justify-content: space-between; align-items: center; background: rgba(15, 23, 42, 0.5); padding: 10px 14px; border-radius: 8px; border: 1px solid var(--border); }
.switch-btn { padding: 6px 14px; border-radius: 6px; border: none; font-weight: 600; cursor: pointer; }
.btn-on { background: var(--green); color: #000; }
.btn-off { background: #475569; color: #cbd5e1; }

/* Diagnostic Log Terminal */
.log-terminal { background: #050811; border: 1px solid #1e293b; border-radius: 8px; padding: 10px; height: 380px; overflow-y: auto; font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, monospace; font-size: 0.8rem; line-height: 1.45; }
.log-line { margin-bottom: 3px; word-break: break-all; }
.log-time { color: #64748b; margin-right: 6px; }
.log-tag { color: var(--accent); font-weight: 600; margin-right: 6px; }
.log-lvl-0 { color: #cbd5e1; }
.log-lvl-1 { color: var(--yellow); }
.log-lvl-2 { color: var(--red); font-weight: 600; }
.log-lvl-3 { color: var(--cyan); }
.log-lvl-4 { color: var(--green); }

.diag-controls { display: flex; gap: 8px; flex-wrap: wrap; margin-top: 10px; }
.input-raw { flex: 1; min-width: 200px; background: #0f172a; border: 1px solid var(--border); color: #fff; padding: 8px 12px; border-radius: 6px; font-family: monospace; }
.btn { background: var(--accent); color: #000; border: none; padding: 8px 14px; border-radius: 6px; font-weight: 600; cursor: pointer; }
.btn-secondary { background: #334155; color: #fff; }
.btn:hover { opacity: 0.9; }

.footer { text-align: center; font-size: 0.75rem; color: var(--muted); margin-top: 8px; }
</style>
</head>
<body>
<div class="container">
  <div class="header">
    <h1>⚡ <span id="bms-title">BMS Diagnostic Monitor</span></h1>
    <div style="display: flex; gap: 6px; align-items: center;">
      <span id="ts-badge" class="badge badge-ts">Tailscale</span>
      <span id="status-badge" class="badge badge-offline">Підключення...</span>
    </div>
  </div>

  <div class="tabs">
    <button class="tab-btn active" onclick="switchTab('tab-dash')">📊 Дашборд</button>
    <button class="tab-btn" onclick="switchTab('tab-diag')">🔍 Live BLE Діагностика</button>
    <button class="tab-btn" onclick="location.href='/setup'">⚙️ Налаштування</button>
  </div>

  <!-- TAB 1: DASHBOARD -->
  <div id="tab-dash" class="tab-content">
    <div class="grid" style="margin-bottom: 12px;">
      <div class="card"><div class="label">Загальна напруга</div><div class="value"><span id="v-tot">--</span><span class="unit">V</span></div></div>
      <div class="card"><div class="label">Струм</div><div class="value"><span id="i-cur">--</span><span class="unit">A</span></div></div>
      <div class="card"><div class="label">Потужність</div><div class="value"><span id="p-tot">--</span><span class="unit">W</span></div></div>
      <div class="card"><div class="label">Заряд (SOC)</div><div class="value"><span id="soc">--</span><span class="unit">%</span></div></div>
      <div class="card"><div class="label">Дельта комірок</div><div class="value"><span id="v-delta">--</span><span class="unit">mV</span></div></div>
      <div class="card"><div class="label">Темп. MOS / T1</div><div class="value"><span id="t-mos">--</span><span class="unit">°C</span></div></div>
    </div>

    <div class="section" style="margin-bottom: 12px;">
      <h2>🔋 Напруги комірок (<span id="cell-count-lbl">4S</span>) <span style="font-size: 0.75rem; color: var(--muted);" id="min-max-lbl"></span></h2>
      <div id="cells-container" class="cells-grid"></div>
    </div>

    <div class="section">
      <h2>🔌 Керування захистом</h2>
      <div class="switches">
        <div class="switch-item">
          <span>Заряд (Charge MOS)</span>
          <button id="btn-chg" class="switch-btn btn-off" onclick="toggleSwitch('charging')">--</button>
        </div>
        <div class="switch-item">
          <span>Розряд (Discharge MOS)</span>
          <button id="btn-dsg" class="switch-btn btn-off" onclick="toggleSwitch('discharging')">--</button>
        </div>
      </div>
    </div>
  </div>

  <!-- TAB 2: LIVE BLE DIAGNOSTICS -->
  <div id="tab-diag" class="tab-content" style="display: none;">
    <div class="section">
      <h2>
        <span>🔍 Live BLE Packet & Protocol Trace</span>
        <span style="font-size: 0.75rem; color: var(--muted);">Auto-refresh 1.5s</span>
      </h2>
      <div id="log-term" class="log-terminal">
        <div style="color: #64748b;">[Ініціалізація віддаленого логгера...]</div>
      </div>

      <div style="margin-top: 10px; display: flex; gap: 6px; flex-wrap: wrap;">
        <span style="font-size: 0.75rem; color: var(--muted); align-self: center; margin-right: 4px;">Швидкі тести JBD:</span>
        <button class="btn btn-secondary" style="font-size:0.75rem; padding: 4px 10px;" onclick="sendHexCmd('DD A5 03 00 FF FD 77')">📌 Basic Info (0x03)</button>
        <button class="btn btn-secondary" style="font-size:0.75rem; padding: 4px 10px;" onclick="sendHexCmd('DD A5 04 00 FF FC 77')">🔋 Cells (0x04)</button>
        <button class="btn btn-secondary" style="font-size:0.75rem; padding: 4px 10px;" onclick="sendHexCmd('DD A5 05 00 FF FB 77')">🏷️ Name (0x05)</button>
        <button class="btn btn-secondary" style="font-size:0.75rem; padding: 4px 10px;" onclick="sendHexCmd('FF AA 15 06 31 32 33 34 35 36 53')">🔑 PIN 123456</button>
        <button class="btn btn-secondary" style="font-size:0.75rem; padding: 4px 10px;" onclick="sendHexCmd('FF AA 15 04 31 32 33 34 47')">🔑 PIN 1234</button>
        <button class="btn btn-secondary" style="font-size:0.75rem; padding: 4px 10px;" onclick="sendHexCmd('FF AA 15 06 30 30 30 30 30 30 3B')">🔑 PIN 000000</button>
      </div>

      <div class="diag-controls">
        <input type="text" id="raw-hex-input" class="input-raw" placeholder="HEX команда (наприклад: DD A5 03 00 FF FD 77)">
        <button class="btn" onclick="sendRawHex()">Надіслати HEX</button>
        <button class="btn btn-secondary" onclick="reconnectBle()">🔄 Перепідключити BLE</button>
        <button class="btn btn-secondary" onclick="clearLog()">🗑️ Очистити</button>
        <button class="btn btn-secondary" onclick="copyLog()">📋 Скопіювати</button>
      </div>
    </div>
  </div>

  <div class="footer">
    Universal BMS Diagnostic Monitor | ESP32-S3 (Tailscale Probe) | <span id="ts-info">VPN Connected</span>
  </div>
</div>

<script>
let currentTab = 'tab-dash';
let diagInterval = null;
let rawLogData = [];
let switchStates = { charging: false, discharging: false };

function switchTab(tabId) {
  document.querySelectorAll('.tab-btn').forEach(b => b.classList.remove('active'));
  document.querySelectorAll('.tab-content').forEach(c => c.style.display = 'none');
  
  if (tabId === 'tab-dash') {
    document.querySelectorAll('.tab-btn')[0].classList.add('active');
    document.getElementById('tab-dash').style.display = 'block';
  } else if (tabId === 'tab-diag') {
    document.querySelectorAll('.tab-btn')[1].classList.add('active');
    document.getElementById('tab-diag').style.display = 'block';
    fetchDebugLog();
  }
}

async function updateData() {
  try {
    const res = await fetch('/api/data');
    if (!res.ok) return;
    const d = await res.json();

    document.getElementById('bms-title').innerText = (d.bms_type || 'BMS') + ' Monitor';
    const sBadge = document.getElementById('status-badge');
    if (d.connected) {
      sBadge.innerText = '● ' + (d.bms_type || 'BMS') + ' Онлайн';
      sBadge.className = 'badge badge-online';
    } else {
      sBadge.innerText = '○ Відключено';
      sBadge.className = 'badge badge-offline';
    }

    const tsBadge = document.getElementById('ts-badge');
    if (d.ts_connected) {
      tsBadge.innerText = 'Tailscale: ' + (d.ts_ip || 'Connected');
      tsBadge.className = 'badge badge-ts';
    } else {
      tsBadge.innerText = 'Tailscale: ' + (d.ts_status || 'Offline');
      tsBadge.className = 'badge badge-offline';
    }

    document.getElementById('v-tot').innerText = (d.total_voltage || 0).toFixed(2);
    document.getElementById('i-cur').innerText = (d.current || 0).toFixed(2);
    document.getElementById('p-tot').innerText = (d.power || 0).toFixed(1);
    document.getElementById('soc').innerText = Math.round(d.soc || 0);
    document.getElementById('v-delta').innerText = ((d.delta_cell_v || 0) * 1000).toFixed(0);
    document.getElementById('t-mos').innerText = (d.temp_mos || d.temp_sensor1 || 0).toFixed(1);

    // Render cells
    document.getElementById('cell-count-lbl').innerText = (d.cell_count || 4) + 'S';
    document.getElementById('min-max-lbl').innerText = `Min: C${d.min_cell_idx || 1} (${((d.min_cell_v || 0)).toFixed(3)}V) | Max: C${d.max_cell_idx || 1} (${((d.max_cell_v || 0)).toFixed(3)}V)`;

    const cContainer = document.getElementById('cells-container');
    cContainer.innerHTML = '';
    const cells = d.cells || [];
    for (let i = 0; i < (d.cell_count || cells.length); i++) {
      const v = cells[i] || 0;
      const isMin = (i + 1 === d.min_cell_idx && v > 0.5);
      const isMax = (i + 1 === d.max_cell_idx && v > 0.5);
      const cDiv = document.createElement('div');
      cDiv.className = 'cell-box ' + (isMin ? 'min-cell' : (isMax ? 'max-cell' : ''));
      cDiv.innerHTML = `<div class="c-num">C${i + 1}</div><div class="c-val">${v.toFixed(3)}<span style="font-size:0.65rem;color:var(--muted)">V</span></div>`;
      cContainer.appendChild(cDiv);
    }

    // Update switches
    switchStates.charging = d.switch_charging;
    switchStates.discharging = d.switch_discharging;
    const btnChg = document.getElementById('btn-chg');
    const btnDsg = document.getElementById('btn-dsg');
    btnChg.innerText = d.switch_charging ? 'ВКЛ' : 'ВИКЛ';
    btnChg.className = 'switch-btn ' + (d.switch_charging ? 'btn-on' : 'btn-off');
    btnDsg.innerText = d.switch_discharging ? 'ВКЛ' : 'ВИКЛ';
    btnDsg.className = 'switch-btn ' + (d.switch_discharging ? 'btn-on' : 'btn-off');

  } catch (e) {
    console.warn('Update failed:', e);
  }
}

async function toggleSwitch(sw) {
  const nextState = !switchStates[sw];
  try {
    await fetch('/api/switch', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ switch: sw, state: nextState })
    });
    setTimeout(updateData, 400);
  } catch (e) {
    alert('Помилка перемикання');
  }
}

async function fetchDebugLog() {
  try {
    const res = await fetch('/api/debug-log');
    if (!res.ok) return;
    const logs = await res.json();
    rawLogData = logs;
    const term = document.getElementById('log-term');
    const wasAtBottom = (term.scrollHeight - term.clientHeight <= term.scrollTop + 30);

    let html = '';
    for (const l of logs) {
      const sec = (l.time / 1000).toFixed(2);
      html += `<div class="log-line"><span class="log-time">[${sec}s]</span><span class="log-tag">[${l.tag}]</span><span class="log-lvl-${l.lvl}">${escapeHtml(l.msg)}</span></div>`;
    }
    term.innerHTML = html || '<div style="color:#64748b;">[Логів поки немає]</div>';
    if (wasAtBottom) {
      term.scrollTop = term.scrollHeight;
    }
  } catch (e) {
    console.warn('Debug log fetch failed:', e);
  }
}

async function sendRawHex() {
  const input = document.getElementById('raw-hex-input');
  const hex = input.value.trim();
  if (!hex) return;
  await sendHexCmd(hex);
  input.value = '';
}

async function sendHexCmd(hex) {
  try {
    const res = await fetch('/api/send-raw-ble', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ hex: hex })
    });
    const ans = await res.json();
    if (ans.status === 'ok') {
      setTimeout(fetchDebugLog, 300);
    } else {
      alert('Помилка: ' + (ans.error || 'Failed'));
    }
  } catch (e) {
    alert('Помилка відправки: ' + e);
  }
}

async function reconnectBle() {
  if (!confirm('Виконати повторне підключення до BMS?')) return;
  await fetch('/api/reconnect-ble', { method: 'POST' });
  setTimeout(fetchDebugLog, 500);
}

async function clearLog() {
  await fetch('/api/clear-log', { method: 'POST' });
  document.getElementById('log-term').innerHTML = '<div style="color:#64748b;">[Лог очищено]</div>';
}

function copyLog() {
  let txt = '';
  for (const l of rawLogData) {
    txt += `[${(l.time/1000).toFixed(2)}s] [${l.tag}] ${l.msg}\n`;
  }
  navigator.clipboard.writeText(txt).then(() => alert('Логи скопійовано в буфер!'));
}

function escapeHtml(str) {
  return (str || '').replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");
}

setInterval(updateData, 2000);
setInterval(() => {
  if (document.getElementById('tab-diag').style.display !== 'none') {
    fetchDebugLog();
  }
}, 1500);
updateData();
</script>
</body>
</html>
)rawliteral";

const char SETUP_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="uk">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Налаштування BMS Diagnostic Monitor</title>
<link rel="icon" type="image/svg+xml" href="/favicon.svg">
<style>
:root { --bg: #0f172a; --card: #1e293b; --border: #334155; --text: #f8fafc; --muted: #94a3b8; --accent: #38bdf8; --green: #22c55e; }
* { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; }
body { background: var(--bg); color: var(--text); padding: 14px; min-height: 100vh; display: flex; justify-content: center; }
.box { max-width: 520px; width: 100%; background: var(--card); border: 1px solid var(--border); border-radius: 14px; padding: 20px; }
h1 { font-size: 1.25rem; margin-bottom: 16px; color: var(--accent); }
.field { margin-bottom: 14px; }
label { display: block; font-size: 0.8rem; color: var(--muted); margin-bottom: 5px; font-weight: 600; text-transform: uppercase; }
input, select { width: 100%; padding: 10px; border-radius: 8px; border: 1px solid var(--border); background: #0f172a; color: #fff; font-size: 0.95rem; }
.btn-row { display: flex; gap: 8px; margin-top: 6px; }
.btn { flex: 1; background: var(--accent); color: #000; border: none; padding: 10px; border-radius: 8px; font-weight: 700; cursor: pointer; }
.btn-sec { background: #334155; color: #fff; }
.btn-save { background: var(--green); color: #000; width: 100%; margin-top: 14px; }
</style>
</head>
<body>
<div class="box">
  <h1>⚙️ Налаштування адаптера</h1>

  <div class="field">
    <label>Wi-Fi Мережа (SSID)</label>
    <input type="text" id="wifi-ssid" placeholder="Назва домашньої мережі">
    <div class="btn-row"><button class="btn btn-sec" onclick="scanWifi()">🔄 Сканувати Wi-Fi</button></div>
    <select id="wifi-select" style="display:none; margin-top:6px;" onchange="document.getElementById('wifi-ssid').value=this.value"></select>
  </div>

  <div class="field">
    <label>Пароль Wi-Fi</label>
    <input type="password" id="wifi-pass" placeholder="Пароль Wi-Fi">
  </div>

  <div class="field">
    <label>Bluetooth MAC-адреса BMS</label>
    <input type="text" id="bms-mac" placeholder="AA:BB:CC:DD:EE:FF">
    <div class="btn-row"><button class="btn btn-sec" onclick="scanBle()">🔍 Знайти BMS в ефірі</button></div>
    <select id="ble-select" style="display:none; margin-top:6px;" onchange="applyBleChoice(this.value)"></select>
  </div>

  <div class="field">
    <label>Тип BMS</label>
    <select id="bms-type">
      <option value="0">Auto Detect (JBD / JK)</option>
      <option value="2">JBD-BMS (Xiaoxiang / Smart BMS)</option>
      <option value="1">JK-BMS (Hankzor / JiKong)</option>
    </select>
  </div>

  <div class="field">
    <label>Bluetooth PIN / Пароль (JBD)</label>
    <input type="text" id="bms-pin" value="123456" placeholder="123456 або 1234">
  </div>

  <div class="field">
    <label>Конфігурація комірок</label>
    <select id="cell-count">
      <option value="4">4S (12V)</option>
      <option value="8">8S (24V)</option>
      <option value="16">16S (48V)</option>
      <option value="24">24S (72V)</option>
    </select>
  </div>

  <div class="field">
    <label>Tailscale VPN Hostname</label>
    <input type="text" id="ts-host" value="jbd-bms-probe">
  </div>

  <div class="field">
    <label>Tailscale Auth Key</label>
    <input type="password" id="ts-key" placeholder="tskey-auth-..." autocomplete="off">
  </div>

  <button class="btn btn-save" onclick="saveConfig()">💾 Зберегти та перезавантажити</button>
  <div style="margin-top: 10px; text-align: center;">
    <a href="/" style="color: var(--accent); font-size: 0.85rem; text-decoration: none;">← Повернутися до Дашборду</a>
  </div>
</div>

<script>
async function loadConfig() {
  try {
    const res = await fetch('/api/config');
    const d = await res.json();
    if (d.ssid) document.getElementById('wifi-ssid').value = d.ssid;
    if (d.mac) document.getElementById('bms-mac').value = d.mac;
    if (d.pin) document.getElementById('bms-pin').value = d.pin;
    if (d.bms_type !== undefined) document.getElementById('bms-type').value = d.bms_type;
    if (d.cells) document.getElementById('cell-count').value = d.cells;
    if (d.ts_hostname) document.getElementById('ts-host').value = d.ts_hostname;
  } catch(e) {}
}

async function scanWifi() {
  const sel = document.getElementById('wifi-select');
  sel.style.display = 'block';
  sel.innerHTML = '<option>Сканування...</option>';
  try {
    const res = await fetch('/api/scan-wifi');
    const list = await res.json();
    sel.innerHTML = '<option value="">-- Оберіть мережу --</option>';
    list.forEach(w => {
      sel.innerHTML += `<option value="${w.ssid}">${w.ssid} (${w.rssi} dBm)</option>`;
    });
  } catch(e) { sel.innerHTML = '<option>Помилка сканування</option>'; }
}

async function scanBle() {
  const sel = document.getElementById('ble-select');
  sel.style.display = 'block';
  sel.innerHTML = '<option>Сканування Bluetooth (4с)...</option>';
  try {
    const res = await fetch('/api/scan-ble');
    const list = await res.json();
    sel.innerHTML = '<option value="">-- Оберіть знайдену BMS --</option>';
    list.forEach(b => {
      sel.innerHTML += `<option value="${b.mac}|${b.name}|${b.type}">${b.name} [${b.mac}] (${b.rssi} dBm, ${b.type})</option>`;
    });
  } catch(e) { sel.innerHTML = '<option>Помилка сканування</option>'; }
}

function applyBleChoice(val) {
  if (!val) return;
  const parts = val.split('|');
  document.getElementById('bms-mac').value = parts[0];
  if (parts[2] === 'JBD-BMS') document.getElementById('bms-type').value = "2";
  else if (parts[2] === 'JK-BMS') document.getElementById('bms-type').value = "1";
}

async function saveConfig() {
  const cfg = {
    ssid: document.getElementById('wifi-ssid').value.trim(),
    pass: document.getElementById('wifi-pass').value,
    mac: document.getElementById('bms-mac').value.trim(),
    pin: document.getElementById('bms-pin').value.trim(),
    bms_type: parseInt(document.getElementById('bms-type').value) || 0,
    cells: parseInt(document.getElementById('cell-count').value) || 4,
    ts_enabled: true,
    ts_hostname: document.getElementById('ts-host').value.trim() || 'jbd-bms-probe',
    ts_auth_key: document.getElementById('ts-key').value.trim()
  };

  try {
    await fetch('/api/save-config', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(cfg)
    });
    alert('Конфігурацію збережено! Адаптер перезавантажується...');
  } catch(e) { alert('Помилка збереження'); }
}

loadConfig();
</script>
</body>
</html>
)rawliteral";
