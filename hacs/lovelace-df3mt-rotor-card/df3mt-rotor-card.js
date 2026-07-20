/**
 * DF3MT Rotor Card
 * A Lovelace card that controls the DF3MT Portable Rotor like its web UI:
 * CCW / STOP / CW buttons plus a speed slider (1-100 %).
 *
 * It drives the MQTT entities created by the DF3MT firmware discovery /
 * the `homeassistant/packages/df3mt_rotor.yaml` package.
 *
 * Minimal example:
 *   type: custom:df3mt-rotor-card
 *
 * Full config (defaults shown):
 *   type: custom:df3mt-rotor-card
 *   title: DF3MT Rotor
 *   ccw_button: button.df3mt_rotor_rotate_ccw
 *   stop_button: button.df3mt_rotor_stop
 *   cw_button: button.df3mt_rotor_rotate_cw
 *   speed: number.df3mt_rotor_speed_percent   # 1-100 % (mapped to PWM 150-255)
 *   running: binary_sensor.df3mt_rotor_running
 *   direction: select.df3mt_rotor_direction
 *   url: sensor.df3mt_rotor_web_url
 */

const DEFAULTS = {
  title: "DF3MT Rotor",
  ccw_button: "button.df3mt_rotor_rotate_ccw",
  stop_button: "button.df3mt_rotor_stop",
  cw_button: "button.df3mt_rotor_rotate_cw",
  speed: "number.df3mt_rotor_speed_percent",
  running: "binary_sensor.df3mt_rotor_running",
  direction: "select.df3mt_rotor_direction",
  url: "sensor.df3mt_rotor_web_url",
};

class DF3MTRotorCard extends HTMLElement {
  setConfig(config) {
    this._config = Object.assign({}, DEFAULTS, config || {});
    this._dragging = false;
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

  _press(entityId) {
    if (!this._hass || !entityId) return;
    const domain = entityId.split(".")[0];
    this._hass.callService(domain, "press", { entity_id: entityId });
  }

  _setSpeed(value) {
    const ent = this._config.speed;
    if (!this._hass || !ent) return;
    this._hass.callService("number", "set_value", {
      entity_id: ent,
      value: Number(value),
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
        input[type=range] { width: 100%; accent-color: var(--primary-color); height: 26px; }
        .status { margin-top: 14px; display: flex; flex-wrap: wrap; gap: 10px 18px; color: var(--secondary-text-color); font-size: 0.9rem; }
        .status .dot { display: inline-block; width: 9px; height: 9px; border-radius: 50%; margin-right: 6px; background: var(--disabled-text-color); vertical-align: middle; }
        .status .dot.on { background: var(--success-color, #4ade80); }
        .status a { color: var(--primary-color); text-decoration: none; }
        .unavail { opacity: 0.5; pointer-events: none; }
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
          <div class="speed-head"><span class="lbl">Speed</span><span class="val" id="speedval">--</span></div>
          <input type="range" id="speed" min="1" max="100" step="1" value="1" />
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

    card.querySelector("#ccw").addEventListener("click", () => this._press(this._config.ccw_button));
    card.querySelector("#stop").addEventListener("click", () => this._press(this._config.stop_button));
    card.querySelector("#cw").addEventListener("click", () => this._press(this._config.cw_button));

    const slider = card.querySelector("#speed");
    slider.addEventListener("input", () => {
      this._dragging = true;
      card.querySelector("#speedval").textContent = slider.value + " %";
    });
    slider.addEventListener("change", () => {
      this._dragging = false;
      this._setSpeed(slider.value);
    });

    this._update();
  }

  _update() {
    if (!this._root || !this._hass) return;
    const q = (id) => this._root.querySelector(id);
    const cfg = this._config;

    q("#title").textContent = cfg.title;

    const setAvail = (id, entId) => {
      const el = q(id);
      const st = this._state(entId);
      const ok = st && st.state !== "unavailable" && st.state !== "unknown";
      el.classList.toggle("unavail", !ok);
      return st;
    };
    setAvail("#ccw", cfg.ccw_button);
    setAvail("#stop", cfg.stop_button);
    setAvail("#cw", cfg.cw_button);

    const speedSt = setAvail("#speed", cfg.speed);
    if (speedSt && !this._dragging) {
      const v = Math.round(Number(speedSt.state));
      if (!Number.isNaN(v)) {
        q("#speed").value = String(v);
        q("#speedval").textContent = v + " %";
      }
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
  description: "Control the DF3MT Portable Rotor (CCW / STOP / CW + speed %), like the web UI.",
  preview: false,
  documentation: "https://github.com/DF3MT/DF3MT-Portable-Antenna-Wifi-Rotor",
});

console.info("%c DF3MT-ROTOR-CARD %c loaded ", "background:#38bdf8;color:#0c1222;font-weight:700;border-radius:4px 0 0 4px;padding:2px 6px", "background:#141c32;color:#eef2ff;border-radius:0 4px 4px 0;padding:2px 6px");
