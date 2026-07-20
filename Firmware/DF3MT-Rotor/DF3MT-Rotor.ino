/*
 * DF3MT-Rotor.ino — ESP32 SoftAP + optional STA (Heim-WLAN), L298N, Web-UI.
 * Arduino-ESP32 3.x: ledcAttach(PIN, freq, bits) / ledcWrite(PIN, duty)
 * Zugangsdaten: NVS (Preferences), Klartext — nur im vertrauenswürdigen Netz nutzen.
 *
 * Debug: Serial Monitor 115200 Baud — Zeilen wie [12345][ERR][modul] Text
 *        (ERR= Fehler, WRN= Warnung, INF= Info)
 *
 * OTA: Arduino IDE → Netzwerk-Port (3232) oder Web-UI: .bin-Upload. Partition Scheme mit OTA.
 * OTA-Passwort: in der Web-UI setzen (NVS) oder optional #define DF3MT_OTA_PASSWORD (Fallback wenn NVS leer).
 * Open Source / Hardware-Infos: https://github.com/DF3MT/PortableRotor · https://df3mt.de
 *
 * MQTT (optional): Arduino Library Manager → „PubSubClient“ (Nick O’Leary). Home Assistant: Gerät erscheint
 * nach Verbindung per MQTT-Discovery (Number −255…255 auf {prefix}/set bzw. /state).
 * MQTT /set wirkt ohne Web-Motor-Lock (Home-Automation unabhängig von Browser-Sitzung); Details siehe README.md.
 *
 * Web-API: bei zu häufigen Anfragen ohne gültigen Lock antwortet /api/motor ggf. mit HTTP 429 (siehe README).
 */

#include "DF3MT_Config.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>
#include <Update.h>
#include <WebServer.h>
#include <Preferences.h>
#include <esp_netif.h>
#include <esp_idf_version.h>
#include <esp_sleep.h>
#include <DNSServer.h>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include "kIndexHtml.h"

/** Serielles Debug-Log: [ms][LEVEL][Modul] Nachricht */
static void rotoLogFmt(const char *level, const char *mod, const char *fmt, va_list ap) {
  Serial.printf("[%8lu][%s][%s] ", static_cast<unsigned long>(millis()), level, mod);
  char buf[DF3MT_ROTO_LOG_BUF];
  vsnprintf(buf, sizeof(buf), fmt, ap);
  Serial.println(buf);
}

static void logE(const char *mod, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  rotoLogFmt("ERR", mod, fmt, ap);
  va_end(ap);
}

static void logW(const char *mod, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  rotoLogFmt("WRN", mod, fmt, ap);
  va_end(ap);
}

static void logI(const char *mod, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  rotoLogFmt("INF", mod, fmt, ap);
  va_end(ap);
}

/** Ad-hoc / SoftAP: festes IPv4; DHCP-Server verteilt Adressen an Clients (Standard im ESP-IDF). */
static const IPAddress kApIp(DF3MT_AP_IP1, DF3MT_AP_IP2, DF3MT_AP_IP3, DF3MT_AP_IP4);
static const IPAddress kApGw(DF3MT_AP_IP1, DF3MT_AP_IP2, DF3MT_AP_IP3, DF3MT_AP_IP4);
static const IPAddress kApSn(255, 255, 255, 0);

const char *AP_SSID = DF3MT_AP_SSID;
const char *AP_PASS = DF3MT_AP_PASS;

/** Optional: OTA-Upload in der Arduino IDE nur mit diesem Passwort (Kommentar entfernen + setzen). */
// #define DF3MT_OTA_PASSWORD "change-me"

#define L298N_IN1 DF3MT_L298N_IN1
#define L298N_IN2 DF3MT_L298N_IN2
#define L298N_ENA DF3MT_L298N_ENA

static const char *PREF_NS = DF3MT_PREF_NS;
static const char *KEY_SSID = "sta_ssid";
static const char *KEY_PASS = "sta_pass";
static const char *KEY_OTA_PASS = "ota_pass";
static const char *KEY_MQTT_EN = "mqtt_en";
static const char *KEY_MQTT_HOST = "mqtt_host";
static const char *KEY_MQTT_PORT = "mqtt_port";
static const char *KEY_MQTT_USER = "mqtt_user";
static const char *KEY_MQTT_PASS = "mqtt_pass";
static const char *KEY_MQTT_PREFIX = "mqtt_pre";

WebServer server(DF3MT_HTTP_PORT);
Preferences prefs;
DNSServer dnsServer;
WiFiClient gMqttTcp;
PubSubClient gMqttClient(gMqttTcp);
/** Letzter Sollwert −255…255 (Web + MQTT), für /state und Anzeige. */
static int gMotorPwmSigned = 0;
/** Getrennte Speed/Richtung-Sicht (Home Assistant): Speed 0…255, Richtung −1 CCW / 0 Stopp / +1 CW. */
static int gMotorSpeed = 0;
/** Idle-Deep-Sleep: letzte echte Interaktion (ms seit Boot). */
static uint32_t gLastActivityMs = 0;
/** Während Timer-Wake-Lauschfenster: MQTT-/Web-Befehl empfangen. */
static volatile bool gSleepPollGotActivity = false;
/** Arduino-OTA oder Web-Flash läuft — kein Sleep. */
static bool gOtaBusy = false;
static int gMotorDir = 0;
static uint32_t gMqttLastReconnectMs = 0;

static void noteActivity();
static void enterDeepSleep(const char *reason);
static bool sleepPrerequisitesMet();
static void maintainIdleSleep();
static bool runMqttPollWakeCycle();

static void logHttpClient(const char *what) {
  Serial.printf("[%8lu][ERR][http] %s method=%s uri=%s args=%d\n",
                static_cast<unsigned long>(millis()), what,
                server.method() == HTTP_GET ? "GET" :
                server.method() == HTTP_HEAD ? "HEAD" :
                server.method() == HTTP_POST ? "POST" : "?",
                server.uri().c_str(), server.args());
}

/** STA: erneut verbinden (nicht im WiFi-Event-Handler selbst aufrufen). */
static volatile bool gStaDisconnectedFlag = false;
static volatile uint32_t gStaDisconnectAt = 0;
static uint32_t gStaLastBeginMs = 0;

const int PWM_FREQ = DF3MT_PWM_FREQ;
/** 10 Bit → feinere Stufen im nutzbaren Drehzahlbereich (Rampe in der UI unverändert). */
const int PWM_RES = DF3MT_PWM_RES_BITS;
static const int PWM_DUTY_MAX = (1 << PWM_RES) - 1;

/**
 * Unter diesem UI-Wert (0–255 von /api/motor) dreht der Motor hier nicht (L298N + Reibung).
 * Ab MOTOR_UI_START wird linear auf volle PWM-Auflösung gemappt — mehr Zwischenstufen als mit 8 Bit.
 */
static const int MOTOR_UI_START = DF3MT_MOTOR_UI_START;
/** Duty bei UI=MOTOR_UI_START (gleicher Tastgrad wie früher ~150/255 bei 8 Bit). */
static const int PWM_DUTY_MIN_RUN = (MOTOR_UI_START * PWM_DUTY_MAX) / DF3MT_MOTOR_PWM_MAX;

/** DHCP-Server für das AP-Interface explizit starten (läuft sonst meist schon automatisch). */
static void startSoftApDhcpServer() {
  esp_netif_t *ap = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
  if (!ap) {
    logE("dhcp", "Netif WIFI_AP_DEF nicht gefunden");
    return;
  }
  esp_err_t e = esp_netif_dhcps_start(ap);
  if (e == ESP_OK) {
    logI("dhcp", "dhcps_start ok");
  } else if (e == ESP_ERR_ESP_NETIF_DHCP_ALREADY_STARTED) {
    logI("dhcp", "dhcps bereits aktiv");
  } else {
    logE("dhcp", "dhcps_start: %s", esp_err_to_name(e));
  }
}

/** Puffer für DHCP-Opt. 114 (Captive Portal): Pointer muss für DHCPS-Laufzeit gültig bleiben. */
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 4, 2)
static char gDhcpCaptivePortalUri[72];
#endif

