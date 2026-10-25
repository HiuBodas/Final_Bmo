// =============================================================================
// SpotifyWidget.cpp - Implementasi Widget "Now Playing" Spotify BMO & Lirik Lagu
// Bagian dari Roadmap BMO ESP32 (Seksi 2.4 & Lirik Karaoke)
// Mendukung Lirik Real-Time via BLE (Opsi A) & Fetch Mandiri via Wi-Fi LRCLIB (Opsi B)
// =============================================================================
#include "SpotifyWidget.h"
#include "Display.h"
#include "Touch.h"
#include "WifiManager.h"
#include <math.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#define MAX_LYRIC_LINES 60

static bool     s_spotifyActive    = false;
static String   s_trackTitle       = "Theme Song";
static String   s_trackArtist      = "Adventure Time";
static bool     s_isPlaying        = true;
static uint32_t s_lastUpdateMs     = 0;
static uint32_t s_lastActivityMs   = 0;

// Variabel Lirik
static String   s_lyricLines[MAX_LYRIC_LINES];
static int      s_lyricCount       = 0;
static int      s_currentLyricIdx  = 0;
static String   s_liveCurrentLine  = "";
static String   s_liveNextLine     = "";
static bool     s_hasLyrics        = false;
static bool     s_isFetchingLyrics = false;
static bool     s_needFetchLyrics  = false;
static uint32_t s_lastAutoScrollMs = 0;

void spotifyInit() {
  s_spotifyActive    = false;
  s_trackTitle       = "Theme Song";
  s_trackArtist      = "Adventure Time";
  s_isPlaying        = true;
  s_hasLyrics        = false;
  s_isFetchingLyrics = false;
  s_needFetchLyrics  = false;
  s_lyricCount       = 0;
  s_currentLyricIdx  = 0;
  s_liveCurrentLine  = "";
  s_liveNextLine     = "";
  s_lastUpdateMs     = millis();
}

void spotifySetTrack(const String& title, const String& artist) {
  s_trackTitle  = title;
  s_trackArtist = artist;
  s_trackTitle.trim();
  s_trackArtist.trim();
  s_isPlaying        = true;
  s_hasLyrics        = false;
  s_isFetchingLyrics = false;
  s_lyricCount       = 0;
  s_currentLyricIdx  = 0;
  s_liveCurrentLine  = "";
  s_liveNextLine     = "";
  s_lastUpdateMs     = millis();

  // Otomatis picu fetch lirik jika Wi-Fi tersambung (Opsi B)
  if (WiFi.status() == WL_CONNECTED && s_trackTitle.length() > 0) {
    s_needFetchLyrics = true;
  }
}

String spotifyGetTitle() {
  return s_trackTitle;
}

String spotifyGetArtist() {
  return s_trackArtist;
}

void spotifySetLyricsLine(const String& currentLine, const String& nextLine) {
  s_liveCurrentLine = currentLine;
  s_liveNextLine    = nextLine;
  s_hasLyrics       = true;
  s_isFetchingLyrics = false;
  s_lastActivityMs  = millis();
  if (!s_spotifyActive) {
    spotifyOpen();
  }
}

