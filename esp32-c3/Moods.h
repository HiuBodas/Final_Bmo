// =============================================================================
// Moods.h - 26 Seluruh Mood Ekspresi Wajah & 4 Seluruh Reaksi Sentuh
// =============================================================================
#pragma once
#include <Arduino.h>
#include "Display.h"
#include "Touch.h"

enum Mood : uint8_t {
  MOOD_COOL,
  MOOD_HAPPY,
  MOOD_LAUGH,
  MOOD_LOVE,
  MOOD_WINK,
  MOOD_IDLE,
  MOOD_BLINK,
  MOOD_TALK,
  MOOD_LISTEN,
  MOOD_FOCUSED,
  MOOD_THINKING,
  MOOD_SURPRISE,
  MOOD_EXCITED,
  MOOD_HUNGRY,
  MOOD_COLD,
  MOOD_HOT,
  MOOD_SICK,
  MOOD_DIZZY,
  MOOD_GLITCH,
  MOOD_CONFUSED,
  MOOD_BORED,
  MOOD_SAD,
  MOOD_SCARED,
  MOOD_ANGRY,
  MOOD_WAKE,
  MOOD_SLEEPY,
  MOOD_COUNT
};

const char* moodName(Mood m);
void playMood(Mood m, uint32_t durationMs);
void playReaction(TouchKind k);

// Generator parameter wajah prosedural untuk mood tertentu
void getMoodFaceState(Mood m, float t, uint32_t now, FaceState &s);

// Personality Engine ala Dasai Mochi (Living Idle, Rantai Emosi, & Kedipan Alami)
void playMochiBlink(uint8_t count = 1);
void playMochiHomeBreath(uint32_t durationMs);
void bmoStepMochiPersonality();

