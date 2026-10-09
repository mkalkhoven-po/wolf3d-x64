#pragma once

/*
 * The AdLib half of the original sound manager (ID_SD.C, ID_SD_A.ASM) on the
 * OPL2 emulator: sound effects on channel 0, music on channels 1-8, both
 * driven by the 700 Hz timer the original installed (music every tick,
 * effects every fifth: 140 Hz). No SDL: id_sd.c renders it into the mixer.
 *
 * Not thread safe; id_sd.c holds its audio lock around every call.
 */

#include <stddef.h>
#include <stdint.h>

#include "opl2.h"
#include "wl_types.h"

#define SDA_RATE OPL2_RATE
#define SDA_TICKRATE 700 /* the original's timer 0 with AdLib music on */

/* SDL_DetectAdLib's register reset (WSE on), then SDL_StartAL. */
void SDA_Startup(void);

/*
 * SDL_ALPlaySound: an AdLibSound chunk from AUDIOT (SoundCommon, Instrument,
 * block, data). number and priority are SD_PlaySound's SoundNumber and
 * SoundPriority, cleared when the sound ends. False for a malformed chunk.
 */
boolean SDA_PlaySound(const byte *chunk, size_t size, int number, int priority);
void SDA_StopSound(void);     /* SDL_ALStopSound plus SDL_SoundFinished */
int SDA_SoundPlaying(void);   /* SD_SoundPlaying: the sound number, 0 when idle */
int SDA_SoundPriority(void);  /* SoundPriority: 0 when idle */

/* SD_StartMusic with a MusicGroup chunk (word length, then reg/value/delay words). */
void SDA_StartMusic(const byte *chunk, size_t size);
void SDA_MusicOn(void);
void SDA_MusicOff(void);
boolean SDA_MusicActive(void);

/* Advance by frames samples at SDA_RATE, running the timer service on the way. */
void SDA_Render(int32_t *out, int frames);

/* The chip, for tests and the smoke check. */
const opl2_t *SDA_Chip(void);
