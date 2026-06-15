#pragma once
/* Embedded index page — linked into firmware flash (PROGMEM), no SPIFFS. */
#include <Arduino.h>
#include "DF3MT_Config.h"

static const char kIndexHtml[] PROGMEM =
  R"DF3MT_H1(
<!DOCTYPE html>
<html lang="de">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
  <meta name="theme-color" content="#0c1222">
  <title>DF3MT-Rotor</title>
  
  <style>
    :root {
      --bg0: #0c1222;
      --bg1: #141c32;
      --surface: rgba(255,255,255,0.06);
      --surface2: rgba(255,255,255,0.1);
      --border: rgba(255,255,255,0.12);
      --text: #eef2ff;
      --muted: #94a3b8;
      --accent: #38bdf8;
      --accent2: #a78bfa;
      --danger: #fb7185;
      --ccw: #34d399;
      --cw: #60a5fa;
      --ok: #4ade80;
      --shadow: 0 24px 48px rgba(0,0,0,0.45);
      --radius: clamp(12px, 3vw, 20px);
      --tap-min: max(48px, 12mm);
      --font: system-ui, -apple-system, "Segoe UI", Roboto, "Helvetica Neue", Arial, sans-serif;
      --mono: ui-monospace, "Cascadia Mono", "SF Mono", Menlo, Consolas, "Liberation Mono", monospace;
    }
    @media (prefers-color-scheme: light) {
      :root {
        --bg0: #f1f5f9;
        --bg1: #e2e8f0;
        --surface: rgba(15,23,42,0.06);
        --surface2: rgba(15,23,42,0.1);
        --border: rgba(15,23,42,0.12);
        --text: #0f172a;
        --muted: #64748b;
        --shadow: 0 20px 40px rgba(15,23,42,0.12);
      }
    }
    @media (prefers-reduced-motion: reduce) {
      * { animation-duration: 0.01ms !important; animation-iteration-count: 1 !important; transition-duration: 0.01ms !important; }
    }
    *, *::before, *::after { box-sizing: border-box; }
    html { -webkit-text-size-adjust: 100%; }
    body {
      margin: 0;
      min-height: 100dvh;
      font-family: var(--font);
      color: var(--text);
      background:
        radial-gradient(120% 80% at 100% 0%, rgba(56,189,248,0.15) 0%, transparent 50%),
        radial-gradient(80% 60% at 0% 100%, rgba(167,139,250,0.12) 0%, transparent 45%),
        linear-gradient(165deg, var(--bg0), var(--bg1));
      padding: max(16px, env(safe-area-inset-top)) max(16px, env(safe-area-inset-right)) max(24px, env(safe-area-inset-bottom)) max(16px, env(safe-area-inset-left));
    }
    .wrap {
      width: min(100%, 420px);
      margin-inline: auto;
      display: flex;
      flex-direction: column;
      gap: clamp(1rem, 4vw, 1.5rem);
    }
    #motorUiBlock {
      min-width: 0;
      width: 100%;
    }
    header {
      display: flex;
      flex-direction: column;
      align-items: flex-start;
      gap: 0.35rem;
    }
    h1 {
      margin: 0;
      font-size: clamp(1.35rem, 4.5vw, 1.75rem);
      font-weight: 700;
      letter-spacing: -0.03em;
    }
    .head-links {
      margin: 0;
      font-size: 0.78rem;
      font-weight: 500;
      line-height: 1.45;
      color: var(--muted);
    }
    .head-links a {
      color: var(--accent);
      text-decoration: none;
    }
    .head-links a:hover { text-decoration: underline; }
    .head-links .head-sep {
      opacity: 0.45;
      margin: 0 0.35em;
    }
    h2 {
      margin: 0 0 0.65rem 0;
      font-size: 0.78rem;
      font-weight: 700;
      text-transform: uppercase;
      letter-spacing: 0.06em;
      color: var(--muted);
    }
    .badge {
      font-size: 0.7rem;
      font-weight: 600;
      text-transform: uppercase;
      letter-spacing: 0.08em;
      color: var(--muted);
      padding: 0.35rem 0.65rem;
      border-radius: 999px;
      border: 1px solid var(--border);
      background: var(--surface);
    }
    .panel {
      border-radius: var(--radius);
      border: 1px solid var(--border);
      background: var(--surface);
      box-shadow: var(--shadow);
      padding: clamp(1rem, 4vw, 1.35rem);
      backdrop-filter: blur(12px);
    }
    .speed-row {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 1rem;
      margin-bottom: 0.35rem;
    }
    .speed-label { font-size: 0.85rem; color: var(--muted); font-weight: 600; }
    .speed-val {
      font-family: var(--mono);
      font-size: clamp(1.75rem, 8vw, 2.25rem);
      font-weight: 700;
      letter-spacing: -0.04em;
      line-height: 1;
      background: linear-gradient(135deg, var(--accent), var(--accent2));
      -webkit-background-clip: text;
      background-clip: text;
      color: transparent;
    }
    .speed-max { font-size: 0.9rem; color: var(--muted); font-weight: 500; }
    .speed-val.speed-val--neg {
      background: linear-gradient(135deg, #34d399, #6ee7b7);
      -webkit-background-clip: text;
      background-clip: text;
      color: transparent;
    }
    .speed-val.speed-val--pos {
      background: linear-gradient(135deg, #93c5fd, #60a5fa);
      -webkit-background-clip: text;
      background-clip: text;
      color: transparent;
    }
    .speed-val.speed-val--zero {
      background: none;
      -webkit-background-clip: unset;
      background-clip: unset;
      color: var(--muted);
    }
    .meter-bi-wrap {
      position: relative;
      margin-top: 0.75rem;
    }
    .meter-bi {
      height: 10px;
      border-radius: 999px;
      background: var(--surface2);
      position: relative;
      overflow: hidden;
    }
    .meter-bi-center {
      position: absolute;
      left: 50%;
      top: -3px;
      bottom: -3px;
      width: 2px;
      margin-left: -1px;
      background: var(--text);
      opacity: 0.35;
      border-radius: 1px;
      z-index: 2;
      pointer-events: none;
    }
    .meter-bi-fill {
      position: absolute;
      top: 0;
      left: 50%;
      height: 100%;
      width: 0;
      border-radius: 999px;
      transition: left 0.08s ease-out, width 0.08s ease-out, background 0.12s ease;
    }
    .maxspeed-head {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 0.75rem;
      margin-bottom: 0.5rem;
    }
    .maxspeed-head label {
      font-size: 0.78rem;
      font-weight: 600;
      color: var(--muted);
      text-transform: uppercase;
      letter-spacing: 0.05em;
    }
    .maxspeed-val {
      font-family: var(--mono);
      font-size: 1rem;
      font-weight: 700;
      color: var(--accent2);
    }
    input[type="range"]#maxSpd {
      display: block;
      width: 100%;
      height: 2.25rem;
      margin: 0;
      padding: 0;
      background: transparent;
      -webkit-appearance: none;
      appearance: none;
    }
    input[type="range"]#maxSpd::-webkit-slider-runnable-track {
      height: 8px;
      border-radius: 999px;
      background: var(--surface2);
      border: 1px solid var(--border);
    }
    input[type="range"]#maxSpd::-webkit-slider-thumb {
      -webkit-appearance: none;
      appearance: none;
      width: 22px;
      height: 22px;
      border-radius: 50%;
      margin-top: -8px;
      background: linear-gradient(135deg, var(--accent), var(--accent2));
      border: 2px solid var(--surface);
      box-shadow: var(--shadow);
      cursor: pointer;
    }
    input[type="range"]#maxSpd::-moz-range-track {
      height: 8px;
      border-radius: 999px;
      background: var(--surface2);
      border: 1px solid var(--border);
    }
    input[type="range"]#maxSpd::-moz-range-thumb {
      width: 22px;
      height: 22px;
      border-radius: 50%;
      border: 2px solid var(--surface);
      background: linear-gradient(135deg, var(--accent), var(--accent2));
      cursor: pointer;
    }
    .hint {
      margin: 0;
      font-size: clamp(0.8rem, 2.8vw, 0.9rem);
      color: var(--muted);
      line-height: 1.45;
    }
    .form-row { margin-bottom: 0.75rem; }
    .form-row label {
      display: block;
      font-size: 0.78rem;
      font-weight: 600;
      color: var(--muted);
      margin-bottom: 0.35rem;
    }
    .form-row input {
      width: 100%;
      min-height: 44px;
      padding: 0.5rem 0.75rem;
      border-radius: calc(var(--radius) - 6px);
      border: 1px solid var(--border);
      background: var(--surface2);
      color: var(--text);
      font-family: inherit;
      font-size: 1rem;
    }
    .form-row input:focus {
      outline: 2px solid var(--accent);
      outline-offset: 2px;
    }
    .form-row-check label {
      display: flex;
      align-items: center;
      gap: 0.5rem;
      cursor: pointer;
      margin-bottom: 0;
    }
    .form-row-check input[type="checkbox"] {
      width: auto;
      min-height: auto;
      margin: 0;
    }
    .form-row input[type="file"] {
      width: 100%;
      font-size: 0.85rem;
      color: var(--muted);
      padding: 0.4rem 0;
      min-height: auto;
      border: none;
      background: transparent;
    }
    .ota-divider {
      border: none;
      border-top: 1px solid var(--border);
      margin: 1rem 0;
    }
    .wifi-actions {
      display: flex;
      flex-wrap: wrap;
      gap: 0.5rem;
      margin-top: 0.85rem;
    }
    .wifi-actions .btn { flex: 1; min-width: 8rem; }
    .status-line {
      font-family: var(--mono);
      font-size: 0.72rem;
      color: var(--muted);
      line-height: 1.5;
      margin-top: 0.75rem;
      word-break: break-all;
    }
    .status-line .ok { color: var(--ok); }
    .controls {
      display: grid;
      width: 100%;
      /* minmax(0,1fr): Spalten dürfen schmaler als langer Button-Text werden (sonst Overflow). */
      grid-template-columns: minmax(0, 1fr) minmax(0, 1fr);
      grid-template-areas:
        "ccw cw"
        "stop stop";
      gap: clamp(0.65rem, 2.5vw, 0.85rem);
    }
    @media (min-width: 380px) {
      .controls {
        grid-template-columns: minmax(0, 1fr) minmax(0, 1fr) minmax(0, 1fr);
        grid-template-areas: "ccw stop cw";
      }
    }
    .controls .btn {
      min-width: 0;
      max-width: 100%;
      overflow-wrap: anywhere;
      text-align: center;
      line-height: 1.25;
      padding-left: clamp(0.35rem, 2vw, 0.75rem);
      padding-right: clamp(0.35rem, 2vw, 0.75rem);
    }
    .btn {
      appearance: none;
      border: 1px solid var(--border);
      border-radius: calc(var(--radius) - 4px);
      min-height: var(--tap-min);
      padding: 0.75rem 1rem;
      font-family: inherit;
      font-size: clamp(0.95rem, 3.5vw, 1.05rem);
      font-weight: 600;
      cursor: pointer;
      color: var(--text);
      background: var(--surface2);
      transition: transform 0.12s ease, box-shadow 0.12s ease, border-color 0.12s ease;
      touch-action: manipulation;
      user-select: none;
      -webkit-tap-highlight-color: transparent;
    }
    .btn:active { transform: scale(0.97); }
    .btn:focus-visible {
      outline: 2px solid var(--accent);
      outline-offset: 3px;
    }
    .btn-primary {
      border-color: rgba(56,189,248,0.5);
      background: rgba(56,189,248,0.18);
    }
    .btn-danger {
      border-color: rgba(251,113,133,0.45);
      background: rgba(251,113,133,0.12);
    }
    #ccw { grid-area: ccw; border-color: rgba(52,211,153,0.45); background: rgba(52,211,153,0.14); }
    #cw { grid-area: cw; border-color: rgba(96,165,250,0.45); background: rgba(96,165,250,0.14); }
    #stop { grid-area: stop; border-color: rgba(251,113,133,0.45); background: rgba(251,113,133,0.12); }
    .motor-lock-banner {
      font-size: 0.85rem;
      padding: 0.65rem 0.85rem;
      margin-top: 0.65rem;
      border-radius: var(--radius);
      border: 1px solid rgba(56,189,248,0.45);
      background: rgba(56,189,248,0.1);
      color: var(--text);
      line-height: 1.45;
    }
    .motor-lock-banner.motor-lock-banner--held {
      border-color: rgba(52,211,153,0.45);
      background: rgba(52,211,153,0.1);
    }
    footer {
      text-align: center;
      font-size: 0.72rem;
      color: var(--muted);
      letter-spacing: 0.02em;
    }
    .foot-links {
      margin: 0 0 0.4rem 0;
      line-height: 1.5;
    }
    .foot-links a {
      color: var(--accent);
      text-decoration: none;
    }
    .foot-links a:hover { text-decoration: underline; }
    .foot-sep {
      opacity: 0.45;
      margin: 0 0.35em;
    }
    .foot-ip { font-size: 0.7rem; }
  </style>
