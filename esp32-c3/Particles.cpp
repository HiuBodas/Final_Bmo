// =============================================================================
// Particles.cpp - Implementasi Efek Partikel Monokrom untuk Layar OLED 128x64
// =============================================================================
#include "Particles.h"

static Particle g_particles[MAX_PARTICLES];

void particlesClear() {
  for (int i = 0; i < MAX_PARTICLES; ++i) {
    g_particles[i].life = 0.0f;
  }
}

void particlesEmit(int count, int cx, int cy, uint8_t kind) {
  for (int i = 0; i < count && i < MAX_PARTICLES; ++i) {
    int slot = -1;
    for (int s = 0; s < MAX_PARTICLES; ++s) {
      if (g_particles[s].life <= 0.0f) { slot = s; break; }
    }
    if (slot < 0) return;

    float ang = (float)i / (float)count * 6.28318f + (millis() & 31) * 0.05f;
    float speed = 0.6f + (i & 3) * 0.2f;
    g_particles[slot] = {
      (float)cx, (float)cy,
      cosf(ang) * speed, sinf(ang) * speed - 0.2f,
      1.0f, kind
    };
  }
}

void particlesStep() {
  for (int i = 0; i < MAX_PARTICLES; ++i) {
    Particle &p = g_particles[i];
    if (p.life <= 0.0f) continue;
    p.x += p.vx;
    p.y += p.vy;
    p.vy += 0.04f;     // Gravitasi halus
    p.life -= 0.035f;  // Reduksi waktu hidup
  }
}

void particlesDraw() {
  for (int i = 0; i < MAX_PARTICLES; ++i) {
    Particle &p = g_particles[i];
    if (p.life <= 0.0f) continue;
    if (p.x < 0 || p.x >= OLED_W || p.y < 0 || p.y >= OLED_H) continue;

    int sz = 1 + (int)(2.5f * p.life);
    switch (p.kind) {
      case 0: // Bintang
        fbDrawStar((int)p.x, (int)p.y, sz, C_BLACK);
        break;
      case 1: // Hati
        fbDrawHeart((int)p.x, (int)p.y, sz, C_BLACK);
        break;
      case 2: // Tanda seru
        fbVLine((int)p.x, (int)p.y - sz, sz * 2, C_BLACK);
        putPx((int)p.x, (int)p.y + sz + 1, C_BLACK);
        break;
      case 3: // Titik kepulan pipi (blush puff)
        putPx((int)p.x, (int)p.y, C_BLACK);
        if (sz >= 2) putPx((int)p.x + 1, (int)p.y, C_BLACK);
        break;
    }
  }
}

void renderFrameWithParticles(const FaceState &s, uint32_t now) {
  drawFaceToBuffer(s, now);
  particlesDraw();
  displayFlush();
}
