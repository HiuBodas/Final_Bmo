// =============================================================================
// BMO ESP32-C3: Standalone Expressions & Touch Reactions
// Display: Layar OLED 0.96" I2C SSD1306 (128x64) - Background Putih, Garis Hitam
// =============================================================================

#include <Arduino.h>
#include "Config.h"
#include "Display.h"
#include "Touch.h"
#include "Moods.h"
#include "Menu.h"
#include "BleManager.h"
#include "WifiManager.h"
#include "NtpClock.h"
#include "WeatherManager.h"
#include "SpotifyWidget.h"
#include "WebDashboard.h"
#include "DoodleWidget.h"

// -----------------------------------------------------------------------------
// Pilihan Mode Idle Wajah BMO (saat tidak di Menu & tidak Terkunci):
// 0 = Siklus Asli 26 Mood Rutin (Mode Awal / Original Standalone BMO)
// 1 = Personality Engine Dinamis (Gaya Dasai Mochi: nafas santai & rantai mood)
// -----------------------------------------------------------------------------
#define ENABLE_MOCHI_ENGINE 0

void setup() {
#if ARDUINO_USB_CDC_ON_BOOT
  Serial.begin(115200);
#else
  Serial.begin(115200);
#endif
  delay(100);

  Serial.println("\n=================================");
  Serial.println("  BMO ESP32-C3 Standalone System  ");
  Serial.println("  Display: OLED SSD1306 (128x64)  ");
  Serial.println("  Graphics: Procedural Vector     ");
  Serial.println("=================================");

  // 1. Inisialisasi Dual Sensor Sentuh: Sensor 1 (Wajah/Aksi) & Sensor 2 (Navigasi)
  touchInit();
  Serial.println("[init] Dual touch sensors ready.");

  // 2. Inisialisasi Layar OLED 0.96\" SSD1306 via I2C
  displayInit();
  Serial.println("[init] Display SSD1306 ready.");

  // 3. Inisialisasi Sistem Menu Vektor Prosedural & BLE Stack
  // BLE tidak dinyalakan otomatis; nyalakan/matikan manual lewat menu BLUETOOTH.
  menuInit();
  Serial.println("[init] Menu system ready (BLE OFF, nyalakan manual via menu).");

  // 4. Inisialisasi Modul Jaringan & Layanan Produktivitas (Seksi 1 & 2)
  ntpInit();
  weatherInit();
  spotifyInit();
  webDashboardInit();
  doodleInit();
  wifiInit();
  Serial.println("[init] Wi-Fi manager & Network Services initialized.");

  randomSeed(analogRead(0) + micros());
  if (menuIsMochiEngineEnabled()) {
    Serial.println("[init] Personality Engine active.");
  } else {
    Serial.println("[init] Routine 26 Mood Cycle active.");
  }
}

void loop() {
  // 0. Prioritas Layanan Web Dashboard Lokal (http://bmo.local)
  webDashboardHandle();

  // 1. Interupsi Layar Doodle / Kanvas Gambar
  if (doodleIsActive()) {
    doodleUpdate();
    return;
  }

  // 2. Interupsi Layar Widget Spotify Now Playing
  if (spotifyIsActive()) {
    spotifyUpdate();
    return;
  }

  // 2. Prioritas Layar Notifikasi BLE Real-time
  if (bleNotifIsActive()) {
    bleNotifUpdate();
    return;
  }

  // 2. Prioritas Kedua: Eksekusi Trigger Ekspresi / Sentuhan dari APK (Thread-safe)
  Mood bleM;
  if (bleHasPendingMood(bleM)) {
    playMood(bleM, 2500);
    return;
  }
  TouchKind bleK;
  if (bleHasPendingReaction(bleK)) {
    playReaction(bleK);
    return;
  }

  // 3. Cek jika sensor navigasi ditekan saat di luar menu
  if (g_navPending) {
    NavEvent ev = g_pendingNav;
    g_navPending = false;

    if (ev == NAV_TAP) {
      menuOpen();
    } else if (ev == NAV_HOLD && menuIsLocked()) {
      menuUnlock(); // Buka kunci jika ditahan saat terkunci
    }
  }

  // 3. Jika Menu sedang aktif, jalankan navigasi live menu
  if (menuIsActive()) {
    menuUpdate();
    return;
  }

  // 4. Tangani jika ada respon sentuh fisik pada sensor wajah
  if (g_touchPending) {
    g_touchPending = false;
    playReaction(g_pendingKind);
    while (g_touchPending) {
      g_touchPending = false;
      playReaction(g_pendingKind);
    }
  }

  // 5. Jika pengguna telah mengunci/memilih ekspresi tertentu dari menu
  if (menuIsLocked()) {
    playMood(menuGetLockedMood(), 2500);
    return;
  }

  // 6. Siklus Otomatis Normal saat tidak di menu dan tidak dikunci
  if (menuIsMochiEngineEnabled()) {
    // Mode Personality Engine Dinamis (Dasai Mochi Style)
    bmoStepMochiPersonality();
  } else {
    // Mode Original: Siklus 26 Mood Rutin & Interupsi Sensor
    struct Scene {
      Mood m;
      uint32_t ms;
    };

    static const Scene script[] = {
      { MOOD_COOL,     3600 },
      { MOOD_HAPPY,    2000 },
      { MOOD_LAUGH,    1800 },
      { MOOD_LOVE,     2200 },
      { MOOD_WINK,     1400 },
      { MOOD_IDLE,     2400 },
      { MOOD_BLINK,     300 },
      { MOOD_TALK,     1800 },
      { MOOD_LISTEN,   1600 },
      { MOOD_FOCUSED,  1800 },
      { MOOD_THINKING, 2400 },
      { MOOD_SURPRISE, 1200 },
      { MOOD_EXCITED,  1800 },
      { MOOD_HUNGRY,   1800 },
      { MOOD_COLD,     2000 },
      { MOOD_HOT,      2000 },
      { MOOD_SICK,     2000 },
      { MOOD_DIZZY,    1800 },
      { MOOD_GLITCH,    900 },
      { MOOD_CONFUSED, 2000 },
      { MOOD_BORED,    2000 },
      { MOOD_SAD,      2000 },
      { MOOD_SCARED,   1800 },
      { MOOD_ANGRY,    1600 },
      { MOOD_WAKE,     1800 },
      { MOOD_SLEEPY,   2200 },
    };

    for (const auto &sc : script) {
      if (g_navPending || menuIsActive() || menuIsLocked() || bleNotifIsActive() || bleHasPendingAction() || doodleIsActive() || spotifyIsActive()) break;

      // Periksa jika ada sentuhan sebelum mood berikutnya
      if (g_touchPending) {
        g_touchPending = false;
        playReaction(g_pendingKind);

        while (g_touchPending) {
          g_touchPending = false;
          playReaction(g_pendingKind);
        }
      }

      // Jalankan animasi mood (dapat diinterupsi seketika oleh sensor)
      playMood(sc.m, sc.ms);
    }
  }
}

