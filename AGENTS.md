# AGENTS.md

## Cursor Cloud specific instructions

### What this repo is
PortableRotor (DF3MT) — an ESP32 Arduino firmware project for a portable antenna rotator, plus a Home Assistant MQTT integration (`homeassistant/`), a static landing page (`docs/`), and CAD/video assets. There is **no host-runnable server app**; the "application" is firmware that runs on the ESP32.

### Toolchain / build (dev)
- The dev toolchain is `arduino-cli` + the `esp32:esp32` core + the `PubSubClient` library (same as `.github/workflows/build.yml`). These are installed by the startup update script and persist in the VM snapshot.
- Build (also the primary way to verify changes, since no hardware is attached):
  `arduino-cli compile --fqbn esp32:esp32:esp32:PartitionScheme=default,FlashSize=4M --output-dir Firmware/DF3MT-Rotor/build Firmware/DF3MT-Rotor`
- Flashing/running requires a physical ESP32 + L298N + motor and cannot be done in this VM. Treat a clean compile (produces `DF3MT-Rotor.ino.bin`) as the build/run signal.

### Non-obvious gotchas
- The `homeassistant/*.yaml` files are stored **UTF-16LE** (no BOM) in the repo. Editing tools preserve this; if you parse them with a plain UTF-8 loader (e.g. PyYAML) it fails with "special characters are not allowed" — decode as `utf-16-le` first. The firmware `.ino`/`.h`/`.md` files are UTF-8.
- The embedded web UI lives in `Firmware/DF3MT-Rotor/kIndexHtml.h` as concatenated C++ raw-string chunks with two macro splices (`DF3MT_XSTR(DF3MT_MOTOR_PWM_MAX)` and `DF3MT_UI_NUMPACK_STR` from `DF3MT_Config.h`); it is not a standalone HTML file.
- MQTT: firmware publishes Home Assistant discovery on (re)connect and mirrors state to retained `{prefix}/...` topics (default prefix `df3mt/rotor`). The HA discovery entity set in the firmware (`mqttPublishDiscovery`) must stay in sync with `homeassistant/packages/df3mt_rotor.yaml` and the topics in `mqttCallback`/`mqttReconnect`.
- MQTT/HA changes can be validated without hardware by running a local Mosquitto broker and replaying the discovery JSON + command topics (see the PR that added the Web URL sensor and CW/CCW buttons).
