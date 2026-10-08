// =============================================================================
// Display.cpp - Implementasi Driver Layar OLED SSD1306 128x64 & Grafis BMO
// Tema: Background Putih, Garis & Fitur Wajah Hitam
// =============================================================================
#include "Display.h"
#include <Wire.h>

// Framebuffer 128x64 bit monokrom = 128 * 64 / 8 = 1024 byte
static uint8_t fb[OLED_W * OLED_H / 8];

// -----------------------------------------------------------------------------
// Driver I2C SSD1306 Mandiri (Tanpa library eksternal)
// -----------------------------------------------------------------------------
static void sendCommand(uint8_t cmd) {
  Wire.beginTransmission(OLED_ADDR);
  Wire.write(0x00); // Mode instruksi / command
  Wire.write(cmd);
  Wire.endTransmission();
}

void displayInit() {
  Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL, 400000); // I2C Fast Mode 400 kHz
  delay(50);

  // Sekuens Inisialisasi SSD1306 128x64
  static const uint8_t initCommands[] = {
    0xAE,        // Display OFF
    0xD5, 0x80,  // Display Clock Divide Ratio / Oscillator Frequency
    0xA8, 0x3F,  // Multiplex Ratio 1/64 (0x3F)
    0xD3, 0x00,  // Display Offset 0
    0x40,        // Start Line 0
    0x8D, 0x14,  // Charge Pump Regulator: Enable (Wajib untuk OLED 0.96")
    0x20, 0x00,  // Memory Addressing Mode: Horizontal
    0xA1,        // Segment Re-map: column 127 mapped to SEG0
    0xC8,        // COM Output Scan Direction: remapped
    0xDA, 0x12,  // COM Pins Hardware Configuration
    0x81, 0xCF,  // Kontras
    0xD9, 0xF1,  // Pre-charge Period
    0xDB, 0x40,  // VCOMH Deselect Level
    0xA4,        // Output follows RAM
    0xA6,        // Normal display (1 = pixel lit/white, 0 = pixel unlit/black)
    0xAF         // Display ON!
  };

  for (size_t i = 0; i < sizeof(initCommands); ++i) {
    sendCommand(initCommands[i]);
  }

  // Bersihkan layar menjadi putih (semua piksel menyala)
  displayClear();
  displayFlush();
}

void displayClear() {
  // Semua piksel diaktifkan (0xFF) untuk menghasilkan Background Putih
  memset(fb, 0xFF, sizeof(fb));
}

void displayFlush() {
  // Set Column Address: 0 .. 127
  Wire.beginTransmission(OLED_ADDR);
  Wire.write(0x00);
  Wire.write(0x21);
  Wire.write(0);
  Wire.write(OLED_W - 1);
  // Set Page Address: 0 .. 7
  Wire.write(0x22);
  Wire.write(0);
  Wire.write((OLED_H / 8) - 1);
  Wire.endTransmission();

  // Kirim data buffer dalam blok 32-byte (batas buffer Wire Arduino)
  for (size_t i = 0; i < sizeof(fb); i += 32) {
    Wire.beginTransmission(OLED_ADDR);
    Wire.write(0x40); // Mode data piksel
    Wire.write(&fb[i], 32);
    Wire.endTransmission();
  }
}

void displaySetContrast(uint8_t contrast) {
  sendCommand(0x81);
  sendCommand(contrast);
}

void displaySetFlip(bool flipped) {
  if (flipped) {
    sendCommand(0xA0); // SEG0 mapped to column 0
    sendCommand(0xC0); // COM0 to COM[N-1]
  } else {
    sendCommand(0xA1); // Column 127 mapped to SEG0
    sendCommand(0xC8); // COM[N-1] to COM0
  }
}

// -----------------------------------------------------------------------------
// Primitif Grafis Monokrom (128x64)
// -----------------------------------------------------------------------------
void putPx(int x, int y, uint8_t color) {
  if ((unsigned)x >= (unsigned)OLED_W || (unsigned)y >= (unsigned)OLED_H) return;
  if (color) {
    fb[x + (y >> 3) * OLED_W] |= (1 << (y & 7));
  } else {
    fb[x + (y >> 3) * OLED_W] &= ~(1 << (y & 7));
  }
}

void fbFill(uint8_t color) {
  memset(fb, color ? 0xFF : 0x00, sizeof(fb));
}

void fbHLine(int x, int y, int w, uint8_t color) {
  if ((unsigned)y >= (unsigned)OLED_H) return;
  if (x < 0) { w += x; x = 0; }
  if (x + w > OLED_W) w = OLED_W - x;
  if (w <= 0) return;
  for (int i = 0; i < w; ++i) {
    putPx(x + i, y, color);
  }
}

void fbVLine(int x, int y, int h, uint8_t color) {
  if ((unsigned)x >= (unsigned)OLED_W) return;
  if (y < 0) { h += y; y = 0; }
  if (y + h > OLED_H) h = OLED_H - y;
  if (h <= 0 || (unsigned)y >= (unsigned)OLED_H) return;
  for (int i = 0; i < h; ++i) {
    putPx(x, y + i, color);
  }
}

void fbRect(int x, int y, int w, int h, uint8_t color) {
  fbHLine(x, y, w, color);
  fbHLine(x, y + h - 1, w, color);
  fbVLine(x, y, h, color);
  fbVLine(x + w - 1, y, h, color);
}

void fbFillRect(int x, int y, int w, int h, uint8_t color) {
  if (w <= 0 || h <= 0) return;
  for (int j = 0; j < h; ++j) {
    fbHLine(x, y + j, w, color);
  }
}

