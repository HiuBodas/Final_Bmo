// =============================================================================
// WifiManager.cpp - Implementasi Modul Jaringan Wi-Fi & Background Task FreeRTOS
// Bagian dari Roadmap BMO ESP32-C3 (Seksi 1.1, 1.2, 1.3)
// 1.1 Penyimpanan NVS Flash ("bmo_wifi")
// 1.2 FreeRTOS Background Task (Stack 4096B, Priority tskIDLE_PRIORITY + 1)
// 1.3 Ikon Vektor Prosedural Status Wi-Fi 3-Lengkung pada Header OLED
// =============================================================================
#include "WifiManager.h"
#include "Display.h"
#include "BleManager.h"
#include "NtpClock.h"
#include "WeatherManager.h"
#include "WebDashboard.h"

#include <WiFi.h>
#include <Preferences.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Namespace NVS Preferences
static const char* NVS_NAMESPACE = "bmo_wifi";

// Variabel Konfigurasi Tersimpan
static String s_ssid        = "";
static String s_pass        = "";
static String s_url         = "";
static bool   s_autoConnect = true;

// Variabel Kontrol Thread-Safe FreeRTOS
static TaskHandle_t           s_wifiTaskHandle       = nullptr;
static volatile WifiState     s_wifiState            = WIFI_STATE_OFF;
static volatile bool          s_connectRequested     = false;
static volatile bool          s_disconnectRequested  = false;
static portMUX_TYPE           s_wifiMux              = portMUX_INITIALIZER_UNLOCKED;

