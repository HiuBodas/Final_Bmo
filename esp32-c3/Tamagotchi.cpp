// =============================================================================
// Tamagotchi.cpp - Mode Virtual Pet BMO Sebagai Menu Mandiri
// 100% Vektor Prosedural Tanpa Bitmaps (OLED 128x64)
// Tetap menjaga siklus ekspresi normal saat keluar dari menu ini
// =============================================================================
#include "Tamagotchi.h"
#include "Display.h"
#include "Touch.h"
#include "Moods.h"
#include "Menu.h"

enum TamaView : uint8_t {
  TAMA_VIEW_FACE = 0,
  TAMA_VIEW_STATS
};

enum TamaFeedback : uint8_t {
  FEEDBACK_NONE = 0,
  FEEDBACK_FEED,
  FEEDBACK_PET
};

static bool         s_tamaActive     = false;
static TamaView     s_tamaView       = TAMA_VIEW_FACE;
static uint32_t     s_lastActivityMs = 0;
static uint32_t     s_lastDecayMs    = 0;

// Statistik Virtual Pet BMO (0 - 100)
static int16_t      s_hunger         = 80; // 100 = Kenyang, 0 = Lapar
static int16_t      s_happiness      = 85; // 100 = Bahagia, 0 = Sedih
static int16_t      s_energy         = 90; // 100 = Segar, 0 = Lelah
static uint16_t     s_feedCount      = 0;
static uint16_t     s_petCount       = 0;

// Animasi Feedback Aksi
static TamaFeedback s_feedback       = FEEDBACK_NONE;
static uint32_t     s_feedbackMs     = 0;

void tamagotchiInit() {
  s_tamaActive     = false;
  s_tamaView       = TAMA_VIEW_FACE;
  s_hunger         = 80;
  s_happiness      = 85;
  s_energy         = 90;
  s_feedCount      = 0;
  s_petCount       = 0;
  s_feedback       = FEEDBACK_NONE;
  s_lastDecayMs    = millis();
}

void tamagotchiOpen() {
  s_tamaActive     = true;
  s_tamaView       = TAMA_VIEW_FACE;
  s_feedback       = FEEDBACK_NONE;
  s_lastActivityMs = millis();
  s_lastDecayMs    = millis();
  Serial.println(F("[Tamagotchi] Masuk ke Mode Virtual Pet BMO."));
}

void tamagotchiClose() {
  s_tamaActive = false;
  Serial.println(F("[Tamagotchi] Keluar dari Mode Pet. Kembali ke Menu BMO."));
}

bool tamagotchiIsActive() {
  return s_tamaActive;
}

// Gambar Bar Status Vektor dengan Bingkai & Isi
static void drawStatusBar(int x, int y, int w, int h, int val, const char* label) {
  // Label teks
  fbDrawString(x, y - 1, label, C_BLACK);

  // Bingkai Bar
  int barX = x + 44;
  int barW = w - 44;
  fbRect(barX, y, barW, h, C_BLACK);

  // Isi Bar
  if (val > 100) val = 100;
  if (val < 0)   val = 0;
  int fillW = (val * (barW - 4)) / 100;
  if (fillW > 0) {
    fbFillRect(barX + 2, y + 2, fillW, h - 4, C_BLACK);
  }

  // Teks Nilai Angka
  char valStr[8];
  snprintf(valStr, sizeof(valStr), "%d%%", val);
  fbDrawString(barX + barW + 3, y - 1, valStr, C_BLACK);
}

