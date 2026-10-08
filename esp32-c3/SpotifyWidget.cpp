// =============================================================================
// SpotifyWidget.cpp - Implementasi Widget "Now Playing" Spotify BMO
// Bagian dari Roadmap BMO ESP32 (Seksi 2.4)
// Teks judul lagu berjalan (Marquee) + 5-Bar Equalizer Audio Animasi
// =============================================================================
#include "SpotifyWidget.h"
#include "Display.h"
#include "Touch.h"
#include "WifiManager.h"
#include <math.h>

static bool     s_spotifyActive  = false;
static String   s_trackTitle     = "Theme Song";
static String   s_trackArtist    = "Adventure Time";
static bool     s_isPlaying      = true;
static uint32_t s_lastUpdateMs   = 0;
static uint32_t s_lastActivityMs = 0;

void spotifyInit() {
  s_spotifyActive  = false;
  s_trackTitle     = "Theme Song";
  s_trackArtist    = "Adventure Time";
  s_isPlaying      = true;
  s_lastUpdateMs   = millis();
}

void spotifySetTrack(const String& title, const String& artist) {
  s_trackTitle  = title;
  s_trackArtist = artist;
  s_trackTitle.trim();
  s_trackArtist.trim();
  s_isPlaying    = true;
  s_lastUpdateMs = millis();
}

String spotifyGetTitle() {
  return s_trackTitle;
}

String spotifyGetArtist() {
  return s_trackArtist;
}

bool spotifyIsActive() {
  return s_spotifyActive;
}

void spotifyOpen() {
  s_spotifyActive  = true;
  s_lastActivityMs = millis();
  g_touchPending   = false;
  g_navPending     = false;
}

void spotifyClose() {
  s_spotifyActive = false;
}

void spotifyUpdate() {
  uint32_t now = millis();
  pollTouch();

  // Sensor 2: Navigasi / Keluar
  if (g_navPending) {
    NavEvent ev = g_pendingNav;
    g_navPending = false;
    s_lastActivityMs = now;
    if (ev == NAV_TAP || ev == NAV_HOLD || ev == NAV_DOUBLE_TAP) {
      spotifyClose();
      return;
    }
  }

  // Sensor 1: Play / Pause toggle
  if (g_touchPending) {
    g_touchPending = false;
    s_lastActivityMs = now;
    s_isPlaying = !s_isPlaying;
  }

  // Timeout inaktivitas 30 detik
  if (now - s_lastActivityMs > 30000) {
    spotifyClose();
    return;
  }

  displayClear();

  // 1. Header Bar Hitam (0..13)
  fbFillRect(0, 0, OLED_W, 13, C_BLACK);
  fbDrawMusicNote(6, 2, C_WHITE);
  fbDrawString(16, 3, "SPOTIFY NOW PLAYING", C_WHITE);
  wifiDrawHeaderIcon(116, 2, C_WHITE);

  // 2. Judul Lagu (Marquee Horizontal jika teks panjang)
  int titleW = fbGetStringWidth(s_trackTitle.c_str(), 1);
  if (titleW <= 120) {
    int xPos = (OLED_W - titleW) / 2;
    fbDrawString(xPos, 19, s_trackTitle.c_str(), C_BLACK);
  } else {
    int totalSpan = titleW + 36;
    int scrollOffset = (int)((now / 35) % totalSpan);
    int drawX = 4 - scrollOffset;

    fbDrawString(drawX, 19, s_trackTitle.c_str(), C_BLACK);
    if (drawX + titleW < 124) {
      fbDrawString(drawX + totalSpan, 19, s_trackTitle.c_str(), C_BLACK);
    }
  }

  // 3. Nama Artis (Baris kedua, terpusat)
  int artistW = fbGetStringWidth(s_trackArtist.c_str(), 1);
  int artistX = (OLED_W - artistW) / 2;
  fbDrawString(artistX, 31, s_trackArtist.c_str(), C_BLACK);

  // 4. Garis Pemisah Tipis
  fbHLine(14, 43, 100, C_BLACK);

  // 5. Equalizer Audio 5 Bar Prosedural Animasi (y = 46..60)
  static const int BAR_X[5] = { 40, 52, 64, 76, 88 };
  static const int BAR_W    = 7;
  static const int BASE_Y   = 60;

  for (int b = 0; b < 5; ++b) {
    int h = 3;
    if (s_isPlaying) {
      float phase = (float)b * 0.9f;
      float wave1 = sinf(now * 0.009f + phase);
      float wave2 = cosf(now * 0.014f + phase * 1.5f);
      h = 4 + (int)((wave1 * 0.5f + wave2 * 0.5f + 1.0f) * 5.0f);
      if (h < 3)  h = 3;
      if (h > 14) h = 14;
    }

    fbFillRect(BAR_X[b], BASE_Y - h, BAR_W, h, C_BLACK);
    // Detail puncak bar (garis pemisah estetik)
    if (h > 6) {
      putPx(BAR_X[b] + 1, BASE_Y - h + 2, C_WHITE);
      putPx(BAR_X[b] + 5, BASE_Y - h + 2, C_WHITE);
    }
  }

  displayFlush();
  delay(20);
}
