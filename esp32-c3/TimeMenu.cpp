// =============================================================================
// TimeMenu.cpp - Implementasi Suite Waktu: Stopwatch, Timer, & Pomodoro
// 100% Vektor Prosedural Tanpa Bitmaps (Background Putih, Garis/Fitur Hitam)
// =============================================================================
#include "TimeMenu.h"
#include "Display.h"
#include "Touch.h"
#include "WifiManager.h"
#include "NtpClock.h"
#include "WeatherManager.h"
#include "SpotifyWidget.h"

static bool         s_timeActive      = false;
static TimeSubState s_subState        = TIME_SUB_MENU;
static int          s_timeMenuIdx     = 0;
static int          s_timeTopIdx      = 0;
static uint32_t     s_lastActivityMs  = 0;

// -----------------------------------------------------------------------------
// 1. Variabel Stopwatch
// -----------------------------------------------------------------------------
static bool         s_swRunning       = false;
static uint32_t     s_swElapsedMs     = 0;
static uint32_t     s_swLastTickMs    = 0;

// -----------------------------------------------------------------------------
// 2. Variabel Countdown Timer
// -----------------------------------------------------------------------------
static const uint16_t s_tmPresets[]   = { 1, 3, 5, 10, 15, 25 };
static const uint8_t  s_tmPresetCount = 6;
static uint8_t        s_tmPresetIdx   = 2; // Default 5 Menit
static uint32_t       s_tmTotalSec    = 300;
static uint32_t       s_tmRemainSec   = 300;
static bool           s_tmRunning     = false;
static uint32_t       s_tmLastTickMs  = 0;
static bool           s_tmAlarm       = false;

// -----------------------------------------------------------------------------
// 3. Variabel Pomodoro (Podomoro)
// -----------------------------------------------------------------------------
static const uint16_t s_pomoPresets[]   = { 5, 10, 15, 20, 25, 30, 45, 50, 60 };
static const uint8_t  s_pomoPresetCount = sizeof(s_pomoPresets) / sizeof(s_pomoPresets[0]);
static uint8_t        s_pomoPresetIdx   = 4;    // Default 25 Menit
static bool           s_pomoRunning     = false;
static bool           s_pomoIsWork      = true; // true = Fokus, false = Istirahat
static uint32_t       s_pomoTotalSec    = 25 * 60;
static uint32_t       s_pomoRemainSec   = 25 * 60;
static uint8_t        s_pomoCycle       = 1;    // Siklus 1 s/d 4
static uint32_t       s_pomoLastTickMs  = 0;
static bool           s_pomoAlarm       = false;

void timeMenuInit() {
  s_timeActive     = false;
  s_subState       = TIME_SUB_MENU;
  s_timeMenuIdx    = 0;
  s_swRunning      = false;
  s_swElapsedMs    = 0;
  s_tmRunning      = false;
  s_tmAlarm        = false;
  s_tmPresetIdx    = 2;
  s_tmTotalSec     = s_tmPresets[s_tmPresetIdx] * 60;
  s_tmRemainSec    = s_tmTotalSec;
  s_pomoRunning    = false;
  s_pomoIsWork     = true;
  s_pomoPresetIdx  = 4; // 25 Menit
  s_pomoTotalSec   = s_pomoPresets[s_pomoPresetIdx] * 60;
  s_pomoRemainSec  = s_pomoTotalSec;
  s_pomoCycle      = 1;
  s_pomoAlarm      = false;
}

bool timeMenuIsActive() {
  return s_timeActive;
}

TimeSubState timeMenuGetSubState() {
  return s_subState;
}

void timeMenuOpen() {
  s_timeActive     = true;
  s_subState       = TIME_SUB_MENU;
  s_timeMenuIdx    = 0;
  s_lastActivityMs = millis();
  Serial.println("[time] Masuk ke Menu TIME.");
}

void timeMenuClose() {
  s_timeActive = false;
  s_subState   = TIME_SUB_MENU;
  Serial.println("[time] Keluar dari Menu TIME.");
}

// -----------------------------------------------------------------------------
// Render Halaman Submenu Pilihan TIME (1. Jam & Cuaca, 2. Spotify, 3. Stopwatch, 4. Timer, 5. Pomodoro, 6. Kembali)
// -----------------------------------------------------------------------------
static const char* const s_timeMenuItems[] = {
  "JAM & CUACA",
  "SPOTIFY NOW",
  "STOPWATCH",
  "HITUNG MUNDUR",
  "POMODORO",
  "KEMBALI"
};
static const int TIME_ITEM_COUNT = 6;