void fbLine(int x0, int y0, int x1, int y1, int thickness, uint8_t color) {
  int dx = abs(x1 - x0);
  int dy = -abs(y1 - y0);
  int sx = x0 < x1 ? 1 : -1;
  int sy = y0 < y1 ? 1 : -1;
  int err = dx + dy;
  while (true) {
    if (thickness <= 1) {
      putPx(x0, y0, color);
    } else {
      fbFillCircle(x0, y0, thickness / 2, color);
    }
    if (x0 == x1 && y0 == y1) break;
    int e2 = 2 * err;
    if (e2 >= dy) { err += dy; x0 += sx; }
    if (e2 <= dx) { err += dx; y0 += sy; }
  }
}

void fbFillCircle(int cx, int cy, int r, uint8_t color) {
  if (r < 0) return;
  for (int y = -r; y <= r; ++y) {
    int dx = (int)floorf(sqrtf((float)(r * r - y * y)));
    fbHLine(cx - dx, cy + y, 2 * dx + 1, color);
  }
}

void fbFillEllipse(int cx, int cy, int rx, int ry, uint8_t color) {
  if (rx <= 0 || ry <= 0) return;
  for (int y = -ry; y <= ry; ++y) {
    float t = (float)y / (float)ry;
    int dx = (int)floorf(rx * sqrtf(1.0f - t * t));
    fbHLine(cx - dx, cy + y, 2 * dx + 1, color);
  }
}

void fbFillRoundRect(int x, int y, int w, int h, int r, uint8_t color) {
  if (r * 2 > w) r = w / 2;
  if (r * 2 > h) r = h / 2;
  if (r < 0) r = 0;
  fbFillRect(x + r, y, w - 2 * r, h, color);
  fbFillRect(x,         y + r, r, h - 2 * r, color);
  fbFillRect(x + w - r, y + r, r, h - 2 * r, color);
  fbFillCircle(x + r,         y + r,         r, color);
  fbFillCircle(x + w - r - 1, y + r,         r, color);
  fbFillCircle(x + r,         y + h - r - 1, r, color);
  fbFillCircle(x + w - r - 1, y + h - r - 1, r, color);
}

void fbDrawHeart(int cx, int cy, int size, uint8_t color) {
  if (size <= 0) return;
  int r = size;
  fbFillCircle(cx - r + 1, cy - r / 2, r, color);
  fbFillCircle(cx + r - 1, cy - r / 2, r, color);
  for (int dy = -r / 2; dy <= 2 * r; ++dy) {
    float t = (float)(dy + r / 2) / (float)(2 * r + r / 2);
    int half = (int)((1.0f - t) * (2 * r));
    if (half < 0) half = 0;
    fbHLine(cx - half, cy + dy, 2 * half + 1, color);
  }
}

void fbDrawStar(int cx, int cy, int size, uint8_t color) {
  if (size < 1) return;
  fbVLine(cx, cy - size, 2 * size + 1, color);
  fbHLine(cx - size, cy, 2 * size + 1, color);
  if (size >= 3) {
    putPx(cx + 1, cy + 1, color);
    putPx(cx - 1, cy - 1, color);
    putPx(cx + 1, cy - 1, color);
    putPx(cx - 1, cy + 1, color);
  }
}

void fbDrawSmile(int cx, int cy, int width, int dipDepth, int thickness, int cornerCurl, uint8_t color) {
  if (width < 2) return;
  int half = width / 2;
  for (int x = -half; x <= half; ++x) {
    float t = (float)x / half;
    float dip = dipDepth * (1.0f - t * t);
    float curl = cornerCurl * (t * t * t * t);
    int y = cy + (int)dip - (int)curl;
    fbVLine(cx + x, y - thickness / 2, thickness, color);
  }
}

void fbDrawFrown(int cx, int cy, int width, int dipDepth, int thickness, uint8_t color) {
  if (width < 2) return;
  int half = width / 2;
  for (int x = -half; x <= half; ++x) {
    float t = (float)x / half;
    float bump = dipDepth * (1.0f - t * t);
    int y = cy - (int)bump;
    fbVLine(cx + x, y - thickness / 2, thickness, color);
  }
}

void fbDrawFlatMouth(int cx, int cy, int width, int thickness, uint8_t color) {
  fbFillRoundRect(cx - width / 2, cy - thickness / 2, width, thickness, thickness / 2, color);
}

void fbDrawBrow(int cx, int cy, int width, int slope, int thickness, uint8_t color) {
  int x0 = cx - width / 2;
  int x1 = cx + width / 2;
  int y0 = cy + slope / 2;
  int y1 = cy - slope / 2;
  fbLine(x0, y0, x1, y1, thickness, color);
}

// Mata Normal: Mata hitam pekat dengan kelopak putih (background) & sparkle putih
void fbDrawEye(int cx, int cy, int w, int h, float lid, int dx, int dy) {
  int top = cy - h / 2;
  int rx  = w / 2;
  int ry  = h / 2;
  if (lid >= 0.95f) {
    // Mata terpejam rapat: garis lurus hitam
    fbDrawFlatMouth(cx, cy, w + 2, 2, C_BLACK);
    return;
  }

  // 1. Gambar mata hitam solid
  fbFillEllipse(cx, cy, rx, ry, C_BLACK);

  // 2. Kelopak mata (menutup dari atas dengan warna putih background)
  if (lid > 0.05f) {
    int lidH = (int)(h * lid);
    fbFillRect(cx - rx - 1, top - 1, w + 2, lidH + 1, C_WHITE);
  }

  // 3. Titik kilau (sparkle shine) putih kecil di dalam mata hitam agar mata hidup
  if (lid < 0.85f && w >= 5 && h >= 5) {
    int px = cx + dx;
    int py = cy + dy;
    if (px < cx - rx + 1) px = cx - rx + 1;
    if (px > cx + rx - 1) px = cx + rx - 1;
    if (py < cy - ry + 1) py = cy - ry + 1;
    if (py > cy + ry - 1) py = cy + ry - 1;
    putPx(px, py, C_WHITE);
    putPx(px + 1, py, C_WHITE);
  }
}

