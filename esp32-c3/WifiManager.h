// =============================================================================
// WifiManager.h - Modul Fondasi Jaringan Wi-Fi & Arsitektur Non-Blocking ESP32-C3
// Bagian dari Roadmap BMO ESP32 (Seksi 1.1, 1.2, 1.3)
// - Penyimpanan NVS Flash ("bmo_wifi")
// - FreeRTOS Background Task (Stack 4096B, Priority tskIDLE_PRIORITY + 1)
// - Ikon Vektor Prosedural Status Wi-Fi 3-Lengkung (Header OLED)
// =============================================================================
#pragma once
#include <Arduino.h>

// Status Koneksi Wi-Fi
enum WifiState : uint8_t {
  WIFI_STATE_OFF = 0,          // Wi-Fi dinonaktifkan / belum dikonfigurasi
  WIFI_STATE_DISCONNECTED,     // Terputus dari Access Point
  WIFI_STATE_CONNECTING,       // Sedang menghubungkan (Animasi Kedip / Blinking)
  WIFI_STATE_CONNECTED,        // Berhasil terhubung (Solid 3 Lengkung)
  WIFI_STATE_FAILED            // Gagal terhubung / Timeout (Silang / Cross)
};

// Inisialisasi & Pengaturan
void wifiInit();
void wifiSaveCredentials(const String& ssid, const String& pass, const String& url = "");
void wifiConnect();
void wifiDisconnect();

// Status & Getter
WifiState wifiGetState();
bool wifiIsConnected();
String wifiGetSSID();
String wifiGetIP();
int8_t wifiGetRSSI();
bool wifiGetAutoConnect();
void wifiSetAutoConnect(bool enable);

// Balasan Status ke Smartphone via BLE
void wifiSendBleStatus();

// Penggambar Ikon Header OLED
void wifiDrawHeaderIcon(int x = 116, int y = 2, uint8_t color = 1);