/** DHCP-Option: DNS = AP; optional Opt. 114 = http://<AP-IP>/ (OS öffnet „Anmelden“-Browser mit ESP-IP). */
static void setDhcpDnsServerToAp() {
  esp_netif_t *ap = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
  if (!ap) {
    logE("dhcp", "setDns: Netif WIFI_AP_DEF fehlt");
    return;
  }
  uint32_t dns = static_cast<uint32_t>(WiFi.softAPIP());
  esp_netif_dhcps_stop(ap);
  esp_err_t err = esp_netif_dhcps_option(ap, ESP_NETIF_OP_SET, ESP_NETIF_DOMAIN_NAME_SERVER, &dns, sizeof(dns));
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 4, 2)
  snprintf(gDhcpCaptivePortalUri, sizeof(gDhcpCaptivePortalUri), "http://%s/", WiFi.softAPIP().toString().c_str());
  esp_err_t capErr =
      esp_netif_dhcps_option(ap, ESP_NETIF_OP_SET, ESP_NETIF_CAPTIVEPORTAL_URI, gDhcpCaptivePortalUri,
                             static_cast<uint32_t>(strlen(gDhcpCaptivePortalUri) + 1));
  if (capErr != ESP_OK) {
    logW("dhcp", "CAPTIVEPORTAL_URI: %s", esp_err_to_name(capErr));
  } else {
    logI("dhcp", "Captive-Portal-URI: %s", gDhcpCaptivePortalUri);
  }
#endif
  esp_err_t st = esp_netif_dhcps_start(ap);
  if (err == ESP_OK && st == ESP_OK) {
    logI("dhcp", "DNS-Option fuer Clients = AP-IP");
  } else {
    logE("dhcp", "DNS-Option: opt=%s dhcps_start=%s", esp_err_to_name(err), esp_err_to_name(st));
  }
}

/** DNS auf Port 53: alle Anfragen → SoftAP-IP (Captive-Portal / „Internet erkannt“). */
static void startDnsForCaptivePortal() {
  dnsServer.setTTL(DF3MT_DNS_TTL_S);
  if (!dnsServer.start(DF3MT_DNS_PORT, "*", WiFi.softAPIP())) {
    logE("dns", "start(53) fehlgeschlagen");
    return;
  }
  logI("dns", "alle Namen -> AP-IP (Port %d)", DF3MT_DNS_PORT);
}

/** HTTP 204 wie von Google-Konnektivitätstests erwartet (GET + HEAD). */
static void replyConnectivityProbe204() {
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, private");
  server.sendHeader("Pragma", "no-cache");
  server.send(204, "text/plain", "");
}

static void handleHttpConnectivityChecks() {
  auto reg204 = [](const char *path) {
    server.on(path, HTTP_GET, []() { replyConnectivityProbe204(); });
    server.on(path, HTTP_HEAD, []() { replyConnectivityProbe204(); });
  };
  /* Google / AOSP (z. B. connectivitycheck.gstatic.com) */
  reg204("/generate_204");
  reg204("/gen_204");

  /* Microsoft */
  server.on("/connecttest.txt", HTTP_GET, []() {
    server.send(200, "text/plain", "Microsoft Connect Test");
  });
  server.on("/redirect", HTTP_GET, []() {
    server.send(200, "text/plain", "Microsoft Connect Test");
  });
  server.on("/ncsi.txt", HTTP_GET, []() {
    server.send(200, "text/plain", "Microsoft NCSI");
  });

  /* Apple */
  server.on("/hotspot-detect.html", HTTP_GET, []() {
    server.send(200, "text/html", "<HTML><HEAD><TITLE>Success</TITLE></HEAD><BODY>Success</BODY></HTML>");
  });
  server.on("/hotspot-detect.html", HTTP_HEAD, []() {
    server.send(200, "text/html", "<HTML><HEAD><TITLE>Success</TITLE></HEAD><BODY>Success</BODY></HTML>");
  });

  /* Firefox */
  server.on("/success.txt", HTTP_GET, []() {
    server.send(200, "text/plain", "success");
  });

  /* Huawei (Hostname per DNS → gleicher Pfad) */
  reg204("/mobile/status.php");
}

static String jsonEscape(const String &s) {
  String o;
  o.reserve(s.length() + 8);
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '"' || c == '\\') o += '\\';
    if (c == '\n' || c == '\r') continue;
    o += c;
  }
  return o;
}

void motorStop() {
  digitalWrite(L298N_IN1, LOW);
  digitalWrite(L298N_IN2, LOW);
  ledcWrite(L298N_ENA, 0);
}

void motorRun(int speed, bool forward) {
  speed = constrain(speed, 0, DF3MT_MOTOR_PWM_MAX);
  if (speed < MOTOR_UI_START) {
    motorStop();
    return;
  }
  if (forward) {
    digitalWrite(L298N_IN1, HIGH);
    digitalWrite(L298N_IN2, LOW);
  } else {
    digitalWrite(L298N_IN1, LOW);
    digitalWrite(L298N_IN2, HIGH);
  }
  const int spanUi = DF3MT_MOTOR_PWM_MAX - MOTOR_UI_START;
  int duty = PWM_DUTY_MIN_RUN +
             (speed - MOTOR_UI_START) * (PWM_DUTY_MAX - PWM_DUTY_MIN_RUN) / spanUi;
  duty = constrain(duty, PWM_DUTY_MIN_RUN, PWM_DUTY_MAX);
  ledcWrite(L298N_ENA, duty);
}

/** Sollwert −255…255: 0 Stopp, negativ CCW, positiv CW (gleiche Logik wie Web-UI). */
static void applyMotorSignedPwm(int pwm) {
  pwm = constrain(pwm, -DF3MT_MOTOR_PWM_MAX, DF3MT_MOTOR_PWM_MAX);
  gMotorPwmSigned = pwm;
  if (pwm == 0) {
    motorStop();
  } else if (pwm > 0) {
    motorRun(pwm, true);
  } else {
    motorRun(-pwm, false);
  }
}

/** Effektiven PWM aus dem Speed/Richtung-Modell berechnen und ausgeben. */
static void motorApplyModel() {
  applyMotorSignedPwm(gMotorDir == 0 ? 0 : gMotorDir * gMotorSpeed);
}

/**
 * Signierten Sollwert setzen UND das Speed/Richtung-Modell synchron halten.
 * Wird von Web-UI (/api/motor) und dem klassischen MQTT-/set genutzt, damit die
 * getrennten Home-Assistant-Entities (Speed, Richtung) immer konsistent sind.
 * Bei pwm==0 bleibt der zuletzt gewählte Speed erhalten (nur Richtung = Stopp).
 */
static void motorSetSignedSynced(int pwm) {
  pwm = constrain(pwm, -DF3MT_MOTOR_PWM_MAX, DF3MT_MOTOR_PWM_MAX);
  if (pwm > 0) {
    gMotorDir = 1;
    gMotorSpeed = pwm;
  } else if (pwm < 0) {
    gMotorDir = -1;
    gMotorSpeed = -pwm;
  } else {
    gMotorDir = 0;
  }
  applyMotorSignedPwm(pwm);
}

/** Home-Assistant „Speed“ (0…255): Magnitude setzen, Richtung beibehalten. */
static void motorSetSpeed(int spd) {
  gMotorSpeed = constrain(spd, 0, DF3MT_MOTOR_PWM_MAX);
  motorApplyModel();
}

/** Home-Assistant „Direction“: −1 CCW / 0 Stopp / +1 CW; Speed bleibt gemerkt. */
static void motorSetDir(int dir) {
  gMotorDir = (dir > 0) ? 1 : (dir < 0 ? -1 : 0);
  motorApplyModel();
}

/** Richtung als HA-Select-Text. */
static const char *motorDirText() {
  return gMotorDir > 0 ? "CW" : (gMotorDir < 0 ? "CCW" : "STOP");
}

static void staBeginSaved() {
  String ssid = prefs.getString(KEY_SSID, "");
  if (ssid.length() == 0) return;
  String pass = prefs.getString(KEY_PASS, "");
  gStaLastBeginMs = millis();
  WiFi.disconnect(false, false);
  delay(DF3MT_WIFI_STA_BEGIN_DELAY_MS);
  WiFi.begin(ssid.c_str(), pass.c_str());
  logI("sta", "Verbindungsaufbau zu \"%s\" …", ssid.c_str());
}

void startStaFromPrefs() {
  staBeginSaved();
}

/** Disconnect-Grund (esp_wifi_types) – hilfreich für die Fehlersuche. */
static void logStaDisconnectReason(uint8_t reason) {
  const char *txt = "?";
  switch (reason) {
    case 1:  txt = "UNSPECIFIED"; break;
    case 2:  txt = "AUTH_EXPIRE"; break;
    case 3:  txt = "ASSOC_LEAVE"; break;
    case 4:  txt = "ASSOC_NOT_AUTHED"; break;
    case 8:  txt = "ASSOC_LEAVE/AP_voll"; break;
    case 15: txt = "4WAY_HANDSHAKE_TIMEOUT"; break;
    case 200: txt = "BEACON_TIMEOUT"; break;
    case 201: txt = "NO_AP_FOUND"; break;
    case 202: txt = "AUTH_FAIL"; break;
    case 203: txt = "ASSOC_FAIL"; break;
    case 204: txt = "HANDSHAKE_TIMEOUT"; break;
    case 205: txt = "CONNECTION_FAIL"; break;
    default: break;
  }
  logW("wifi", "STA getrennt reason=%u (%s)", static_cast<unsigned>(reason), txt);
}

