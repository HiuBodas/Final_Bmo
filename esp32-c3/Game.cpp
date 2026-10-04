// =============================================================================
// Game.cpp - Implementasi Mini Game Arcade BMO: Procedural Jump Runner
// 100% Vektor Prosedural Tanpa Bitmaps (OLED 128x64)
// High Score otomatis tersimpan di Flash ESP32 (Preferences NVS)
// =============================================================================
#include "Game.h"
#include "Display.h"
#include "Touch.h"
#include "Menu.h"
#include <Preferences.h>

enum GameState : uint8_t {
  GAME_STATE_START = 0,
  GAME_STATE_PLAYING,
  GAME_STATE_GAMEOVER
};

struct Obstacle {
  float   x;
  int8_t  w;
  int8_t  h;
  uint8_t type; // 0 = Spike segitiga, 1 = Balok glitch, 2 = Double spike
  bool    active;
};

static bool        s_gameActive      = false;
static GameState   s_gameState       = GAME_STATE_START;
static uint32_t    s_lastActivityMs  = 0;
static uint32_t    s_score           = 0;
static uint32_t    s_highScore       = 0;
static uint32_t    s_gameTick        = 0;

// Fisika Pemain Mini-BMO
static const float GROUND_Y          = 52.0f;
static const float PLAYER_H          = 11.0f;
static const float PLAYER_STAND_Y    = GROUND_Y - PLAYER_H; // 41.0f
static const float GRAVITY           = 0.50f;
static const float JUMP_IMPULSE      = -5.3f;

static float       s_playerY         = PLAYER_STAND_Y;
static float       s_playerVy        = 0.0f;
static bool        s_onGround        = true;

// Rintangan Vektor
static const int   MAX_OBSTACLES     = 2;
static Obstacle    s_obstacles[MAX_OBSTACLES];
static float       s_gameSpeed       = 2.6f;

// Objek Penyimpan NVS Flash
static Preferences s_prefs;

static void loadHighScore() {
  s_prefs.begin("bmo_game", true); // Read-only
  s_highScore = s_prefs.getUInt("hiscore", 0);
  s_prefs.end();
}

static void saveHighScore(uint32_t hs) {
  s_prefs.begin("bmo_game", false); // Read-write
  s_prefs.putUInt("hiscore", hs);
  s_prefs.end();
}

static void resetGame() {
  s_score       = 0;
  s_gameTick    = 0;
  s_gameSpeed   = 2.6f;
  s_playerY     = PLAYER_STAND_Y;
  s_playerVy    = 0.0f;
  s_onGround    = true;

  // Inisialisasi posisi awal 2 rintangan
  s_obstacles[0].x      = 130.0f;
  s_obstacles[0].w      = 7;
  s_obstacles[0].h      = 8;
  s_obstacles[0].type   = 0;
  s_obstacles[0].active = true;

  s_obstacles[1].x      = 210.0f;
  s_obstacles[1].w      = 8;
  s_obstacles[1].h      = 11;
  s_obstacles[1].type   = 1;
  s_obstacles[1].active = true;
}

void gameInit() {
  s_gameActive = false;
  s_gameState  = GAME_STATE_START;
  loadHighScore();
}

void gameOpen() {
  s_gameActive     = true;
  s_gameState      = GAME_STATE_START;
  s_lastActivityMs = millis();
  loadHighScore();
  resetGame();
  Serial.println(F("[Game] Masuk ke BMO Arcade Runner."));
}

void gameClose() {
  s_gameActive = false;
  Serial.println(F("[Game] Keluar dari BMO Arcade."));
}

bool gameIsActive() {
  return s_gameActive;
}

