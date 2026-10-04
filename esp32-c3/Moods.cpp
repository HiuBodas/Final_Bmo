// =============================================================================
// Moods.cpp - Implementasi Animasi 26 Mood & Reaksi Sentuhan Monokrom (OLED)
// =============================================================================
#include "Moods.h"
#include "Particles.h"
#include "BleManager.h"

const char* moodName(Mood m) {
  switch (m) {
    case MOOD_COOL:     return "cool";
    case MOOD_HAPPY:    return "happy";
    case MOOD_LAUGH:    return "laugh";
    case MOOD_LOVE:     return "love";
    case MOOD_WINK:     return "wink";
    case MOOD_IDLE:     return "idle";
    case MOOD_BLINK:    return "blink";
    case MOOD_TALK:     return "talk";
    case MOOD_LISTEN:   return "listen";
    case MOOD_FOCUSED:  return "focused";
    case MOOD_THINKING: return "thinking";
    case MOOD_SURPRISE: return "surprise";
    case MOOD_EXCITED:  return "excited";
    case MOOD_HUNGRY:   return "hungry";
    case MOOD_COLD:     return "cold";
    case MOOD_HOT:      return "hot";
    case MOOD_SICK:     return "sick";
    case MOOD_DIZZY:    return "dizzy";
    case MOOD_GLITCH:   return "glitch";
    case MOOD_CONFUSED: return "confused";
    case MOOD_BORED:    return "bored";
    case MOOD_SAD:      return "sad";
    case MOOD_SCARED:   return "scared";
    case MOOD_ANGRY:    return "angry";
    case MOOD_WAKE:     return "wake";
    case MOOD_SLEEPY:   return "sleepy";
    default: return "?";
  }
}

// Lirikan sekilas mata ke arah posisi sentuhan
static void playReactionGlance(TouchKind k) {
  int dir = 0, up = 0;
  switch (k) {
    case TOUCH_QUICK_POKE: dir = -2; up = -1; break;
    case TOUCH_HOLD:       dir =  2; up =  0; break;
    case TOUCH_LONG_HOLD:  dir =  1; up = -1; break;
    case TOUCH_TICKLE:     dir = -1; up =  1; break;
    default: break;
  }
  uint32_t start = millis();
  while (millis() - start < 110) {
    uint32_t now = millis();
    float t = (now - start) / 110.0f;
    FaceState s;
    s.mouth = FaceState::M_SMILE;
    s.smileWidth = 24;
    s.smileDip = 4;
    s.pupilDx = (int)(dir * t);
    s.pupilDy = (int)(up * t);
    renderFrameWithParticles(s, now);
    delay(18);
  }
}