static void onWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_START:
      logI("wifi", "STA-Stack gestartet");
      break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      logI("wifi", "STA assoziiert (Layer2)");
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      logI("wifi", "STA IP: %s", WiFi.localIP().toString().c_str());
      gStaDisconnectedFlag = false;
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      logStaDisconnectReason(info.wifi_sta_disconnected.reason);
      gStaDisconnectedFlag = true;
      gStaDisconnectAt = millis();
      break;
    default:
      break;
  }
}

static void maintainStaConnection() {
  static uint32_t lastMs = 0;
  const uint32_t kPeriodMs = DF3MT_STA_RECONNECT_PERIOD_MS;
  const uint32_t kReconnectAfterDiscMs = DF3MT_STA_RECONNECT_AFTER_DISC_MS;

  String ssid = prefs.getString(KEY_SSID, "");
  if (ssid.length() == 0) return;

  uint32_t now = millis();
  if (WiFi.status() == WL_CONNECTED) return;

  if (gStaDisconnectedFlag) {
    if ((uint32_t)(now - gStaDisconnectAt) >= kReconnectAfterDiscMs) {
      gStaDisconnectedFlag = false;
      logI("sta", "Wiederverbindung …");
      staBeginSaved();
    }
    return;
  }

  if ((uint32_t)(now - gStaLastBeginMs) < DF3MT_STA_ASSOC_GRACE_MS) return;

  if (now - lastMs < kPeriodMs) return;
  lastMs = now;
  logW("sta", "noch nicht verbunden, erneuter Versuch …");
  staBeginSaved();
}

void handleWifiStatus() {
  bool staOk = (WiFi.status() == WL_CONNECTED);
  String savedSsid = prefs.getString(KEY_SSID, "");
  bool hasPass = prefs.getString(KEY_PASS, "").length() > 0;

  String j = "{";
  j += "\"ap_ip\":\"" + WiFi.softAPIP().toString() + "\"";
  j += ",\"sta_connected\":" + String(staOk ? "true" : "false");
  j += ",\"sta_ip\":\"" + String(staOk ? WiFi.localIP().toString() : "") + "\"";
  j += ",\"sta_ssid\":\"" + jsonEscape(savedSsid) + "\"";
  j += ",\"has_saved_pass\":" + String(hasPass ? "true" : "false");
  j += "}";
  server.send(200, "application/json", j);
}

void handleWifiSave() {
  if (!server.hasArg("ssid")) {
    logE("api/wifi", "save: ssid fehlt");
    server.send(400, "application/json", "{\"ok\":false,\"err\":\"ssid fehlt\"}");
    return;
  }
  String ssid = server.arg("ssid");
  ssid.trim();
  if (ssid.length() == 0) {
    logE("api/wifi", "save: SSID leer");
    server.send(400, "application/json", "{\"ok\":false,\"err\":\"SSID leer\"}");
    return;
  }

  String newPass = server.hasArg("password") ? server.arg("password") : "";
  String oldSsid = prefs.getString(KEY_SSID, "");
  String oldPass = prefs.getString(KEY_PASS, "");
  String passToStore;

  if (newPass.length() > 0) {
    passToStore = newPass;
  } else if (ssid == oldSsid) {
    passToStore = oldPass;
  } else {
    passToStore = "";
  }

  const bool sameCredentials = (ssid == oldSsid) && (passToStore == oldPass);
  prefs.putString(KEY_SSID, ssid);
  prefs.putString(KEY_PASS, passToStore);

  /* Gleiche Daten + schon verbunden: kein disconnect/begin (sonst wirkt jeder Klick wie „WLAN weg“). */
  if (!sameCredentials || WiFi.status() != WL_CONNECTED) {
    staBeginSaved();
  } else {
    logI("sta", "WLAN-Daten unveraendert, Verbindung bleibt bestehen");
  }

  noteActivity();
  server.send(200, "application/json", "{\"ok\":true}");
}

void handleWifiClear() {
  prefs.remove(KEY_SSID);
  prefs.remove(KEY_PASS);
  WiFi.disconnect(false, false);
  delay(DF3MT_WIFI_CLEAR_DELAY_MS);
  server.send(200, "application/json", "{\"ok\":true}");
}

static bool mqttCfgEnabled() { return prefs.getUChar(KEY_MQTT_EN, 0) != 0; }

static String mqttMacChipId() {
  uint8_t m[6];
  WiFi.macAddress(m);
  String s;
  for (int i = 0; i < 6; i++) {
    if (m[i] < 16) s += "0";
    s += String(m[i], HEX);
  }
  return s;
}

static String mqttSanitizePrefix(const String &in) {
  String o;
  o.reserve(in.length() + 8);
  for (size_t i = 0; i < in.length(); i++) {
    char c = in[i];
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '/' || c == '_' ||
        c == '-') {
      o += c;
    }
  }
  o.trim();
  while (o.length() > 0 && o.endsWith("/")) o.remove(o.length() - 1);
  if (o.length() == 0) o = "df3mt/rotor";
  return o;
}

static String mqttTopicBase() {
  return mqttSanitizePrefix(prefs.getString(KEY_MQTT_PREFIX, "df3mt/rotor"));
}

/** „online“/„offline“ für Home-Assistant-Verfügbarkeit (auch als MQTT-LWT genutzt). */
static String mqttAvailabilityTopic() { return mqttTopicBase() + "/availability"; }

static void mqttPublishAvailability(bool online) {
  if (!gMqttClient.connected()) return;
  gMqttClient.publish(mqttAvailabilityTopic().c_str(), online ? "online" : "offline", true);
}

/** Basis-URL der eingebauten Web-UI (STA-IP bevorzugt, sonst SoftAP-IP). */
static String mqttDeviceUrl() {
  IPAddress ip = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP() : WiFi.softAPIP();
  String url = "http://" + ip.toString();
  if (DF3MT_HTTP_PORT != 80) url += ":" + String(static_cast<unsigned>(DF3MT_HTTP_PORT));
  return url;
}

/** Web-URL der Rotor-Oberfläche als retained Sensor veröffentlichen. */
static void mqttPublishUrl() {
  if (!gMqttClient.connected()) return;
  gMqttClient.publish((mqttTopicBase() + "/url/state").c_str(), mqttDeviceUrl().c_str(), true);
}

static void mqttPublishState() {
  if (!gMqttClient.connected()) return;
  String base = mqttTopicBase();
  gMqttClient.publish((base + "/state").c_str(), String(gMotorPwmSigned).c_str(), true);
  gMqttClient.publish((base + "/speed/state").c_str(), String(gMotorSpeed).c_str(), true);
  gMqttClient.publish((base + "/direction/state").c_str(), motorDirText(), true);
  gMqttClient.publish((base + "/running/state").c_str(), gMotorPwmSigned != 0 ? "ON" : "OFF", true);
}

/** MQTT-Payload muss exakt eine ganze Zahl −DF3MT_MOTOR_PWM_MAX…+MAX sein (optional +/−, keine Leerzeichen innen). */
static bool mqttPayloadToSignedPwm(const char *str, int *out) {
  if (!str || !*str) return false;
  const char *p = str;
  if (*p == '+') ++p;
  int sign = 1;
  if (*p == '-') {
    sign = -1;
    ++p;
  }
  if (*p == '\0') return false;
  long acc = 0;
  while (*p) {
    if (*p < '0' || *p > '9') return false;
    acc = acc * 10 + static_cast<long>(*p - '0');
    if (acc > static_cast<long>(DF3MT_MOTOR_PWM_MAX)) return false;
    ++p;
  }
  acc *= sign;
  if (acc < -static_cast<long>(DF3MT_MOTOR_PWM_MAX) || acc > static_cast<long>(DF3MT_MOTOR_PWM_MAX)) return false;
  *out = static_cast<int>(acc);
  return true;
}

/** Gemeinsamer Block für jede HA-Entity: Device-Verknüpfung + Verfügbarkeit. */
static String mqttDiscoveryCommon(const String &did) {
  String j;
  j += "\"device\":{\"identifiers\":[\"" + did + "\"],\"name\":\"DF3MT Rotor\",\"manufacturer\":\"DF3MT\",";
  j += "\"model\":\"Rotor\"},";
  j += "\"avty_t\":\"" + jsonEscape(mqttAvailabilityTopic()) + "\",";
  j += "\"pl_avail\":\"online\",\"pl_not_avail\":\"offline\",";
  return j;
}

