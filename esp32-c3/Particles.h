// =============================================================================
// Particles.h - Sistem Efek Partikel Dinamis (Hati, Bintang, Kepulan Pipi)
// =============================================================================
#pragma once
#include <Arduino.h>
#include "Display.h"

struct Particle {
  float x, y;
  float vx, vy;
  float life;     // 1.0f -> 0.0f
  uint8_t kind;   // 0 = bintang, 1 = hati, 2 = tanda seru, 3 = titik
};

static constexpr int MAX_PARTICLES = 16;

void particlesClear();
void particlesEmit(int count, int cx, int cy, uint8_t kind);
void particlesStep();
void particlesDraw();
void renderFrameWithParticles(const FaceState &s, uint32_t now);
