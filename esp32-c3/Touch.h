// =============================================================================
// Touch.h - Driver Sensor Sentuh Kapasitif & Klasifikasi Gestur
// =============================================================================
#pragma once
#include <Arduino.h>
#include "Config.h"

enum TouchKind : int {
  TOUCH_NONE,
  TOUCH_QUICK_POKE, // Ketukan singkat (< 250ms)
  TOUCH_HOLD,       // Elusan sedang (250ms - 1500ms)
  TOUCH_LONG_HOLD,  // Pelukan lama (> 1500ms)
  TOUCH_TICKLE,     // Digelitik (3+ ketukan cepat berturut-turut)
};

// Event Sensor Navigasi
enum NavEvent : int {
  NAV_NONE,
  NAV_TAP,          // Ketuk 1x: Buka menu / Geser pilihan
  NAV_HOLD,         // Tahan (> 800ms): Tutup menu / Batal / Kembali
  NAV_DOUBLE_TAP    // Ketuk 2x cepat: Tutup menu
};

// Status interupsi Sensor 1 (Aksi / Wajah & Konfirmasi OK)
extern volatile bool      g_touchPending;
extern volatile TouchKind g_pendingKind;

// Status interupsi Sensor 2 (Navigasi Menu)
extern volatile bool      g_navPending;
extern volatile NavEvent  g_pendingNav;

void touchInit();
void pollTouch();
bool touchIsDown();       // Status fisik Sensor 1
bool touchNavIsDown();    // Status fisik Sensor 2
const char* touchKindName(TouchKind k);
const char* navEventName(NavEvent e);