void playReaction(TouchKind k) {
  particlesClear();
  playReactionGlance(k);

  switch (k) {
    // -------------------------------------------------------------------------
    // 1. QUICK POKE: Kaget melompat, melirik kiri-kanan cari pelaku, senyum lega
    // -------------------------------------------------------------------------
    case TOUCH_QUICK_POKE: {
      // Fase A - Squash / antisipasi tertekan (80ms)
      uint32_t pA = millis();
      while (millis() - pA < 80) {
        uint32_t now = millis();
        float t = (now - pA) / 80.0f;
        FaceState s;
        s.mouth = FaceState::M_FLAT;
        s.lidL = s.lidR = 0.2f - 0.2f * t;
        s.shakeY = (int)(2 * t);
        renderFrameWithParticles(s, now);
        delay(15);
      }

      // Fase B - Recoil kaget melompat ke atas (220ms)
      uint32_t pB = millis();
      while (millis() - pB < 220) {
        uint32_t now = millis();
        float t = (now - pB) / 220.0f;
        float ease = 1.0f - (1.0f - t) * (1.0f - t);
        FaceState s;
        s.mouth = FaceState::M_OH;
        s.lidL = s.lidR = 0;
        s.shakeY = -(int)(6 * (1 - ease));
        s.shakeX = (int)(2 * sinf(now * 0.05f));
        s.pupilDy = -1;
        particlesStep();
        renderFrameWithParticles(s, now);
        delay(18);
      }

      // Fase C - Mata melirik cepat mencari siapa yang mencolek (600ms)
      uint32_t pC = millis();
      while (millis() - pC < 600) {
        uint32_t now = millis();
        FaceState s;
        s.mouth = FaceState::M_OH;
        s.lidL = s.lidR = 0;
        float ph = (now - pC) / 200.0f;
        if (ph < 1)      s.pupilDx = -2;
        else if (ph < 2) s.pupilDx =  2;
        else             s.pupilDx =  0;
        s.pupilDy = (int)(sinf(now * 0.02f));
        s.shakeY  = (int)(1 * cosf(now * 0.02f));
        particlesStep();
        renderFrameWithParticles(s, now);
        delay(20);
      }

      // Fase D - Kedip lega & senyum tersipu (700ms)
      uint32_t pD = millis();
      while (millis() - pD < 700) {
        uint32_t now = millis();
        float t = (now - pD) / 700.0f;
        FaceState s;
        s.mouth = FaceState::M_SMILE;
        if (t < 0.15f)      s.lidL = s.lidR = 1.0f;
        else if (t < 0.30f) s.lidL = s.lidR = 0.2f;
        else if (t < 0.42f) s.lidL = s.lidR = 1.0f;
        else                s.lidL = s.lidR = 0.2f;
        s.blush = 0.5f * t;
        s.shakeY = (int)(1 * sinf(now * 0.008f));
        renderFrameWithParticles(s, now);
        delay(25);
      }
      break;
    }

    // -------------------------------------------------------------------------
    // 2. HOLD / PET: Dielus manis, mata sabit santai, mendengkur, blush puff
    // -------------------------------------------------------------------------
    case TOUCH_HOLD: {
      // Fase A - Melirik ke arah sentuhan (220ms)
      uint32_t pA = millis();
      while (millis() - pA < 220) {
        uint32_t now = millis();
        float t = (now - pA) / 220.0f;
        FaceState s;
        s.mouth = FaceState::M_SMILE;
        s.lidL = s.lidR = 0.2f * t;
        s.pupilDx = (int)(2 * t);
        s.blush = 0.3f * t;
        renderFrameWithParticles(s, now);
        delay(20);
      }

      // Fase B - Menutup mata bulan sabit rileks (800ms)
      uint32_t pB = millis();
      while (millis() - pB < 800) {
        uint32_t now = millis();
        float t = (now - pB) / 800.0f;
        FaceState s;
        s.eyeShape = FaceState::EYE_CRESCENT;
        s.mouth = FaceState::M_SMILE;
        s.smileWidth = 24 + (int)(3 * t);
        s.smileDip = 4 + (int)(2 * t);
        s.blush = 0.25f + 0.4f * t;
        s.pupilDx = 2 - (int)(2 * t);
        s.shakeX = (int)(1 * sinf(now * 0.005f));
        renderFrameWithParticles(s, now);
        delay(22);
      }

      // Fase C - Mendengkur puas & kepulan partikel blush puff (1500ms)
      uint32_t pC = millis();
      uint32_t lastPuff = 0;
      while (millis() - pC < 1500) {
        uint32_t now = millis();
        if (now - lastPuff > 420) {
          particlesEmit(1, 26,           34, 3);
          particlesEmit(1, OLED_W - 26,  34, 3);
          lastPuff = now;
        }
        FaceState s;
        s.eyeShape = FaceState::EYE_CRESCENT;
        s.mouth = FaceState::M_SMILE;
        s.smileWidth = 26 + (int)(2 * sinf(now * 0.006f));
        s.smileDip = 5;
        s.blush = 0.65f;
        s.shakeX = (int)(1 * sinf(now * 0.004f));
        s.shakeY = (int)(1 * sinf(now * 0.006f));
        particlesStep();
        renderFrameWithParticles(s, now);
        delay(28);
        pollTouch();
        if (g_touchPending) goto reactionDone;
      }
      break;
    }

    // -------------------------------------------------------------------------
    // 3. LONG HOLD / LOVE: Ledakan Hati (HEART BURST) & MATA HATI!
    // -------------------------------------------------------------------------
    case TOUCH_LONG_HOLD: {
      // Fase A - Belaian kasih sayang mendalam (1000ms)
      uint32_t pA = millis();
      while (millis() - pA < 1000) {
        uint32_t now = millis();
        float t = (now - pA) / 1000.0f;
        FaceState s;
        s.eyeShape = FaceState::EYE_CRESCENT;
        s.mouth = FaceState::M_SMILE;
        s.smileWidth = 24 + (int)(4 * t);
        s.smileDip = 4 + (int)(2 * t);
        s.blush = 0.35f + 0.35f * t;
        s.shakeX = (int)(1 * sinf(now * 0.005f));
        renderFrameWithParticles(s, now);
        delay(25);
      }

      // Fase B - Tubuh terangkat antisipasi (700ms)
      uint32_t pB = millis();
      while (millis() - pB < 700) {
        uint32_t now = millis();
        float t = (now - pB) / 700.0f;
        float ease = t * t;
        FaceState s;
        s.eyeShape = FaceState::EYE_CRESCENT;
        s.mouth = FaceState::M_SMILE;
        s.smileWidth = 26 + (int)(3 * sinf(now * 0.012f));
        s.smileDip = 5;
        s.blush = 0.7f;
        s.shakeY = -(int)(2 * ease);
        renderFrameWithParticles(s, now);
        delay(28);
      }

      // Fase C - LEDAKAN PARTIKEL HATI (HEART BURST) (250ms)
      particlesEmit(8, MOUTH_CX, MOUTH_Y - 4, 1);
      uint32_t pC = millis();
      while (millis() - pC < 250) {
        uint32_t now = millis();
        FaceState s;
        s.mouth = FaceState::M_OH;
        s.lidL = s.lidR = 0;
        s.shakeY = -2;
        particlesStep();
        renderFrameWithParticles(s, now);
        delay(20);
      }

      // Fase D - MATA HATI (HEART EYES) (1700ms)
      uint32_t pD = millis();
      uint32_t lastHeart = 0;
      while (millis() - pD < 1700) {
        uint32_t now = millis();
        float t = (now - pD) / 1700.0f;
        if (now - lastHeart > 450) {
          particlesEmit(1, 24 + ((int)(now / 100) % 6), 46, 1);
          particlesEmit(1, OLED_W - 24 - ((int)(now / 100) % 6), 46, 1);
          lastHeart = now;
        }
        FaceState s;
        s.eyeShape = (t < 0.65f) ? FaceState::EYE_HEART : FaceState::EYE_CRESCENT;
        s.mouth = FaceState::M_SMILE;
        s.smileWidth = 26 + (int)(2 * sinf(now * 0.008f));
        s.smileDip = 5;
        s.blush = 0.85f;
        s.shakeY = -(int)(1 + 1.0f * sinf(now * 0.008f));
        particlesStep();
        renderFrameWithParticles(s, now);
        delay(28);
        pollTouch();
        if (g_touchPending) goto reactionDone;
      }
      break;
    }

    // -------------------------------------------------------------------------
    // 4. TICKLE: Tertawa terbahak-bahak, konfeti bintang, melompat-lompat
    // -------------------------------------------------------------------------
    case TOUCH_TICKLE: {
      // Fase A - Letupan tawa kaget & konfeti bintang (350ms)
      particlesEmit(6, MOUTH_CX, MOUTH_Y - 10, 0);
      uint32_t pA = millis();
      while (millis() - pA < 350) {
        uint32_t now = millis();
        FaceState s;
        s.eyeShape = FaceState::EYE_CRESCENT;
        s.lidL = s.lidR = 0.4f + 0.2f * sinf(now * 0.04f);
        s.mouth = FaceState::M_LAUGH;
        s.mouthOpen = 0.9f;
        s.blush = 1.0f;
        s.shakeX = (int)(3 * sinf(now * 0.04f));
        s.shakeY = (int)(2 * fabsf(sinf(now * 0.04f)));
        particlesStep();
        renderFrameWithParticles(s, now);
        delay(20);
      }

      // Fase B - Tertawa terbahak-bahak melompat (1900ms)
      uint32_t lastStar = 0;
      uint32_t pB = millis();
      while (millis() - pB < 1900) {
        uint32_t now = millis();
        float wave = sinf(now * 0.025f);
        if (wave > 0.75f && now - lastStar > 220) {
          particlesEmit(1, 15 + (rand() % 98), 10 + (rand() % 20), 0);
          lastStar = now;
        }
        FaceState s;
        s.eyeShape = FaceState::EYE_CRESCENT;
        s.lidL = s.lidR = 0.55f + 0.15f * fabsf(wave);
        s.mouth = FaceState::M_LAUGH;
        s.mouthOpen = 0.6f + 0.4f * fabsf(wave);
        s.blush = 1.0f;
        s.shakeX = (int)(3 * sinf(now * 0.03f));
        s.shakeY = (int)(2 * fabsf(sinf(now * 0.03f)));
        particlesStep();
        renderFrameWithParticles(s, now);
        delay(22);
        pollTouch();
        if (g_touchPending) goto reactionDone;
      }

      // Fase C - Rebahan lemas setelah tertawa puas (300ms)
      uint32_t pC = millis();
      while (millis() - pC < 300) {
        uint32_t now = millis();
        float t = (now - pC) / 300.0f;
        FaceState s;
        s.eyeShape = FaceState::EYE_CRESCENT;
        s.lidL = s.lidR = 0.55f + 0.15f * (1 - t);
        s.mouth = FaceState::M_OPEN;
        s.mouthOpen = 0.4f - 0.3f * t;
        s.blush = 1.0f;
        s.shakeY = (int)(1 * t);
        particlesStep();
        renderFrameWithParticles(s, now);
        delay(25);
      }
      break;
    }

    default: break;
  }

reactionDone:
  // Settle (250ms)
  uint32_t settleStart = millis();
  while (millis() - settleStart < 250) {
    uint32_t now = millis();
    float t = (now - settleStart) / 250.0f;
    FaceState s;
    s.eyeShape = FaceState::EYE_CRESCENT;
    s.mouth = FaceState::M_SMILE;
    s.smileWidth = 24;
    s.lidL = s.lidR = 0.2f + 0.05f * sinf(now * 0.01f);
    s.blush = 0.3f + 0.2f * (1 - t);
    particlesStep();
    renderFrameWithParticles(s, now);
    delay(28);
  }
  particlesClear();
}

