// =============================================================================
// Config.h - Konfigurasi Pin Hardware & Parameter Layar OLED 0.96"
// =============================================================================
#pragma once
#include <Arduino.h>

// -----------------------------------------------------------------------------
// Pin Hardware (ESP32-C3 Super Mini)
// -----------------------------------------------------------------------------

// Layar OLED 0.96" (I2C SSD1306 128x64)
// Pin I2C default ESP32-C3: GPIO 8 (SDA), GPIO 9 (SCL)
static constexpr int PIN_OLED_SDA  = 8;
static constexpr int PIN_OLED_SCL  = 9;
static constexpr uint8_t OLED_ADDR = 0x3C;  // Alamat I2C umum (0x3C atau 0x3D)

// Sensor Sentuh Kapasitif TTP223 pada GPIO 20 (Wajah & Konfirmasi OK)
static constexpr int PIN_TOUCH     = 20;

// Sensor Sentuh Tambahan TTP223 pada GPIO 21 (Navigasi Menu Pindah Ekspresi)
static constexpr int PIN_TOUCH_NAV = 21;

// -----------------------------------------------------------------------------
// Dimensi Layar & Koordinat Wajah BMO (128x64 Monokrom)
// -----------------------------------------------------------------------------
static constexpr int OLED_W        = 128;
static constexpr int OLED_H        = 64;

// Warna Tema: Background Putih, Garis/Fitur Hitam
static constexpr uint8_t C_WHITE   = 1;
static constexpr uint8_t C_BLACK   = 0;
static constexpr uint8_t C_BG      = C_WHITE; // Background Putih
static constexpr uint8_t C_INK     = C_BLACK; // Garis & Fitur Hitam

// Titik Acuan Fitur Wajah pada Layar 128x64
static constexpr int EYE_Y         = 22;   // Posisi vertikal mata
static constexpr int EYE_DX        = 28;   // Jarak mata dari titik tengah
static constexpr int EYE_W         = 6;    // Lebar mata default
static constexpr int EYE_H         = 7;    // Tinggi mata default

static constexpr int MOUTH_CX      = 64;   // Titik tengah horizontal mulut
static constexpr int MOUTH_Y       = 44;   // Posisi vertikal mulut