void fbDrawCrescentEye(int cx, int cy, int width, uint8_t color) {
  fbDrawSmile(cx, cy - 2, width, 3, 2, 1, color);
}

void fbDrawHeartEye(int cx, int cy, int size, uint8_t color) {
  fbDrawHeart(cx, cy, size, color);
}

void fbDrawSpiralEye(int cx, int cy, int radius, float phase, uint8_t color) {
  int prevX = cx, prevY = cy;
  for (int i = 0; i < 20; ++i) {
    float t = i / 20.0f;
    float r = radius * t;
    float a = phase + t * 6.28318f * 2.0f;
    int x = cx + (int)(cosf(a) * r);
    int y = cy + (int)(sinf(a) * r);
    if (i > 0) fbLine(prevX, prevY, x, y, 1, color);
    prevX = x; prevY = y;
  }
}

void fbDrawSpinnerEye(int cx, int cy, int radius, float phase, uint8_t color) {
  const int count = 8;
  for (int i = 0; i < count; ++i) {
    if (i > 5) continue;
    float a = phase - i * (6.28318f / count);
    int x = cx + (int)(cosf(a) * radius);
    int y = cy + (int)(sinf(a) * radius);
    int r = (i < 2) ? 2 : 1;
    fbFillCircle(x, y, r, color);
  }
}

void fbDrawXEye(int cx, int cy, int size, uint8_t color) {
  fbLine(cx - size, cy - size, cx + size, cy + size, 2, color);
  fbLine(cx - size, cy + size, cx + size, cy - size, 2, color);
}

// Kacamata Visor Pit Viper (Frame & lensa hitam, pantulan kilau putih)
void fbDrawShades(int leftCx, int rightCx, int cy, uint32_t now) {
  const int x0 = leftCx  - 14;
  const int x1 = rightCx + 14;
  const int w  = x1 - x0;
  const int top = cy - 10;
  const int h   = 20;
  const int noseX = (leftCx + rightCx) / 2;

  // Frame dan bodi kacamata hitam pekat
  fbFillRoundRect(x0, top, w, h, 4, C_BLACK);

  // Cutout celah hidung (warna background putih)
  fbFillCircle(noseX, top + h - 1, 3, C_WHITE);

  // Garis pantulan putih diagonal pada lensa kacamata hitam
  fbLine(x0 + 10, top + h - 4, x0 + 22, top + 4, 1, C_WHITE);
  fbLine(x0 + 16, top + h - 4, x0 + 26, top + 5, 1, C_WHITE);
  fbLine(x1 - 24, top + 5, x1 - 14, top + h - 5, 1, C_WHITE);

  // Animasi sapuan cahaya cermin horizontal putih
  const int sweep = x0 + 4 + (int)((now / 32) % (w - 8));
  fbLine(sweep - 3, top + h - 4, sweep + 3, top + 4, 2, C_WHITE);
}

void fbDrawTear(int cx, int cy, int yPx, uint8_t color) {
  int y = cy + yPx;
  fbFillEllipse(cx, y + 2, 2, 3, color);
  putPx(cx, y, color);
}

void fbDrawSweat(int cx, int cy, int yPx) {
  int y = cy + yPx;
  fbFillEllipse(cx, y + 1, 1, 2, C_BLACK);
}

// Blush pipi bertekstur titik (stipple hitam) khas manga/retro
void fbDrawBlush(int cx, int cy, float amount) {
  int r = 2 + (int)(2 * amount);
  for (int y = -r; y <= r; ++y) {
    for (int x = -r; x <= r; ++x) {
      if ((x * x + y * y) <= r * r) {
        if ((x + y + 100) % 2 == 0) putPx(cx + x, cy + y, C_BLACK);
      }
    }
  }
}

void fbDrawBmoMouth(int cx, int cy, int w, int h, bool tooth, bool tongue) {
  if (w < 8) w = 8;
  if (h < 5) h = 5;
  int rx = w / 2;
  int ry = h / 2;

  // Rongga mulut hitam
  fbFillEllipse(cx, cy, rx, ry, C_BLACK);

  // Gigi putih di langit-langit mulut hitam
  if (tooth && rx >= 6) {
    fbFillRect(cx - 2, cy - ry + 1, 4, 2, C_WHITE);
  }

  // Lidah putih di dasar mulut hitam
  if (tongue && ry >= 4) {
    fbFillEllipse(cx, cy + ry - 2, rx / 2, 2, C_WHITE);
  }
}

void fbDrawOpenMouth(int cx, int cy, int rx, int ry, bool tongue) {
  fbDrawBmoMouth(cx, cy, rx * 2, ry * 2, rx > 7 && ry > 4, tongue);
}

void fbDrawDrool(int cx, int cy, int len) {
  fbFillEllipse(cx, cy + len / 2, 1, len / 2 + 1, C_BLACK);
}

void fbDrawTongueOut(int cx, int cy) {
  fbDrawBmoMouth(cx, cy - 2, 14, 7, true, false);
  fbFillRoundRect(cx - 3, cy + 1, 6, 6, 2, C_BLACK);
  fbVLine(cx, cy + 2, 3, C_WHITE);
}

void fbDrawSnow(int cx, int cy, int size) {
  fbVLine(cx, cy - size, 2 * size + 1, C_BLACK);
  fbHLine(cx - size, cy, 2 * size + 1, C_BLACK);
  putPx(cx + 1, cy + 1, C_BLACK);
  putPx(cx - 1, cy - 1, C_BLACK);
  putPx(cx + 1, cy - 1, C_BLACK);
  putPx(cx - 1, cy + 1, C_BLACK);
}