// Generator parameter wajah prosedural untuk 26 mood ekspresi wajah BMO
void getMoodFaceState(Mood m, float t, uint32_t now, FaceState &s) {
  switch (m) {
      case MOOD_COOL: {
        s.eyeShape = FaceState::EYE_SHADES;
        s.mouth = FaceState::M_SMILE;
        s.smileWidth = 32;
        s.smileDip = 4;
        s.shakeX = (int)(1 * sinf(now * 0.003f));
        s.shakeY = (int)(1 * sinf(now * 0.004f));
        break;
      }
      case MOOD_HAPPY: {
        s.eyeShape = FaceState::EYE_CRESCENT;
        s.mouth = FaceState::M_OPEN;
        s.mouthOpen = 0.6f + 0.4f * sinf(now * 0.012f);
        s.blush = 1.0f;
        break;
      }
      case MOOD_LAUGH: {
        s.eyeShape = FaceState::EYE_CRESCENT;
        s.mouth = FaceState::M_LAUGH;
        s.mouthOpen = 0.7f + 0.3f * sinf(now * 0.025f);
        s.shakeY = (int)(2 * fabsf(sinf(now * 0.025f)));
        s.blush = 1.0f;
        break;
      }
      case MOOD_LOVE: {
        s.eyeShape = FaceState::EYE_HEART;
        s.mouth = FaceState::M_OPEN;
        s.mouthOpen = 0.5f + 0.4f * sinf(now * 0.012f);
        s.blush = 1.0f;
        s.heartsAround = true;
        break;
      }
      case MOOD_WINK: {
        s.mouth = FaceState::M_GRIN;
        if (t < 0.7f) s.eyeShape = FaceState::EYE_HALF_WINK_R;
        s.blush = 0.5f;
        break;
      }
      case MOOD_IDLE: {
        s.mouth = FaceState::M_OPEN;
        s.mouthOpen = 0.55f + 0.05f * sinf(now * 0.0013f);
        s.pupilDx = (int)(1 * sinf(now * 0.0011f));
        s.pupilDy = (int)(1 * sinf(now * 0.0007f));
        uint32_t bp = now % 5200;
        if (bp < 130) {
          float p = bp / 130.0f;
          float lid = (p < 0.5f) ? p * 2 : (1 - p) * 2;
          s.lidL = s.lidR = lid;
        }
        break;
      }
      case MOOD_BLINK: {
        s.mouth = FaceState::M_SMILE;
        float p = t;
        s.lidL = s.lidR = (p < 0.5f) ? p * 2 : (1 - p) * 2;
        break;
      }
      case MOOD_TALK: {
        s.mouth = FaceState::M_TALK;
        s.mouthOpen = 0.5f + 0.5f * sinf(now * 0.018f);
        uint8_t talkPhase = (uint8_t)((now / 120) % 3);
        if (talkPhase == 0) s.pupilDx = -1;
        else if (talkPhase == 1) s.pupilDx = 1;
        s.pupilDy = (int)(1 * sinf(now * 0.01f));
        break;
      }
      case MOOD_LISTEN: {
        s.mouth = FaceState::M_OPEN;
        s.mouthOpen = 0.18f + 0.06f * fabsf(sinf(now * 0.010f));
        s.pupilDx = (int)(1 * sinf(now * 0.005f));
        s.listeningMarks = true;
        break;
      }
      case MOOD_FOCUSED: {
        s.eyeShape = FaceState::EYE_SQUINT;
        s.mouth = FaceState::M_FLAT;
        s.browVisible = true;
        s.browSlope = 4;
        s.sweatVisible = true;
        s.sweatY = (int)((now / 100) % 8);
        break;
      }
      case MOOD_THINKING: {
        s.eyeShape = FaceState::EYE_SPINNER;
        s.mouth = FaceState::M_FLAT;
        s.flatWidth = 10 + (int)(2 * fabsf(sinf(now * 0.004f)));
        s.thinkingDots = true;
        break;
      }
      case MOOD_SURPRISE: {
        s.mouth = FaceState::M_OH;
        s.lidL = s.lidR = 0;
        s.pupilDx = (int)(1 * sinf(now * 0.05f));
        s.pupilDy = (int)(1 * cosf(now * 0.05f));
        break;
      }
      case MOOD_EXCITED: {
        s.eyeShape = FaceState::EYE_CRESCENT;
        s.mouth = FaceState::M_GRIN;
        s.starsVisible = true;
        s.shakeY = -(int)(2 * fabsf(sinf(now * 0.012f)));
        s.blush = 0.8f;
        break;
      }
      case MOOD_HUNGRY: {
        s.mouth = FaceState::M_DROOL;
        s.mouthOpen = 0.6f + 0.4f * sinf(now * 0.008f);
        s.pupilDy = 2;
        s.lidL = s.lidR = 0.2f;
        s.blush = 0.4f;
        break;
      }
      case MOOD_COLD: {
        s.mouth = FaceState::M_CHATTER;
        s.lidL = s.lidR = 0.6f;
        s.shakeX = (int)((now / 60) % 3) - 1;
        s.shakeY = (int)((now / 40) % 3) - 1;
        s.snowFlakes = true;
        break;
      }
      case MOOD_HOT: {
        s.mouth = FaceState::M_OPEN;
        s.mouthOpen = 0.6f + 0.3f * sinf(now * 0.005f);
        s.lidL = s.lidR = 0.4f;
        s.steamLines = true;
        s.sweatVisible = true;
        s.sweatY = (int)((now / 60) % 12);
        s.blush = 0.7f;
        break;
      }
      case MOOD_SICK: {
        s.mouth = FaceState::M_ZIGZAG;
        s.lidL = s.lidR = 0.55f;
        s.pupilDx = (int)(1 * sinf(now * 0.003f));
        s.pupilDy = 1;
        s.browVisible = true;
        s.browSlope = -3;
        break;
      }
      case MOOD_DIZZY: {
        s.eyeShape = FaceState::EYE_SPIRAL;
        s.mouth = FaceState::M_OPEN;
        s.mouthOpen = 0.4f + 0.3f * sinf(now * 0.02f);
        s.shakeX = (int)(2 * sinf(now * 0.008f));
        break;
      }
      case MOOD_GLITCH: {
        s.mouth = FaceState::M_FLAT;
        s.lidL = s.lidR = 0;
        s.glitchBars = 3;
        if (((int)(now / 200) & 1) == 0) s.eyeShape = FaceState::EYE_X;
        s.shakeX = (int)((now * 7) % 5) - 2;
        break;
      }
      case MOOD_CONFUSED: {
        s.eyeShape = FaceState::EYE_DOT;
        s.mouth = FaceState::M_ZIGZAG;
        s.questionMarks = true;
        s.shakeX = (int)(1 * sinf(now * 0.002f));
        s.browVisible = true;
        s.browSlope = -2;
        break;
      }
      case MOOD_BORED: {
        s.lidL = s.lidR = 0.7f;
        s.pupilDx = -2;
        s.mouth = FaceState::M_FLAT;
        s.zzzLetters = ((now / 800) % 3 == 0);
        break;
      }
      case MOOD_SAD: {
        s.mouth = FaceState::M_FROWN;
        s.browVisible = true;
        s.browSlope = -5;
        s.lidL = s.lidR = 0.5f;
        float tearCycle = fmodf(t * 1.5f, 1.0f);
        s.tearVisible = (tearCycle < 0.6f);
        s.tearY = (int)(tearCycle * 12);
        s.pupilDy = 1;
        break;
      }
      case MOOD_SCARED: {
        s.lidL = s.lidR = 0;
        s.pupilDx = (int)(1 * sinf(now * 0.04f));
        s.pupilDy = (int)(1 * cosf(now * 0.04f));
        s.mouth = FaceState::M_FROWN;
        s.sweatVisible = true;
        s.sweatY = (int)((now / 80) % 10);
        s.shakeX = (int)((now / 50) % 3) - 1;
        s.shakeY = (int)((now / 70) % 3) - 1;
        s.browVisible = true;
        s.browSlope = -4;
        break;
      }
      case MOOD_ANGRY: {
        s.mouth = FaceState::M_FLAT;
        s.browVisible = true;
        s.browSlope = 6;
        s.shakeX = (int)(1 * sinf(now * 0.05f));
        s.pupilDx = -1;
        break;
      }
      case MOOD_WAKE: {
        s.mouth = FaceState::M_OPEN;
        if (t < 0.4f) {
          s.mouthOpen = t / 0.4f;
          s.lidL = s.lidR = 1.0f - t / 0.4f;
        } else if (t < 0.7f) {
          s.mouthOpen = 1.0f;
          s.lidL = s.lidR = 0;
        } else {
          float p = (t - 0.7f) / 0.3f;
          s.mouthOpen = 1.0f - p;
          s.lidL = s.lidR = 0;
        }
        break;
      }
      case MOOD_SLEEPY: {
        s.mouth = FaceState::M_FLAT;
        float breathe = (sinf(now * 0.002f) + 1) * 0.5f;
        s.lidL = s.lidR = 0.75f + 0.2f * breathe;
        break;
      }
      default: break;
    }
  }

