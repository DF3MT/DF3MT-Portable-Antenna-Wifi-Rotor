# DF3MT Rotor Card

A Home Assistant Lovelace card to control the [DF3MT Portable Rotor](https://github.com/DF3MT/DF3MT-Portable-Antenna-Wifi-Rotor) the same way as its built-in web UI: **CCW / STOP / CW** buttons plus a **speed slider (1–100 %)**.

It uses the MQTT entities the rotor firmware creates via Home Assistant auto-discovery (or the `homeassistant/packages/df3mt_rotor.yaml` package). No build step — plain JavaScript.

![type: custom:df3mt-rotor-card](https://raw.githubusercontent.com/DF3MT/DF3MT-Portable-Antenna-Wifi-Rotor/main/docs/images/hero-banner.png)

## Requirements

- The DF3MT rotor connected to your MQTT broker (entities present in Home Assistant).
- The **Speed (%)** control uses `number.df3mt_rotor_speed_percent` from the `homeassistant/packages/df3mt_rotor.yaml` package (maps 1 % → PWM 150, 100 % → 255). If you don't use that package, point `speed:` at the raw `number.df3mt_rotor_speed` instead.

## Install

### Via HACS (custom repository)

1. HACS → three-dot menu (top right) → **Custom repositories**.
2. Repository: the URL of this card's repository. Type/Category: **Dashboard**.
3. Add, then search HACS for **DF3MT Rotor Card** and download it.
4. HACS adds the Lovelace resource automatically. Reload your browser (Ctrl/Cmd+Shift+R).

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
| `speed`       | `number.df3mt_rotor_speed_percent`     | Speed number in % (1–100).                    |
| `running`     | `binary_sensor.df3mt_rotor_running`    | Optional running indicator.                   |
| `direction`   | `select.df3mt_rotor_direction`         | Optional current-direction text.              |
| `url`         | `sensor.df3mt_rotor_web_url`           | Optional link to the rotor's web UI.          |

Like the web UI, **CW/CCW rotate at the currently set speed**, and changing the speed while running keeps the direction.

## License

GPL-3.0, same as the DF3MT PortableRotor project.