static void mqttPublishDiscovery() {
  String base = mqttTopicBase();
  String did = String("df3mt_rotor_") + mqttMacChipId();
  String common = mqttDiscoveryCommon(did);
  String maxs = String(DF3MT_MOTOR_PWM_MAX);

  // 1) Signierter PWM-Sollwert (−255…255) — kombinierte Speed+Richtung (Rückwärtskompatibel).
  {
    String j = "{";
    j += "\"name\":\"PWM (signed)\",";
    j += "\"uniq_id\":\"" + did + "_pwm\",";
    j += common;
    j += "\"command_topic\":\"" + jsonEscape(base + "/set") + "\",";
    j += "\"state_topic\":\"" + jsonEscape(base + "/state") + "\",";
    j += "\"icon\":\"mdi:rotate-right\",";
    j += "\"min\":-" + maxs + ",\"max\":" + maxs + ",\"step\":1}";
    gMqttClient.publish(("homeassistant/number/" + did + "_pwm/config").c_str(), j.c_str(), true);
  }

  // 2) Speed (0…255) — Drehzahl/Magnitude.
  {
    String j = "{";
    j += "\"name\":\"Speed\",";
    j += "\"uniq_id\":\"" + did + "_speed\",";
    j += common;
    j += "\"command_topic\":\"" + jsonEscape(base + "/speed/set") + "\",";
    j += "\"state_topic\":\"" + jsonEscape(base + "/speed/state") + "\",";
    j += "\"icon\":\"mdi:speedometer\",";
    j += "\"min\":0,\"max\":" + maxs + ",\"step\":1}";
    gMqttClient.publish(("homeassistant/number/" + did + "_speed/config").c_str(), j.c_str(), true);
  }

  // 3) Direction — Select STOP/CW/CCW.
  {
    String j = "{";
    j += "\"name\":\"Direction\",";
    j += "\"uniq_id\":\"" + did + "_dir\",";
    j += common;
    j += "\"command_topic\":\"" + jsonEscape(base + "/direction/set") + "\",";
    j += "\"state_topic\":\"" + jsonEscape(base + "/direction/state") + "\",";
    j += "\"icon\":\"mdi:compass\",";
    j += "\"options\":[\"STOP\",\"CW\",\"CCW\"]}";
    gMqttClient.publish(("homeassistant/select/" + did + "_dir/config").c_str(), j.c_str(), true);
  }

  // 4) Stop — Button.
  {
    String j = "{";
    j += "\"name\":\"Stop\",";
    j += "\"uniq_id\":\"" + did + "_stop\",";
    j += common;
    j += "\"command_topic\":\"" + jsonEscape(base + "/stop/set") + "\",";
    j += "\"payload_press\":\"STOP\",";
    j += "\"icon\":\"mdi:stop\"}";
    gMqttClient.publish(("homeassistant/button/" + did + "_stop/config").c_str(), j.c_str(), true);
  }

  // 5) Running — Binary-Sensor (dreht / steht).
  {
    String j = "{";
    j += "\"name\":\"Running\",";
    j += "\"uniq_id\":\"" + did + "_running\",";
    j += common;
    j += "\"state_topic\":\"" + jsonEscape(base + "/running/state") + "\",";
    j += "\"payload_on\":\"ON\",\"payload_off\":\"OFF\",";
    j += "\"device_class\":\"running\"}";
    gMqttClient.publish(("homeassistant/binary_sensor/" + did + "_running/config").c_str(), j.c_str(), true);
  }

  // 6) Web-URL — Sensor mit Link zur eingebauten Rotor-Oberfläche.
  {
    String j = "{";
    j += "\"name\":\"Web URL\",";
    j += "\"uniq_id\":\"" + did + "_url\",";
    j += common;
    j += "\"state_topic\":\"" + jsonEscape(base + "/url/state") + "\",";
    j += "\"icon\":\"mdi:web\",";
    j += "\"entity_category\":\"diagnostic\"}";
    gMqttClient.publish(("homeassistant/sensor/" + did + "_url/config").c_str(), j.c_str(), true);
  }

  // 7) Rotate CW — Button (dreht im Uhrzeigersinn mit aktueller Speed, wie Web-UI).
  {
    String j = "{";
    j += "\"name\":\"Rotate CW\",";
    j += "\"uniq_id\":\"" + did + "_cw\",";
    j += common;
    j += "\"command_topic\":\"" + jsonEscape(base + "/cw/set") + "\",";
    j += "\"payload_press\":\"CW\",";
    j += "\"icon\":\"mdi:rotate-right\"}";
    gMqttClient.publish(("homeassistant/button/" + did + "_cw/config").c_str(), j.c_str(), true);
  }

  // 8) Rotate CCW — Button (dreht gegen den Uhrzeigersinn mit aktueller Speed, wie Web-UI).
  {
    String j = "{";
    j += "\"name\":\"Rotate CCW\",";
    j += "\"uniq_id\":\"" + did + "_ccw\",";
    j += common;
    j += "\"command_topic\":\"" + jsonEscape(base + "/ccw/set") + "\",";
    j += "\"payload_press\":\"CCW\",";
    j += "\"icon\":\"mdi:rotate-left\"}";
    gMqttClient.publish(("homeassistant/button/" + did + "_ccw/config").c_str(), j.c_str(), true);
  }

  logI("mqtt", "HA discovery (8 entities) fuer %s", did.c_str());
}

static void mqttCallback(char *topic, byte *payload, unsigned int len) {
  char buf[40];
  if (len >= sizeof(buf)) len = sizeof(buf) - 1;
  memcpy(buf, payload, len);
  buf[len] = 0;
  String s(buf);
  s.trim();

  String base = mqttTopicBase();
  String t(topic ? topic : "");
  if (!t.startsWith(base + "/")) return;
  String sub = t.substring(base.length() + 1);

  if (sub == "set") {
    int p = 0;
    if (!mqttPayloadToSignedPwm(s.c_str(), &p)) {
      logW("mqtt", "set ignoriert (erwartet ganze Zahl, Bereich +/-%d)", DF3MT_MOTOR_PWM_MAX);
      return;
    }
    motorSetSignedSynced(p);
    logI("mqtt", "set pwm=%d", p);
  } else if (sub == "speed/set") {
    int p = 0;
    if (!mqttPayloadToSignedPwm(s.c_str(), &p) || p < 0) {
      logW("mqtt", "speed ignoriert (erwartet 0…%d)", DF3MT_MOTOR_PWM_MAX);
      return;
    }
    motorSetSpeed(p);
    logI("mqtt", "set speed=%d", p);
  } else if (sub == "direction/set") {
    if (s.equalsIgnoreCase("CW")) motorSetDir(1);
    else if (s.equalsIgnoreCase("CCW")) motorSetDir(-1);
    else if (s.equalsIgnoreCase("STOP")) motorSetDir(0);
    else {
      logW("mqtt", "direction ignoriert (erwartet CW/CCW/STOP)");
      return;
    }
    logI("mqtt", "set direction=%s", motorDirText());
  } else if (sub == "cw/set") {
    motorSetDir(1);
    logI("mqtt", "rotate CW");
  } else if (sub == "ccw/set") {
    motorSetDir(-1);
    logI("mqtt", "rotate CCW");
  } else if (sub == "stop/set") {
    motorSetDir(0);
    logI("mqtt", "stop");
  } else {
    return;
  }
  noteActivity();
  mqttPublishState();
}

static void mqttDisconnect() {
  if (gMqttClient.connected()) {
    mqttPublishAvailability(false);
    gMqttClient.disconnect();
  }
}