void playMood(Mood m, uint32_t durationMs) {
  uint32_t start = millis();
  Serial.printf("[mood] %s (%u ms)\n", moodName(m), (unsigned)durationMs);

  while (millis() - start < durationMs) {
    uint32_t now = millis();
    float t = (now - start) / (float)durationMs;
    FaceState s;
    getMoodFaceState(m, t, now, s);

    applyMoodTransition(s, t);
    renderFrame(s, now);

    // Non-blocking wait: lakukan polling touch setiap 2ms selama jeda frame
    uint32_t waitStart = millis();
    while (millis() - waitStart < 30) {
      pollTouch();
      if (g_touchPending || g_navPending || bleNotifIsActive() || bleHasPendingAction()) break;
      delay(2);
    }

    // Instant touch / BLE notification interrupt: langsung merespons seketika!
    if (g_touchPending || g_navPending || bleNotifIsActive() || bleHasPendingAction()) break;
  }
}

// =============================================================================
// Personality Engine ala Dasai Mochi (Living Idle, Rantai Emosi, & Kedipan Alami)
// =============================================================================

// Kedipan mata organik alami khas Dasai Mochi (tutup halus -> buka halus)
void playMochiBlink(uint8_t count) {
  for (uint8_t c = 0; c < count; ++c) {
    // Fase tutup mata (55 ms)
    uint32_t t0 = millis();
    while (millis() - t0 < 55) {
      uint32_t now = millis();
      float p = (now - t0) / 55.0f;
      FaceState s;
      s.mouth = FaceState::M_SMILE;
      s.smileWidth = 20;
      s.smileDip = 3;
      s.lidL = s.lidR = p;
      renderFrame(s, now);
      pollTouch();
      if (g_touchPending || g_navPending || bleNotifIsActive()) return;
      delay(2);
    }
    // Fase buka mata (55 ms)
    t0 = millis();
    while (millis() - t0 < 55) {
      uint32_t now = millis();
      float p = 1.0f - (now - t0) / 55.0f;
      FaceState s;
      s.mouth = FaceState::M_SMILE;
      s.smileWidth = 20;
      s.smileDip = 3;
      s.lidL = s.lidR = p;
      renderFrame(s, now);
      pollTouch();
      if (g_touchPending || g_navPending || bleNotifIsActive()) return;
      delay(2);
    }
    if (c + 1 < count) delay(70);
  }
}

