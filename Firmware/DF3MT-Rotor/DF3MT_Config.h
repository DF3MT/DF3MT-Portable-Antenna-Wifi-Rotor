/**
 * DF3MT-Rotor — zentrale Konfiguration (Firmware + eingebettete Web-UI).
 */
#ifndef DF3MT_CONFIG_H
#define DF3MT_CONFIG_H

/* --- Stringify (für kIndexHtml.h) --- */
#define DF3MT_XSTR(a) DF3MT_XSTR_(a)
#define DF3MT_XSTR_(a) #a

/* --- Serial / Debug --- */
#define DF3MT_SERIAL_BAUD 115200
#define DF3MT_BOOT_DELAY_MS 200
#define DF3MT_ROTO_LOG_BUF 384

/* --- SoftAP / Netz --- */
#define DF3MT_AP_IP1 192
#define DF3MT_AP_IP2 168
#define DF3MT_AP_IP3 4
#define DF3MT_AP_IP4 1
#define DF3MT_AP_DHCP_POOL4 2
#define DF3MT_AP_SSID "DF3MT-Rotor"
#define DF3MT_AP_PASS "321qwe321"
#define DF3MT_SOFTAP_CHANNEL 6
#define DF3MT_SOFTAP_MAX_CLIENTS 4
/** DHCP-Clientname (STA) — Anzeige im Router bei Heim-WLAN; unabhängig von DF3MT_OTA_HOSTNAME (mDNS, siehe README). */
#define DF3MT_WIFI_STA_HOSTNAME "DF3MT-Rotor"
#define DF3MT_WIFI_STA_BEGIN_DELAY_MS 80
#define DF3MT_WIFI_CLEAR_DELAY_MS 100
#define DF3MT_DNS_TTL_S 60
#define DF3MT_DNS_PORT 53
#define DF3MT_STA_RECONNECT_PERIOD_MS 10000
#define DF3MT_STA_RECONNECT_AFTER_DISC_MS 1600
/** Nach WiFi.begin(): kein erneuter Versuch über maintain (vermeidet Abbruch laufender Assoziierung). */
#define DF3MT_STA_ASSOC_GRACE_MS 25000

/* --- HTTP --- */
#define DF3MT_HTTP_PORT 80
#define DF3MT_HTTP_INDEX_CHUNK 1024

/* --- L298N GPIO --- */
#define DF3MT_L298N_IN1 25
#define DF3MT_L298N_IN2 26
#define DF3MT_L298N_ENA 27

/* --- Motor / PWM (muss zu Web-UI / MQTT passen) --- */
#define DF3MT_PWM_FREQ 20000
#define DF3MT_PWM_RES_BITS 10
#define DF3MT_MOTOR_UI_START 150
#define DF3MT_MOTOR_PWM_MAX 255

/* --- NVS (Preferences) --- */
#define DF3MT_PREF_NS "roto"

/* --- MQTT --- */
#define DF3MT_MQTT_DEFAULT_PORT 1883
#define DF3MT_MQTT_BUFFER_SIZE 1024
#define DF3MT_MQTT_RECONNECT_MS 5000

/* --- OTA / mDNS (Arduino IDE „Netzwerk-Port“) ---
 * Kleinbuchstaben + Bindestrich: üblich für mDNS (.local). Unabhängig vom STA-DHCP-Namen
 * (DF3MT_WIFI_STA_HOSTNAME), den der Router bei Heim-WLAN anzeigt. */
#define DF3MT_OTA_HOSTNAME "df3mt-rotor"
#define DF3MT_OTA_ARDUINO_PORT 3232
#define DF3MT_OTA_WEB_DONE_DELAY_MS 300
#define DF3MT_OTA_PW_RESTART_MS 200
#define DF3MT_OTA_PASSWORD_MAX_LEN 48

/* --- Motor-Lock: Browser-Session (Cookie df3mt_motor_sid + Header X-DF3MT-Session) --- */

