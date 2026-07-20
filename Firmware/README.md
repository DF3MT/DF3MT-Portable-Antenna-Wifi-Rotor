<div align="center">

# 💾 Firmware (ESP32)

### Arduino-Sketch **DF3MT-Rotor** für den PortableRotor

[![ESP32](https://img.shields.io/badge/platform-ESP32-E7352C?logo=espressif&logoColor=white)](https://www.espressif.com/)
[![Arduino](https://img.shields.io/badge/Arduino-IDE-00979D?logo=arduino&logoColor=white)](https://www.arduino.cc/)
[![MQTT](https://img.shields.io/badge/MQTT-PubSubClient-660066)](https://pubsubclient.knolleary.net/)

[⬆️ Zurück zum Projekt-README](../README.md) · [📖 Detail-Doku Sketch](./DF3MT-Rotor/README.md) · [⬇️ Releases](https://github.com/DF3MT/DF3MT-Portable-Antenna-Wifi-Rotor/releases/latest)

</div>

---

## 📂 Sketch öffnen

> ⚠️ In der **Arduino IDE** den Ordner `DF3MT-Rotor/` öffnen — die `.ino` muss in einem **gleichnamigen** Unterordner liegen.

```text
Firmware/
└── 📂 DF3MT-Rotor/
    ├── 📄 DF3MT-Rotor.ino   # Hauptsketch
    ├── ⚙️ DF3MT_Config.h    # Zentrale Konfiguration
    ├── 🌐 kIndexHtml.h      # Eingebettete Web-UI (PROGMEM)
    ├── 📖 README.md         # Build-, MQTT- & OTA-Hinweise
    └── 🙈 .gitignore
```

---

## ✅ Voraussetzungen

| | Was | Hinweis |
|:-:|:--|:--|
| 🧩 | Board-Paket **esp32** | Arduino-ESP32 **3.x** |
| 📦 | Partitionsschema | **mit OTA** wählen |
| 📨 | Bibliothek **PubSubClient** | für MQTT |

---

## 🚀 Schnellstart

1. 📥 Arduino IDE + ESP32-Board-Paket installieren
2. 📚 *Sketch → Bibliothek einbinden → Bibliotheken verwalten* → **PubSubClient**
3. 📂 Ordner `Firmware/DF3MT-Rotor/` öffnen
4. 🔌 Board **ESP32**, Partition **mit OTA**, Port wählen → **Upload**
5. 📶 Mit SoftAP verbinden und Web-UI im Browser öffnen

> 📖 Alle Details (MQTT-Topics, Motor-Lock, Secrets, OTA-Binaries): **[`DF3MT-Rotor/README.md`](./DF3MT-Rotor/README.md)**