void fbDrawSteam(int cx, int cy, int phase, uint8_t color) {
  int dy = phase % 6;
  fbFillEllipse(cx - 1, cy - dy,     2, 1, color);
  fbFillEllipse(cx + 1, cy - dy - 4, 2, 1, color);
}

void fbDrawQuestion(int cx, int cy, uint8_t color) {
  fbHLine(cx - 2, cy - 4, 4, color);
  putPx(cx + 2, cy - 3, color);
  putPx(cx + 1, cy - 2, color);
  putPx(cx, cy - 1, color);
  putPx(cx, cy + 2, color);
}

void fbDrawZ(int cx, int cy, int size, uint8_t color) {
  fbHLine(cx - size, cy - size, 2 * size + 1, color);
  fbHLine(cx - size, cy + size, 2 * size + 1, color);
  fbLine(cx + size, cy - size, cx - size, cy + size, 1, color);
}

void fbDrawZigzag(int cx, int cy, int width, int amp, int thickness, uint8_t color) {
  int half = width / 2;
  int seg = 4;
  int prevY = cy;
  int prevX = cx - half;
  for (int x = -half; x <= half; ++x) {
    int phase = ((x + half) / seg) % 2;
    int y = cy + (phase ? -amp : amp);
    if (x > -half) fbLine(prevX, prevY, cx + x, y, thickness, color);
    prevX = cx + x; prevY = y;
  }
}

void fbDrawPulseMouth(int cx, int cy, float phase) {
  float b = 0.5f + 0.5f * sinf(phase * 6.28318f);
  int rOuter = 4 + (int)(5 * b);
  fbFillCircle(cx, cy, rOuter, C_BLACK);
  if (rOuter > 2) fbFillCircle(cx, cy, rOuter - 2, C_WHITE);
  fbFillCircle(cx, cy, 1 + (int)(1 * (1.0f - b)), C_BLACK);
}

void fbDrawChatter(int cx, int cy, uint32_t now) {
  fbFillRoundRect(cx - 14, cy - 5, 28, 10, 3, C_BLACK);
  bool odd = ((now / 80) & 1) == 0;
  for (int i = -2; i <= 2; ++i) {
    int x = cx + i * 4;
    if ((i & 1) ^ (odd ? 0 : 1)) {
      fbVLine(x, cy - 2, 4, C_WHITE);
    }
  }
}

void fbDrawLaugh(int cx, int cy, float openness, uint32_t now) {
  int rx = 10;
  int ry = 3 + (int)(6 * openness);
  fbDrawBmoMouth(cx, cy, rx * 2, ry * 2, true, true);
  int t = (int)(now / 80) % 4;
  fbDrawZ(cx - 18, cy - 4 - t, 1, C_BLACK);
  fbDrawZ(cx + 18, cy - 4 - t, 1, C_BLACK);
}

void fbApplyGlitch(uint8_t intensity, uint32_t now) {
  if (intensity == 0) return;
  const int bands = 1 + intensity;
  for (int b = 0; b < bands; ++b) {
    uint32_t r = now * 2654435761u + (uint32_t)(b * 40503);
    int y0 = (int)(r % OLED_H);
    int h  = 1 + (int)((r >> 8) % 3);
    int shift = (int)((r >> 16) % (uint32_t)(2 + intensity * 2)) - 1;
    for (int y = y0; y < y0 + h && y < OLED_H; ++y) {
      for (int x = 0; x < OLED_W; ++x) {
        int srcX = ((x - shift) % OLED_W + OLED_W) % OLED_W;
        uint8_t px = (fb[srcX + (y >> 3) * OLED_W] >> (y & 7)) & 1;
        putPx(x, y, px ^ ((r >> 24) & 1));
      }
    }
  }
}

static void fbDrawListeningMarks(uint32_t now) {
  int phase = (int)((now / 120) % 3);
  for (int i = 0; i < 3; ++i) {
    int h = 3 + ((phase + i) % 3) * 2;
    int y = MOUTH_Y - h / 2;
    fbVLine(10 + i * 3, y, h, C_BLACK);
    fbVLine(OLED_W - 12 - i * 3, y, h, C_BLACK);
  }
}

static void fbDrawThinkingPips(uint32_t now) {
  const int phase = (int)((now / 180) % 4);
  const int baseX = MOUTH_CX + 14;
  const int baseY = EYE_Y - 8;
  for (int i = 0; i < 3; ++i) {
    const bool active = i == phase;
    const int x = baseX + i * 6;
    const int y = baseY - (active ? 2 : 0);
    fbFillCircle(x, y, active ? 2 : 1, C_BLACK);
  }
}

