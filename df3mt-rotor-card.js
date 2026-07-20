/**
 * DF3MT Rotor Card
 * A Lovelace card that controls the DF3MT Portable Rotor like its web UI:
 * CCW / STOP / CW buttons plus a signed speed slider.
 *
 * The slider is centered at 0 (Stop). Positive = CW, negative = CCW, as a
 * percentage of the usable PWM range:
 *   +1 % -> PWM 150 ... +100 % -> 255   (clockwise)
 *   -1 % -> PWM -150 ... -100 % -> -255  (counter-clockwise)
 *    0 % -> 0 (stop)
 *
 * Commands are published as INTEGERS directly to the firmware setpoint topic
 * (default df3mt/rotor/set) via the mqtt.publish service. This matches exactly
 * what the firmware expects (it accepts only whole numbers) and avoids the
 * HA number entity publishing floats like "202.0", which the firmware ignores.
 * State for the slider is read from the signed-PWM entity when available.
 *
 * Minimal example:
 *   type: custom:df3mt-rotor-card
 *
 * Full config (defaults shown):
 *   type: custom:df3mt-rotor-card
 *   title: DF3MT Rotor
 *   command_topic: df3mt/rotor/set   # firmware signed setpoint topic (match your MQTT prefix)
 *   signed_pwm: number.df3mt_rotor_pwm_signed   # read-back for the slider (optional)
 *   default_speed: 50                # % used by CW/CCW when the slider is at 0
 *   running: binary_sensor.df3mt_rotor_running
 *   direction: select.df3mt_rotor_direction
 *   url: sensor.df3mt_rotor_web_url
 */

const CARD_VERSION = "1.4.1";

const DEFAULTS = {
  title: "DF3MT Rotor",
  command_topic: "df3mt/rotor/set",
  signed_pwm: "number.df3mt_rotor_pwm_signed",
  default_speed: 50,
  running: "binary_sensor.df3mt_rotor_running",
  direction: "select.df3mt_rotor_direction",
  url: "sensor.df3mt_rotor_web_url",
};

const PWM_MIN = 150; // slowest PWM that actually moves the motor
const PWM_MAX = 255; // full speed
const SPAN = PWM_MAX - PWM_MIN;

// signed percent (-100..100) -> signed PWM (0, or +/-150..255)
function pctToPwm(pct) {
  const p = Math.round(Number(pct));
  if (!p) return 0;
  const mag = Math.round(PWM_MIN + (Math.abs(p) - 1) * SPAN / 99);
  return p > 0 ? mag : -mag;
}

// signed PWM -> signed percent (-100..100)
function pwmToPct(pwm) {
  const r = Math.round(Number(pwm));
  if (Number.isNaN(r) || Math.abs(r) < PWM_MIN) return 0;
  const mag = Math.round((Math.abs(r) - PWM_MIN) * 99 / SPAN + 1);
  return r > 0 ? mag : -mag;
}

function speedLabel(pct) {
  const n = Math.round(Number(pct));
  if (!n) return "Stop";
  return (n > 0 ? "CW " : "CCW ") + Math.abs(n) + " %";
}

class DF3MTRotorCard extends HTMLElement {
  setConfig(config) {
    this._config = Object.assign({}, DEFAULTS, config || {});
    this._dragging = false;
    this._lastMag = Math.min(100, Math.max(1, Number(this._config.default_speed) || 50));
    this._render();
  }

  set hass(hass) {
    this._hass = hass;
    this._update();
  }

  getCardSize() {
    return 4;
  }

  static getStubConfig() {
    return { type: "custom:df3mt-rotor-card" };
  }

  _state(entityId) {
    if (!this._hass || !entityId) return undefined;
    return this._hass.states[entityId];
  }

  // Current magnitude (percent) for the CW/CCW buttons: the slider's magnitude
  // if set, otherwise the last non-zero value the user chose.
  _currentMagPct() {
    const slider = this._root && this._root.querySelector("#speed");
    const mag = slider ? Math.abs(Math.round(Number(slider.value))) : 0;
    return mag || this._lastMag || 50;
  }

  // Publish the signed PWM as a plain integer to the firmware's setpoint topic.
  _setSignedPct(pct) {
    if (!this._hass) return;
    const pwm = pctToPwm(pct);
    this._hass.callService("mqtt", "publish", {
      topic: this._config.command_topic,
      payload: String(pwm),
    });
  }

