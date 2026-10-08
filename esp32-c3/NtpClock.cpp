// =============================================================================
// NtpClock.cpp - Implementasi Jam & Kalender Internet Otomatis ESP32-C3
// Bagian dari Roadmap BMO ESP32 (Seksi 2.1)
// =============================================================================
#include "NtpClock.h"
#include <time.h>
#include <WiFi.h>

// Offset WIB (UTC+7) = 7 jam * 3600 detik = 25200 detik
static const long  GMT_OFFSET_SEC      = 7 * 3600;
static const int   DAYLIGHT_OFFSET_SEC = 0;
static const char* NTP_SERVER_1        = "pool.ntp.org";
static const char* NTP_SERVER_2        = "time.google.com";

static bool s_ntpSynced = false;

static const char* const DAYS_ID[] = {
  "Minggu", "Senin", "Selasa", "Rabu", "Kamis", "Jumat", "Sabtu"
};

void ntpInit() {
  s_ntpSynced = false;
}

void ntpStartSync() {
  Serial.println(F("[NTP] Memulai sinkronisasi waktu global (UTC+7 WIB)..."));
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER_1, NTP_SERVER_2);

  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 2000)) {
    s_ntpSynced = true;
    Serial.printf("[NTP] Waktu tersinkronisasi: %02d:%02d:%02d WIB\n",
                  timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  } else {
    Serial.println(F("[NTP] Menunggu respon dari server NTP..."));
  }
}

bool ntpIsSynced() {
  if (!s_ntpSynced) {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 10)) {
      if (timeinfo.tm_year > (2020 - 1900)) {
        s_ntpSynced = true;
      }
    }
  }
  return s_ntpSynced;
}

void ntpGetTime(int &hour, int &minute, int &second) {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 10)) {
    hour   = timeinfo.tm_hour;
    minute = timeinfo.tm_min;
    second = timeinfo.tm_sec;
  } else {
    hour   = 0;
    minute = 0;
    second = 0;
  }
}

String ntpGetTimeStr() {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 10)) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    return String(buf);
  }
  return "00:00:00";
}

String ntpGetDateStr() {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 10)) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d/%02d/%04d", timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
    return String(buf);
  }
  return "01/01/2026";
}

String ntpGetDayStr() {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 10)) {
    int w = timeinfo.tm_wday;
    if (w >= 0 && w < 7) {
      return String(DAYS_ID[w]);
    }
  }
  return "Hari";
}

String ntpGetFullDateTimeStr() {
  return ntpGetDayStr() + ", " + ntpGetDateStr() + " " + ntpGetTimeStr();
}