// -----------------------------------------------------------------------------
// Komposisi Frame Wajah BMO (Background Putih, Fitur Hitam)
// -----------------------------------------------------------------------------
void drawFaceToBuffer(const FaceState &s, uint32_t now) {
  displayClear();

  int leftCx  = MOUTH_CX - EYE_DX + s.shakeX;
  int rightCx = MOUTH_CX + EYE_DX + s.shakeX;
  int eyeY    = EYE_Y + s.shakeY;

  // Pipi Blush
  if (s.blush > 0.05f) {
    fbDrawBlush(leftCx - 10,  eyeY + 11, s.blush);
    fbDrawBlush(rightCx + 10, eyeY + 11, s.blush);
  }

  // Mata
  switch (s.eyeShape) {
    case FaceState::EYE_NORMAL:
      fbDrawEye(leftCx,  eyeY, EYE_W, EYE_H, s.lidL, s.pupilDx, s.pupilDy);
      fbDrawEye(rightCx, eyeY, EYE_W, EYE_H, s.lidR, s.pupilDx, s.pupilDy);
      break;
    case FaceState::EYE_HEART:
      fbDrawHeartEye(leftCx,  eyeY, 4, C_BLACK);
      fbDrawHeartEye(rightCx, eyeY, 4, C_BLACK);
      break;
    case FaceState::EYE_X:
      fbDrawXEye(leftCx,  eyeY, 4, C_BLACK);
      fbDrawXEye(rightCx, eyeY, 4, C_BLACK);
      break;
    case FaceState::EYE_SPIRAL: {
      float phase = (now % 1500) / 1500.0f * 6.28f;
      fbDrawSpiralEye(leftCx,  eyeY, 6, phase,         C_BLACK);
      fbDrawSpiralEye(rightCx, eyeY, 6, phase + 1.57f, C_BLACK);
      break;
    }
    case FaceState::EYE_SPINNER: {
      float phase = (now % 960) / 960.0f * 6.28f;
      fbDrawSpinnerEye(leftCx,  eyeY, 5, phase,        C_BLACK);
      fbDrawSpinnerEye(rightCx, eyeY, 5, phase + 0.8f, C_BLACK);
      break;
    }
    case FaceState::EYE_HALF_WINK_L:
      fbDrawEye(leftCx,  eyeY, EYE_W, EYE_H, 1.0f, s.pupilDx, s.pupilDy);
      fbDrawEye(rightCx, eyeY, EYE_W, EYE_H, s.lidR, s.pupilDx, s.pupilDy);
      break;
    case FaceState::EYE_HALF_WINK_R:
      fbDrawEye(leftCx,  eyeY, EYE_W, EYE_H, s.lidL, s.pupilDx, s.pupilDy);
      fbDrawEye(rightCx, eyeY, EYE_W, EYE_H, 1.0f, s.pupilDx, s.pupilDy);
      break;
    case FaceState::EYE_DOT:
      fbFillCircle(leftCx,  eyeY, 2, C_BLACK);
      fbFillCircle(rightCx, eyeY, 2, C_BLACK);
      break;
    case FaceState::EYE_SHADES:
      fbDrawShades(leftCx, rightCx, eyeY, now);
      break;
    case FaceState::EYE_SQUINT:
      fbFillRoundRect(leftCx  - EYE_W / 2, eyeY - 1, EYE_W, 3, 1, C_BLACK);
      fbFillRoundRect(rightCx - EYE_W / 2, eyeY - 1, EYE_W, 3, 1, C_BLACK);
      break;
    case FaceState::EYE_CRESCENT:
      fbDrawCrescentEye(leftCx,  eyeY + 2, 12, C_BLACK);
      fbDrawCrescentEye(rightCx, eyeY + 2, 12, C_BLACK);
      break;
  }

  // Alis
  if (s.browVisible) {
    fbDrawBrow(leftCx,  eyeY - EYE_H / 2 - 4, 12, s.browSlope, 2, C_BLACK);
    fbDrawBrow(rightCx, eyeY - EYE_H / 2 - 4, 12, -s.browSlope, 2, C_BLACK);
  }

  // Mulut
  if (s.pulseMouth) {
    float phase = (now % 1200) / 1200.0f;
    fbDrawPulseMouth(MOUTH_CX + s.shakeX, MOUTH_Y + s.shakeY, phase);
  } else {
    switch (s.mouth) {
      case FaceState::M_SMILE:
        fbDrawSmile(MOUTH_CX + s.shakeX, MOUTH_Y - 2 + s.shakeY, s.smileWidth, s.smileDip, 2, 1, C_BLACK);
        break;
      case FaceState::M_OPEN: {
        int rx = 6 + (int)(8 * s.mouthOpen);
        int ry = 3 + (int)(5 * s.mouthOpen);
        fbDrawOpenMouth(MOUTH_CX + s.shakeX, MOUTH_Y + s.shakeY, rx, ry, true);
        break;
      }
      case FaceState::M_FLAT:
        fbDrawFlatMouth(MOUTH_CX + s.shakeX, MOUTH_Y + s.shakeY, s.flatWidth, 2, C_BLACK);
        break;
      case FaceState::M_TALK: {
        uint8_t talkPhase = (uint8_t)((now / 120) % 3);
        switch (talkPhase) {
          case 0:  fbDrawFlatMouth(MOUTH_CX + s.shakeX, MOUTH_Y + s.shakeY, 14, 2, C_BLACK); break;
          case 1:  fbDrawOpenMouth(MOUTH_CX + s.shakeX, MOUTH_Y + s.shakeY, 5, 4, false); break;
          default: fbDrawOpenMouth(MOUTH_CX + s.shakeX, MOUTH_Y + s.shakeY, 9, 5 + (int)(2 * s.mouthOpen), true); break;
        }
        break;
      }
      case FaceState::M_FROWN:
        fbDrawFrown(MOUTH_CX + s.shakeX, MOUTH_Y + 2 + s.shakeY, 24, 5, 2, C_BLACK);
        break;
      case FaceState::M_GRIN:
        fbDrawSmile(MOUTH_CX + s.shakeX, MOUTH_Y - 2 + s.shakeY, 32, 7, 2, 2, C_BLACK);
        break;
      case FaceState::M_OH:
        fbDrawOpenMouth(MOUTH_CX + s.shakeX, MOUTH_Y + s.shakeY, 4, 5, false);
        break;
      case FaceState::M_TONGUE_OUT:
        fbDrawTongueOut(MOUTH_CX + s.shakeX, MOUTH_Y + 1 + s.shakeY);
        break;
      case FaceState::M_DROOL: {
        int ry = 3 + (int)(4 * s.mouthOpen);
        fbDrawBmoMouth(MOUTH_CX + s.shakeX, MOUTH_Y + s.shakeY, 16, ry * 2, true, false);
        int droolLen = 4 + (int)(4 * s.mouthOpen);
        fbDrawDrool(MOUTH_CX + s.shakeX - 1, MOUTH_Y + ry + 1, droolLen);
        break;
      }
      case FaceState::M_ZIGZAG:
        fbDrawZigzag(MOUTH_CX + s.shakeX, MOUTH_Y + s.shakeY, 24, 2, 2, C_BLACK);
        break;
      case FaceState::M_CHATTER:
        fbDrawChatter(MOUTH_CX + s.shakeX, MOUTH_Y + s.shakeY, now);
        break;
      case FaceState::M_LAUGH:
        fbDrawLaugh(MOUTH_CX + s.shakeX, MOUTH_Y + s.shakeY, s.mouthOpen, now);
        break;
    }
  }

  // Elemen Dekoratif & Partikel Statis
  if (s.tearVisible)   fbDrawTear(leftCx + 5, eyeY + EYE_H / 2 + 1, s.tearY, C_BLACK);
  if (s.sweatVisible)  fbDrawSweat(rightCx + 7, eyeY - EYE_H / 2 - 2, s.sweatY);
  if (s.starsVisible) {
    int t = (int)(now / 80) % 3;
    fbDrawStar(12  + t,  10, 3, C_BLACK);
    fbDrawStar(116 - t,  12, 3, C_BLACK);
  }
  if (s.heartsAround) {
    int t = (int)(now / 100) % 4;
    fbDrawHeart(14 + t,  12, 3, C_BLACK);
    fbDrawHeart(114 - t, 14, 3, C_BLACK);
  }
  if (s.questionMarks) {
    int phase = (int)(now / 200) % 3;
    fbDrawQuestion(16 + phase, 12, C_BLACK);
    fbDrawQuestion(112 - phase, 10, C_BLACK);
  }
  if (s.listeningMarks) fbDrawListeningMarks(now);
  if (s.thinkingDots)   fbDrawThinkingPips(now);
  if (s.snowFlakes) {
    int t = (int)(now / 100);
    fbDrawSnow(14 + (t % 6),  10 + ((t * 2) % 4), 2);
    fbDrawSnow(114 - (t % 5), 12 + ((t * 3) % 5), 2);
    fbDrawSnow(64,            8  + ((t * 4) % 4), 2);
  }
  if (s.steamLines) {
    int phase = (int)(now / 80);
    fbDrawSteam(16, 12, phase,     C_BLACK);
    fbDrawSteam(64, 10, phase + 2, C_BLACK);
    fbDrawSteam(112, 14, phase + 4, C_BLACK);
  }
  if (s.zzzLetters) {
    int t = (int)(now / 200) % 3;
    fbDrawZ(106 + t, 14 - t, 2, C_BLACK);
    fbDrawZ(116,     8,     3, C_BLACK);
  }
  if (s.glitchBars > 0) {
    for (uint8_t i = 0; i < s.glitchBars; ++i) {
      int y = (int)((now * 11 + i * 37) % OLED_H);
      fbHLine(0, y, OLED_W, C_BLACK);
    }
  }
  if (s.glitchShift > 0) {
    fbApplyGlitch(s.glitchShift, now);
  }
}