static void renderTimeSubMenu() {
  displayClear();

  // Header Title Bar
  fbFillRect(0, 0, OLED_W, 13, C_BLACK);
  int titleW = fbGetStringWidth("WAKTU & TIMER");
  fbDrawString((OLED_W - titleW) / 2, 3, "WAKTU & TIMER", C_WHITE);
  wifiDrawHeaderIcon(116, 2, C_WHITE);

  // Viewport scrolling 4 item
  if (s_timeMenuIdx >= s_timeTopIdx + 4) {
    s_timeTopIdx = s_timeMenuIdx - 3;
  } else if (s_timeMenuIdx < s_timeTopIdx) {
    s_timeTopIdx = s_timeMenuIdx;
  }

  for (int i = 0; i < 4; ++i) {
    int itemIdx = s_timeTopIdx + i;
    if (itemIdx >= TIME_ITEM_COUNT) break;

    int y = 16 + i * 11;
    if (itemIdx == s_timeMenuIdx) {
      fbFillRoundRect(6, y, 114, 11, 2, C_BLACK);
      fbDrawRightArrow(10, y + 3, C_WHITE);
      fbDrawString(20, y + 2, s_timeMenuItems[itemIdx], C_WHITE);
    } else {
      fbDrawString(20, y + 2, s_timeMenuItems[itemIdx], C_BLACK);
    }
  }

  // Scrollbar vertikal
  fbVLine(123, 16, 44, C_BLACK);
  int thumbH = 8;
  int thumbY = 16 + (s_timeMenuIdx * (44 - thumbH)) / (TIME_ITEM_COUNT - 1);
  fbFillRect(121, thumbY, 3, thumbH, C_BLACK);

  displayFlush();
}

// -----------------------------------------------------------------------------
// Render Jam Internet Otomatis (NTP) & Cuaca HUD (Seksi 2.1 & 2.2)
// -----------------------------------------------------------------------------
static void renderClockAndWeather(uint32_t now) {
  // Sentuhan Sensor 1 atau Sensor 2 kembali ke menu
  if (g_touchPending || g_navPending) {
    g_touchPending = false;
    g_navPending   = false;
    s_subState     = TIME_SUB_MENU;
    s_lastActivityMs = now;
    return;
  }

  displayClear();

  // 1. Header Bar Hitam (0..13)
  fbFillRect(0, 0, OLED_W, 13, C_BLACK);
  int titleW = fbGetStringWidth("JAM & CUACA");
  fbDrawString((OLED_W - titleW) / 2, 3, "JAM & CUACA", C_WHITE);
  wifiDrawHeaderIcon(116, 2, C_WHITE);

  // 2. Jam Digital Besar (Scale 2)
  String timeStr = ntpGetTimeStr();
  int tW = fbGetStringWidth(timeStr.c_str(), 2);
  fbDrawString((OLED_W - tW) / 2, 17, timeStr.c_str(), C_BLACK, 255, 2);

  // 3. Hari & Tanggal
  String dateStr = ntpGetDayStr() + ", " + ntpGetDateStr();
  int dW = fbGetStringWidth(dateStr.c_str(), 1);
  fbDrawString((OLED_W - dW) / 2, 35, dateStr.c_str(), C_BLACK);

  // 4. Garis Pemisah Tipis
  fbHLine(8, 45, 112, C_BLACK);

  // 5. Cuaca & Suhu HUD
  if (weatherHasData()) {
    // Ikon Cuaca Vektor Animasi di x=10, y=47
    fbDrawWeatherIcon(10, 47, (uint8_t)weatherGetKind(), C_BLACK, now);

    char wBuf[32];
    snprintf(wBuf, sizeof(wBuf), "%d^C | %s (%d%%)", weatherGetTemp(), weatherGetCondition().c_str(), weatherGetHumidity());
    fbDrawString(30, 51, wBuf, C_BLACK);
  } else {
    const char* info = wifiIsConnected() ? "SINKRON CUACA..." : "OFFLINE (BUTUH WIFI)";
    int w = fbGetStringWidth(info);
    fbDrawString((OLED_W - w) / 2, 51, info, C_BLACK);
  }

  displayFlush();
  delay(20);
}