void spotifyFetchLyrics() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("[Spotify Lirik] Wi-Fi belum tersambung untuk fetch."));
    return;
  }

  s_isFetchingLyrics = true;
  s_hasLyrics        = false;
  s_lyricCount       = 0;
  s_currentLyricIdx  = 0;

  WiFiClientSecure client;
  client.setInsecure(); // Bebas SSL CA verification untuk LRCLIB
  HTTPClient http;
  http.setTimeout(7000);

  // Encode URL
  String url = "https://lrclib.net/api/get?track_name=";
  for (unsigned int i = 0; i < s_trackTitle.length(); i++) {
    char c = s_trackTitle[i];
    if (c == ' ') url += "%20";
    else if (c == '&') url += "%26";
    else url += c;
  }
  if (s_trackArtist.length() > 0) {
    url += "&artist_name=";
    for (unsigned int i = 0; i < s_trackArtist.length(); i++) {
      char c = s_trackArtist[i];
      if (c == ' ') url += "%20";
      else if (c == '&') url += "%26";
      else url += c;
    }
  }

  Serial.printf("[Spotify Lirik] Mengambil dari: %s\n", url.c_str());
  http.begin(client, url);
  http.setUserAgent("BMO-ESP32-Client/1.0");

  int code = http.GET();
  if (code == HTTP_CODE_OK) {
    String payload = http.getString();
    // Cari "plainLyrics":"
    int pIdx = payload.indexOf("\"plainLyrics\":\"");
    if (pIdx != -1) {
      int start = pIdx + 15;
      int end = payload.indexOf("\",\"syncedLyrics\"", start);
      if (end == -1) end = payload.indexOf("\"", start);
      if (end != -1 && end > start) {
        String lyricsRaw = payload.substring(start, end);
        lyricsRaw.replace("\\n", "\n");
        lyricsRaw.replace("\\r", "");
        lyricsRaw.replace("\\\"", "\"");

        // Split per baris
        int lineStart = 0;
        s_lyricCount = 0;
        while (lineStart < (int)lyricsRaw.length() && s_lyricCount < MAX_LYRIC_LINES) {
          int lineEnd = lyricsRaw.indexOf('\n', lineStart);
          if (lineEnd == -1) lineEnd = lyricsRaw.length();
          String line = lyricsRaw.substring(lineStart, lineEnd);
          line.trim();
          if (line.length() > 0) {
            s_lyricLines[s_lyricCount++] = line;
          }
          lineStart = lineEnd + 1;
        }

        if (s_lyricCount > 0) {
          s_hasLyrics = true;
          s_currentLyricIdx = 0;
          s_lastAutoScrollMs = millis();
          Serial.printf("[Spotify Lirik] Berhasil memuat %d baris lirik!\n", s_lyricCount);
        }
      }
    }
  } else {
    Serial.printf("[Spotify Lirik] HTTP response code: %d\n", code);
  }
  http.end();
  s_isFetchingLyrics = false;
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

  // Handle background fetch jika ada permintaan
  if (s_needFetchLyrics) {
    s_needFetchLyrics = false;
    spotifyFetchLyrics();
  }

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

  // Sensor 1: Interaksi Sentuh (Aksi)
  if (g_touchPending) {
    g_touchPending = false;
    s_lastActivityMs = now;
    if (s_hasLyrics && s_lyricCount > 0) {
      // Manual ganti baris lirik (Teleprompter mode)
      s_currentLyricIdx = (s_currentLyricIdx + 1) % s_lyricCount;
      s_lastAutoScrollMs = now;
    } else {
      s_isPlaying = !s_isPlaying;
    }
  }

  // Auto-scroll lirik per 4 detik jika ada daftar lirik (Opsi B)
  if (s_hasLyrics && s_lyricCount > 0 && s_isPlaying) {
    if (now - s_lastAutoScrollMs > 4000) {
      s_lastAutoScrollMs = now;
      if (s_currentLyricIdx + 1 < s_lyricCount) {
        s_currentLyricIdx++;
      }
    }
  }

  // Timeout inaktivitas 60 detik jika tidak ada lirik aktif
  if (now - s_lastActivityMs > 60000) {
    spotifyClose();
    return;
  }

  displayClear();

  // 1. Header Bar Hitam (0..12)
  fbFillRect(0, 0, OLED_W, 13, C_BLACK);
  fbDrawMusicNote(6, 2, C_WHITE);
  String headerText = s_trackTitle + " - " + s_trackArtist;
  int headerW = fbGetStringWidth(headerText.c_str(), 1);
  if (headerW <= 92) {
    fbDrawString(16, 2, headerText.c_str(), C_WHITE);
  } else {
    int totalSpan = headerW + 24;
    int scrollOffset = (int)((now / 35) % totalSpan);
    int drawX = 16 - scrollOffset;
    fbDrawString(drawX, 2, headerText.c_str(), C_WHITE);
    if (drawX + headerW < 112) {
      fbDrawString(drawX + totalSpan, 2, headerText.c_str(), C_WHITE);
    }
  }
  wifiDrawHeaderIcon(116, 2, C_WHITE);

  // 2. Mode Lirik Aktif (Opsi A & Opsi B)
  if (s_hasLyrics) {
    String cur = (s_liveCurrentLine.length() > 0)
        ? s_liveCurrentLine
        : ((s_currentLyricIdx < s_lyricCount) ? s_lyricLines[s_currentLyricIdx] : "");
    String nxt = (s_liveNextLine.length() > 0)
        ? s_liveNextLine
        : ((s_currentLyricIdx + 1 < s_lyricCount) ? s_lyricLines[s_currentLyricIdx + 1] : "");

    // Gambar Baris Aktif (Word-wrap 2 baris jika > 20 karakter)
    if (cur.length() <= 20) {
      int curW = fbGetStringWidth(cur.c_str(), 1);
      int curX = (OLED_W - curW) / 2;
      if (curX < 4) curX = 4;
      fbDrawString(curX, 20, cur.c_str(), C_BLACK);
    } else {
      // Potong baris cerdas berdasarkan spasi
      int splitIdx = cur.lastIndexOf(' ', 20);
      if (splitIdx == -1) splitIdx = 18;
      String line1 = cur.substring(0, splitIdx);
      String line2 = cur.substring(splitIdx + 1);
      line1.trim();
      line2.trim();

      int w1 = fbGetStringWidth(line1.c_str(), 1);
      int w2 = fbGetStringWidth(line2.c_str(), 1);
      fbDrawString((OLED_W - w1) / 2, 16, line1.c_str(), C_BLACK);
      fbDrawString((OLED_W - w2) / 2, 26, line2.c_str(), C_BLACK);
    }

    // Baris Berikutnya (Preview Redup)
    if (nxt.length() > 0) {
      String preview = "> " + nxt;
      if (preview.length() > 21) {
        preview = preview.substring(0, 19) + "..";
      }
      fbDrawString(6, 38, preview.c_str(), C_BLACK);
    }

    // Garis Pemisah Bawah
    fbHLine(6, 49, 116, C_BLACK);

    // Mini Equalizer 4 Bar di Kiri Bawah (y = 52..61)
    static const int MINI_BAR_X[4] = { 8, 14, 20, 26 };
    for (int b = 0; b < 4; ++b) {
      int h = 3;
      if (s_isPlaying) {
        float wave = sinf(now * 0.012f + (float)b * 1.2f);
        h = 3 + (int)((wave + 1.0f) * 3.5f);
        if (h < 2) h = 2;
        if (h > 9) h = 9;
      }
      fbFillRect(MINI_BAR_X[b], 61 - h, 4, h, C_BLACK);
    }

    // Indikator Lirik di Kanan Bawah
    if (s_lyricCount > 0) {
      char buf[16];
      snprintf(buf, sizeof(buf), "%02d/%02d", s_currentLyricIdx + 1, s_lyricCount);
      fbDrawString(92, 53, buf, C_BLACK);
    } else {
      fbDrawString(74, 53, "KARAOKE", C_BLACK);
    }
  }
  // 3. Status Sedang Mengambil Lirik (Loading)
  else if (s_isFetchingLyrics) {
    fbDrawString(14, 25, "MEMUAT LIRIK...", C_BLACK);
    int dotCount = (int)((now / 300) % 4);
    String dots = "";
    for (int i = 0; i < dotCount; i++) dots += ".";
    fbDrawString(100, 25, dots.c_str(), C_BLACK);

    fbDrawString(16, 40, "Mengambil dari LRCLIB", C_BLACK);
  }
  // 4. Default: Tampilan Judul + 5-Bar Equalizer Penuh
  else {
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

    int artistW = fbGetStringWidth(s_trackArtist.c_str(), 1);
    int artistX = (OLED_W - artistW) / 2;
    fbDrawString(artistX, 31, s_trackArtist.c_str(), C_BLACK);

    fbHLine(14, 43, 100, C_BLACK);

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
      if (h > 6) {
        putPx(BAR_X[b] + 1, BASE_Y - h + 2, C_WHITE);
        putPx(BAR_X[b] + 5, BASE_Y - h + 2, C_WHITE);
      }
    }
  }

  displayFlush();
  delay(20);
}