// -----------------------------------------------------------------------------
// FreeRTOS Background Task (Seksi 1.2)
// Berjalan di prioritas rendah (tskIDLE_PRIORITY + 1) dengan stack 4096B
// Menjamin main loop OLED & sensor sentuh bebas lag (35-40 FPS)
// -----------------------------------------------------------------------------
static void wifiBackgroundTask(void* pvParameters) {
  uint32_t lastReconnectAttemptMs = 0;

  for (;;) {
    // 1. Tangani Permintaan Disconnect
    if (s_disconnectRequested) {
      taskENTER_CRITICAL(&s_wifiMux);
      s_disconnectRequested = false;
      taskEXIT_CRITICAL(&s_wifiMux);

      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
      s_wifiState = WIFI_STATE_OFF;
      webDashboardStop();
      Serial.println(F("[WiFi] Disconnected & Radio OFF."));
    }

    // 2. Tangani Permintaan Connect Baru
    if (s_connectRequested) {
      taskENTER_CRITICAL(&s_wifiMux);
      s_connectRequested = false;
      taskEXIT_CRITICAL(&s_wifiMux);

      if (s_ssid.length() > 0) {
        s_wifiState = WIFI_STATE_CONNECTING;
        Serial.printf("[WiFi] Memulai koneksi ke: \"%s\" ...\n", s_ssid.c_str());

        WiFi.mode(WIFI_STA);
        WiFi.begin(s_ssid.c_str(), s_pass.c_str());

        uint32_t startMs = millis();
        bool connected = false;

        // Tunggu hingga tersambung atau timeout 15 detik secara non-blocking
        while (millis() - startMs < 15000) {
          if (WiFi.status() == WL_CONNECTED) {
            connected = true;
            break;
          }
          if (s_disconnectRequested) break;
          vTaskDelay(pdMS_TO_TICKS(150));
        }

        if (connected) {
          s_wifiState = WIFI_STATE_CONNECTED;
          String ipStr = WiFi.localIP().toString();
          Serial.printf("[WiFi] Terhubung! IP Address: %s\n", ipStr.c_str());
          bleSend("[WIFI] CONNECTED:" + ipStr);

          // Seksi 2.1: Sinkronisasi Jam Global (NTP)
          ntpStartSync();

          // Seksi 2.2: Ambil Data Cuaca & Suhu HUD
          weatherUpdate();

          // Seksi 2.3: Mulai Web Dashboard Lokal (http://bmo.local)
          webDashboardStart();
        } else {
          s_wifiState = WIFI_STATE_FAILED;
          Serial.printf("[WiFi] Gagal terhubung ke \"%s\" (Timeout 15s)\n", s_ssid.c_str());
          bleSend("[WIFI] FAILED");
          lastReconnectAttemptMs = millis();
        }
      }
    }

    // 3. Pantau Status Koneksi jika Sedang Terhubung
    if (s_wifiState == WIFI_STATE_CONNECTED) {
      if (WiFi.status() != WL_CONNECTED) {
        s_wifiState = WIFI_STATE_DISCONNECTED;
        webDashboardStop();
        Serial.println(F("[WiFi] Koneksi terputus dari AP!"));
        bleSend("[WIFI] DISCONNECTED");
        lastReconnectAttemptMs = millis();
      } else {
        // Pembaruan berkala data cuaca secara non-blocking di background task
        weatherUpdate();
      }
    }
    // 4. Mekanisme Reconnect Otomatis jika Terputus / Gagal (Setiap 30 Detik)
    else if ((s_wifiState == WIFI_STATE_DISCONNECTED || s_wifiState == WIFI_STATE_FAILED) && s_autoConnect && s_ssid.length() > 0) {
      if (millis() - lastReconnectAttemptMs >= 30000) {
        Serial.println(F("[WiFi] Mencoba menyambung ulang otomatis..."));
        lastReconnectAttemptMs = millis();
        s_connectRequested = true;
      }
    }

    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

// -----------------------------------------------------------------------------
// Inisialisasi Modul Wi-Fi & NVS Flash (Seksi 1.1 & 1.2)
// -----------------------------------------------------------------------------
void wifiInit() {
  Preferences prefs;
  prefs.begin(NVS_NAMESPACE, true); // Read-only mode
  s_ssid        = prefs.getString("ssid", "");
  s_pass        = prefs.getString("pass", "");
  s_url         = prefs.getString("url", "");
  s_autoConnect = prefs.getBool("auto_connect", true);
  prefs.end();

  Serial.printf("[WiFi] Inisialisasi NVS: SSID='%s', AutoConnect=%s\n",
                s_ssid.length() > 0 ? s_ssid.c_str() : "(Kosong)",
                s_autoConnect ? "AKTIF" : "NONAKTIF");

  if (s_autoConnect && s_ssid.length() > 0) {
    s_wifiState = WIFI_STATE_CONNECTING;
    s_connectRequested = true;
  } else {
    s_wifiState = WIFI_STATE_OFF;
  }

  // Buat background FreeRTOS task khusus untuk operasi jaringan
  if (s_wifiTaskHandle == nullptr) {
    xTaskCreate(
      wifiBackgroundTask,
      "bmo_wifi",
      4096,
      nullptr,
      tskIDLE_PRIORITY + 1,
      &s_wifiTaskHandle
    );
    Serial.println(F("[WiFi] FreeRTOS Background Task aktif (Stack 4096B, Priority Idle+1)."));
  }
}

// -----------------------------------------------------------------------------
// Simpan Kredensial Permanen ke NVS Flash (Seksi 1.1)
// -----------------------------------------------------------------------------
void wifiSaveCredentials(const String& ssid, const String& pass, const String& url) {
  Preferences prefs;
  prefs.begin(NVS_NAMESPACE, false); // Read-write mode
  prefs.putString("ssid", ssid.substring(0, 32));
  prefs.putString("pass", pass.substring(0, 64));
  if (url.length() > 0) {
    prefs.putString("url", url.substring(0, 128));
  }
  prefs.putBool("auto_connect", true);
  prefs.end();

  s_ssid        = ssid.substring(0, 32);
  s_pass        = pass.substring(0, 64);
  if (url.length() > 0) s_url = url.substring(0, 128);
  s_autoConnect = true;

  Serial.printf("[WiFi] Kredensial baru disimpan ke NVS Flash: SSID='%s'\n", s_ssid.c_str());

  // Picu koneksi seketika melalui FreeRTOS Task
  taskENTER_CRITICAL(&s_wifiMux);
  s_connectRequested = true;
  taskEXIT_CRITICAL(&s_wifiMux);
}

void wifiConnect() {
  if (s_ssid.length() > 0) {
    taskENTER_CRITICAL(&s_wifiMux);
    s_connectRequested = true;
    taskEXIT_CRITICAL(&s_wifiMux);
  }
}

void wifiDisconnect() {
  taskENTER_CRITICAL(&s_wifiMux);
  s_disconnectRequested = true;
  taskEXIT_CRITICAL(&s_wifiMux);
}

// -----------------------------------------------------------------------------
// Getters & Status Wi-Fi
// -----------------------------------------------------------------------------
WifiState wifiGetState() {
  return s_wifiState;
}

bool wifiIsConnected() {
  return (s_wifiState == WIFI_STATE_CONNECTED);
}

String wifiGetSSID() {
  return s_ssid;
}

String wifiGetIP() {
  if (s_wifiState == WIFI_STATE_CONNECTED) {
    return WiFi.localIP().toString();
  }
  return "0.0.0.0";
}

int8_t wifiGetRSSI() {
  if (s_wifiState == WIFI_STATE_CONNECTED) {
    return WiFi.RSSI();
  }
  return -100;
}

bool wifiGetAutoConnect() {
  return s_autoConnect;
}

void wifiSetAutoConnect(bool enable) {
  s_autoConnect = enable;
  Preferences prefs;
  prefs.begin(NVS_NAMESPACE, false);
  prefs.putBool("auto_connect", enable);
  prefs.end();
}

// -----------------------------------------------------------------------------
// Balas Status ke Smartphone via BLE
// -----------------------------------------------------------------------------
void wifiSendBleStatus() {
  String resp = "[WIFI] STATUS:";
  switch (s_wifiState) {
    case WIFI_STATE_CONNECTED:
      resp += "CONNECTED|" + WiFi.localIP().toString();
      break;
    case WIFI_STATE_CONNECTING:
      resp += "CONNECTING|" + s_ssid;
      break;
    case WIFI_STATE_FAILED:
      resp += "FAILED|" + s_ssid;
      break;
    case WIFI_STATE_DISCONNECTED:
      resp += "DISCONNECTED";
      break;
    case WIFI_STATE_OFF:
    default:
      resp += "OFF";
      break;
  }
  bleSend(resp);
  Serial.printf("[WiFi BLE TX] %s\n", resp.c_str());
}

// -----------------------------------------------------------------------------
// Gambar Ikon Header OLED (Seksi 1.3)
// -----------------------------------------------------------------------------
void wifiDrawHeaderIcon(int x, int y, uint8_t color) {
  fbDrawWifiIcon(x, y, (uint8_t)s_wifiState, color, millis());
}
