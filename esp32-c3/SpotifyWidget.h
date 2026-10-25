// =============================================================================
// SpotifyWidget.h - Widget "Now Playing" Spotify, Equalizer & Lirik Lagu
// Mendukung Lirik Real-Time (Opsi A) & Fetch Lirik Mandiri via Wi-Fi LRCLIB (Opsi B)
// =============================================================================
#pragma once
#include <Arduino.h>

void spotifyInit();
void spotifySetTrack(const String& title, const String& artist);
String spotifyGetTitle();
String spotifyGetArtist();

// Lirik Lagu (Opsi A - Real-time dari Aplikasi)
void spotifySetLyricsLine(const String& currentLine, const String& nextLine = "");

// Lirik Lagu (Opsi B - Fetch mandiri via Wi-Fi ESP32 dari LRCLIB)
void spotifyFetchLyrics();

bool spotifyIsActive();
void spotifyOpen();
void spotifyClose();
void spotifyUpdate();
