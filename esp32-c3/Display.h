// =============================================================================
// Display.h - Driver Layar OLED SSD1306 128x64 & Mesin Grafis Wajah BMO
// Tema: Background Putih, Garis/Fitur Hitam
// =============================================================================
#pragma once
#include <Arduino.h>
#include <stdint.h>
#include "Config.h"

// -----------------------------------------------------------------------------
// Struktur Status Ekspresi Wajah (FaceState)
// -----------------------------------------------------------------------------
struct FaceState {
  float lidL = 0.0f;
  float lidR = 0.0f;
  int pupilDx = 0;
  int pupilDy = 0;

  enum EyeShape {
    EYE_NORMAL,
    EYE_HEART,
    EYE_X,
    EYE_SPIRAL,
    EYE_SPINNER,
    EYE_HALF_WINK_L,
    EYE_HALF_WINK_R,
    EYE_DOT,
    EYE_SHADES,
    EYE_SQUINT,
    EYE_CRESCENT,
  } eyeShape = EYE_NORMAL;

  bool browVisible = false;
  int browSlope = 0;

  enum MouthShape {
    M_SMILE, M_OPEN, M_FLAT, M_TALK, M_FROWN, M_GRIN, M_OH,
    M_TONGUE_OUT, M_DROOL, M_ZIGZAG, M_CHATTER, M_LAUGH
  } mouth = M_SMILE;

  float mouthOpen = 0.0f;
  int smileWidth = 26;
  int smileDip = 5;
  int flatWidth = 22;

  float blush = 0.0f;

  bool tearVisible = false;
  int  tearY = 0;
  bool sweatVisible = false;
  int  sweatY = 0;
  bool starsVisible = false;
  bool heartsAround = false;
  bool questionMarks = false;
  bool listeningMarks = false;
  bool thinkingDots = false;
  bool snowFlakes = false;
  bool steamLines = false;
  bool zzzLetters = false;

  bool pulseMouth = false;
  uint8_t glitchShift = 0;
  int shakeX = 0;
  int shakeY = 0;
  uint8_t glitchBars = 0;
};

// -----------------------------------------------------------------------------
// Prototipe Fungsi Driver & Grafis
// -----------------------------------------------------------------------------
void displayInit();
void displayClear();
void displayFlush();
void displaySetContrast(uint8_t contrast);
void displaySetFlip(bool flipped);

void putPx(int x, int y, uint8_t color);
void fbFill(uint8_t color);
void fbHLine(int x, int y, int w, uint8_t color);
void fbVLine(int x, int y, int h, uint8_t color);
void fbRect(int x, int y, int w, int h, uint8_t color);
void fbFillRect(int x, int y, int w, int h, uint8_t color);
void fbLine(int x0, int y0, int x1, int y1, int thickness, uint8_t color);
void fbFillCircle(int cx, int cy, int r, uint8_t color);
void fbFillEllipse(int cx, int cy, int rx, int ry, uint8_t color);
void fbFillRoundRect(int x, int y, int w, int h, int r, uint8_t color);

void fbDrawHeart(int cx, int cy, int size, uint8_t color);
void fbDrawStar(int cx, int cy, int size, uint8_t color);
void fbDrawSmile(int cx, int cy, int width, int dipDepth, int thickness, int cornerCurl, uint8_t color);
void fbDrawFrown(int cx, int cy, int width, int dipDepth, int thickness, uint8_t color);
void fbDrawFlatMouth(int cx, int cy, int width, int thickness, uint8_t color);
void fbDrawBrow(int cx, int cy, int width, int slope, int thickness, uint8_t color);

void fbDrawEye(int cx, int cy, int w, int h, float lid, int dx, int dy);
void fbDrawCrescentEye(int cx, int cy, int width, uint8_t color);
void fbDrawHeartEye(int cx, int cy, int size, uint8_t color);
void fbDrawSpiralEye(int cx, int cy, int radius, float phase, uint8_t color);
void fbDrawSpinnerEye(int cx, int cy, int radius, float phase, uint8_t color);
void fbDrawXEye(int cx, int cy, int size, uint8_t color);
void fbDrawShades(int leftCx, int rightCx, int cy, uint32_t now);

void fbDrawTear(int cx, int cy, int yPx, uint8_t color);
void fbDrawSweat(int cx, int cy, int yPx);
void fbDrawBlush(int cx, int cy, float amount);
void fbDrawBmoMouth(int cx, int cy, int w, int h, bool tooth, bool tongue);
void fbDrawOpenMouth(int cx, int cy, int rx, int ry, bool tongue);
void fbDrawDrool(int cx, int cy, int len);
void fbDrawTongueOut(int cx, int cy);
void fbDrawSnow(int cx, int cy, int size);
void fbDrawSteam(int cx, int cy, int phase, uint8_t color);
void fbDrawQuestion(int cx, int cy, uint8_t color);
void fbDrawZ(int cx, int cy, int size, uint8_t color);
void fbDrawZigzag(int cx, int cy, int width, int amp, int thickness, uint8_t color);
void fbDrawPulseMouth(int cx, int cy, float phase);
void fbDrawChatter(int cx, int cy, uint32_t now);
void fbDrawLaugh(int cx, int cy, float openness, uint32_t now);
void fbApplyGlitch(uint8_t intensity, uint32_t now);

// -----------------------------------------------------------------------------
// Driver Font Vektor Prosedural 5x7 & Simbol Navigasi Menu
// -----------------------------------------------------------------------------
void fbDrawChar(int x, int y, char c, uint8_t color, uint8_t bg = 255, uint8_t scale = 1);
void fbDrawString(int x, int y, const char* str, uint8_t color, uint8_t bg = 255, uint8_t scale = 1);
int  fbGetStringWidth(const char* str, uint8_t scale = 1);
void fbDrawLeftArrow(int x, int y, uint8_t color);
void fbDrawRightArrow(int x, int y, uint8_t color);
void fbDrawCheckmark(int x, int y, uint8_t color);
void fbDrawWifiIcon(int x, int y, uint8_t state, uint8_t color, uint32_t now);
void fbDrawWeatherIcon(int x, int y, uint8_t kind, uint8_t color, uint32_t now);
void fbDrawMusicNote(int x, int y, uint8_t color);

void drawFaceToBuffer(const FaceState &s, uint32_t now);
void renderFrame(const FaceState &s, uint32_t now);
void applyMoodTransition(FaceState &s, float t);