// -----------------------------------------------------------------------------
// Update Loop Utama Tamagotchi
// -----------------------------------------------------------------------------
void tamagotchiUpdate() {
  uint32_t now = millis();
  pollTouch();

  // 1. Pengurangan Stat Berkala (Setiap 12 detik turun 1 poin)
  if (now - s_lastDecayMs >= 12000) {
    s_lastDecayMs = now;
    if (s_hunger > 5)    s_hunger -= 1;
    if (s_happiness > 5) s_happiness -= 1;
    if (s_energy > 5)    s_energy -= 1;
  }

  // Matikan feedback animasi setelah 1.4 detik
  if (s_feedback != FEEDBACK_NONE && (now - s_feedbackMs > 1400)) {
    s_feedback = FEEDBACK_NONE;
  }

  // 2. Input Sensor 2 (Navigasi / Keluar)
  if (g_navPending) {
    NavEvent ev = g_pendingNav;
    g_navPending = false;
    s_lastActivityMs = now;

    if (ev == NAV_TAP) {
      // Beralih antara Tampilan Wajah Pet & Status Detail
      s_tamaView = (s_tamaView == TAMA_VIEW_FACE) ? TAMA_VIEW_STATS : TAMA_VIEW_FACE;
    } else if (ev == NAV_HOLD || ev == NAV_DOUBLE_TAP) {
      // Keluar dari Tamagotchi kembali ke Menu Utama
      tamagotchiClose();
      return;
    }
  }

  // 3. Input Sensor 1 (Aksi Interaksi Virtual Pet)
  if (g_touchPending) {
    TouchKind kind = g_pendingKind;
    g_touchPending = false;
    s_lastActivityMs = now;

    if (kind == TOUCH_LONG_PRESS) {
      // Aksi 2: Elus / Sayang BMO (Tahan Sensor 1)
      s_happiness = min((int16_t)100, (int16_t)(s_happiness + 25));
      s_energy    = min((int16_t)100, (int16_t)(s_energy + 5));
      s_petCount++;
      s_feedback   = FEEDBACK_PET;
      s_feedbackMs = now;
      s_tamaView   = TAMA_VIEW_FACE;
      Serial.printf("[Tamagotchi] BMO Dielus! Senang: %d%%\n", s_happiness);
    } else {
      // Aksi 1: Beri Makan BMO (Sentuh Cepat Sensor 1)
      s_hunger    = min((int16_t)100, (int16_t)(s_hunger + 20));
      s_happiness = min((int16_t)100, (int16_t)(s_happiness + 5));
      s_feedCount++;
      s_feedback   = FEEDBACK_FEED;
      s_feedbackMs = now;
      s_tamaView   = TAMA_VIEW_FACE;
      Serial.printf("[Tamagotchi] BMO Diberi Makan! Kenyang: %d%%\n", s_hunger);
    }
  }

  // Timeout Inaktivitas 30 detik (Otomatis kembali ke Menu jika ditinggal)
  if (now - s_lastActivityMs > 30000) {
    tamagotchiClose();
    return;
  }

  // ---------------------------------------------------------------------------
  // TAMPILAN 1: WAJAH LIVE TAMAGOTCHI DENGAN MINI HUD
  // ---------------------------------------------------------------------------
  if (s_tamaView == TAMA_VIEW_FACE) {
    // Tentukan ekspresi wajah sesuai kondisi virtual pet
    Mood tamaMood = MOOD_IDLE;
    if (s_feedback == FEEDBACK_FEED) {
      tamaMood = MOOD_HAPPY;
    } else if (s_feedback == FEEDBACK_PET) {
      tamaMood = MOOD_LOVE;
    } else if (s_hunger < 30) {
      tamaMood = MOOD_HUNGRY;
    } else if (s_happiness < 30) {
      tamaMood = MOOD_SAD;
    } else if (s_energy < 25) {
      tamaMood = MOOD_SLEEPY;
    } else if (s_hunger > 80 && s_happiness > 80) {
      tamaMood = MOOD_EXCITED;
    } else {
      tamaMood = MOOD_COOL;
    }

    FaceState fs;
    float t = (float)((now / 50) % 100) / 100.0f;
    getMoodFaceState(tamaMood, t, now, fs);

    // Render ekspresi wajah ke buffer
    drawFaceToBuffer(fs, now);

    // 1. Mini Top HUD Bar (y = 0..11)
    fbFillRect(0, 0, OLED_W, 11, C_BLACK);

    // Ikon Hati & Senang
    fbDrawHeart(8, 5, 2, C_WHITE);
    char hapBuf[12];
    snprintf(hapBuf, sizeof(hapBuf), "%d%%", s_happiness);
    fbDrawString(14, 2, hapBuf, C_WHITE);

    // Pembatas tengah
    fbVLine(46, 1, 9, C_WHITE);

    // Ikon Makan & Kenyang
    fbRect(52, 2, 7, 7, C_WHITE);
    fbFillRect(54, 4, 3, 3, C_WHITE);
    char hngBuf[12];
    snprintf(hngBuf, sizeof(hngBuf), "%d%%", s_hunger);
    fbDrawString(63, 2, hngBuf, C_WHITE);

    // Pembatas kanan
    fbVLine(94, 1, 9, C_WHITE);

    // Label Mode
    fbDrawString(98, 2, "[PET]", C_WHITE);

    // 2. Banner Feedback Aksi (Muncul saat Baru Saja Diberi Makan / Dielus)
    if (s_feedback == FEEDBACK_FEED) {
      fbFillRoundRect(16, 26, 96, 14, 2, C_BLACK);
      fbRect(17, 27, 94, 12, C_WHITE);
      fbDrawString(24, 29, "+MAKAN NYAM! (+20)", C_WHITE);
    } else if (s_feedback == FEEDBACK_PET) {
      fbFillRoundRect(16, 26, 96, 14, 2, C_BLACK);
      fbRect(17, 27, 94, 12, C_WHITE);
      fbDrawHeart(22, 33, 2, C_WHITE);
      fbDrawString(30, 29, "DISAYANG! (+25)", C_WHITE);
    }

    // 3. Footer Bawah Hint Kontrol (y = 54..62)
    fbFillRect(0, 54, OLED_W, 10, C_WHITE);
    fbHLine(0, 53, OLED_W, C_BLACK);
    fbDrawString(4, 55, "S1:MAKAN|ELUS  S2:INFO", C_BLACK);

    displayFlush();
    delay(20);
    return;
  }

  // ---------------------------------------------------------------------------
  // TAMPILAN 2: STATUS DETAIL & PET STATS
  // ---------------------------------------------------------------------------
  displayClear();

  // Header Title Bar
  fbFillRect(0, 0, OLED_W, 13, C_BLACK);
  int titleW = fbGetStringWidth("STATUS BMO PET");
  fbDrawString((OLED_W - titleW) / 2, 3, "STATUS BMO PET", C_WHITE);

  // Bar 1: Kenyang
  drawStatusBar(6, 17, 56, 8, s_hunger, "Kenyang");

  // Bar 2: Senang
  drawStatusBar(6, 28, 56, 8, s_happiness, "Senang ");

  // Bar 3: Energi
  drawStatusBar(6, 39, 56, 8, s_energy, "Energi ");

  // Informasi Riwayat Rawat
  fbHLine(6, 50, 116, C_BLACK);
  char countBuf[32];
  snprintf(countBuf, sizeof(countBuf), "Makan: %dx  Elus: %dx", s_feedCount, s_petCount);
  fbDrawString(12, 53, countBuf, C_BLACK);

  displayFlush();
  delay(20);
}
