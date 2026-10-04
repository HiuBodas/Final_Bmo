// =============================================================================
// Game.h - Mini Game Arcade BMO: Procedural Jump Runner
// 100% Vektor Prosedural Tanpa Bitmaps (Layar OLED 128x64)
// Menampilkan mini-BMO melompati rintangan dengan skor & high score tersimpan
// =============================================================================
#pragma once
#include <Arduino.h>

void gameInit();
void gameOpen();
void gameClose();
bool gameIsActive();
void gameUpdate();