// -----------------------------------------------------------------------------
// Render & Logika Stopwatch
// -----------------------------------------------------------------------------
static void renderStopwatch(uint32_t now) {
  if (s_swRunning) {
    uint32_t dt = now - s_swLastTickMs;
    s_swElapsedMs += dt;
    s_swLastTickMs = now;
  }

  displayClear();

  // Header Bar
  fbFillRect(0, 0, OLED_W, 13, C_BLACK);
  fbDrawString(8, 3, "STOPWATCH", C_WHITE);

  if (s_swRunning) {
    fbDrawString(92, 3, "AKTIF", C_WHITE);
  } else if (s_swElapsedMs > 0) {
    fbDrawString(92, 3, "JEDA", C_WHITE);
  } else {
    fbDrawString(92, 3, "SIAP", C_WHITE);
  }

  // Format Waktu Besar: MM:SS.d
  uint32_t m = (s_swElapsedMs / 60000) % 100;
  uint32_t sec = (s_swElapsedMs / 1000) % 60;
  uint32_t tenth = (s_swElapsedMs / 100) % 10;

  char timeStr[16];
  snprintf(timeStr, sizeof(timeStr), "%02lu:%02lu.%lu", m, sec, tenth);

  // Gambar Angka Digital Besar (Scale 2) tepat di tengah layar
  int textW = fbGetStringWidth(timeStr, 2);
  int textX = (OLED_W - textW) / 2;
  fbDrawString(textX, 22, timeStr, C_BLACK, 255, 2);

  // Status & Indikator Minimalis
  fbHLine(20, 44, 88, C_BLACK);
  if (s_swRunning) {
    int step = (now / 200) % 3;
    fbFillCircle(56 + step * 8, 52, 2, C_BLACK);
  } else if (s_swElapsedMs > 0) {
    int w = fbGetStringWidth("TERJEDA");
    fbDrawString((OLED_W - w) / 2, 49, "TERJEDA", C_BLACK);
  } else {
    int w = fbGetStringWidth("SIAP");
    fbDrawString((OLED_W - w) / 2, 49, "SIAP", C_BLACK);
  }

  displayFlush();
}

// -----------------------------------------------------------------------------
// Render & Logika Countdown Timer
// -----------------------------------------------------------------------------
static void renderTimer(uint32_t now) {
  if (s_tmRunning) {
    if (now - s_tmLastTickMs >= 1000) {
      s_tmLastTickMs = now;
      if (s_tmRemainSec > 0) {
        s_tmRemainSec--;
      } else {
        s_tmRunning = false;
        s_tmAlarm   = true;
      }
    }
  }

  displayClear();

  // Header Bar (Bersih, Rapi & Elegan Tanpa Teks Bertumpuk [05M])
  fbFillRect(0, 0, OLED_W, 13, C_BLACK);
  int titleW = fbGetStringWidth("HITUNG MUNDUR");
  fbDrawString((OLED_W - titleW) / 2, 3, "HITUNG MUNDUR", C_WHITE);
  wifiDrawHeaderIcon(116, 2, C_WHITE);

  // Format Hitung Mundur Besar: MM:SS
  uint32_t m = s_tmRemainSec / 60;
  uint32_t sec = s_tmRemainSec % 60;
  char timeStr[12];
  snprintf(timeStr, sizeof(timeStr), "%02lu:%02lu", m, sec);

  int textW = fbGetStringWidth(timeStr, 2);
  fbDrawString((OLED_W - textW) / 2, 19, timeStr, C_BLACK, 255, 2);

  // Progress Bar Horizontal
  fbRect(8, 38, 112, 6, C_BLACK);
  if (s_tmTotalSec > 0) {
    int fillW = (int)((uint64_t)s_tmRemainSec * 108 / s_tmTotalSec);
    if (fillW > 108) fillW = 108;
    if (fillW > 0) fbFillRect(10, 40, fillW, 2, C_BLACK);
  }

  // Status Indikator Minimalis
  if (s_tmAlarm) {
    if ((now / 250) & 1) {
      fbFillRoundRect(16, 48, 96, 13, 2, C_BLACK);
      fbDrawString(26, 51, "WAKTU HABIS!", C_WHITE);
    }
  } else if (s_tmRunning) {
    int w = fbGetStringWidth("BERJALAN");
    fbDrawString((OLED_W - w) / 2, 49, "BERJALAN", C_BLACK);
  } else if (s_tmRemainSec < s_tmTotalSec) {
    int w = fbGetStringWidth("TERJEDA");
    fbDrawString((OLED_W - w) / 2, 49, "TERJEDA", C_BLACK);
  } else {
    int w = fbGetStringWidth("SIAP");
    fbDrawString((OLED_W - w) / 2, 49, "SIAP", C_BLACK);
  }

  displayFlush();
}

