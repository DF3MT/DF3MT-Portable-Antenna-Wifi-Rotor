# DF3MT-Rotor (ESP32)

Firmware für den **DF3MT Portable Rotor**: SoftAP + optional STA, Web-UI (`kIndexHtml.h`), L298N-PWM, optional MQTT (Home Assistant) und OTA.

Im Repository **PortableRotor** liegt dieser Sketch unter **`Firmware/DF3MT-Rotor/`** — in der Arduino IDE **diesen Ordner** öffnen (nicht nur die `.ino`-Datei aus einem anderen Pfad).

- Hardware / Projektinfos: [PortableRotor](https://github.com/DF3MT/PortableRotor) · [df3mt.de](https://df3mt.de)

## Build

- **Arduino IDE** mit Board-Paket *esp32* (Arduino-ESP32 3.x), passendes **Partition Scheme mit OTA**.
- Sketch: `DF3MT-Rotor.ino`, Konfiguration: `DF3MT_Config.h`, eingebettetes HTML: `kIndexHtml.h` (alle Dateien in diesem Ordner).
- Serielles Log: `115200` Baud (`DF3MT_SERIAL_BAUD`).

## Konfiguration (`DF3MT_Config.h`)

Wichtige Schalter: GPIO (`DF3MT_L298N_*`), AP-SSID/Passwort, STA-Hostname, HTTP-Port, MQTT-Defaults, UI-Rampenzeiten, OTA.

### Geheimnisse nicht ins öffentliche Repository

`DF3MT_AP_PASS`, WLAN-/MQTT-Zugangsdaten in der Web-UI (NVS) und ggf. `#define DF3MT_OTA_PASSWORD` sind **sensible Daten**.

- **`.gitignore`** ignoriert u. a. `secrets.h` und `DF3MT_Secrets.h`. Du kannst z. B. eine lokale Header-Datei anlegen und sie **nur auf deinem Rechner** per `#include` vor `DF3MT_Config.h` einbinden, oder Passwörter ausschließlich über die Web-UI in den NVS schreiben.
- Vor einem Push: prüfen, dass keine echten Passwörter in der geteilten `DF3MT_Config.h` stehen.

### Router-Name (STA) vs. mDNS (OTA)

| Einstellung | Zweck |
|-------------|--------|
| **`DF3MT_WIFI_STA_HOSTNAME`** (z. B. `DF3MT-Rotor`) | DHCP-Clientname im **Heim-WLAN** — erscheint oft so in der Router-Geräteliste. |
| **`DF3MT_OTA_HOSTNAME`** (z. B. `df3mt-rotor`) | **mDNS** für Arduino-IDE-OTA (`*.local`) — üblich kleingeschrieben, unabhängig vom Router-Namen. |

Beide bewusst getrennt halten; Details stehen auch als Kommentar in `DF3MT_Config.h`.

## Web-UI: Motor-Lock vs. MQTT

- **Browser-Steuerung** nutzt eine **Sitzungs-ID** (Cookie `df3mt_motor_sid` + Header `X-DF3MT-Session`). Pro ESP ist **maximal eine** solche Sitzung „Lock-Inhaber“ für `/api/motor`.
- **MQTT** (`{prefix}/set`): Befehle sind **unabhängig** vom Web-Lock — sinnvoll für Home Assistant, auch wenn gerade jemand die Webseite offen hat. Ungültige Payloads (keine ganze Zahl im Bereich −255…255) werden **ignoriert** und im Seriallog als Warnung vermerkt.

## HTTP 429 (Rate-Limit)

Für dieselbe Session-ID werden sehr häufige Anfragen gedrosselt, die ohnehin nur **423** (falscher Lock), **428** (kein Claim) oder **409** (Claim busy) liefern würden — um die CPU zu schonen und Missbrauch zu begrenzen. Der **aktive Lock-Inhaber** ist davon **nicht** betroffen (normale Rampe bleibt möglich).

Einstellungen: `DF3MT_MOTOR_API_GUEST_COOLDOWN_MS`, `DF3MT_MOTOR_CLAIM_BUSY_COOLDOWN_MS` in `DF3MT_Config.h`.

## OTA (Web-Upload)

Beim Schreiben der Firmware-Chunks wird nach jedem erfolgreichen `Update.write` **`yield()`** aufgerufen, damit der Watchdog und andere Tasks Luft bekommen.

## `.gitignore`

Enthält u. a. Build-Ordner, typische Binärartefakte und Platzhalter für lokale Secret-Header (`secrets.h`, `DF3MT_Secrets.h`). Ergänze bei Bedarf eigene Einträge (IDE-Caches, persönliche Skripte).