/** Mindestabstand für dieselbe Session-ID bei „falsche“ /api/motor-Anfragen (423/428), um Spam zu dämpfen. */
#define DF3MT_MOTOR_API_GUEST_COOLDOWN_MS 280
/** Mindestabstand bei POST /api/motor/claim mit 409 busy (andere Sitzung hält Lock). */
#define DF3MT_MOTOR_CLAIM_BUSY_COOLDOWN_MS 450

/*
 * Web-UI: Motor-Rampe (unabhängig von der Sperrzeit).
 * UI-Sperre für andere Nutzer: spätestens nach DF3MT_UI_HANDOFF_MAX_MS frei
 * (Rampe wird dann abgebrochen, Motor = 0, dann Lock-Release). 0 = nur bei PWM=0.
 */
#define DF3MT_UI_RAMP_UP_MS 22
#define DF3MT_UI_RAMP_UP_STEP 9
#define DF3MT_UI_RAMP_DOWN_MS 35
#define DF3MT_UI_RAMP_DOWN_STEP 6
/** Max. ms nach Loslassen der Tasten bis Lock für andere frei (Motor-Stopp + release). */
#define DF3MT_UI_HANDOFF_MAX_MS 1000
#define DF3MT_UI_MOTOR_TX_POLL_MS 25
#define DF3MT_UI_MOTOR_LOCK_RETRY_MS 1000
/** 0 = kein periodischer WiFi-Status (SSID/Passwort werden sonst regelmäßig überschrieben). */
#define DF3MT_UI_REFRESH_WIFI_MS 0
#define DF3MT_UI_REFRESH_OTA_MS 8000
#define DF3MT_UI_REFRESH_MQTT_MS 10000
#define DF3MT_UI_WIFI_SAVE_DELAY_1_MS 400
#define DF3MT_UI_WIFI_SAVE_DELAY_2_MS 2500
#define DF3MT_UI_WIFI_SAVE_DELAY_3_MS 6000
#define DF3MT_UI_MQTT_SAVE_DELAY_1_MS 400
#define DF3MT_UI_MQTT_SAVE_DELAY_2_MS 3000

/* Komma-getrennt für JS: Reihenfolge muss zu kIndexHtml.h passen */
#define DF3MT_UI_NUMPACK_STR                                                                 \
  DF3MT_XSTR(DF3MT_MOTOR_PWM_MAX) "," DF3MT_XSTR(DF3MT_UI_RAMP_UP_MS) ","                    \
      DF3MT_XSTR(DF3MT_UI_RAMP_UP_STEP) "," DF3MT_XSTR(DF3MT_UI_RAMP_DOWN_MS) ","            \
      DF3MT_XSTR(DF3MT_UI_RAMP_DOWN_STEP) "," DF3MT_XSTR(DF3MT_UI_MOTOR_TX_POLL_MS) ","     \
      DF3MT_XSTR(DF3MT_UI_MOTOR_LOCK_RETRY_MS) "," DF3MT_XSTR(DF3MT_UI_REFRESH_WIFI_MS) "," \
      DF3MT_XSTR(DF3MT_UI_REFRESH_OTA_MS) "," DF3MT_XSTR(DF3MT_UI_REFRESH_MQTT_MS) ","     \
      DF3MT_XSTR(DF3MT_UI_WIFI_SAVE_DELAY_1_MS) "," DF3MT_XSTR(DF3MT_UI_WIFI_SAVE_DELAY_2_MS) "," \
      DF3MT_XSTR(DF3MT_UI_WIFI_SAVE_DELAY_3_MS) "," DF3MT_XSTR(DF3MT_UI_MQTT_SAVE_DELAY_1_MS) "," \
      DF3MT_XSTR(DF3MT_UI_MQTT_SAVE_DELAY_2_MS) "," DF3MT_XSTR(DF3MT_UI_HANDOFF_MAX_MS)

#endif /* DF3MT_CONFIG_H */