static bool mqttReconnect() {
  if (!mqttCfgEnabled()) return false;
  if (WiFi.status() != WL_CONNECTED) return false;
  String host = prefs.getString(KEY_MQTT_HOST, "");
  host.trim();
  if (host.length() == 0) return false;
  uint32_t po = prefs.getUInt(KEY_MQTT_PORT, DF3MT_MQTT_DEFAULT_PORT);
  if (po == 0 || po > 65535) po = DF3MT_MQTT_DEFAULT_PORT;
  uint16_t port = static_cast<uint16_t>(po);

  gMqttClient.setServer(host.c_str(), port);
  gMqttClient.setCallback(mqttCallback);
  gMqttClient.setBufferSize(DF3MT_MQTT_BUFFER_SIZE);

  String cid = String("df3mt-") + mqttMacChipId();
  String user = prefs.getString(KEY_MQTT_USER, "");
  String pass = prefs.getString(KEY_MQTT_PASS, "");
  String will = mqttAvailabilityTopic();
  bool ok;
  if (user.length() > 0) {
    ok = gMqttClient.connect(cid.c_str(), user.c_str(), pass.c_str(), will.c_str(), 0, true, "offline");
  } else {
    ok = gMqttClient.connect(cid.c_str(), will.c_str(), 0, true, "offline");
  }
  if (!ok) {
    logW("mqtt", "connect fehlgeschlagen (state=%d)", gMqttClient.state());
    return false;
  }
  logI("mqtt", "verbunden %s:%u", host.c_str(), static_cast<unsigned>(port));
  String base = mqttTopicBase();
  gMqttClient.subscribe((base + "/set").c_str());
  gMqttClient.subscribe((base + "/speed/set").c_str());
  gMqttClient.subscribe((base + "/direction/set").c_str());
  gMqttClient.subscribe((base + "/cw/set").c_str());
  gMqttClient.subscribe((base + "/ccw/set").c_str());
  gMqttClient.subscribe((base + "/stop/set").c_str());
  mqttPublishAvailability(true);
  mqttPublishDiscovery();
  mqttPublishState();
  mqttPublishUrl();
  return true;
}

static void maintainMqtt() {
  if (!mqttCfgEnabled()) {
    mqttDisconnect();
    return;
  }
  if (WiFi.status() != WL_CONNECTED) {
    mqttDisconnect();
    return;
  }
  if (!gMqttClient.connected()) {
    uint32_t now = millis();
    if ((uint32_t)(now - gMqttLastReconnectMs) >= DF3MT_MQTT_RECONNECT_MS) {
      gMqttLastReconnectMs = now;
      mqttReconnect();
    }
  } else {
    gMqttClient.loop();
  }
}

void handleMqttStatus() {
  String host = prefs.getString(KEY_MQTT_HOST, "");
  bool en = mqttCfgEnabled();
  uint32_t po = prefs.getUInt(KEY_MQTT_PORT, DF3MT_MQTT_DEFAULT_PORT);
  String pre = mqttTopicBase();
  bool hasPass = prefs.getString(KEY_MQTT_PASS, "").length() > 0;
  bool conn = gMqttClient.connected();
  String j = "{\"ok\":true";
  j += ",\"enabled\":" + String(en ? "true" : "false");
  j += ",\"host\":\"" + jsonEscape(host) + "\"";
  j += ",\"port\":" + String(po);
  j += ",\"prefix\":\"" + jsonEscape(pre) + "\"";
  j += ",\"has_pass\":" + String(hasPass ? "true" : "false");
  j += ",\"connected\":" + String(conn ? "true" : "false");
  j += ",\"pwm\":" + String(gMotorPwmSigned);
  j += "}";
  server.send(200, "application/json", j);
}

void handleMqttSave() {
  bool en = server.hasArg("enabled") && (server.arg("enabled") == "1" || server.arg("enabled") == "true");
  String host = server.hasArg("host") ? server.arg("host") : "";
  host.trim();
  uint32_t port = server.hasArg("port") ? static_cast<uint32_t>(server.arg("port").toInt()) : DF3MT_MQTT_DEFAULT_PORT;
  if (port == 0 || port > 65535) port = DF3MT_MQTT_DEFAULT_PORT;
  String user = server.hasArg("user") ? server.arg("user") : "";
  String pass = server.hasArg("password") ? server.arg("password") : "";
  String prefix = server.hasArg("prefix") ? server.arg("prefix") : "df3mt/rotor";
  prefix = mqttSanitizePrefix(prefix);

  if (en && host.length() == 0) {
    server.send(400, "application/json", "{\"ok\":false,\"err\":\"Broker-Host fehlt\"}");
    return;
  }

  prefs.putUChar(KEY_MQTT_EN, en ? 1 : 0);
  prefs.putString(KEY_MQTT_HOST, host);
  prefs.putUInt(KEY_MQTT_PORT, port);
  prefs.putString(KEY_MQTT_USER, user);
  bool keepPass = server.hasArg("keep_mqtt_pass") && server.arg("keep_mqtt_pass") == "1";
  if (!keepPass) {
    prefs.putString(KEY_MQTT_PASS, pass);
  }
  prefs.putString(KEY_MQTT_PREFIX, prefix);

  mqttDisconnect();
  gMqttLastReconnectMs = 0;
  logI("mqtt", "NVS gespeichert en=%d", en ? 1 : 0);
  noteActivity();
  server.send(200, "application/json", "{\"ok\":true}");
}

void handleMqttClear() {
  prefs.putUChar(KEY_MQTT_EN, 0);
  prefs.remove(KEY_MQTT_HOST);
  prefs.remove(KEY_MQTT_PORT);
  prefs.remove(KEY_MQTT_USER);
  prefs.remove(KEY_MQTT_PASS);
  prefs.remove(KEY_MQTT_PREFIX);
  mqttDisconnect();
  gMqttLastReconnectMs = 0;
  logI("mqtt", "NVS MQTT geloescht");
  server.send(200, "application/json", "{\"ok\":true}");
}

static String otaPasswordNvsOrEmpty() { return prefs.getString(KEY_OTA_PASS, ""); }

static bool otaCompilePasswordDefined() {
#if defined(DF3MT_OTA_PASSWORD)
  return true;
#else
  return false;
#endif
}

/** Effektives OTA-Passwort: NVS schlägt Build-Fallback. */
static String otaPasswordEffective() {
  String p = otaPasswordNvsOrEmpty();
#if defined(DF3MT_OTA_PASSWORD)
  if (p.length() == 0) return String(DF3MT_OTA_PASSWORD);
#endif
  return p;
}

static bool otaPasswordRequired() { return otaPasswordEffective().length() > 0; }

void handleOtaStatus() {
  bool nvsSet = otaPasswordNvsOrEmpty().length() > 0;
  String j = "{\"ok\":true,\"ota_nvs_password\":" + String(nvsSet ? "true" : "false");
  j += ",\"ota_compile_fallback\":" + String(otaCompilePasswordDefined() ? "true" : "false");
  j += ",\"ota_auth_required\":" + String(otaPasswordRequired() ? "true" : "false");
  j += "}";
  server.send(200, "application/json", j);
}

void handleOtaPassword() {
  String curStored = otaPasswordNvsOrEmpty();
  const bool clearReq = server.hasArg("clear") && server.arg("clear") == "1";
  String newPass = server.hasArg("new_password") ? server.arg("new_password") : "";

  if (clearReq) {
#if defined(DF3MT_OTA_PASSWORD)
    if (curStored.length() == 0) {
      server.send(400, "application/json",
                   "{\"ok\":false,\"err\":\"Kein NVS-Passwort — Build #define bleibt aktiv\"}");
      return;
    }
#else
    if (curStored.length() == 0) {
      server.send(400, "application/json", "{\"ok\":false,\"err\":\"Kein gespeichertes OTA-Passwort\"}");
      return;
    }
#endif
    String curIn = server.hasArg("current_password") ? server.arg("current_password") : "";
    if (curIn != curStored) {
      server.send(403, "application/json", "{\"ok\":false,\"err\":\"aktuelles Passwort falsch\"}");
      return;
    }
    prefs.remove(KEY_OTA_PASS);
    logI("ota", "OTA-Passwort (NVS) geloescht");
    server.send(200, "application/json", "{\"ok\":true,\"restart\":true}");
    delay(DF3MT_OTA_PW_RESTART_MS);
    ESP.restart();
    return;
  }

  if (newPass.length() == 0) {
    server.send(400, "application/json",
                "{\"ok\":false,\"err\":\"Neues Passwort leer oder Löschen-Button nutzen\"}");
    return;
  }
  if (newPass.length() > DF3MT_OTA_PASSWORD_MAX_LEN) {
    server.send(400, "application/json",
                String("{\"ok\":false,\"err\":\"Passwort zu lang (max ") + String(DF3MT_OTA_PASSWORD_MAX_LEN) +
                ")\"}");
    return;
  }
  if (curStored.length() > 0) {
    String curIn = server.hasArg("current_password") ? server.arg("current_password") : "";
    if (curIn != curStored) {
      server.send(403, "application/json", "{\"ok\":false,\"err\":\"aktuelles Passwort falsch\"}");
      return;
    }
  }
  prefs.putString(KEY_OTA_PASS, newPass);
  logI("ota", "OTA-Passwort (NVS) gespeichert");
  server.send(200, "application/json", "{\"ok\":true,\"restart\":true}");
  delay(DF3MT_OTA_PW_RESTART_MS);
  ESP.restart();
}