  _render() {
    if (this._root) return;
    this.attachShadow({ mode: "open" });
    const card = document.createElement("ha-card");
    card.innerHTML = `
      <style>
        .wrap { padding: 16px; }
        .title { font-size: 1.25rem; font-weight: 600; margin-bottom: 12px; }
        .row { display: grid; grid-template-columns: 1fr 1fr 1fr; gap: 10px; }
        button.ctrl {
          display: flex; flex-direction: column; align-items: center; justify-content: center;
          gap: 6px; padding: 14px 8px; border: none; border-radius: 14px; cursor: pointer;
          background: var(--secondary-background-color); color: var(--primary-text-color);
          font-size: 0.95rem; font-weight: 600; transition: filter .12s ease, transform .05s ease;
        }
        button.ctrl:hover { filter: brightness(1.08); }
        button.ctrl:active { transform: scale(0.97); }
        button.ccw { background: rgba(52,211,153,0.18); }
        button.cw  { background: rgba(96,165,250,0.18); }
        button.stop { background: rgba(251,113,133,0.18); }
        button.ctrl ha-icon { --mdc-icon-size: 30px; }
        .speed { margin-top: 18px; }
        .speed-head { display: flex; justify-content: space-between; align-items: baseline; margin-bottom: 6px; }
        .speed-head .lbl { color: var(--secondary-text-color); }
        .speed-head .val { font-variant-numeric: tabular-nums; font-weight: 600; }
        input[type=range] { width: 100%; accent-color: var(--primary-color); height: 30px; cursor: pointer; }
        .scale { display: flex; justify-content: space-between; color: var(--secondary-text-color); font-size: 0.72rem; margin-top: 2px; }
        .status { margin-top: 14px; display: flex; flex-wrap: wrap; gap: 10px 18px; color: var(--secondary-text-color); font-size: 0.9rem; }
        .status .dot { display: inline-block; width: 9px; height: 9px; border-radius: 50%; margin-right: 6px; background: var(--disabled-text-color); vertical-align: middle; }
        .status .dot.on { background: var(--success-color, #4ade80); }
        .status a { color: var(--primary-color); text-decoration: none; }
        .ver { margin-top: 10px; text-align: right; font-size: 0.7rem; color: var(--secondary-text-color); opacity: 0.7; }
      </style>
      <div class="wrap">
        <div class="title" id="title"></div>
        <div class="row">
          <button class="ctrl ccw" id="ccw" title="Counter-clockwise">
            <ha-icon icon="mdi:rotate-left"></ha-icon><span>CCW</span>
          </button>
          <button class="ctrl stop" id="stop" title="Stop">
            <ha-icon icon="mdi:stop"></ha-icon><span>STOP</span>
          </button>
          <button class="ctrl cw" id="cw" title="Clockwise">
            <ha-icon icon="mdi:rotate-right"></ha-icon><span>CW</span>
          </button>
        </div>
        <div class="speed">
          <div class="speed-head"><span class="lbl">Speed &amp; direction</span><span class="val" id="speedval">Stop</span></div>
          <input type="range" id="speed" min="-100" max="100" step="1" value="0" />
          <div class="scale"><span>CCW 100 %</span><span>Stop</span><span>CW 100 %</span></div>
        </div>
        <div class="status">
          <span id="run"><span class="dot" id="rundot"></span><span id="runtxt">Idle</span></span>
          <span id="dirwrap">Direction: <b id="dir">--</b></span>
          <span id="urlwrap"></span>
        </div>
        <div class="ver">card v${CARD_VERSION}</div>
      </div>
    `;
    this.shadowRoot.appendChild(card);
    this._root = card;

    // CW/CCW rotate at the current (or last) slider magnitude, mapped to a
    // moving PWM (>=150). STOP sets 0.
    card.querySelector("#ccw").addEventListener("click", () => this._setSignedPct(-this._currentMagPct()));
    card.querySelector("#stop").addEventListener("click", () => this._setSignedPct(0));
    card.querySelector("#cw").addEventListener("click", () => this._setSignedPct(this._currentMagPct()));

    const slider = card.querySelector("#speed");
    slider.addEventListener("input", () => {
      this._dragging = true;
      const mag = Math.abs(Math.round(Number(slider.value)));
      if (mag) this._lastMag = mag;
      card.querySelector("#speedval").textContent = speedLabel(slider.value);
    });
    slider.addEventListener("change", () => {
      this._dragging = false;
      const mag = Math.abs(Math.round(Number(slider.value)));
      if (mag) this._lastMag = mag;
      this._setSignedPct(slider.value);
    });

    this._update();
  }

  _update() {
    if (!this._root || !this._hass) return;
    const q = (id) => this._root.querySelector(id);
    const cfg = this._config;

    q("#title").textContent = cfg.title;

    // Controls are ALWAYS enabled and clickable. Sync the slider position from
    // the signed-PWM entity when available and not dragging (display only).
    const pwmSt = this._state(cfg.signed_pwm);
    if (pwmSt && pwmSt.state !== "unavailable" && pwmSt.state !== "unknown" && !this._dragging) {
      const pct = pwmToPct(pwmSt.state);
      q("#speed").value = String(pct);
      q("#speedval").textContent = speedLabel(pct);
    }

    const runSt = this._state(cfg.running);
    if (runSt) {
      const on = runSt.state === "on";
      q("#rundot").classList.toggle("on", on);
      q("#runtxt").textContent = on ? "Running" : "Idle";
      q("#run").style.display = "";
    } else {
      q("#run").style.display = "none";
    }

    const dirSt = this._state(cfg.direction);
    if (dirSt) {
      q("#dir").textContent = dirSt.state;
      q("#dirwrap").style.display = "";
    } else {
      q("#dirwrap").style.display = "none";
    }

    const urlSt = this._state(cfg.url);
    const urlWrap = q("#urlwrap");
    if (urlSt && /^https?:\/\//i.test(urlSt.state)) {
      urlWrap.innerHTML = `<a href="${urlSt.state}" target="_blank" rel="noopener">Open web UI</a>`;
      urlWrap.style.display = "";
    } else {
      urlWrap.style.display = "none";
    }
  }
}

if (!customElements.get("df3mt-rotor-card")) {
  customElements.define("df3mt-rotor-card", DF3MTRotorCard);
}

window.customCards = window.customCards || [];
window.customCards.push({
  type: "df3mt-rotor-card",
  name: "DF3MT Rotor Card",
  description: "Control the DF3MT Portable Rotor (CCW / STOP / CW + signed speed %), like the web UI.",
  preview: false,
  documentation: "https://github.com/DF3MT/DF3MT-Portable-Antenna-Wifi-Rotor",
});

console.info("%c DF3MT-ROTOR-CARD %c v" + CARD_VERSION + " ", "background:#38bdf8;color:#0c1222;font-weight:700;border-radius:4px 0 0 4px;padding:2px 6px", "background:#141c32;color:#eef2ff;border-radius:0 4px 4px 0;padding:2px 6px");