void renderFrame(const FaceState &s, uint32_t now) {
  drawFaceToBuffer(s, now);
  displayFlush();
}

void applyMoodTransition(FaceState &s, float t) {
  float edge = 1.0f;
  if (t < 0.12f)      edge = t / 0.12f;
  else if (t > 0.88f) edge = (1.0f - t) / 0.12f;
  if (edge >= 1.0f) return;
  if (edge < 0.0f)  edge = 0.0f;
  float lid = (1.0f - edge) * 0.18f;
  if (s.eyeShape == FaceState::EYE_NORMAL) {
    if (s.lidL < lid) s.lidL = lid;
    if (s.lidR < lid) s.lidR = lid;
  }
  s.shakeY += (int)((1.0f - edge) * 1.5f);
}

// -----------------------------------------------------------------------------
// Tabel Font Vektor Prosedural 5x7 (ASCII 32 ' ' s/d 93 ']') - 310 Byte Flash
// -----------------------------------------------------------------------------
static const uint8_t font5x7[][5] PROGMEM = {
  { 0x00, 0x00, 0x00, 0x00, 0x00 }, // 32 ' '
  { 0x00, 0x00, 0x5F, 0x00, 0x00 }, // 33 '!'
  { 0x00, 0x07, 0x00, 0x07, 0x00 }, // 34 '"'
  { 0x14, 0x7F, 0x14, 0x7F, 0x14 }, // 35 '#'
  { 0x24, 0x2A, 0x7F, 0x2A, 0x12 }, // 36 '$'
  { 0x23, 0x13, 0x08, 0x64, 0x62 }, // 37 '%'
  { 0x36, 0x49, 0x55, 0x22, 0x50 }, // 38 '&'
  { 0x00, 0x05, 0x03, 0x00, 0x00 }, // 39 '\''
  { 0x00, 0x1C, 0x22, 0x41, 0x00 }, // 40 '('
  { 0x00, 0x41, 0x22, 0x1C, 0x00 }, // 41 ')'
  { 0x14, 0x08, 0x3E, 0x08, 0x14 }, // 42 '*'
  { 0x08, 0x08, 0x3E, 0x08, 0x08 }, // 43 '+'
  { 0x00, 0x50, 0x30, 0x00, 0x00 }, // 44 ','
  { 0x08, 0x08, 0x08, 0x08, 0x08 }, // 45 '-'
  { 0x00, 0x60, 0x60, 0x00, 0x00 }, // 46 '.'
  { 0x20, 0x10, 0x08, 0x04, 0x02 }, // 47 '/'
  { 0x3E, 0x51, 0x49, 0x45, 0x3E }, // 48 '0'
  { 0x00, 0x42, 0x7F, 0x40, 0x00 }, // 49 '1'
  { 0x42, 0x61, 0x51, 0x49, 0x46 }, // 50 '2'
  { 0x21, 0x41, 0x45, 0x4B, 0x31 }, // 51 '3'
  { 0x18, 0x14, 0x12, 0x7F, 0x10 }, // 52 '4'
  { 0x27, 0x45, 0x45, 0x45, 0x39 }, // 53 '5'
  { 0x3C, 0x4A, 0x49, 0x49, 0x30 }, // 54 '6'
  { 0x01, 0x71, 0x09, 0x05, 0x03 }, // 55 '7'
  { 0x36, 0x49, 0x49, 0x49, 0x36 }, // 56 '8'
  { 0x06, 0x49, 0x49, 0x29, 0x1E }, // 57 '9'
  { 0x00, 0x36, 0x36, 0x00, 0x00 }, // 58 ':'
  { 0x00, 0x56, 0x36, 0x00, 0x00 }, // 59 ';'
  { 0x08, 0x14, 0x22, 0x41, 0x00 }, // 60 '<'
  { 0x14, 0x14, 0x14, 0x14, 0x14 }, // 61 '='
  { 0x00, 0x41, 0x22, 0x14, 0x08 }, // 62 '>'
  { 0x02, 0x01, 0x51, 0x09, 0x06 }, // 63 '?'
  { 0x32, 0x49, 0x79, 0x41, 0x3E }, // 64 '@'
  { 0x7E, 0x11, 0x11, 0x11, 0x7E }, // 65 'A'
  { 0x7F, 0x49, 0x49, 0x49, 0x36 }, // 66 'B'
  { 0x3E, 0x41, 0x41, 0x41, 0x22 }, // 67 'C'
  { 0x7F, 0x41, 0x41, 0x22, 0x1C }, // 68 'D'
  { 0x7F, 0x49, 0x49, 0x49, 0x41 }, // 69 'E'
  { 0x7F, 0x09, 0x09, 0x09, 0x01 }, // 70 'F'
  { 0x3E, 0x41, 0x49, 0x49, 0x7A }, // 71 'G'
  { 0x7F, 0x08, 0x08, 0x08, 0x7F }, // 72 'H'
  { 0x00, 0x41, 0x7F, 0x41, 0x00 }, // 73 'I'
  { 0x20, 0x40, 0x41, 0x3F, 0x01 }, // 74 'J'
  { 0x7F, 0x08, 0x14, 0x22, 0x41 }, // 75 'K'
  { 0x7F, 0x40, 0x40, 0x40, 0x40 }, // 76 'L'
  { 0x7F, 0x02, 0x0C, 0x02, 0x7F }, // 77 'M'
  { 0x7F, 0x04, 0x08, 0x10, 0x7F }, // 78 'N'
  { 0x3E, 0x41, 0x41, 0x41, 0x3E }, // 79 'O'
  { 0x7F, 0x09, 0x09, 0x09, 0x06 }, // 80 'P'
  { 0x3E, 0x41, 0x51, 0x21, 0x5E }, // 81 'Q'
  { 0x7F, 0x09, 0x19, 0x29, 0x46 }, // 82 'R'
  { 0x46, 0x49, 0x49, 0x49, 0x31 }, // 83 'S'
  { 0x01, 0x01, 0x7F, 0x01, 0x01 }, // 84 'T'
  { 0x3F, 0x40, 0x40, 0x40, 0x3F }, // 85 'U'
  { 0x1F, 0x20, 0x40, 0x20, 0x1F }, // 86 'V'
  { 0x3F, 0x40, 0x38, 0x40, 0x3F }, // 87 'W'
  { 0x63, 0x14, 0x08, 0x14, 0x63 }, // 88 'X'
  { 0x07, 0x08, 0x70, 0x08, 0x07 }, // 89 'Y'
  { 0x61, 0x51, 0x49, 0x45, 0x43 }, // 90 'Z'
  { 0x00, 0x7F, 0x41, 0x41, 0x00 }, // 91 '['
  { 0x02, 0x04, 0x08, 0x10, 0x20 }, // 92 '\\'
  { 0x00, 0x41, 0x41, 0x7F, 0x00 }, // 93 ']'
};