static bool sOtaWebAuthOk = true;
static bool sOtaWebUpdateActive = false;
static bool sOtaWebBeginFailed = false;

void handleOtaUpdateUpload() {
  HTTPUpload &upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    sOtaWebBeginFailed = false;
    gOtaBusy = true;
    noteActivity();
    sOtaWebAuthOk = otaPasswordRequired() ? (server.header("X-OTA-Password") == otaPasswordEffective())
                                          : true;
    sOtaWebUpdateActive = false;
    if (!sOtaWebAuthOk) {
      logW("ota", "Web-Flash: Passwort fehlt/falsch");
      return;
    }
    if (upload.name != "firmware") {
      logW("ota", "Web-Flash: erwartet Form-Feld name=firmware");
      sOtaWebAuthOk = false;
      return;
    }
    motorStop();
    logI("ota", "Web-Flash start: %s", upload.filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      logE("ota", "Update.begin: %s", Update.errorString());
      sOtaWebBeginFailed = true;
      return;
    }
    sOtaWebUpdateActive = true;
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (!sOtaWebAuthOk || !sOtaWebUpdateActive) return;
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      logE("ota", "Update.write");
      Update.abort();
      sOtaWebUpdateActive = false;
    } else {
      yield();
    }
  }
}

void handleOtaUpdateDone() {
  server.sendHeader("Connection", "close");
  if (!sOtaWebAuthOk) {
    server.send(401, "application/json", "{\"ok\":false,\"err\":\"OTA-Passwort fehlt oder falsch\"}");
    sOtaWebAuthOk = true;
    sOtaWebUpdateActive = false;
    sOtaWebBeginFailed = false;
    return;
  }
  if (sOtaWebBeginFailed) {
    sOtaWebBeginFailed = false;
    Update.abort();
    server.send(500, "application/json", "{\"ok\":false,\"err\":\"Update.begin / Partition\"}");
    return;
  }
  if (!sOtaWebUpdateActive) {
    Update.abort();
    server.send(500, "application/json", "{\"ok\":false,\"err\":\"Update nicht gestartet\"}");
    return;
  }
  sOtaWebUpdateActive = false;
  if (Update.hasError()) {
    logE("ota", "Update: %s", Update.errorString());
    Update.abort();
    server.send(500, "application/json", "{\"ok\":false,\"err\":\"Schreibfehler\"}");
    return;
  }
  if (!Update.end(true)) {
    logE("ota", "Update.end: %s", Update.errorString());
    server.send(500, "application/json", "{\"ok\":false,\"err\":\"Update abschluss\"}");
    return;
  }
  logI("ota", "Web-Flash ok, Neustart");
  server.send(200, "application/json", "{\"ok\":true}");
  delay(DF3MT_OTA_WEB_DONE_DELAY_MS);
  ESP.restart();
}

/** Windows/Edge senden oft HEAD / — ohne eigenen Handler landet das in onNotFound (204) → „leere Seite“. */
static void handleRootHead() {
  server.sendHeader("Cache-Control", "no-store, no-cache");
  const size_t total = sizeof(kIndexHtml) - 1;
  server.setContentLength(total);
  server.send(200, "text/html; charset=utf-8", "");
  server.setContentLength(CONTENT_LENGTH_NOT_SET);
}

void handleRoot() {
  noteActivity();
  /* Große Seite nicht als String ins RAM kopieren (vermeidet Absturz → ERR_CONNECTION_RESET). */
  const size_t total = sizeof(kIndexHtml) - 1;
  server.setContentLength(total);
  server.send(200, "text/html; charset=utf-8", "");
  const size_t kChunk = DF3MT_HTTP_INDEX_CHUNK;
  for (size_t off = 0; off < total; off += kChunk) {
    size_t n = kChunk;
    if (off + n > total) n = total - off;
    server.sendContent_P(kIndexHtml + off, n);
    yield();
  }
  server.setContentLength(CONTENT_LENGTH_NOT_SET);
}

/** Nur ein Client (Browser-Session per Cookie/Header) darf gleichzeitig /api/motor nutzen. */
static String gMotorLockSession;

static bool motorSessionIdValid(const String &s) {
  if (s.length() < 16 || s.length() > 64) return false;
  for (size_t i = 0; i < s.length(); ++i) {
    const unsigned char c = static_cast<unsigned char>(s[i]);
    if (std::isxdigit(c) == 0) return false;
  }
  return true;
}

static String motorSessionFromCookieHeader(const String &cookie) {
  if (cookie.length() == 0) return "";
  static const char marker[] = "df3mt_motor_sid=";
  const int pos = cookie.indexOf(marker);
  if (pos < 0) return "";
  int start = pos + static_cast<int>(strlen(marker));
  while (start < static_cast<int>(cookie.length()) && cookie.charAt(start) == ' ') ++start;
  int end = start;
  while (end < static_cast<int>(cookie.length()) && cookie.charAt(end) != ';') ++end;
  String out = cookie.substring(start, end);
  out.trim();
  return out;
}

static String motorSessionFromRequest() {
  String s = server.header("X-DF3MT-Session");
  s.trim();
  if (motorSessionIdValid(s)) return s;
  s = server.header("X-Motor-Lock");
  s.trim();
  if (motorSessionIdValid(s)) return s;
  if (server.hasArg("session")) {
    s = server.arg("session");
    s.trim();
    if (motorSessionIdValid(s)) return s;
  }
  if (server.hasArg("token")) {
    s = server.arg("token");
    s.trim();
    if (motorSessionIdValid(s)) return s;
  }
  s = motorSessionFromCookieHeader(server.header("Cookie"));
  s.trim();
  if (motorSessionIdValid(s)) return s;
  return "";
}

/** Pro Session-ID: Drossel für nutzlose / teure Antworten (423/428 bzw. 409), ohne den Lock-Inhaber zu bremsen. */
static const size_t kGuestRateSlots = 6;
static char gRateMotorSid[kGuestRateSlots][33];
static uint32_t gRateMotorMs[kGuestRateSlots];
static char gRateClaimSid[kGuestRateSlots][33];
static uint32_t gRateClaimMs[kGuestRateSlots];

static bool motorGuestRateConsume(char (*slotSid)[33], uint32_t *slotMs, size_t n, const String &sid,
                                  uint32_t minGapMs) {
  if (sid.length() == 0 || sid.length() > 32) return true;
  const uint32_t now = millis();
  int emptyIdx = -1;
  for (size_t i = 0; i < n; ++i) {
    if (slotSid[i][0] == '\0') {
      if (emptyIdx < 0) emptyIdx = static_cast<int>(i);
      continue;
    }
    if (strncmp(slotSid[i], sid.c_str(), 32) == 0) {
      if ((uint32_t)(now - slotMs[i]) < minGapMs) return false;
      slotMs[i] = now;
      return true;
    }
  }
  const size_t slot = (emptyIdx >= 0) ? static_cast<size_t>(emptyIdx) : (now % n);
  strncpy(slotSid[slot], sid.c_str(), 32);
  slotSid[slot][32] = '\0';
  slotMs[slot] = now;
  return true;
}

void handleMotorClaim() {
  const String sid = motorSessionFromRequest();
  if (sid.length() == 0) {
    server.send(400, "application/json", "{\"ok\":false,\"err\":\"need_session\"}");
    return;
  }

  if (gMotorLockSession.length() > 0) {
    if (sid == gMotorLockSession) {
      logI("motor_lock", "reconnect (session)");
      noteActivity();
      server.send(200, "application/json", "{\"ok\":true}");
      return;
    }
    if (!motorGuestRateConsume(gRateClaimSid, gRateClaimMs, kGuestRateSlots, sid,
                               DF3MT_MOTOR_CLAIM_BUSY_COOLDOWN_MS)) {
      server.send(429, "application/json", "{\"ok\":false,\"err\":\"rate\"}");
      return;
    }
    server.send(409, "application/json", "{\"ok\":false,\"err\":\"busy\"}");
    return;
  }

  gMotorLockSession = sid;
  logI("motor_lock", "claim (session)");
  noteActivity();
  server.send(200, "application/json", "{\"ok\":true}");
}

void handleMotorRelease() {
  const String sid = motorSessionFromRequest();
  if (gMotorLockSession.length() == 0) {
    server.send(200, "application/json", "{\"ok\":true}");
    return;
  }
  if (sid.length() == 0 || sid != gMotorLockSession) {
    server.send(403, "application/json", "{\"ok\":false,\"err\":\"session\"}");
    return;
  }
  gMotorLockSession = "";
  motorSetSignedSynced(0);
  mqttPublishState();
  logI("motor_lock", "release");
  server.send(200, "application/json", "{\"ok\":true}");
}