// -----------------------------------------------------------------------------
// Update Loop & Rendering Mini-Game
// -----------------------------------------------------------------------------
void gameUpdate() {
  uint32_t now = millis();
  pollTouch();

  // Input Sensor 2 (Navigasi / Keluar)
  if (g_navPending) {
    NavEvent ev = g_pendingNav;
    g_navPending = false;
    s_lastActivityMs = now;

    if (ev == NAV_HOLD || ev == NAV_DOUBLE_TAP) {
      gameClose();
      return;
    }
  }

  // Input Sensor 1 (Lompat / Mulai / Restart)
  bool actionTrigger = false;
  if (g_touchPending) {
    g_touchPending = false;
    actionTrigger  = true;
    s_lastActivityMs = now;
  }

  // Timeout Inaktivitas 30 detik saat idle
  if (s_gameState != GAME_STATE_PLAYING && (now - s_lastActivityMs > 30000)) {
    gameClose();
    return;
  }

  // ---------------------------------------------------------------------------
  // STATUS 1: LAYAR JUDUL / MULAI
  // ---------------------------------------------------------------------------
  if (s_gameState == GAME_STATE_START) {
    if (actionTrigger) {
      resetGame();
      s_gameState = GAME_STATE_PLAYING;
      return;
    }

    displayClear();

    // Header Bar
    fbFillRect(0, 0, OLED_W, 13, C_BLACK);
    int titleW = fbGetStringWidth("BMO ARCADE GAME");
    fbDrawString((OLED_W - titleW) / 2, 3, "BMO ARCADE GAME", C_WHITE);

    // Ikon BMO Melompat di Tengah
    fbFillRoundRect(58, 20, 12, 12, 2, C_BLACK);
    fbFillRect(60, 22, 8, 6, C_WHITE);
    putPx(62, 24, C_BLACK);
    putPx(65, 24, C_BLACK);
    putPx(64, 26, C_BLACK);

    // Teks Petunjuk Bersih
    char hiBuf[20];
    snprintf(hiBuf, sizeof(hiBuf), "TERTINGGI: %u", s_highScore);
    int hiW = fbGetStringWidth(hiBuf);
    fbDrawString((OLED_W - hiW) / 2, 38, hiBuf, C_BLACK);

    int startW = fbGetStringWidth("KETUK UNTUK MAIN");
    fbDrawString((OLED_W - startW) / 2, 50, "KETUK UNTUK MAIN", C_BLACK);

    displayFlush();
    delay(20);
    return;
  }

  // ---------------------------------------------------------------------------
  // STATUS 2: GAME OVER (TABRAKAN)
  // ---------------------------------------------------------------------------
  if (s_gameState == GAME_STATE_GAMEOVER) {
    if (actionTrigger) {
      resetGame();
      s_gameState = GAME_STATE_PLAYING;
      return;
    }

    displayClear();

    // Header Bar
    fbFillRect(0, 0, OLED_W, 13, C_BLACK);
    int overW = fbGetStringWidth("GAME OVER!");
    fbDrawString((OLED_W - overW) / 2, 3, "GAME OVER!", C_WHITE);

    // Kartu Skor
    fbRect(10, 17, 108, 33, C_BLACK);

    char scBuf[24];
    snprintf(scBuf, sizeof(scBuf), "SKOR ANDA  : %u", s_score);
    fbDrawString(16, 22, scBuf, C_BLACK);

    char hiBuf2[24];
    snprintf(hiBuf2, sizeof(hiBuf2), "TERTINGGI  : %u", s_highScore);
    fbDrawString(16, 34, hiBuf2, C_BLACK);

    // Footer Bersih
    int replayW = fbGetStringWidth("KETUK UNTUK ULANG");
    fbDrawString((OLED_W - replayW) / 2, 53, "KETUK UNTUK ULANG", C_BLACK);

    displayFlush();
    delay(25);
    return;
  }

  // ---------------------------------------------------------------------------
  // STATUS 3: GAMEPLAY AKTIF BERJALAN
  // ---------------------------------------------------------------------------
  s_gameTick++;

  // Logika Lompat Pemain
  if (actionTrigger && s_onGround) {
    s_playerVy = JUMP_IMPULSE;
    s_onGround = false;
  }

  // Pembaruan Fisika Pemain
  if (!s_onGround) {
    s_playerY  += s_playerVy;
    s_playerVy += GRAVITY;

    if (s_playerY >= PLAYER_STAND_Y) {
      s_playerY  = PLAYER_STAND_Y;
      s_playerVy = 0.0f;
      s_onGround = true;
    }
  }

  // Tambah skor setiap 3 frame & percepat laju bertahap
  if (s_gameTick % 3 == 0) {
    s_score++;
    if (s_score > s_highScore) {
      s_highScore = s_score;
    }
    // Kecepatan meningkat dari 2.6 hingga batas aman 5.2
    s_gameSpeed = 2.6f + (s_score / 150.0f) * 0.35f;
    if (s_gameSpeed > 5.2f) s_gameSpeed = 5.2f;
  }

  // Gerakkan & Periksa Rintangan
  for (int i = 0; i < MAX_OBSTACLES; ++i) {
    s_obstacles[i].x -= s_gameSpeed;

    // Jika rintangan melewati sisi kiri layar, buat ulang di kanan
    if (s_obstacles[i].x < -16.0f) {
      // Cari rintangan lainnya untuk menentukan jarak aman
      int otherIdx = (i == 0) ? 1 : 0;
      float baseX = max(130.0f, s_obstacles[otherIdx].x + 65.0f);
      s_obstacles[i].x = baseX + (random(10, 45));

      // Acak jenis rintangan
      uint8_t t = random(0, 3);
      s_obstacles[i].type = t;
      if (t == 0) {
        s_obstacles[i].w = 7;
        s_obstacles[i].h = 8;  // Single spike
      } else if (t == 1) {
        s_obstacles[i].w = 8;
        s_obstacles[i].h = 12; // Tall glitch block
      } else {
        s_obstacles[i].w = 12;
        s_obstacles[i].h = 7;  // Double spike
      }
    }

    // Deteksi Tabrakan (AABB Collision BMO vs Rintangan)
    // Hitbox BMO: x=17, w=9, y=playerY+1, h=9 (Toleransi 1px agar nyaman dimainkan)
    float bx = 17.0f;
    float bw = 9.0f;
    float by = s_playerY + 1.0f;
    float bh = 9.0f;

    float ox = s_obstacles[i].x + 1.0f;
    float ow = s_obstacles[i].w - 2.0f;
    float oy = GROUND_Y - s_obstacles[i].h;
    float oh = s_obstacles[i].h;

    if (bx < ox + ow && bx + bw > ox && by < oy + oh && by + bh > oy) {
      // Tabrakan terjadi!
      s_gameState = GAME_STATE_GAMEOVER;
      saveHighScore(s_highScore);
      Serial.printf("[Game] Tabrakan! Skor: %u, HighScore: %u\n", s_score, s_highScore);
      return;
    }
  }

  // ---------------------------------------------------------------------------
  // RENDER GRAFIS GAMEPLAY VEKTOR
  // ---------------------------------------------------------------------------
  displayClear();

  // 1. Header Bar Hitam (Skor & High Score)
  fbFillRect(0, 0, OLED_W, 11, C_BLACK);
  fbDrawString(4, 2, "BMO RUN", C_WHITE);

  char scoreTxt[24];
  snprintf(scoreTxt, sizeof(scoreTxt), "S:%u HI:%u", s_score, s_highScore);
  fbDrawString(52, 2, scoreTxt, C_WHITE);

  // 2. Awan Dekoratif Mengambang di Langit
  int cloudX = 130 - (int)((s_gameTick / 2) % 170);
  if (cloudX > -25 && cloudX < OLED_W) {
    fbFillRoundRect(cloudX, 18, 14, 5, 2, C_BLACK);
    fbFillCircle(cloudX + 4, 17, 3, C_BLACK);
    fbFillCircle(cloudX + 9, 16, 4, C_BLACK);
  }

  // 3. Garis Tanah Vektor & Tekstur Bergulir (Speed Sensation)
  fbHLine(0, (int)GROUND_Y, OLED_W, C_BLACK);
  int dashOffset = (int)(s_gameTick * (int)s_gameSpeed) % 14;
  for (int x = -dashOffset; x < OLED_W; x += 14) {
    if (x + 6 > 0) fbHLine(x, (int)GROUND_Y + 3, 6, C_BLACK);
  }

  // 4. Karakter Mini-BMO
  int py = (int)s_playerY;
  fbFillRoundRect(16, py, 11, 11, 2, C_BLACK);
  fbFillRect(18, py + 2, 7, 5, C_WHITE);
  putPx(19, py + 4, C_BLACK); // Mata kiri
  putPx(23, py + 4, C_BLACK); // Mata kanan
  putPx(21, py + 5, C_BLACK); // Senyum

  // Animasi Kaki / Roda Skateboard Berlari
  if (s_onGround) {
    int legStep = (s_gameTick / 3) % 2;
    if (legStep == 0) {
      fbVLine(18, py + 11, 2, C_BLACK);
      fbVLine(23, py + 11, 1, C_BLACK);
    } else {
      fbVLine(18, py + 11, 1, C_BLACK);
      fbVLine(23, py + 11, 2, C_BLACK);
    }
  } else {
    // Kaki melipat saat di udara
    fbHLine(18, py + 11, 7, C_BLACK);
  }

  // 5. Rintangan Vektor (Spike & Blok)
  for (int i = 0; i < MAX_OBSTACLES; ++i) {
    int ox = (int)s_obstacles[i].x;
    int ow = s_obstacles[i].w;
    int oh = s_obstacles[i].h;
    int oy = (int)GROUND_Y - oh;

    if (ox + ow < 0 || ox >= OLED_W) continue;

    if (s_obstacles[i].type == 0) {
      // Spike Segitiga Vektor
      for (int row = 0; row < oh; ++row) {
        int widthAtRow = (row * ow) / oh;
        int rowX = ox + (ow - widthAtRow) / 2;
        fbHLine(rowX, (int)GROUND_Y - 1 - row, widthAtRow + 1, C_BLACK);
      }
    } else if (s_obstacles[i].type == 1) {
      // Blok Glitch / Kaktus Vektor
      fbFillRect(ox + 1, oy, ow - 2, oh, C_BLACK);
      fbRect(ox, oy, ow, oh, C_BLACK);
      // Salib kecil di tengah blok
      putPx(ox + ow / 2, oy + oh / 2, C_WHITE);
    } else {
      // Double Spike Segitiga
      int halfW = ow / 2;
      for (int row = 0; row < oh; ++row) {
        int w1 = (row * halfW) / oh;
        fbHLine(ox + (halfW - w1) / 2, (int)GROUND_Y - 1 - row, w1 + 1, C_BLACK);
        fbHLine(ox + halfW + (halfW - w1) / 2, (int)GROUND_Y - 1 - row, w1 + 1, C_BLACK);
      }
    }
  }

  displayFlush();
  delay(20);
}