void fbDrawChar(int x, int y, char c, uint8_t color, uint8_t bg, uint8_t scale) {
  if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';
  if (c < 32 || c > 93) c = ' ';
  uint8_t idx = c - 32;

  for (int col = 0; col < 5; ++col) {
    uint8_t line = pgm_read_byte(&font5x7[idx][col]);
    for (int row = 0; row < 7; ++row) {
      if ((line >> row) & 1) {
        if (scale == 1) putPx(x + col, y + row, color);
        else fbFillRect(x + col * scale, y + row * scale, scale, scale, color);
      } else if (bg != 255) {
        if (scale == 1) putPx(x + col, y + row, bg);
        else fbFillRect(x + col * scale, y + row * scale, scale, scale, bg);
      }
    }
  }
}

void fbDrawString(int x, int y, const char* str, uint8_t color, uint8_t bg, uint8_t scale) {
  if (!str) return;
  while (*str) {
    fbDrawChar(x, y, *str, color, bg, scale);
    x += 6 * scale;
    str++;
  }
}

int fbGetStringWidth(const char* str, uint8_t scale) {
  if (!str) return 0;
  int len = 0;
  while (str[len]) len++;
  return (len > 0) ? (len * 6 * scale - scale) : 0;
}

void fbDrawLeftArrow(int x, int y, uint8_t color) {
  putPx(x,     y + 2, color);
  fbVLine(x + 1, y + 1, 3, color);
  fbVLine(x + 2, y,     5, color);
}

