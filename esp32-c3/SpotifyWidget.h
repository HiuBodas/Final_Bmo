// =============================================================================
// SpotifyWidget.h - Widget "Now Playing" Spotify & Audio Equalizer (Seksi 2.4)
// Judul lagu berjalan (Marquee) + Equalizer 5 Bar Prosedural Animasi
// =============================================================================
#pragma once
#include <Arduino.h>

void spotifyInit();
void spotifySetTrack(const String& title, const String& artist);
String spotifyGetTitle();
String spotifyGetArtist();

bool spotifyIsActive();
void spotifyOpen();
void spotifyClose();
void spotifyUpdate();