// -----------------------------------------------------------------------------
// Render & Logika Podomoro (Pomodoro)
// -----------------------------------------------------------------------------
static void renderPomodoro(uint32_t now) {
  if (s_pomoRunning) {
    if (now - s_pomoLastTickMs >= 1000) {
      s_pomoLastTickMs = now;
      if (s_pomoRemainSec > 0) {
        s_pomoRemainSec--;
      } else {
        s_pomoRunning = false;
        s_pomoAlarm   = true;
      }
    }
  }

  displayClear();

  // Header Title Bar
  fbFillRect(0, 0, OLED_W, 13, C_BLACK);
  fbDrawString(8, 3, "POMODORO", C_WHITE);

  char cycBuf[8];
  snprintf(cycBuf, sizeof(cycBuf), "#%d/4", s_pomoCycle);
  fbDrawString(98, 3, cycBuf, C_WHITE);

  // Badge Status Sesi
  if (s_pomoIsWork) {
    fbFillRoundRect(34, 15, 60, 10, 2, C_BLACK);
    fbDrawString(44, 16, "FOKUS", C_WHITE);
  } else {
    fbFillRoundRect(28, 15, 72, 10, 2, C_BLACK);
    fbDrawString(34, 16, "ISTIRAHAT", C_WHITE);
  }

  // Format Hitung Mundur Besar: MM:SS
  uint32_t m = s_pomoRemainSec / 60;
  uint32_t sec = s_pomoRemainSec % 60;
  char timeStr[12];
  snprintf(timeStr, sizeof(timeStr), "%02lu:%02lu", m, sec);

  int textW = fbGetStringWidth(timeStr, 2);
  fbDrawString((OLED_W - textW) / 2, 28, timeStr, C_BLACK, 255, 2);

  // Progress Bar Horizontal
  fbRect(8, 46, 112, 6, C_BLACK);
  if (s_pomoTotalSec > 0) {
    int fillW = (int)((uint64_t)s_pomoRemainSec * 108 / s_pomoTotalSec);
    if (fillW > 108) fillW = 108;
    if (fillW > 0) fbFillRect(10, 48, fillW, 2, C_BLACK);
  }

  // Status Indikator Bawah
  if (s_pomoAlarm) {
    if ((now / 250) & 1) {
      const char* msg = s_pomoIsWork ? "ISTIRAHAT DULU!" : "MULAI FOKUS!";
      int w = fbGetStringWidth(msg);
      fbDrawString((OLED_W - w) / 2, 55, msg, C_BLACK);
    }
  } else if (s_pomoRunning) {
    int w = fbGetStringWidth(s_pomoIsWork ? "SESI FOKUS" : "SESI ISTIRAHAT");
    fbDrawString((OLED_W - w) / 2, 55, s_pomoIsWork ? "SESI FOKUS" : "SESI ISTIRAHAT", C_BLACK);
  } else if (s_pomoRemainSec < s_pomoTotalSec) {
    int w = fbGetStringWidth("TERJEDA");
    fbDrawString((OLED_W - w) / 2, 55, "TERJEDA", C_BLACK);
  } else {
    int w = fbGetStringWidth("SIAP FOKUS");
    fbDrawString((OLED_W - w) / 2, 55, "SIAP FOKUS", C_BLACK);
  }

  displayFlush();
}