void fbDrawRightArrow(int x, int y, uint8_t color) {
  fbVLine(x,     y,     5, color);
  fbVLine(x + 1, y + 1, 3, color);
  putPx(x + 2,   y + 2, color);
}

void fbDrawCheckmark(int x, int y, uint8_t color) {
  putPx(x,     y + 2, color);
  putPx(x + 1, y + 3, color);
  putPx(x + 2, y + 4, color);
  putPx(x + 3, y + 2, color);
  putPx(x + 4, y,     color);
}

void fbDrawWifiIcon(int x, int y, uint8_t state, uint8_t color, uint32_t now) {
  // state: 0 = OFF, 1 = DISCONNECTED, 2 = CONNECTING, 3 = CONNECTED, 4 = FAILED
  if (state == 0) return;

  int cx = x + 4;

  // Animasi kedip (blinking) saat CONNECTING:
  if (state == 2) {
    if ((now / 350) % 2 == 0) {
      putPx(cx, y + 7, color);
      return;
    }
  }

  // 1. Titik Dasar (Base Dot)
  putPx(cx, y + 7, color);

  // 2. Lengkung 1 (Inner Arc - Radius ~2)
  putPx(cx - 1, y + 5, color);
  putPx(cx,     y + 5, color);
  putPx(cx + 1, y + 5, color);

  // 3. Lengkung 2 (Middle Arc - Radius ~4)
  putPx(cx - 3, y + 4, color);
  putPx(cx - 2, y + 3, color);
  putPx(cx - 1, y + 3, color);
  putPx(cx,     y + 3, color);
  putPx(cx + 1, y + 3, color);
  putPx(cx + 2, y + 3, color);
  putPx(cx + 3, y + 4, color);

  // 4. Lengkung 3 (Outer Arc - Radius ~6)
  putPx(cx - 4, y + 2, color);
  putPx(cx - 3, y + 1, color);
  putPx(cx - 2, y + 1, color);
  putPx(cx - 1, y + 1, color);
  putPx(cx,     y + 1, color);
  putPx(cx + 1, y + 1, color);
  putPx(cx + 2, y + 1, color);
  putPx(cx + 3, y + 1, color);
  putPx(cx + 4, y + 2, color);

  // 5. Silang (cross) jika terputus atau gagal
  if (state == 1 || state == 4) {
    fbLine(cx - 3, y + 1, cx + 3, y + 7, 1, color);
    fbLine(cx + 3, y + 1, cx - 3, y + 7, 1, color);
  }
}

// -----------------------------------------------------------------------------
// Ikon Cuaca Vektor Prosedural (Matahari, Awan, Hujan, Badai)
// -----------------------------------------------------------------------------
void fbDrawWeatherIcon(int x, int y, uint8_t kind, uint8_t color, uint32_t now) {
  // Kotak acuan: lebar 16 x tinggi 16 px
  if (kind == 0) {
    // 0: MATAHARI CERAH (SUN)
    int cx = x + 8;
    int cy = y + 8;
    fbFillCircle(cx, cy, 3, color);

    // 4 Sinar Utama (Atas, Bawah, Kiri, Kanan)
    fbVLine(cx, cy - 6, 2, color);
    fbVLine(cx, cy + 5, 2, color);
    fbHLine(cx - 6, cy, 2, color);
    fbHLine(cx + 5, cy, 2, color);

    // 4 Sinar Diagonal
    putPx(cx - 4, cy - 4, color);
    putPx(cx + 4, cy - 4, color);
    putPx(cx - 4, cy + 4, color);
    putPx(cx + 4, cy + 4, color);
  } else if (kind == 1) {
    // 1: AWAN MENDUNG (CLOUD)
    fbFillCircle(x + 5,  y + 9, 3, color);
    fbFillCircle(x + 9,  y + 6, 4, color);
    fbFillCircle(x + 13, y + 9, 3, color);
    fbFillRect(x + 3,    y + 9, 12, 4, color);
  } else if (kind == 2) {
    // 2: HUJAN (RAIN)
    // Awan di bagian atas (sedikit dinaikkan)
    fbFillCircle(x + 5,  y + 6, 3, color);
    fbFillCircle(x + 9,  y + 3, 4, color);
    fbFillCircle(x + 13, y + 6, 3, color);
    fbFillRect(x + 3,    y + 6, 12, 3, color);

    // Tetesan Air Hujan Miring Beranimasi
    int cycle = (now / 180) % 3;
    for (int i = 0; i < 3; ++i) {
      int rx = x + 4 + i * 4;
      int ry = y + 10 + ((i + cycle) % 3);
      putPx(rx,     ry,     color);
      putPx(rx - 1, ry + 1, color);
    }
  } else {
    // 3: BADAI PETIR (THUNDER)
    // Awan di atas
    fbFillCircle(x + 5,  y + 5, 3, color);
    fbFillCircle(x + 9,  y + 2, 4, color);
    fbFillCircle(x + 13, y + 5, 3, color);
    fbFillRect(x + 3,    y + 5, 12, 3, color);

    // Kilat Petir (Zigzag)
    int kx = x + 8;
    int ky = y + 9;
    fbLine(kx,     ky,     kx - 2, ky + 3, 1, color);
    fbLine(kx - 2, ky + 3, kx + 1, ky + 3, 1, color);
    fbLine(kx + 1, ky + 3, kx - 1, ky + 7, 1, color);
  }
}

// -----------------------------------------------------------------------------
// Ikon Not Musik Vektor (Spotify)
// -----------------------------------------------------------------------------
void fbDrawMusicNote(int x, int y, uint8_t color) {
  fbFillCircle(x + 2, y + 6, 2, color);
  fbVLine(x + 4, y + 1, 6, color);
  fbHLine(x + 4, y + 1, 3, color);
  fbVLine(x + 6, y + 2, 2, color);
}