</head>
<body>
  <div class="wrap">
    <header>
      <h1>DF3MT-Rotor</h1>
      <p class="head-links">
        <a href="https://df3mt.de" target="_blank" rel="noopener noreferrer">df3mt.de</a><span class="head-sep" aria-hidden="true">·</span><a href="https://github.com/DF3MT/PortableRotor" target="_blank" rel="noopener noreferrer">PortableRotor auf GitHub (Open Source)</a>
      </p>
    </header>
    <div id="motorUiBlock">
    <div class="panel">
      <div class="speed-row">
        <span class="speed-label">Speed</span>
        <span><span class="speed-val speed-val--zero" id="sv">0</span><span class="speed-max" id="speedMaxCap"> ±255</span></span>
      </div>
      <div class="meter-bi-wrap" aria-hidden="true">
        <div class="meter-bi-center"></div>
        <div class="meter-bi"><i id="meterFill" class="meter-bi-fill"></i></div>
      </div>
      <p class="hint" id="motorHintSign">Mitte = Stopp (0). CCW (Gegenuhrzeigersinn): negativ bis −255. CW: positiv bis +255.</p>
      <p class="hint">Gedrückt halten: Leistung erhöhen. Loslassen oder Stopp: sanft zur Mitte (0).</p>
    </div>

    <div class="panel">
      <div class="maxspeed-head">
        <label for="maxSpd">Max. Speed</label>
        <span class="maxspeed-val" id="maxSpdLbl">255</span>
      </div>
      <input type="range" id="maxSpd" name="maxSpd" min="1" max=")DF3MT_H1" DF3MT_XSTR(DF3MT_MOTOR_PWM_MAX) R"DF3MT_H1M(" value=")DF3MT_H1M" DF3MT_XSTR(DF3MT_MOTOR_PWM_MAX) R"DF3MT_H1N(" step="1" aria-label="Maximale Motor-PWM">)DF3MT_H1N"
  R"DF3MT_H1(
    </div>
    <div class="panel">
      <div class="controls">
        <button type="button" class="btn" id="ccw" data-dir="-1">gegen den Uhrzeigersinn</button>
        <button type="button" class="btn" id="stop">Stopp</button>
        <button type="button" class="btn" id="cw" data-dir="1">im Uhrzeigersinn</button>
      </div>
    </div>
    <div class="motor-lock-banner" id="motorLockBanner" role="status" aria-live="polite"></div>
    </div>
    <div class="panel">
      <h2>Heim-WLAN (STA)</h2>
      <p class="hint">Zugangsdaten werden im Flash gespeichert. Der konfigurierbare Access-Point bleibt parallel erreichbar.</p>
      <div class="form-row">
        <label for="wifiSsid">SSID</label>
        <input type="text" id="wifiSsid" name="ssid" autocomplete="off" placeholder="Netzwerkname">
      </div>
      <div class="form-row">
        <label for="wifiPass">Passwort</label>
        <input type="password" id="wifiPass" name="password" autocomplete="current-password" placeholder="leer = gespeichertes behalten (gleiche SSID)">
      </div>
      <div class="wifi-actions">
        <button type="button" class="btn btn-primary" id="wifiSave">Speichern &amp; verbinden</button>
        <button type="button" class="btn btn-danger" id="wifiClear">WLAN-Daten löschen</button>
      </div>
      <div class="status-line" id="wifiStatus" aria-live="polite">…</div>
    </div>
    <div class="panel">
      <h2>MQTT (Home Assistant)</h2>
      <p class="hint">Braucht <strong>Heim-WLAN (STA)</strong>. Bibliothek <strong>PubSubClient</strong> installieren. Nach Verbindung erscheint in HA eine Number (−255…255); Befehl = reine Zahl auf <code>…/set</code>, Status auf <code>…/state</code>.</p>
      <div class="form-row form-row-check">
        <label for="mqttEn"><input type="checkbox" id="mqttEn" name="mqtt_en"> MQTT aktiv</label>
      </div>
      <div class="form-row">
        <label for="mqttHost">Broker-Host</label>
        <input type="text" id="mqttHost" name="mqtt_host" autocomplete="off" placeholder="z. B. homeassistant.local oder IP">
      </div>
      <div class="form-row">
        <label for="mqttPort">Port</label>
        <input type="number" id="mqttPort" name="mqtt_port" min="1" max="65535" value="1883">
      </div>
      <div class="form-row">
        <label for="mqttUser">Benutzer (optional)</label>
        <input type="text" id="mqttUser" name="mqtt_user" autocomplete="off">
      </div>
      <div class="form-row">
        <label for="mqttPass">Passwort (optional)</label>
        <input type="password" id="mqttPass" name="mqtt_pass" autocomplete="off" placeholder="leer = gespeichertes behalten">
      </div>
      <div class="form-row">
        <label for="mqttPrefix">Topic-Prefix</label>
        <input type="text" id="mqttPrefix" name="mqtt_prefix" autocomplete="off" placeholder="df3mt/rotor" value="df3mt/rotor">
      </div>
      <div class="wifi-actions">
        <button type="button" class="btn btn-primary" id="mqttSave">MQTT speichern</button>
        <button type="button" class="btn btn-danger" id="mqttClear">MQTT löschen</button>
      </div>
      <div class="status-line" id="mqttStatus" aria-live="polite"></div>
    </div>
    <div class="panel">
      <h2>Firmware (OTA)</h2>
      <p class="hint">.bin aus der Arduino-IDE: <strong>Sketch</strong> → <strong>Export kompiliertes Binary</strong>. Nach Upload startet der ESP neu (kurz warten, dann Seite neu laden).</p>
      <p class="hint" id="otaCompileHint" style="display:none">Zusätzlich ist ein OTA-Passwort im Sketch (#define) hinterlegt — ein hier gespeichertes Passwort (NVS) hat nach dem Speichern Vorrang.</p>
      <div class="form-row" id="otaUploadPwRow" style="display:none">
        <label for="otaUploadPw">OTA-Passwort für Upload</label>
        <input type="password" id="otaUploadPw" name="ota_upload_pw" autocomplete="off" placeholder="erforderlich wenn gesetzt">
      </div>
      <div class="form-row">
        <label for="otaFile">Firmware-Datei (.bin)</label>
        <input type="file" id="otaFile" name="firmware" accept=".bin,.binary,application/octet-stream">
      </div>
      <button type="button" class="btn btn-primary" id="otaFlash">Firmware flashen</button>
      <div class="status-line" id="otaFlashStatus" aria-live="polite"></div>
      <hr class="ota-divider">
      <p class="hint">OTA-Passwort gilt für <strong>Web-Upload</strong> und <strong>Arduino-IDE</strong> (Netzwerk-Port). Wird im Flash (NVS) gespeichert — nur im vertrauenswürdigen Netz nutzen.</p>
      <div class="form-row" id="otaCurRow" style="display:none">
        <label for="otaCurPw">Aktuelles OTA-Passwort</label>
        <input type="password" id="otaCurPw" name="ota_cur_pw" autocomplete="off" placeholder="zum Ändern oder Löschen nötig">
      </div>
      <div class="form-row">
        <label for="otaNewPw">Neues OTA-Passwort</label>
        <input type="password" id="otaNewPw" name="ota_new_pw" autocomplete="new-password" placeholder="mind. 1 Zeichen">
      </div>
      <div class="wifi-actions">
        <button type="button" class="btn btn-primary" id="otaSavePw">OTA-Passwort speichern</button>
        <button type="button" class="btn btn-danger" id="otaClearPw">OTA-Passwort löschen (NVS)</button>
      </div>
      <div class="status-line" id="otaPwStatus" aria-live="polite"></div>
    </div>
    <footer>
      <p class="foot-links">
        <a href="https://df3mt.de" target="_blank" rel="noopener noreferrer">Website: df3mt.de</a><span class="foot-sep" aria-hidden="true">·</span><a href="https://github.com/DF3MT/PortableRotor" target="_blank" rel="noopener noreferrer">Quellcode: PortableRotor (GPL-3.0)</a>
      </p>
      <div class="foot-ip" id="footIpLine">…</div>
    </footer>
  </div>
  <script>
)DF3MT_H1"
  R"DF3MT_H2(
