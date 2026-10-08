// =============================================================================
// WeatherManager.cpp - Implementasi Live Weather & Temperature HUD
// Bagian dari Roadmap BMO ESP32 (Seksi 2.2)
// Mengambil data format ringkas dari wttr.in secara non-blocking via HTTP
// =============================================================================
#include "WeatherManager.h"
#include "BleManager.h"
#include "Moods.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <Preferences.h>

static String      s_city           = "Jakarta"; // Default kota
static int         s_temp           = 28;
static int         s_humidity       = 75;
static String      s_condition      = "Cerah";
static WeatherKind s_kind           = WEATHER_SUN;
static bool        s_hasData        = false;
static uint32_t    s_lastUpdateMs   = 0;

void weatherInit() {
  Preferences prefs;
  prefs.begin("bmo_cfg", true);
  s_city = prefs.getString("city", "Jakarta");
  prefs.end();

  s_hasData = false;
  s_lastUpdateMs = 0;
}

bool weatherHasData() {
  return s_hasData;
}

int weatherGetTemp() {
  return s_temp;
}

int weatherGetHumidity() {
  return s_humidity;
}

String weatherGetCondition() {
  return s_condition;
}

WeatherKind weatherGetKind() {
  return s_kind;
}

String weatherGetCity() {
  return s_city;
}

void weatherSetCity(const String& city) {
  s_city = city;
  s_city.trim();
  Preferences prefs;
  prefs.begin("bmo_cfg", false);
  prefs.putString("city", s_city);
  prefs.end();
  s_lastUpdateMs = 0; // Picu update segera
}

void weatherUpdate() {
  if (WiFi.status() != WL_CONNECTED) return;

  uint32_t now = millis();
  // Perbarui setiap 20 menit (1200000 ms), atau segera jika belum punya data
  if (s_hasData && (now - s_lastUpdateMs < 1200000)) return;

  Serial.println(F("[Weather] Mengambil data cuaca terkini..."));

  HTTPClient http;
  http.setTimeout(5000);

  // Gunakan format minimal: %t|%h|%C (misal: +29°C|75%|Partly cloudy)
  String url = "http://wttr.in/" + s_city + "?format=%t|%h|%C";
  http.begin(url);
  http.setUserAgent("BMO-ESP32-Client/1.0");

  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    payload.trim();
    Serial.printf("[Weather RX] %s\n", payload.c_str());

    int firstPipe = payload.indexOf('|');
    int secondPipe = payload.indexOf('|', firstPipe + 1);

    if (firstPipe != -1 && secondPipe != -1) {
      String tempRaw = payload.substring(0, firstPipe);
      String humRaw  = payload.substring(firstPipe + 1, secondPipe);
      String condRaw = payload.substring(secondPipe + 1);

      // Parsing Suhu (misal: "+29°C" -> 29)
      tempRaw.replace("+", "");
      tempRaw.replace("°C", "");
      tempRaw.replace("C", "");
      tempRaw.trim();
      if (tempRaw.length() > 0) s_temp = tempRaw.toInt();

      // Parsing Kelembaban (misal: "75%" -> 75)
      humRaw.replace("%", "");
      humRaw.trim();
      if (humRaw.length() > 0) s_humidity = humRaw.toInt();

      // Parsing Kondisi & Penentuan Ikon
      condRaw.trim();
      String condLower = condRaw;
      condLower.toLowerCase();

      if (condLower.indexOf("thunder") != -1 || condLower.indexOf("storm") != -1) {
        s_kind = WEATHER_THUNDER;
        s_condition = "Badai";
      } else if (condLower.indexOf("rain") != -1 || condLower.indexOf("drizzle") != -1 || condLower.indexOf("shower") != -1) {
        s_kind = WEATHER_RAIN;
        s_condition = "Hujan";
      } else if (condLower.indexOf("cloud") != -1 || condLower.indexOf("overcast") != -1 || condLower.indexOf("fog") != -1) {
        s_kind = WEATHER_CLOUD;
        s_condition = "Berawan";
      } else {
        s_kind = WEATHER_SUN;
        s_condition = "Cerah";
      }

      s_hasData = true;
      s_lastUpdateMs = now;
      Serial.printf("[Weather] Sukses: %d°C, %d%% RH, Kondisi: %s\n", s_temp, s_humidity, s_condition.c_str());

      // Interaksi Ekspresi Wajah BMO Berdasarkan Suhu & Cuaca (Seksi 2.2)
      if (s_temp < 24 || s_kind == WEATHER_RAIN) {
        bleTriggerMood(MOOD_COLD);
      } else if (s_temp > 33) {
        bleTriggerMood(MOOD_HOT);
      } else if (s_temp >= 25 && s_temp <= 30) {
        bleTriggerMood(MOOD_HAPPY);
      }
    }
  } else {
    Serial.printf("[Weather] HTTP GET gagal, kode: %d\n", httpCode);
  }

  http.end();
}
