// =============================================================================
// DoodleWidget.cpp - Implementasi Live Drawing & Canvas Display OLED 128x64
// Menerima input gambar langsung dari aplikasi HP dan menampilkannya di BMO
// =============================================================================
#include "DoodleWidget.h"
#include "Config.h"
#include "Touch.h"
#include <Wire.h>

static bool     s_doodleActive   = false;
static bool     s_invert         = false;
static uint32_t s_lastActivityMs = 0;
static uint8_t  s_doodleFb[OLED_W * OLED_H / 8]; // 1024 byte monokrom

static void doodlePutPx(int x, int y, uint8_t color) {
  if ((unsigned)x >= (unsigned)OLED_W || (unsigned)y >= (unsigned)OLED_H) return;
  int idx = x + (y >> 3) * OLED_W;
  int bit = y & 7;
  if (color) {
    s_doodleFb[idx] |= (1 << bit);
  } else {
    s_doodleFb[idx] &= ~(1 << bit);
  }
}

void doodleInit() {
  s_doodleActive   = false;
  s_invert         = false;
  s_lastActivityMs = 0;
  memset(s_doodleFb, 0xFF, sizeof(s_doodleFb)); // Background putih default
}

bool doodleIsActive() {
  return s_doodleActive;
}

void doodleOpen() {
  s_doodleActive   = true;
  s_lastActivityMs = millis();
  g_touchPending   = false;
  g_navPending     = false;
}

void doodleClose() {
  s_doodleActive = false;
}

void doodleClear(bool whiteBackground) {
  memset(s_doodleFb, whiteBackground ? 0xFF : 0x00, sizeof(s_doodleFb));
  s_lastActivityMs = millis();
}

void doodleDrawLine(int x0, int y0, int x1, int y1, uint8_t color) {
  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = dx + dy, e2;
  while (true) {
    doodlePutPx(x0, y0, color);
    if (x0 == x1 && y0 == y1) break;
    e2 = 2 * err;
    if (e2 >= dy) { err += dy; x0 += sx; }
    if (e2 <= dx) { err += dx; y0 += sy; }
  }
  s_lastActivityMs = millis();
}

void doodleSetPage(int pageIdx, const uint8_t* data, int len) {
  if (pageIdx < 0 || pageIdx >= 8 || data == nullptr || len <= 0) return;
  int offset = pageIdx * 128;
  int copyLen = (offset + len > (int)sizeof(s_doodleFb)) ? (sizeof(s_doodleFb) - offset) : len;
  memcpy(&s_doodleFb[offset], data, copyLen);
  s_lastActivityMs = millis();
}

void doodleSetInvert(bool inv) {
  s_invert = inv;
  s_lastActivityMs = millis();
}

void doodleUpdate() {
  uint32_t now = millis();
  pollTouch();

  // Sensor 2: Navigasi / Keluar dari mode gambar
  if (g_navPending) {
    NavEvent ev = g_pendingNav;
    g_navPending = false;
    s_lastActivityMs = now;
    if (ev == NAV_TAP || ev == NAV_HOLD || ev == NAV_DOUBLE_TAP) {
      doodleClose();
      return;
    }
  }

  // Sensor 1: Interaksi Sentuh (Invert Tampilan / Hapus)
  if (g_touchPending) {
    g_touchPending = false;
    s_lastActivityMs = now;
    s_invert = !s_invert;
  }

  // Timeout inaktivitas 120 detik (kembali otomatis ke ekspresi wajah)
  if (now - s_lastActivityMs > 120000) {
    doodleClose();
    return;
  }

  // Flush buffer gambar doodle ke layar OLED SSD1306
  Wire.beginTransmission(OLED_ADDR);
  Wire.write(0x00);
  Wire.write(0x21); // Set Column Address: 0 .. 127
  Wire.write(0);
  Wire.write(OLED_W - 1);
  Wire.write(0x22); // Set Page Address: 0 .. 7
  Wire.write(0);
  Wire.write((OLED_H / 8) - 1);
  Wire.endTransmission();

  // Kirim data buffer dalam blok 32-byte (batas buffer Wire Arduino)
  for (size_t i = 0; i < sizeof(s_doodleFb); i += 32) {
    Wire.beginTransmission(OLED_ADDR);
    Wire.write(0x40); // Mode data piksel
    if (s_invert) {
      uint8_t invertedChunk[32];
      for (int k = 0; k < 32; k++) {
        invertedChunk[k] = ~s_doodleFb[i + k];
      }
      Wire.write(invertedChunk, 32);
    } else {
      Wire.write(&s_doodleFb[i], 32);
    }
    Wire.endTransmission();
  }

  delay(20);
}
