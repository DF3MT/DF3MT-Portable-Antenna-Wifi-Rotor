# Firmware (ESP32)

Arduino-Sketch **DF3MT-Rotor** für den PortableRotor.

## Sketch öffnen

**Arduino IDE:** Ordner `DF3MT-Rotor/` öffnen (die `.ino` muss in einem gleichnamigen Unterordner liegen).

```
Firmware/
└── DF3MT-Rotor/
    ├── DF3MT-Rotor.ino   # Hauptsketch
    ├── DF3MT_Config.h    # Zentrale Konfiguration
    ├── kIndexHtml.h      # Eingebettete Web-UI (PROGMEM)
    ├── README.md         # Build- & Konfigurationshinweise
    └── .gitignore
```

## Voraussetzungen

- Board-Paket **esp32** (Arduino-ESP32 3.x)
- Partitionsschema **mit OTA**
- Bibliothek **PubSubClient** (MQTT)

Details siehe `DF3MT-Rotor/README.md`.