// -----------------------------------------------------------------------------
// Update Loop Utama Suite Time
// -----------------------------------------------------------------------------
void timeMenuUpdate() {
  uint32_t now = millis();
  pollTouch();

  // ===========================================================================
  // SUB-MODE 0: MENU PILIHAN ALAT TIME
  // ===========================================================================
  if (s_subState == TIME_SUB_MENU) {
    // Sensor 2: Geser kursor pilihan
    if (g_navPending) {
      NavEvent ev = g_pendingNav;
      g_navPending = false;

      if (ev == NAV_TAP) {
        s_timeMenuIdx = (s_timeMenuIdx + 1) % TIME_ITEM_COUNT;
        s_lastActivityMs = now;
      } else if (ev == NAV_HOLD || ev == NAV_DOUBLE_TAP) {
        timeMenuClose();
        return;
      }
    }

    // Sensor 1: Masuk ke fitur yang dipilih
    if (g_touchPending) {
      g_touchPending = false;
      s_lastActivityMs = now;

      if (s_timeMenuIdx == 0) {
        s_subState = TIME_SUB_CLOCK;
      } else if (s_timeMenuIdx == 1) {
        s_subState = TIME_SUB_SPOTIFY;
        spotifyOpen();
      } else if (s_timeMenuIdx == 2) {
        s_subState    = TIME_SUB_STOPWATCH;
        s_swRunning   = false;
        s_swElapsedMs = 0;
      } else if (s_timeMenuIdx == 3) {
        s_subState      = TIME_SUB_TIMER;
        s_tmRunning     = false;
        s_tmAlarm       = false;
        s_tmRemainSec   = s_tmTotalSec;
      } else if (s_timeMenuIdx == 4) {
        s_subState       = TIME_SUB_POMODORO;
        s_pomoRunning    = false;
        s_pomoAlarm      = false;
        s_pomoIsWork     = true;
        s_pomoCycle      = 1;
        s_pomoTotalSec   = s_pomoPresets[s_pomoPresetIdx] * 60;
        s_pomoRemainSec  = s_pomoTotalSec;
      } else {
        timeMenuClose();
        return;
      }
    }

    if (now - s_lastActivityMs > 20000) {
      timeMenuClose();
      return;
    }

    renderTimeSubMenu();
    delay(20);
    return;
  }

  // ===========================================================================
  // SUB-MODE: JAM & CUACA HUD (Seksi 2.1 & 2.2)
  // ===========================================================================
  if (s_subState == TIME_SUB_CLOCK) {
    renderClockAndWeather(now);
    return;
  }

  // ===========================================================================
  // SUB-MODE: SPOTIFY "NOW PLAYING" (Seksi 2.4)
  // ===========================================================================
  if (s_subState == TIME_SUB_SPOTIFY) {
    if (!spotifyIsActive()) {
      s_subState = TIME_SUB_MENU;
      s_lastActivityMs = now;
      return;
    }
    spotifyUpdate();
    return;
  }

  // ===========================================================================
  // SUB-MODE 1: STOPWATCH
  // ===========================================================================
  if (s_subState == TIME_SUB_STOPWATCH) {
    // Sensor 1: Start / Pause
    if (g_touchPending) {
      g_touchPending = false;
      s_swRunning = !s_swRunning;
      if (s_swRunning) s_swLastTickMs = now;
    }

    // Sensor 2: Reset jika berhenti, atau Tahan untuk Keluar
    if (g_navPending) {
      NavEvent ev = g_pendingNav;
      g_navPending = false;

      if (ev == NAV_HOLD || ev == NAV_DOUBLE_TAP) {
        s_swRunning = false;
        s_subState  = TIME_SUB_MENU;
        s_lastActivityMs = now;
        return;
      } else if (ev == NAV_TAP && !s_swRunning) {
        s_swElapsedMs = 0;
      }
    }

    renderStopwatch(now);
    delay(20);
    return;
  }

  // ===========================================================================
  // SUB-MODE 2: TIMER
  // ===========================================================================
  if (s_subState == TIME_SUB_TIMER) {
    // Sensor 1: Start / Pause atau Reset Alarm
    if (g_touchPending) {
      g_touchPending = false;
      if (s_tmAlarm) {
        s_tmAlarm     = false;
        s_tmRemainSec = s_tmTotalSec;
      } else {
        s_tmRunning = !s_tmRunning;
        if (s_tmRunning) s_tmLastTickMs = now;
      }
    }

    // Sensor 2: Ganti preset waktu jika berhenti, atau Tahan untuk Keluar
    if (g_navPending) {
      NavEvent ev = g_pendingNav;
      g_navPending = false;

      if (ev == NAV_HOLD || ev == NAV_DOUBLE_TAP) {
        s_tmRunning = false;
        s_subState  = TIME_SUB_MENU;
        s_lastActivityMs = now;
        return;
      } else if (ev == NAV_TAP && !s_tmRunning && !s_tmAlarm) {
        if (s_tmRemainSec < s_tmTotalSec) {
          s_tmRemainSec = s_tmTotalSec; // Reset jika terjeda
        } else {
          s_tmPresetIdx = (s_tmPresetIdx + 1) % s_tmPresetCount;
          s_tmTotalSec  = s_tmPresets[s_tmPresetIdx] * 60;
          s_tmRemainSec = s_tmTotalSec;
        }
      }
    }

    renderTimer(now);
    delay(25);
    return;
  }

  // ===========================================================================
  // SUB-MODE 3: POMODORO
  // ===========================================================================
  if (s_subState == TIME_SUB_POMODORO) {
    // Sensor 1: Start / Pause atau Lanjut ke sesi berikutnya saat alarm
    if (g_touchPending) {
      g_touchPending = false;
      if (s_pomoAlarm) {
        s_pomoAlarm = false;
        if (s_pomoIsWork) {
          // Beralih ke sesi istirahat
          s_pomoIsWork    = false;
          uint32_t breakSec = (s_pomoPresets[s_pomoPresetIdx] <= 10) ? (3 * 60) : (5 * 60);
          s_pomoTotalSec  = breakSec;
          s_pomoRemainSec = breakSec;
        } else {
          // Beralih ke sesi kerja berikutnya
          s_pomoCycle++;
          if (s_pomoCycle > 4) s_pomoCycle = 1;
          s_pomoIsWork    = true;
          s_pomoTotalSec  = s_pomoPresets[s_pomoPresetIdx] * 60;
          s_pomoRemainSec = s_pomoTotalSec;
        }
      } else {
        s_pomoRunning = !s_pomoRunning;
        if (s_pomoRunning) s_pomoLastTickMs = now;
      }
    }

    // Sensor 2: Atur menit saat standby, reset jika terjeda, atau Tahan untuk Keluar
    if (g_navPending) {
      NavEvent ev = g_pendingNav;
      g_navPending = false;

      if (ev == NAV_HOLD || ev == NAV_DOUBLE_TAP) {
        s_pomoRunning = false;
        s_subState    = TIME_SUB_MENU;
        s_lastActivityMs = now;
        return;
      } else if (ev == NAV_TAP && !s_pomoRunning && !s_pomoAlarm) {
        if (s_pomoRemainSec < s_pomoTotalSec) {
          // Reset sesi yang sedang terjeda kembali ke awal
          s_pomoRemainSec = s_pomoTotalSec;
        } else if (s_pomoIsWork) {
          // Atur/ganti preset menit Pomodoro saat standby sebelum mulai!
          s_pomoPresetIdx = (s_pomoPresetIdx + 1) % s_pomoPresetCount;
          s_pomoTotalSec  = s_pomoPresets[s_pomoPresetIdx] * 60;
          s_pomoRemainSec = s_pomoTotalSec;
          s_pomoCycle     = 1;
        }
      }
    }

    renderPomodoro(now);
    delay(25);
    return;
  }
}

// -----------------------------------------------------------------------------
// Kontrol Pomodoro Jarak Jauh (Web Dashboard / API)
// -----------------------------------------------------------------------------
void timeMenuStartPomodoro() {
  s_pomoRunning = true;
  s_pomoLastTickMs = millis();
}

void timeMenuPausePomodoro() {
  s_pomoRunning = false;
}

void timeMenuResetPomodoro() {
  s_pomoRunning = false;
  s_pomoIsWork = true;
  s_pomoCycle = 1;
  s_pomoTotalSec = s_pomoPresets[s_pomoPresetIdx] * 60;
  s_pomoRemainSec = s_pomoTotalSec;
}

bool timeMenuIsPomodoroRunning() {
  return s_pomoRunning;
}

uint32_t timeMenuGetPomodoroRemain() {
  return s_pomoRemainSec;
}

bool timeMenuIsPomodoroWork() {
  return s_pomoIsWork;
}

uint8_t timeMenuGetPomodoroCycle() {
  return s_pomoCycle;
}