// Home State Hidup: Bernafas halus, melirik kiri-kanan-tengah, dan kedipan spontan (Tempo Santai)
void playMochiHomeBreath(uint32_t durationMs) {
  uint32_t start = millis();
  int glanceX = 0;
  int glanceY = 0;
  uint32_t lastGlance = start;
  uint32_t nextGlanceInterval = 2500 + random(2500); // Lirik tiap 2.5 - 5 detik
  uint32_t lastBlink = start;
  uint32_t nextBlinkInterval = 2500 + random(3500);  // Kedip tiap 2.5 - 6 detik

  while (millis() - start < durationMs) {
    uint32_t now = millis();

    // 1. Kedipan spontan berkala (kadang 1x kedip, kadang kedip ganda)
    if (now - lastBlink > nextBlinkInterval) {
      uint8_t blinks = (random(4) == 0) ? 2 : 1;
      playMochiBlink(blinks);
      if (g_touchPending || bleNotifIsActive()) return;
      lastBlink = millis();
      nextBlinkInterval = 2500 + random(3500);
      continue;
    }

    // 2. Micro glance: lirikan mata acak natural ke kiri/kanan/atas/tengah
    if (now - lastGlance > nextGlanceInterval) {
      int r = random(5);
      if (r == 0)      { glanceX = -2; glanceY =  0; } // Lirik kiri
      else if (r == 1) { glanceX =  2; glanceY =  0; } // Lirik kanan
      else if (r == 2) { glanceX =  1; glanceY = -1; } // Lirik atas kanan
      else             { glanceX =  0; glanceY =  0; } // Tengah
      lastGlance = now;
      nextGlanceInterval = 2000 + random(2500);
    }

    FaceState s;
    s.mouth = FaceState::M_SMILE;
    s.smileWidth = 22;
    s.smileDip = 3;
    s.pupilDx = glanceX;
    s.pupilDy = glanceY;

    // Nafas sinusoidal tenang & halus
    float breathe = sinf(now * 0.002f);
    s.shakeY = (int)(1.0f * breathe);

    renderFrame(s, now);

    // Fast non-blocking polling sensor sentuh
    uint32_t wStart = millis();
    while (millis() - wStart < 25) {
      pollTouch();
      if (g_touchPending || g_navPending || bleNotifIsActive()) return;
      delay(2);
    }
  }
}

