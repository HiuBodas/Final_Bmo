// =============================================================================
// DoodleWidget.h - Live Drawing & Doodle Canvas Display di Layar OLED 128x64
// Menerima input gambar langsung dari aplikasi HP dan menampilkannya di BMO
// =============================================================================
#pragma once
#include <Arduino.h>

void doodleInit();
bool doodleIsActive();
void doodleOpen();
void doodleClose();
void doodleClear(bool whiteBackground = true);
void doodleDrawLine(int x0, int y0, int x1, int y1, uint8_t color);
void doodleSetPage(int pageIdx, const uint8_t* data, int len);
void doodleSetInvert(bool inv);
void doodleUpdate();
