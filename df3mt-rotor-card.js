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
 * It writes the signed PWM to the firmware's "PWM (signed)" number entity
 * (created by MQTT discovery: number.df3mt_rotor_pwm_signed), so no YAML
 * package is required. The mapping is done in the card, in JavaScript.
 *
 * Minimal example:
 *   type: custom:df3mt-rotor-card
 *
 * The CW/CCW/STOP buttons and the slider all drive the signed setpoint entity
 * (mapped in JS), so CW/CCW always rotate at a moving speed (PWM >= 150) equal
 * to the current/last slider magnitude - never a stale sub-threshold value.
 *
 * Full config (defaults shown):
 *   type: custom:df3mt-rotor-card
 *   title: DF3MT Rotor
 *   signed_pwm: number.df3mt_rotor_pwm_signed   # signed PWM -255..255
 *   default_speed: 50                            # % used by CW/CCW if slider is at 0
 *   running: binary_sensor.df3mt_rotor_running
 *   direction: select.df3mt_rotor_direction
 *   url: sensor.df3mt_rotor_web_url
 */

const DEFAULTS = {
  title: "DF3MT Rotor",
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
    // Last non-zero magnitude the user selected (percent), used by the
    // CW/CCW buttons so they always rotate at a mapped, moving speed.
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

  // Current magnitude (percent) to use for the CW/CCW buttons: the slider's
  // magnitude if set, otherwise the last non-zero value the user chose.
  _currentMagPct() {
    const slider = this._root && this._root.querySelector("#speed");
    const mag = slider ? Math.abs(Math.round(Number(slider.value))) : 0;
    return mag || this._lastMag || 50;
  }

  _setSignedPct(pct) {
    const ent = this._config.signed_pwm;
    if (!this._hass || !ent) return;
    this._hass.callService("number", "set_value", {
      entity_id: ent,
      value: pctToPwm(pct),
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
        .dim { opacity: 0.5; }
        button.dim { pointer-events: none; }
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
      </div>
    `;
    this.shadowRoot.appendChild(card);
    this._root = card;

    // CW/CCW rotate at the current (or last) slider magnitude, mapped to a
    // moving PWM (>=150). STOP sets 0. This avoids the firmware rotating at a
    // stale sub-threshold speed (e.g. PWM 13, which never moves the motor).
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

    // Buttons and slider both drive the signed-PWM entity; dim them together
    // if it is unavailable (but never make the slider un-draggable).
    const pwmSt = this._state(cfg.signed_pwm);
    const ctrlOk = pwmSt && pwmSt.state !== "unavailable" && pwmSt.state !== "unknown";
    q("#ccw").classList.toggle("dim", !ctrlOk);
    q("#stop").classList.toggle("dim", !ctrlOk);
    q("#cw").classList.toggle("dim", !ctrlOk);

    // The slider stays interactive at all times; we only sync its value from
    // the signed-PWM entity when a fresh value is available and not dragging.
    q(".speed").classList.toggle("dim", !pwmSt);
    if (pwmSt && !this._dragging) {
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

console.info("%c DF3MT-ROTOR-CARD %c loaded ", "background:#38bdf8;color:#0c1222;font-weight:700;border-radius:4px 0 0 4px;padding:2px 6px", "background:#141c32;color:#eef2ff;border-radius:0 4px 4px 0;padding:2px 6px");