void handleMotorKeepalive() {
  const String sid = motorSessionFromRequest();
  if (gMotorLockSession.length() == 0) {
    server.send(410, "application/json", "{\"ok\":false,\"err\":\"gone\"}");
    return;
  }
  if (sid.length() == 0 || sid != gMotorLockSession) {
    server.send(403, "application/json", "{\"ok\":false,\"err\":\"session\"}");
    return;
  }
  noteActivity();
  server.send(200, "application/json", "{\"ok\":true}");
}

void handleMotorApi() {
  const String sid = motorSessionFromRequest();
  if (gMotorLockSession.length() == 0) {
    if (sid.length() > 0 &&
        !motorGuestRateConsume(gRateMotorSid, gRateMotorMs, kGuestRateSlots, sid,
                                 DF3MT_MOTOR_API_GUEST_COOLDOWN_MS)) {
      server.send(429, "application/json", "{\"ok\":false,\"err\":\"rate\"}");
      return;
    }
    server.send(428, "application/json", "{\"ok\":false,\"err\":\"need_claim\"}");
    return;
  }
  if (sid.length() == 0 || sid != gMotorLockSession) {
    if (sid.length() > 0 &&
        !motorGuestRateConsume(gRateMotorSid, gRateMotorMs, kGuestRateSlots, sid,
                                 DF3MT_MOTOR_API_GUEST_COOLDOWN_MS)) {
      server.send(429, "application/json", "{\"ok\":false,\"err\":\"rate\"}");
      return;
    }
    server.send(423, "application/json", "{\"ok\":false,\"err\":\"lock\"}");
    return;
  }

  if (!server.hasArg("speed") || !server.hasArg("dir")) {
    logHttpClient("motor: fehlende Parameter speed/dir");
    server.send(400, "text/plain", "missing speed or dir");
    return;
  }
  int speed = server.arg("speed").toInt();
  int dir   = server.arg("dir").toInt();
  speed = constrain(speed, 0, DF3MT_MOTOR_PWM_MAX);

  if (dir == 0) {
    motorSetSignedSynced(0);
  } else if (dir > 0) {
    motorSetSignedSynced(speed);
  } else {
    motorSetSignedSynced(-speed);
  }
  noteActivity();
  mqttPublishState();
  server.send(200, "text/plain", "ok");
}

/** Letzter ausgegebener OTA-Prozentwert (in onStart zurücksetzen). */
static unsigned sOtaLogPct = 255;

static void setupArduinoOta() {
  ArduinoOTA.setHostname(DF3MT_OTA_HOSTNAME);
  ArduinoOTA.setPort(DF3MT_OTA_ARDUINO_PORT);
  {
    String p = otaPasswordNvsOrEmpty();
#if defined(DF3MT_OTA_PASSWORD)
    if (p.length() == 0) {
      ArduinoOTA.setPassword(DF3MT_OTA_PASSWORD);
    } else {
      ArduinoOTA.setPassword(p.c_str());
    }
#else
    if (p.length() > 0) {
      ArduinoOTA.setPassword(p.c_str());
    }
#endif
  }

  ArduinoOTA.onStart([]() {
    motorSetSignedSynced(0);
    gOtaBusy = true;
    noteActivity();
    sOtaLogPct = 255;
    const char *t =
        (ArduinoOTA.getCommand() == U_FLASH) ? "Firmware" : "Dateisystem";
    logI("ota", "Update start (%s)", t);
  });
  ArduinoOTA.onEnd([]() { logI("ota", "Update fertig — Neustart …"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    if (total == 0) return;
    unsigned pct = (progress * 100U) / total;
    if (progress == 0) {
      sOtaLogPct = 0;
      logI("ota", "Fortschritt 0 %%");
      return;
    }
    if (pct >= sOtaLogPct + 10U || pct == 100U) {
      sOtaLogPct = (pct / 10U) * 10U;
      logI("ota", "Fortschritt %u %%", pct);
    }
  });
  ArduinoOTA.onError([](ota_error_t e) {
    const char *msg = "?";
    switch (e) {
      case OTA_AUTH_ERROR: msg = "Auth"; break;
      case OTA_BEGIN_ERROR: msg = "Begin"; break;
      case OTA_CONNECT_ERROR: msg = "Connect"; break;
      case OTA_RECEIVE_ERROR: msg = "Receive"; break;
      case OTA_END_ERROR: msg = "End"; break;
      default: break;
    }
    logE("ota", "Fehler %s (%d)", msg, static_cast<int>(e));
  });

  ArduinoOTA.begin();
  logI("ota", "ArduinoOTA Port %u, Hostname %s.local (mDNS)", static_cast<unsigned>(DF3MT_OTA_ARDUINO_PORT),
       DF3MT_OTA_HOSTNAME);
  logI("ota", "IDE-Upload: Netzwerk-Port; Web-Flash: UI oder POST /api/ota/update");
  if (otaPasswordRequired()) {
    logI("ota", "OTA-Passwort aktiv (NVS oder Build-Fallback)");
  }
}

#if DF3MT_IDLE_SLEEP_ENABLE
static void noteActivity() {
  gLastActivityMs = millis();
  gSleepPollGotActivity = true;
}

static bool sleepPrerequisitesMet() {
  // Ohne Heim-WLAN + MQTT wäre SoftAP die einzige Erreichbarkeit — dann nicht schlafen.
  if (!mqttCfgEnabled()) return false;
  String ssid = prefs.getString(KEY_SSID, "");
  ssid.trim();
  if (ssid.length() == 0) return false;
  String host = prefs.getString(KEY_MQTT_HOST, "");
  host.trim();
  if (host.length() == 0) return false;
  return true;
}

static void sleepConfigureWakeSources() {
  esp_sleep_enable_timer_wakeup(static_cast<uint64_t>(DF3MT_SLEEP_POLL_MS) * 1000ULL);
#if DF3MT_SLEEP_WAKE_GPIO >= 0
  {
    const gpio_num_t pin = static_cast<gpio_num_t>(DF3MT_SLEEP_WAKE_GPIO);
    pinMode(DF3MT_SLEEP_WAKE_GPIO, INPUT_PULLUP);
    esp_sleep_enable_ext0_wakeup(pin, 0);  // aktiv low (Taster nach GND)
  }
#endif
}

static void enterDeepSleep(const char *reason) {
  logI("sleep", "Deep Sleep (%s) — Timer %u ms%s", reason ? reason : "idle",
       static_cast<unsigned>(DF3MT_SLEEP_POLL_MS),
#if DF3MT_SLEEP_WAKE_GPIO >= 0
       ", GPIO-Wake aktiv"
#else
       ""
#endif
  );
  motorStop();
  if (gMqttClient.connected()) {
    mqttPublishAvailability(false);
    gMqttClient.disconnect();
  }
  delay(30);
  WiFi.disconnect(true, false);
  WiFi.mode(WIFI_OFF);
  delay(20);
  sleepConfigureWakeSources();
  esp_deep_sleep_start();
}

/** Timer-Wake: kurz STA+MQTT, lauschen; true = Aktivität → voller Neustart. Schläft sonst erneut. */
static bool runMqttPollWakeCycle() {
  if (!sleepPrerequisitesMet()) {
    logI("sleep", "Poll-Wake: Sleep-Voraussetzungen fehlen — bleibe wach");
    return true;
  }

  gSleepPollGotActivity = false;
  pinMode(L298N_IN1, OUTPUT);
  pinMode(L298N_IN2, OUTPUT);
  if (!ledcAttach(L298N_ENA, PWM_FREQ, PWM_RES)) {
    logW("sleep", "ledcAttach fehlgeschlagen (Poll)");
  }
  motorStop();

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.setHostname(DF3MT_WIFI_STA_HOSTNAME);
  String ssid = prefs.getString(KEY_SSID, "");
  String pass = prefs.getString(KEY_PASS, "");
  logI("sleep", "Poll-Wake: verbinde STA \"%s\" …", ssid.c_str());
  WiFi.begin(ssid.c_str(), pass.c_str());

  const uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED &&
         (uint32_t)(millis() - t0) < DF3MT_SLEEP_STA_WAIT_MS) {
    delay(50);
    yield();
  }
  if (WiFi.status() != WL_CONNECTED) {
    logW("sleep", "Poll-Wake: STA Timeout — erneut schlafen");
    enterDeepSleep("sta-timeout");
  }

  gMqttLastReconnectMs = 0;
  if (!mqttReconnect()) {
    logW("sleep", "Poll-Wake: MQTT fehlgeschlagen — erneut schlafen");
    enterDeepSleep("mqtt-fail");
  }

  logI("sleep", "Poll-Wake: lausche %u ms auf MQTT …", static_cast<unsigned>(DF3MT_SLEEP_LISTEN_MS));
  const uint32_t listenStart = millis();
  while ((uint32_t)(millis() - listenStart) < DF3MT_SLEEP_LISTEN_MS) {
    gMqttClient.loop();
    if (gSleepPollGotActivity) {
      logI("sleep", "Poll-Wake: MQTT-Aktivitaet — voller Start");
      mqttPublishAvailability(false);
      gMqttClient.disconnect();
      WiFi.disconnect(true, false);
      WiFi.mode(WIFI_OFF);
      delay(50);
      return true;
    }
    delay(10);
    yield();
  }

  logI("sleep", "Poll-Wake: keine Aktivitaet — weiter schlafen");
  enterDeepSleep("poll-idle");
  return false;  // unreachable
}