// Personality Engine Utama Dasai Mochi (Tempo Santai & Elegan)
void bmoStepMochiPersonality() {
  if (g_navPending || bleNotifIsActive()) return;

  // Tangani jika ada sentuhan yang belum diproses
  if (g_touchPending) {
    g_touchPending = false;
    playReaction(g_pendingKind);
    while (g_touchPending) {
      g_touchPending = false;
      playReaction(g_pendingKind);
    }
    return;
  }

  // Pilih mode aksi Personality Engine:
  // 0..54  (55%): Living Idle Home State (Bernafas tenang, lirik pelan, kedip natural 4.5s - 8s)
  // 55..79 (25%): Rantai Suasana Hati / Mood Chaining (3.0s - 5.5s per mood)
  // 80..99 (20%): Ekspresi Tunggal Spontan Berbobot (4.0s - 6.0s)
  int dice = random(100);

  if (dice < 55) {
    // -------------------------------------------------------------------------
    // Mode A: Living Idle Home State (Santai, Bernafas & Melirik Perlahan)
    // -------------------------------------------------------------------------
    uint32_t idleDur = 4500 + random(3500); // 4.5s - 8.0s
    playMochiHomeBreath(idleDur);
  } else if (dice < 80) {
    // -------------------------------------------------------------------------
    // Mode B: Rantai Suasana Hati (Thematic Mood Chaining - Tempo Santai)
    // -------------------------------------------------------------------------
    struct MoodStep {
      Mood m;
      uint32_t ms;
    };

    // 1. Rantai Ngantuk: Bosan -> Menguap -> Tidur Sayu
    static const MoodStep chainSleepy[] = {
      { MOOD_IDLE,    3000 },
      { MOOD_BORED,   3500 },
      { MOOD_WAKE,    3200 },
      { MOOD_SLEEPY,  4500 }
    };

    // 2. Rantai Cerdas & Keren: Mikir -> Ide Bintang -> Kacamata Pit Viper
    static const MoodStep chainCool[] = {
      { MOOD_THINKING, 3500 },
      { MOOD_EXCITED,  3000 },
      { MOOD_COOL,     5500 }
    };

    // 3. Rantai Konyol / Lapar: Lapar Liur -> Bingung -> Ketawa Puas
    static const MoodStep chainFunny[] = {
      { MOOD_HUNGRY,   3500 },
      { MOOD_CONFUSED, 3200 },
      { MOOD_LAUGH,    4000 }
    };

    // 4. Rantai Cuaca / Kondisi: Kepanasan -> Kedinginan Menggigil -> Sakit
    static const MoodStep chainWeather[] = {
      { MOOD_HOT,      3500 },
      { MOOD_COLD,     3500 },
      { MOOD_SICK,     4000 }
    };

    // 5. Rantai Sayang / Manis: Senyum Manis -> Kedip Nakal -> Mata Hati
    static const MoodStep chainLove[] = {
      { MOOD_HAPPY,    3500 },
      { MOOD_WINK,     2500 },
      { MOOD_LOVE,     4500 }
    };

    // 6. Rantai Siaga / Komunikasi: Menyimak -> Fokus Serius -> Bicara
    static const MoodStep chainAlert[] = {
      { MOOD_LISTEN,   3000 },
      { MOOD_FOCUSED,  3200 },
      { MOOD_TALK,     3800 }
    };

    // 7. Rantai Dramatis: Kaget -> Takut Gemetar -> Glitch Rusak -> Pusing Spiral
    static const MoodStep chainDrama[] = {
      { MOOD_SURPRISE, 2200 },
      { MOOD_SCARED,   3000 },
      { MOOD_GLITCH,   1800 },
      { MOOD_DIZZY,    3500 }
    };

    const MoodStep* selectedChain = nullptr;
    size_t chainLen = 0;
    int cPick = random(7);

    switch (cPick) {
      case 0: selectedChain = chainSleepy;  chainLen = sizeof(chainSleepy)/sizeof(chainSleepy[0]); break;
      case 1: selectedChain = chainCool;    chainLen = sizeof(chainCool)/sizeof(chainCool[0]); break;
      case 2: selectedChain = chainFunny;   chainLen = sizeof(chainFunny)/sizeof(chainFunny[0]); break;
      case 3: selectedChain = chainWeather; chainLen = sizeof(chainWeather)/sizeof(chainWeather[0]); break;
      case 4: selectedChain = chainLove;    chainLen = sizeof(chainLove)/sizeof(chainLove[0]); break;
      case 5: selectedChain = chainAlert;   chainLen = sizeof(chainAlert)/sizeof(chainAlert[0]); break;
      default: selectedChain = chainDrama;  chainLen = sizeof(chainDrama)/sizeof(chainDrama[0]); break;
    }

    for (size_t i = 0; i < chainLen; ++i) {
      if (g_touchPending || g_navPending) break;
      playMochiBlink(1);
      playMood(selectedChain[i].m, selectedChain[i].ms);
    }
  } else {
    // -------------------------------------------------------------------------
    // Mode C: Ekspresi Tunggal Spontan Berbobot (Puas Dinikmati 4s - 6s)
    // -------------------------------------------------------------------------
    static const Mood weightedPool[] = {
      MOOD_COOL, MOOD_COOL, MOOD_COOL,
      MOOD_HAPPY, MOOD_HAPPY,
      MOOD_LOVE, MOOD_LOVE,
      MOOD_WINK, MOOD_WINK,
      MOOD_LAUGH, MOOD_LAUGH,
      MOOD_IDLE, MOOD_IDLE,
      MOOD_TALK, MOOD_TALK,
      MOOD_EXCITED,
      MOOD_THINKING,
      MOOD_LISTEN,
      MOOD_FOCUSED,
      MOOD_HUNGRY,
      MOOD_CONFUSED,
      MOOD_BORED,
      MOOD_SLEEPY,
      MOOD_WAKE,
      MOOD_SURPRISE,
      MOOD_COLD,
      MOOD_HOT,
      MOOD_SICK,
      MOOD_DIZZY,
      MOOD_SAD,
      MOOD_SCARED,
      MOOD_ANGRY,
      MOOD_GLITCH
    };

    size_t poolSize = sizeof(weightedPool) / sizeof(weightedPool[0]);
    Mood chosen = weightedPool[random(poolSize)];
    uint32_t dur = 3800 + random(2200); // 3.8s - 6.0s

    playMochiBlink(1);
    playMood(chosen, dur);
  }
}

