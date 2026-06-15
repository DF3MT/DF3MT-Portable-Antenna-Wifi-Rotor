<div align="center">

# PortableRotor

### Tragbarer Antennen-Rotor — ESP32, Web-Oberfläche, MQTT & Open Hardware

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![ESP32](https://img.shields.io/badge/platform-ESP32-E7352C?logo=espressif&logoColor=white)](https://www.espressif.com/)
[![Home Assistant](https://img.shields.io/badge/Home%20Assistant-MQTT-41BDF5?logo=homeassistant&logoColor=white)](https://www.home-assistant.io/)

<img src="docs/images/hero-banner.png" alt="PortableRotor — Konzeptgrafik Antennen-Rotor" width="100%" />

**Mechanik:** [`3D Model/`](./3D%20Model/) · **Firmware:** [`Firmware/`](./Firmware/) · **Website:** [df3mt.de](https://df3mt.de) · **Landing Page (GitHub Pages):** [„Pages“ aktivieren, Quelle `/docs`](https://docs.github.com/de/pages/getting-started-with-github-pages/configuring-a-publishing-source-for-your-github-pages-site)

[Funktionen](#funktionen) · [Repository-Struktur](#repository-struktur) · [Firmware](#firmware) · [Erste Schritte](#erste-schritte) · [Lizenz](#lizenz)

</div>

---

## Überblick

**PortableRotor** ist ein **tragbarer Antennen-Rotor** für den Amateurfunk: **3D-druckbare Mechanik**, Steuerung mit **ESP32**, **Webbrowser** und optional **MQTT / Home Assistant**.  
Dieses Repository bündelt **CAD**, **Firmware** und Dokumentation; die Firmware basiert auf **Arduino-ESP32** mit eingebetteter **Web-UI** (ohne SPIFFS).

---

## Funktionen

| | |
|--|--|
| **Web-Oberfläche** | Bedienung per Smartphone/PC, PWM-Rampen, touchfreundlich; Motor-Sitzung pro Browser (Cookie + Header) |
| **Netzwerk** | Parallel **SoftAP** (Einrichtung) und **STA** (Heim-WLAN); DHCP-Hostname für die Router-Anzeige |
| **MQTT** | Home-Assistant-**Discovery**; Sollwert als Zahl auf `{prefix}/set`, Status auf `{prefix}/state` |
| **OTA** | Update per **Browser** (`.bin`) und **Arduino IDE** (Netzwerk-Port / mDNS) |
| **Stabilität** | Optionales OTA-Passwort (NVS), Drosselung störender API-Anfragen, `yield()` beim Web-Flash |
| **Mechanik** | Modelle im Ordner **`3D Model/`** |

---

## Repository-Struktur

```
PortableRotor/
├── 3D Model/                    # CAD / STL — Mechanik (README + deine Modelle)
├── Firmware/
│   ├── README.md                # Kurzüberblick Arduino
│   └── DF3MT-Rotor/             # Sketch-Ordner (in der IDE hier öffnen)
│       ├── DF3MT-Rotor.ino
│       ├── DF3MT_Config.h
│       ├── kIndexHtml.h
│       ├── README.md
│       └── .gitignore
├── docs/                        # GitHub Pages: Landing Page + Bilder
│   ├── .nojekyll
│   ├── index.html
│   └── images/
├── LICENSE                      # GPL-3.0 (vollständiger Text)
├── .gitignore
└── README.md
```

---

## Firmware

Die empfohlene Firmware (z. B. **DF3MT-Rotor**) enthält unter anderem:

- **L298N** (oder kompatible H-Brücke) mit PWM-Rampen in der UI  
- **Captive-Portal**-Unterstützung (DNS, Konnektivitätstests)  
- **Motor-Lock:** eine Browser-Sitzung steuert den Motor; **MQTT** bleibt für Automatisierung unabhängig  
- **MQTT-Validierung:** nur gültige Ganzzahlen im PWM-Bereich  
- Zentrale Konfiguration in **`DF3MT_Config.h`**

Ausführliche Build- und Sicherheitshinweise: **[`Firmware/DF3MT-Rotor/README.md`](./Firmware/DF3MT-Rotor/README.md)** und **[`Firmware/README.md`](./Firmware/README.md)**.

---

## Erste Schritte

1. **Mechanik:** Bauteile aus **`3D Model/`** drucken und montieren.  
2. **Elektronik:** ESP32 und Motor-Treiber (z. B. L298N) nach Projektunterlagen verdrahten.  
3. **Firmware:** In der **Arduino IDE** den Ordner **`Firmware/DF3MT-Rotor/`** öffnen (`DF3MT-Rotor.ino`), Board **ESP32**, Partitionsschema **mit OTA**, flashen.  
4. **Inbetriebnahme:** Mit dem Geräte-AP (z. B. `DF3MT-Rotor`) verbinden, Browser öffnen, optional Heim-WLAN eintragen.  
5. **GitHub Pages (optional):** *Repository-Einstellungen → Pages → Quelle:* Branch **main** (oder Standard-Branch), Ordner **`/docs`**.  
   Öffentliche Adresse typisch: `https://df3mt.github.io/PortableRotor/` (Organisations- oder Benutzername anpassen).

---

## Bilder

Banner: `docs/images/hero-banner.png`. Eigene Fotos (Aufbau, UI, Platine): siehe [`docs/images/README.md`](./docs/images/README.md).

---

## `.gitignore`

Im Repository-Root: Build-Artefakte, PlatformIO, Editor-Müll sowie Platzhalter für lokale Secret-Header (`secrets.h`, `DF3MT_Secrets.h`). Details in der Datei selbst.

---

## Lizenz

Dieses Projekt steht unter der **GNU General Public License v3.0** — siehe [`LICENSE`](./LICENSE).

---

## Links

- **Quellcode:** [github.com/DF3MT/PortableRotor](https://github.com/DF3MT/PortableRotor)  
- **Projekt / Infos:** [df3mt.de](https://df3mt.de)

---

<div align="center">

**Vy 73 de** · *DF3MT*

</div>