(function () {
  var _cfg = ")DF3MT_H2" DF3MT_UI_NUMPACK_STR R"DF3MT_H2P(".split(",").map(function (x) { return parseInt(x, 10); });)DF3MT_H2P"
  R"DF3MT_H2(
  var ABS_PWM_MAX = _cfg[0];
  var RAMP_UP_MS = _cfg[1];
  var RAMP_UP_STEP = _cfg[2];
  var RAMP_DOWN_MS = _cfg[3];
  var RAMP_DOWN_STEP = _cfg[4];
  var MOTOR_TX_POLL_MS = _cfg[5];
  var MOTOR_LOCK_RETRY_MS = _cfg[6];
  var REFRESH_WIFI_MS = _cfg[7];
  var REFRESH_OTA_MS = _cfg[8];
  var REFRESH_MQTT_MS = _cfg[9];
  var WIFI_SAVE_DELAY_1_MS = _cfg[10];
  var WIFI_SAVE_DELAY_2_MS = _cfg[11];
  var WIFI_SAVE_DELAY_3_MS = _cfg[12];
  var MQTT_SAVE_DELAY_1_MS = _cfg[13];
  var MQTT_SAVE_DELAY_2_MS = _cfg[14];
  var HANDOFF_MAX_MS = _cfg[15];

  var maxSpeed = ABS_PWM_MAX;
  try {
    var _mx = parseInt(localStorage.getItem("df3mt_rotor_max_pwm"), 10);
    if (!isNaN(_mx) && _mx >= 1 && _mx <= ABS_PWM_MAX) maxSpeed = _mx;
  } catch (e) {}

  /** Vorzeichen-PWM: 0 Stopp, negativ CCW, positiv CW; Betrag 0 bis maxSpeed (API speed=Betrags, dir=Vorzeichen). */
  var pwm = 0;
  var rampUpTimer = null;
  var rampDownTimer = null;
  var activeBtn = null;
  /** Nur ein /api/motor gleichzeitig — sonst staut sich die ESP-Warteschlange und alte Befehle laufen nach. */
  var motorTxBusy = false;
  var motorTargetS = 0;
  var motorTargetD = 0;
  var sv = document.getElementById("sv");
  var meterFill = document.getElementById("meterFill");
  var maxSpdInput = document.getElementById("maxSpd");
  if (maxSpdInput) maxSpdInput.max = String(ABS_PWM_MAX);
  var maxSpdLbl = document.getElementById("maxSpdLbl");
  var speedMaxCap = document.getElementById("speedMaxCap");
  var motorHintSign = document.getElementById("motorHintSign");
  var motorUiBlock = document.getElementById("motorUiBlock");
  var motorLockBanner = document.getElementById("motorLockBanner");
  var MOTOR_SESSION_COOKIE = "df3mt_motor_sid";
  /** true, wenn diese Browser-Sitzung (Cookie) die Server-Sperre hält. */
  var motorLockHeld = false;
  var motorLockRetryId = null;
  /** Nach Loslassen: Lock frei wenn PWM=0 oder spätestens nach HANDOFF_MAX_MS (für andere Nutzer). */
  var pendingHandoffRelease = false;
  var handoffDeadlineId = null;
  /** Mehrere gleichzeitige claim-Requests (Seitenstart + erster Klick) zu einem zusammenfassen. */
  var motorLockClaimInFlight = null;

  function readCookie(name) {
    var parts = ("; " + document.cookie).split("; " + name + "=");
    if (parts.length === 2) return (parts.pop().split(";").shift() || "").trim();
    return "";
  }

  /** Eindeutige Sitzungs-ID (Cookie), für genau einen aktiven Steuerer auf dem ESP. */
  function getMotorSessionId() {
    var v = readCookie(MOTOR_SESSION_COOKIE);
    if (v && /^[0-9a-f]{16,64}$/i.test(v)) return v.toLowerCase();
    var a = new Uint8Array(16);
    try {
      crypto.getRandomValues(a);
    } catch (e) {
      for (var i = 0; i < a.length; i++) a[i] = Math.floor(Math.random() * 256);
    }
    var hex = "";
    for (var j = 0; j < a.length; j++) hex += (a[j] < 16 ? "0" : "") + a[j].toString(16);
    document.cookie =
      MOTOR_SESSION_COOKIE + "=" + hex + "; path=/; max-age=31536000; SameSite=Lax";
    return hex;
  }

  function motorSessionHeaders() {
    return { "X-DF3MT-Session": getMotorSessionId() };
  }

  function clearHandoffDeadline() {
    if (handoffDeadlineId) {
      clearTimeout(handoffDeadlineId);
      handoffDeadlineId = null;
    }
  }

  function scheduleHandoffDeadline() {
    clearHandoffDeadline();
    if (!HANDOFF_MAX_MS || HANDOFF_MAX_MS <= 0) return;
    handoffDeadlineId = setTimeout(forceHandoffReleaseByDeadline, HANDOFF_MAX_MS);
  }

  function forceHandoffReleaseByDeadline() {
    handoffDeadlineId = null;
    if (!pendingHandoffRelease || !motorLockHeld) return;
    pendingHandoffRelease = false;
    voluntaryReleaseMotorLock();
  }
  var wifiStatus = document.getElementById("wifiStatus");
  var wifiSsid = document.getElementById("wifiSsid");
  var wifiPass = document.getElementById("wifiPass");
  var footIpLine = document.getElementById("footIpLine");
  var otaFlashStatus = document.getElementById("otaFlashStatus");
  var otaPwStatus = document.getElementById("otaPwStatus");
  var otaClearPwBtn = document.getElementById("otaClearPw");
  var mqttEn = document.getElementById("mqttEn");
  var mqttHost = document.getElementById("mqttHost");
  var mqttPort = document.getElementById("mqttPort");
  var mqttUser = document.getElementById("mqttUser");
  var mqttPass = document.getElementById("mqttPass");
  var mqttPrefix = document.getElementById("mqttPrefix");
  var mqttStatus = document.getElementById("mqttStatus");

  function clearMotorLockRetry() {
    if (motorLockRetryId) {
      clearInterval(motorLockRetryId);
      motorLockRetryId = null;
    }
  }

  function updateMotorLockUi() {
    if (motorUiBlock) motorUiBlock.classList.remove("motor-ui--blocked");
    if (motorLockBanner) {
      motorLockBanner.classList.toggle("motor-lock-banner--held", !!motorLockHeld);
      if (motorLockHeld) {
        motorLockBanner.textContent =
          "Aktive Steuerung: diese Browser-Sitzung (Cookie). Der Motor folgt den Tasten; andere Geräte können erst übernehmen, wenn die Sperre frei ist.";
      } else {
        motorLockBanner.textContent =
          "Tasten sind nie gesperrt. Der Motor folgt nur, wenn diese Sitzung (Cookie) die Steuerung hat. Anderes Gerät aktiv → erneuter Versuch alle 1 s.";
      }
    }
  }

  function voluntaryReleaseMotorLock() {
    if (!motorLockHeld) return;
    clearHandoffDeadline();
    stopRampUp();
    stopRampDown();
    pwm = 0;
    refreshMotorUi();
    motorLockHeld = false;
    clearMotorLockRetry();
    var p = new URLSearchParams();
    p.set("session", getMotorSessionId());
    fetch("/api/motor/release", {
      method: "POST",
      headers: {
        "Content-Type": "application/x-www-form-urlencoded",
        "X-DF3MT-Session": getMotorSessionId()
      },
      body: p.toString(),
      keepalive: true
    }).catch(function () {});
    updateMotorLockUi();
  }

  function tryFinishHandoffRelease() {
    if (!pendingHandoffRelease || !motorLockHeld) return;
    if (pwm !== 0 || rampUpTimer || rampDownTimer) return;
    if (motorTxBusy) {
      setTimeout(tryFinishHandoffRelease, MOTOR_TX_POLL_MS);
      return;
    }
    pendingHandoffRelease = false;
    clearHandoffDeadline();
    voluntaryReleaseMotorLock();
  }

  function handleMotorLockLost() {
    pendingHandoffRelease = false;
    clearHandoffDeadline();
    stopRampUp();
    stopRampDown();
    pwm = 0;
    refreshMotorUi();
    motorLockHeld = false;
    updateMotorLockUi();
    clearMotorLockRetry();
    scheduleMotorLockRetry();
  }

  function scheduleMotorLockRetry() {
    clearMotorLockRetry();
    motorLockRetryId = setInterval(function () { requestMotorLock(); }, MOTOR_LOCK_RETRY_MS);
  }

  function requestMotorLock() {
    if (motorLockClaimInFlight) return motorLockClaimInFlight;
    getMotorSessionId();
    motorLockClaimInFlight = fetch("/api/motor/claim", { method: "POST", headers: motorSessionHeaders() })
      .then(function (r) {
        return r.json().then(function (j) { return { r: r, j: j }; }).catch(function () {
          return { r: r, j: null };
        });
      })
      .then(function (x) {
        if (x.r.ok && x.j && x.j.ok === true) {
          motorLockHeld = true;
          clearMotorLockRetry();
          updateMotorLockUi();
          return true;
        }
        motorLockHeld = false;
        updateMotorLockUi();
        if (!motorLockRetryId) scheduleMotorLockRetry();
        return false;
      })
      .catch(function () {
        updateMotorLockUi();
        if (!motorLockRetryId) scheduleMotorLockRetry();
        return false;
      })
      .finally(function () {
        motorLockClaimInFlight = null;
      });
    return motorLockClaimInFlight;
  }

  function refreshOta() {
    fetch("/api/ota/status")
      .then(function (r) { return r.json(); })
      .then(function (d) {
        var upRow = document.getElementById("otaUploadPwRow");
        var curRow = document.getElementById("otaCurRow");
        var hint = document.getElementById("otaCompileHint");
        if (upRow) upRow.style.display = d.ota_auth_required ? "block" : "none";
        if (curRow) curRow.style.display = d.ota_nvs_password ? "block" : "none";
        if (hint) hint.style.display = d.ota_compile_fallback && !d.ota_nvs_password ? "block" : "none";
        if (otaClearPwBtn) otaClearPwBtn.disabled = !d.ota_nvs_password;
      })
      .catch(function () {});
  }

  function refreshMqtt() {
    fetch("/api/mqtt/status")
      .then(function (r) { return r.json(); })
      .then(function (d) {
        if (!d || d.ok !== true) return;
        if (mqttEn) mqttEn.checked = !!d.enabled;
        if (mqttHost) mqttHost.value = d.host || "";
        if (mqttPort) mqttPort.value = d.port || 1883;
        if (mqttPrefix) mqttPrefix.value = d.prefix || "df3mt/rotor";
        if (mqttPass) mqttPass.value = "";
        if (mqttStatus) {
          var tx = d.connected ? "MQTT verbunden" : "MQTT nicht verbunden";
          if (d.enabled && !d.connected) tx += " (Heim-WLAN / Broker prüfen)";
          tx += " · Anzeige PWM " + d.pwm + " · " + (d.prefix || "");
          mqttStatus.textContent = tx;
        }
      })
      .catch(function () {
        if (mqttStatus) mqttStatus.textContent = "MQTT-Status nicht lesbar.";
      });
  }

  function pwmToSpeedDir(p) {
    p = Math.round(p);
    if (p === 0) return { s: 0, d: 0 };
    if (p > 0) return { s: p, d: 1 };
    return { s: -p, d: -1 };
  }

  function refreshMotorUi() {
    var p = Math.round(pwm);
    if (p > maxSpeed) p = maxSpeed;
    if (p < -maxSpeed) p = -maxSpeed;
    if (p === 0) {
      sv.textContent = "0";
      sv.className = "speed-val speed-val--zero";
    } else if (p < 0) {
      sv.textContent = String(p);
      sv.className = "speed-val speed-val--neg";
    } else {
      sv.textContent = "+" + p;
      sv.className = "speed-val speed-val--pos";
    }
    if (meterFill && maxSpeed > 0) {
      var f = Math.min(1, Math.abs(p) / maxSpeed);
      var w = f * 50;
      if (p < 0) {
        meterFill.style.left = (50 - w) + "%";
        meterFill.style.width = w + "%";
        meterFill.style.background =
          "linear-gradient(90deg, rgba(52,211,153,0.35), var(--ccw))";
      } else if (p > 0) {
        meterFill.style.left = "50%";
        meterFill.style.width = w + "%";
        meterFill.style.background =
          "linear-gradient(90deg, var(--cw), rgba(96,165,250,0.35))";
      } else {
        meterFill.style.left = "50%";
        meterFill.style.width = "0%";
      }
    }
  }

  function syncMaxSpeedFromUi() {
    var v = parseInt(maxSpdInput.value, 10);
    if (isNaN(v)) v = ABS_PWM_MAX;
    maxSpeed = Math.min(ABS_PWM_MAX, Math.max(1, v));
    maxSpdInput.value = String(maxSpeed);
    maxSpdLbl.textContent = String(maxSpeed);
    speedMaxCap.textContent = " ±" + maxSpeed;
    if (motorHintSign) {
      motorHintSign.textContent =
        "Mitte = Stopp (0). CCW (Gegenuhrzeigersinn): negativ bis −" + maxSpeed +
        ". CW: positiv bis +" + maxSpeed + ".";
    }
    try {
      localStorage.setItem("df3mt_rotor_max_pwm", String(maxSpeed));
    } catch (e) {}
    var prev = pwm;
    if (pwm > maxSpeed) pwm = maxSpeed;
    if (pwm < -maxSpeed) pwm = -maxSpeed;
    refreshMotorUi();
    if (prev !== pwm) sendMotor();
  }

  function pumpMotor() {
    if (motorTxBusy) return;
    if (!motorLockHeld) return;
    motorTxBusy = true;
    var s = motorTargetS;
    var d = motorTargetD;
    var hdr = motorSessionHeaders();
    fetch("/api/motor?speed=" + s + "&dir=" + d, {
      keepalive: true,
      headers: hdr
    })
      .then(function (r) {
        if (r.status === 423 || r.status === 428) handleMotorLockLost();
        return r;
      })
      .catch(function () {})
      .finally(function () {
        motorTxBusy = false;
        if (motorLockHeld && (motorTargetS !== s || motorTargetD !== d)) pumpMotor();
      });
  }

  function sendMotor() {
    pwm = Math.round(pwm);
    if (pwm > maxSpeed) pwm = maxSpeed;
    if (pwm < -maxSpeed) pwm = -maxSpeed;
    refreshMotorUi();
    if (!motorLockHeld) return;
    var x = pwmToSpeedDir(pwm);
    motorTargetS = x.s;
    motorTargetD = x.d;
    pumpMotor();
  }

  function stopRampUp() {
    if (rampUpTimer) {
      clearInterval(rampUpTimer);
      rampUpTimer = null;
    }
  }

  function stopRampDown() {
    if (rampDownTimer) {
      clearInterval(rampDownTimer);
      rampDownTimer = null;
    }
  }

  function startRampDown() {
    stopRampUp();
    if (rampDownTimer) return;

    rampDownTimer = setInterval(function () {
      if (pwm === 0) {
        clearInterval(rampDownTimer);
        rampDownTimer = null;
        sendMotor();
        /* Nicht im pumpMotor-finally: sonst löst jeder Motor-Request fälschlich Handoff aus (schneller Klick). */
        setTimeout(tryFinishHandoffRelease, 0);
        return;
      }
      if (pwm > 0) {
        pwm = Math.max(0, pwm - RAMP_DOWN_STEP);
      } else {
        pwm = Math.min(0, pwm + RAMP_DOWN_STEP);
      }
      sendMotor();
    }, RAMP_DOWN_MS);
  }

  function startRampUp(direction) {
    stopRampDown();
    stopRampUp();
    rampUpTimer = setInterval(function () {
      if (direction < 0) {
        if (pwm <= -maxSpeed) {
          pwm = -maxSpeed;
          sendMotor();
          return;
        }
        if (pwm > 0) {
          pwm = Math.max(0, pwm - RAMP_UP_STEP);
        } else {
          pwm = Math.max(-maxSpeed, pwm - RAMP_UP_STEP);
        }
      } else {
        if (pwm >= maxSpeed) {
          pwm = maxSpeed;
          sendMotor();
          return;
        }
        if (pwm < 0) {
          pwm = Math.min(0, pwm + RAMP_UP_STEP);
        } else {
          pwm = Math.min(maxSpeed, pwm + RAMP_UP_STEP);
        }
      }
      sendMotor();
    }, RAMP_UP_MS);
  }

  function onDirDown(ev, direction) {
    ev.preventDefault();
    activeBtn = ev.currentTarget;
    try { ev.currentTarget.setPointerCapture(ev.pointerId); } catch (e) {}
    pendingHandoffRelease = false;
    clearHandoffDeadline();
    var start = function () {
      if ((direction < 0 && pwm > 0) || (direction > 0 && pwm < 0)) {
        pwm = 0;
        sendMotor();
      }
      startRampUp(direction);
    };
    if (motorLockHeld) {
      start();
    } else {
      requestMotorLock().then(function (ok) {
        if (ok) start();
      });
    }
  }

  function onDirUp(ev) {
    ev.preventDefault();
    try {
      if (activeBtn) activeBtn.releasePointerCapture(ev.pointerId);
    } catch (e) {}
    activeBtn = null;
    pendingHandoffRelease = true;
    scheduleHandoffDeadline();
    startRampDown();
  }

  function onStopDown(ev) {
    ev.preventDefault();
    try { ev.currentTarget.setPointerCapture(ev.pointerId); } catch (e) {}
    pendingHandoffRelease = false;
    clearHandoffDeadline();
    startRampDown();
  }

  function applyWifiStatusPayload(d) {
    wifiSsid.value = d.sta_ssid || "";
    wifiPass.value = "";
    wifiPass.placeholder = d.has_saved_pass
      ? "leer = gespeichertes Passwort behalten"
      : "offenes Netz: leer lassen";
    var staTxt = d.sta_connected
      ? "<span class=\"ok\">STA verbunden</span> → " + (d.sta_ip || "—")
      : "STA: nicht verbunden";
    wifiStatus.innerHTML =
      "AP: " + (d.ap_ip || "—") + " · " + staTxt;
    if (footIpLine) {
      footIpLine.textContent =
        "AP " + (d.ap_ip || "") + (d.sta_ip ? " · LAN " + d.sta_ip : "") + " · ESP32";
    }
  }

  function refreshWifi() {
    fetch("/api/wifi/status")
      .then(function (r) { return r.json(); })
      .then(applyWifiStatusPayload)
      .catch(function () {
        wifiStatus.textContent = "Status nicht lesbar.";
      });
  }

  var wifiStaPollTimer = null;
  function clearWifiStaPoll() {
    if (wifiStaPollTimer) {
      clearTimeout(wifiStaPollTimer);
      wifiStaPollTimer = null;
    }
  }

  /** Nach Speichern: Status pollen, bei STA mit IPv4 sofort http://<STA-IP>/ laden (Browser springt ins Heim-WLAN). */
  function schedulePollStaIpRedirect() {
    clearWifiStaPoll();
    var deadline = Date.now() + 50000;
    function tick() {
      fetch("/api/wifi/status")
        .then(function (r) { return r.json(); })
        .then(function (d) {
          applyWifiStatusPayload(d);
          if (d.sta_connected && d.sta_ip) {
            var ip = String(d.sta_ip).trim();
            if (/^(\d{1,3}\.){3}\d{1,3}$/.test(ip)) {
              clearWifiStaPoll();
              wifiStatus.innerHTML =
                "Heim-WLAN verbunden — öffne <span class=\"ok\">http://" + ip + "/</span> …";
              window.location.replace("http://" + ip + "/");
              return;
            }
          }
          if (Date.now() < deadline) {
            wifiStaPollTimer = setTimeout(tick, 450);
          } else {
            wifiStaPollTimer = null;
          }
        })
        .catch(function () {
          if (Date.now() < deadline) wifiStaPollTimer = setTimeout(tick, 700);
          else wifiStaPollTimer = null;
        });
    }
    wifiStaPollTimer = setTimeout(tick, WIFI_SAVE_DELAY_1_MS);
  }

  document.getElementById("wifiSave").addEventListener("click", function () {
    var body = new URLSearchParams();
    body.set("ssid", wifiSsid.value.trim());
    body.set("password", wifiPass.value);
    wifiStatus.textContent = "Verbindung wird versucht…";
    fetch("/api/wifi/save", {
      method: "POST",
      headers: { "Content-Type": "application/x-www-form-urlencoded" },
      body: body.toString()
    })
      .then(function (r) { return r.json(); })
      .then(function (j) {
        if (!j.ok) {
          wifiStatus.textContent = j.err || "Fehler beim Speichern.";
          return;
        }
        schedulePollStaIpRedirect();
      })
      .catch(function () {
        wifiStatus.textContent = "Anfrage fehlgeschlagen.";
      });
  });

  document.getElementById("wifiClear").addEventListener("click", function () {
    if (!confirm("Gespeicherte WLAN-Daten wirklich löschen?")) return;
    wifiStatus.textContent = "Löschen…";
    fetch("/api/wifi/clear", { method: "POST" })
      .then(function (r) { return r.json(); })
      .then(function () {
        wifiSsid.value = "";
        wifiPass.value = "";
        refreshWifi();
      })
      .catch(function () {
        wifiStatus.textContent = "Löschen fehlgeschlagen.";
      });
  });

  document.getElementById("mqttSave").addEventListener("click", function () {
    var body = new URLSearchParams();
    body.set("enabled", mqttEn.checked ? "1" : "0");
    body.set("host", mqttHost.value.trim());
    body.set("port", String(mqttPort.value || "1883"));
    body.set("user", mqttUser.value);
    if (mqttPass.value) body.set("password", mqttPass.value);
    else body.set("keep_mqtt_pass", "1");
    body.set("prefix", mqttPrefix.value.trim() || "df3mt/rotor");
    mqttStatus.textContent = "Speichern…";
    fetch("/api/mqtt/save", {
      method: "POST",
      headers: { "Content-Type": "application/x-www-form-urlencoded" },
      body: body.toString()
    })
      .then(function (r) {
        return r.json().then(function (j) { return { r: r, j: j }; });
      })
      .then(function (x) {
        if (!x.r.ok || !x.j || !x.j.ok) {
          mqttStatus.textContent = (x.j && x.j.err) ? x.j.err : "Fehler";
          return;
        }
        mqttStatus.textContent = "Gespeichert — Verbindung in wenigen Sekunden prüfen.";
        mqttPass.value = "";
        setTimeout(refreshMqtt, MQTT_SAVE_DELAY_1_MS);
        setTimeout(refreshMqtt, MQTT_SAVE_DELAY_2_MS);
      })
      .catch(function () { mqttStatus.textContent = "Anfrage fehlgeschlagen."; });
  });

  document.getElementById("mqttClear").addEventListener("click", function () {
    if (!confirm("MQTT-Konfiguration wirklich löschen?")) return;
    mqttStatus.textContent = "Löschen…";
    fetch("/api/mqtt/clear", { method: "POST" })
      .then(function (r) { return r.json(); })
      .then(function (j) {
        if (j && j.ok) {
          mqttStatus.textContent = "MQTT deaktiviert.";
          refreshMqtt();
        } else {
          mqttStatus.textContent = "Fehler.";
        }
      })
      .catch(function () { mqttStatus.textContent = "Fehler."; });
  });

  document.getElementById("otaFlash").addEventListener("click", function () {
    var fin = document.getElementById("otaFile");
    if (!fin.files || !fin.files[0]) {
      otaFlashStatus.textContent = "Zuerst eine .bin-Datei wählen.";
      return;
    }
    otaFlashStatus.textContent = "Upload läuft …";
    var headers = {};
    var pwUp = document.getElementById("otaUploadPw");
    if (pwUp && pwUp.value) headers["X-OTA-Password"] = pwUp.value;
    var fd = new FormData();
    fd.append("firmware", fin.files[0], fin.files[0].name);
    fetch("/api/ota/update", { method: "POST", headers: headers, body: fd })
      .then(function (r) {
        return r.json().then(function (j) { return { r: r, j: j }; }).catch(function () {
          return { r: r, j: null };
        });
      })
      .then(function (x) {
        if (x.r.ok && x.j && x.j.ok === true) {
          otaFlashStatus.textContent = "OK — Gerät startet neu …";
          return;
        }
        otaFlashStatus.textContent = (x.j && x.j.err) ? x.j.err : ("HTTP " + x.r.status);
      })
      .catch(function () {
        otaFlashStatus.textContent = "Verbindung getrennt (oft normal nach erfolgreichem Flash).";
      });
  });

  document.getElementById("otaSavePw").addEventListener("click", function () {
    var nw = document.getElementById("otaNewPw").value;
    if (!nw) {
      otaPwStatus.textContent = "Neues Passwort eingeben oder „Löschen (NVS)“ nutzen.";
      return;
    }
    var body = new URLSearchParams();
    body.set("new_password", nw);
    var cur = document.getElementById("otaCurPw");
    if (cur && cur.value) body.set("current_password", cur.value);
    otaPwStatus.textContent = "Speichern — Neustart …";
    fetch("/api/ota/password", {
      method: "POST",
      headers: { "Content-Type": "application/x-www-form-urlencoded" },
      body: body.toString()
    })
      .then(function (r) {
        return r.json().then(function (j) { return { r: r, j: j }; });
      })
      .then(function (x) {
        if (x.r.ok && x.j && x.j.ok === true) {
          otaPwStatus.textContent = "OK — startet neu …";
          return;
        }
        otaPwStatus.textContent = (x.j && x.j.err) ? x.j.err : "Fehler";
      })
      .catch(function () { otaPwStatus.textContent = "Anfrage fehlgeschlagen."; });
  });

  document.getElementById("otaClearPw").addEventListener("click", function () {
    if (!confirm("OTA-Passwort im NVS löschen? (Nach Neustart gilt ggf. nur noch ein Passwort aus dem Sketch-Build.)")) return;
    var body = new URLSearchParams();
    body.set("clear", "1");
    var cur = document.getElementById("otaCurPw");
    if (cur && cur.value) body.set("current_password", cur.value);
    otaPwStatus.textContent = "Löschen — Neustart …";
    fetch("/api/ota/password", {
      method: "POST",
      headers: { "Content-Type": "application/x-www-form-urlencoded" },
      body: body.toString()
    })
      .then(function (r) {
        return r.json().then(function (j) { return { r: r, j: j }; });
      })
      .then(function (x) {
        if (x.r.ok && x.j && x.j.ok === true) {
          otaPwStatus.textContent = "OK — startet neu …";
          return;
        }
        otaPwStatus.textContent = (x.j && x.j.err) ? x.j.err : "Fehler";
      })
      .catch(function () { otaPwStatus.textContent = "Anfrage fehlgeschlagen."; });
  });

  var ccw = document.getElementById("ccw");
  var cw = document.getElementById("cw");
  var stopB = document.getElementById("stop");

  ccw.addEventListener("pointerdown", function (e) { onDirDown(e, -1); });
  cw.addEventListener("pointerdown", function (e) { onDirDown(e, 1); });
  ccw.addEventListener("pointerup", onDirUp);
  cw.addEventListener("pointerup", onDirUp);
  ccw.addEventListener("pointercancel", onDirUp);
  cw.addEventListener("pointercancel", onDirUp);

  stopB.addEventListener("pointerdown", onStopDown);
  stopB.addEventListener("pointerup", function (e) {
    try { e.currentTarget.releasePointerCapture(e.pointerId); } catch (err) {}
    pendingHandoffRelease = true;
    scheduleHandoffDeadline();
    tryFinishHandoffRelease();
  });

  document.addEventListener("visibilitychange", function () {
    if (document.visibilityState === "hidden") {
      pendingHandoffRelease = false;
      clearHandoffDeadline();
      stopRampUp();
      stopRampDown();
      pwm = 0;
      refreshMotorUi();
      if (motorLockHeld) {
        var h = motorSessionHeaders();
        fetch("/api/motor?speed=0&dir=0", { keepalive: true, headers: h }).catch(function () {});
        var p = new URLSearchParams();
        p.set("session", getMotorSessionId());
        motorLockHeld = false;
        clearMotorLockRetry();
        fetch("/api/motor/release", {
          method: "POST",
          headers: {
            "Content-Type": "application/x-www-form-urlencoded",
            "X-DF3MT-Session": getMotorSessionId()
          },
          body: p.toString(),
          keepalive: true
        }).catch(function () {});
        updateMotorLockUi();
      }
    } else {
      requestMotorLock();
    }
  });

  window.addEventListener("pagehide", function () {
    pendingHandoffRelease = false;
    clearHandoffDeadline();
    if (!motorLockHeld) return;
    try {
      navigator.sendBeacon("/api/motor/release", new URLSearchParams({ session: getMotorSessionId() }));
    } catch (e) {}
    motorLockHeld = false;
  });

  maxSpdInput.value = String(maxSpeed);
  maxSpdLbl.textContent = String(maxSpeed);
  speedMaxCap.textContent = " ±" + maxSpeed;
  if (motorHintSign) {
    motorHintSign.textContent =
      "Mitte = Stopp (0). CCW (Gegenuhrzeigersinn): negativ bis −" + maxSpeed +
      ". CW: positiv bis +" + maxSpeed + ".";
  }
  refreshMotorUi();
  maxSpdInput.addEventListener("input", syncMaxSpeedFromUi);
  maxSpdInput.addEventListener("change", syncMaxSpeedFromUi);

  updateMotorLockUi();
  requestMotorLock();

  refreshWifi();
  if (REFRESH_WIFI_MS > 0) setInterval(refreshWifi, REFRESH_WIFI_MS);
  refreshOta();
  setInterval(refreshOta, REFRESH_OTA_MS);
  refreshMqtt();
  setInterval(refreshMqtt, REFRESH_MQTT_MS);
})();
  </script>
</body>
</html>
)DF3MT_H2";
