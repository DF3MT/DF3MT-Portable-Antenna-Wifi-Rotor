# DF3MT Rotor Card

A Home Assistant Lovelace card to control the [DF3MT Portable Rotor](https://github.com/DF3MT/DF3MT-Portable-Antenna-Wifi-Rotor) the same way as its built-in web UI: **CCW / STOP / CW** buttons plus a **speed slider (1–100 %)**.

It uses the MQTT entities the rotor firmware creates via Home Assistant auto-discovery (or the `homeassistant/packages/df3mt_rotor.yaml` package). No build step — plain JavaScript.

![type: custom:df3mt-rotor-card](https://raw.githubusercontent.com/DF3MT/DF3MT-Portable-Antenna-Wifi-Rotor/main/docs/images/hero-banner.png)

## Requirements

- The DF3MT rotor connected to your MQTT broker (entities present in Home Assistant via auto-discovery).
- No YAML package required: the card uses the discovered entities directly — buttons `button.df3mt_rotor_rotate_*` / `button.df3mt_rotor_stop` and the signed setpoint `number.df3mt_rotor_pwm_signed`. Override any entity in the card config if your IDs differ.

## Install

This repository is HACS-plugin compliant at its root: `df3mt-rotor-card.js` +
`hacs.json` (`content_in_root: true`, `filename: df3mt-rotor-card.js`).

### Via HACS (custom repository)

1. HACS → three-dot menu (top right) → **Custom repositories**.
2. Repository: `https://github.com/DF3MT/DF3MT-Portable-Antenna-Wifi-Rotor`. Category: **Dashboard**.
3. Add, then search HACS for **DF3MT Rotor Card**.
4. Open it and **Download**. If HACS pre-selects a release that predates this card
   (e.g. `v1.0.0`, which only shipped firmware binaries), use the version dropdown
   and pick the **main** branch or a release `>= v1.1.0` (those include the card).
5. HACS adds the Lovelace resource automatically. Reload your browser (Ctrl/Cmd+Shift+R).

> Note: HACS resolves a plugin from the **latest release** first, then the selected
> branch. Releases `<= v1.0.0` do not contain `df3mt-rotor-card.js`, which is why HACS
> reports *"Repository structure for v1.0.0 is not compliant"* — pick the **main**
> branch (or a newer release) in the version dropdown instead.

### Manual (without HACS)

1. Copy `df3mt-rotor-card.js` to `<config>/www/df3mt-rotor-card.js`.
2. Settings → Dashboards → three-dot menu → **Resources** → **Add resource**
   - URL: `/local/df3mt-rotor-card.js`
   - Type: **JavaScript module**
3. Reload your browser.

## Usage

Add the card to a dashboard (Edit dashboard → **+ Add card** → search "DF3MT Rotor Card"), or in YAML:

```yaml
type: custom:df3mt-rotor-card
title: DF3MT Rotor
```

### Options

| Option        | Default                                | Description                                   |
| ------------- | -------------------------------------- | --------------------------------------------- |
| `title`       | `DF3MT Rotor`                          | Card title.                                   |
| `ccw_button`  | `button.df3mt_rotor_rotate_ccw`        | Rotate counter-clockwise (press).             |
| `stop_button` | `button.df3mt_rotor_stop`              | Stop (press).                                 |
| `cw_button`   | `button.df3mt_rotor_rotate_cw`         | Rotate clockwise (press).                     |
| `signed_pwm`  | `number.df3mt_rotor_pwm_signed`        | Signed setpoint entity the slider writes to.  |
| `running`     | `binary_sensor.df3mt_rotor_running`    | Optional running indicator.                   |
| `direction`   | `select.df3mt_rotor_direction`         | Optional current-direction text.              |
| `url`         | `sensor.df3mt_rotor_web_url`           | Optional link to the rotor's web UI.          |

The **speed slider is signed** and centered at 0 (Stop): drag right for CW, left for
CCW. The percentage maps to the firmware PWM range in the card (JS): `±1 % = ±150`,
`±100 % = ±255`. The slider writes the signed PWM to `number.df3mt_rotor_pwm_signed`
(created by MQTT discovery), so **no YAML package is required**. The CW/CCW buttons
rotate at the currently set speed.

## License

GPL-3.0, same as the DF3MT PortableRotor project.
