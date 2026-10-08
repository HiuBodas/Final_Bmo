// =============================================================================
// Tamagotchi.h - Mode Virtual Pet BMO Sebagai Menu Mandiri
// 100% Vektor Prosedural Tanpa Bitmaps (Layar OLED 128x64)
// Menjadi fitur menu mandiri tanpa menggantikan mode normal / siklus ekspresi utama
// =============================================================================
#pragma once
#include <Arduino.h>

void tamagotchiInit();
void tamagotchiOpen();
void tamagotchiClose();
bool tamagotchiIsActive();
void tamagotchiUpdate();

int16_t tamagotchiGetHunger();
int16_t tamagotchiGetHappiness();
int16_t tamagotchiGetEnergy();
void tamagotchiFeed();
void tamagotchiPet();