static void maintainIdleSleep() {
  if (!sleepPrerequisitesMet()) return;
  if (gOtaBusy || sOtaWebUpdateActive) return;
  if (gMotorPwmSigned != 0) return;
  if (WiFi.softAPgetStationNum() > 0) return;
  if ((uint32_t)(millis() - gLastActivityMs) < DF3MT_IDLE_SLEEP_MS) return;
  enterDeepSleep("idle-30s");
}
#else
static void noteActivity() { gLastActivityMs = millis(); }
static void maintainIdleSleep() {}
static bool runMqttPollWakeCycle() { return true; }
#endif

void setup() {
  Serial.begin(DF3MT_SERIAL_BAUD);
  delay(DF3MT_BOOT_DELAY_MS);
  logI("boot", "DF3MT-Rotor startet (Serial %lu Baud)", static_cast<unsigned long>(DF3MT_SERIAL_BAUD));

  if (!prefs.begin(PREF_NS, false)) {
    logE("prefs", "begin(\"%s\") fehlgeschlagen — NVS/Flash pruefen", PREF_NS);
  }

#if DF3MT_IDLE_SLEEP_ENABLE
  {
    const esp_sleep_wakeup_cause_t wake = esp_sleep_get_wakeup_cause();
    if (wake == ESP_SLEEP_WAKEUP_TIMER) {
      logI("sleep", "Aufgewacht: Timer (MQTT-Poll)");
      if (runMqttPollWakeCycle()) {
        logI("sleep", "MQTT-Befehl empfangen — Neustart in den Vollbetrieb");
        ESP.restart();
      }
    } else if (wake == ESP_SLEEP_WAKEUP_EXT0) {
      logI("sleep", "Aufgewacht: GPIO-Taster");
    } else {
      logI("sleep", "Kaltstart / Reset (Idle-Sleep %u ms)", static_cast<unsigned>(DF3MT_IDLE_SLEEP_MS));
    }
  }
#endif

  gLastActivityMs = millis();
  gSleepPollGotActivity = false;

  pinMode(L298N_IN1, OUTPUT);
  pinMode(L298N_IN2, OUTPUT);
  if (!ledcAttach(L298N_ENA, PWM_FREQ, PWM_RES)) {
    logE("motor", "ledcAttach ENA GPIO %d fehlgeschlagen", L298N_ENA);
  }
  motorStop();

  /* Eigenes NVS für STA vermeidet Konflikte mit unseren Preferences-Zugangsdaten. */
  WiFi.persistent(false);
  WiFi.mode(WIFI_AP_STA);
  WiFi.setSleep(false);
  WiFi.setHostname(DF3MT_WIFI_STA_HOSTNAME);
  WiFi.onEvent(onWiFiEvent);
  /* Vor softAP: Gateway/Subnetz — integrierter DHCP-Server vergibt typisch 192.168.4.2 … */
  /* Optional 4./5. Parameter: DHCP-Pool-Start, DNS = AP (zusätzlich zu esp_netif_dhcps_option) */
  if (!WiFi.softAPConfig(kApIp, kApGw, kApSn,
                         IPAddress(DF3MT_AP_IP1, DF3MT_AP_IP2, DF3MT_AP_IP3, DF3MT_AP_DHCP_POOL4), kApIp)) {
    logE("wifi", "softAPConfig fehlgeschlagen");
  }
  /* Kanal 6: oft stabil; bei Problemen Router-Kanal prüfen (nur 2,4 GHz). */
  bool apOk = WiFi.softAP(AP_SSID, AP_PASS, DF3MT_SOFTAP_CHANNEL, 0, DF3MT_SOFTAP_MAX_CLIENTS);
  if (!apOk) {
    logE("wifi", "softAP(\"%s\") Start fehlgeschlagen", AP_SSID);
  } else {
    startSoftApDhcpServer();
    setDhcpDnsServerToAp();
    /* Opt. 114 (Captive-Portal-URL) wird in setDhcpDnsServerToAp auf http://<AP-IP>/ gesetzt (IDF ≥5.4.2). */
    startDnsForCaptivePortal();
    logI("wifi", "SoftAP bereit, DHCP z. B. 192.168.4.x");
    logW("hint", "Android: Private DNS AUS sonst keine Konnektivitaetstests ueber ESP");
  }

  WiFi.setAutoReconnect(true);
  WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);

  startStaFromPrefs();

  logI("wifi", "AP SSID=\"%s\" IP=%s", AP_SSID, WiFi.softAPIP().toString().c_str());
  logI("sta", "Heim-WLAN-Verbindung laeuft im Hintergrund (falls konfiguriert)");

  handleHttpConnectivityChecks();
  server.on("/favicon.ico", HTTP_GET, []() { server.send(204, "text/plain", ""); });
  server.on("/favicon.ico", HTTP_HEAD, []() { server.send(204, "text/plain", ""); });
  server.on("/", HTTP_HEAD, handleRootHead);
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/motor", HTTP_GET, handleMotorApi);
  server.on("/api/motor/claim", HTTP_POST, handleMotorClaim);
  server.on("/api/motor/release", HTTP_POST, handleMotorRelease);
  server.on("/api/motor/keepalive", HTTP_POST, handleMotorKeepalive);
  server.on("/api/wifi/status", HTTP_GET, handleWifiStatus);
  server.on("/api/wifi/save", HTTP_POST, handleWifiSave);
  server.on("/api/wifi/clear", HTTP_POST, handleWifiClear);
  server.on("/api/mqtt/status", HTTP_GET, handleMqttStatus);
  server.on("/api/mqtt/save", HTTP_POST, handleMqttSave);
  server.on("/api/mqtt/clear", HTTP_POST, handleMqttClear);
  server.on("/api/ota/status", HTTP_GET, handleOtaStatus);
  server.on("/api/ota/password", HTTP_POST, handleOtaPassword);
  server.on("/api/ota/update", HTTP_POST, handleOtaUpdateDone, handleOtaUpdateUpload);
  /* Weitere Konnektivitäts-Anfragen (Android nutzt oft HEAD). */
  server.onNotFound([]() {
    const HTTPMethod m = server.method();
    if (m != HTTP_GET && m != HTTP_HEAD) {
      logW("http", "404 Methode nicht unterstuetzt: %d uri=%s",
           static_cast<int>(m), server.uri().c_str());
      server.send(404, "text/plain", "Not Found");
      return;
    }
    const String u = server.uri();
    if (u.startsWith("/api/")) {
      logW("http", "404 API: %s", u.c_str());
      server.send(404, "application/json", "{\"err\":\"not found\"}");
      return;
    }
    /* OEM-/Varianten-Pfade (Host kommt per DNS auf uns) */
    if (u.indexOf("204") >= 0 || u.indexOf("gen_204") >= 0 || u.indexOf("captive") >= 0
        || u.indexOf("connectivity") >= 0 || u.endsWith("/status.txt")) {
      replyConnectivityProbe204();
      return;
    }
    replyConnectivityProbe204();
  });
  /* Sonst liefert server.header(...) für eigene Header leer (WebServer sammelt sie nicht von selbst). */
  static const char *kCollectHdr[] = {"X-DF3MT-Session", "X-Motor-Lock", "Cookie", "X-OTA-Password"};
  server.collectHeaders(kCollectHdr, sizeof(kCollectHdr) / sizeof(kCollectHdr[0]));
  server.begin();
  logI("http", "WebServer Port %u, Routen registriert", static_cast<unsigned>(DF3MT_HTTP_PORT));

  setupArduinoOta();
}

void loop() {
  dnsServer.processNextRequest();
  ArduinoOTA.handle();
  server.handleClient();
  maintainStaConnection();
  maintainMqtt();
  maintainIdleSleep();
  yield();
}
