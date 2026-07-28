#pragma once

#include "wl_data.h"
#include "wl_types.h"
#include "audio_wl6.h"

typedef enum {
    sdm_Off,
    sdm_PC,
    sdm_AdLib
} SDMode;

typedef enum {
    smm_Off,
    smm_AdLib
} SMMode;

typedef enum {
    sds_Off,
    sds_PC,
    sds_SoundBlaster
} SDSMode;

extern SDMode SoundMode;
extern SMMode MusicMode;
extern SDSMode DigiMode;
extern boolean SD_Silent; /* true when no device / forced silent */
extern int DigiMap[LASTSOUND];

void SD_Startup(void);
void SD_Shutdown(void);

/* Load digi table + optional AUDIOT music rectangle. Call after PM_OpenPageFile. */
boolean SD_SetupFromData(const wolf_data_dir_t *dir);

void SD_PlaySound(word sound);
void SD_StopSound(void);
void SD_WaitSoundDone(void);
void SD_StartMusic(word music_index); /* musicnames index into AUDIOT STARTMUSIC+ */
void SD_MusicOn(void);
void SD_MusicOff(void);
boolean SD_SoundPlaying(void);
boolean SD_MusicPlaying(void);
longword SD_GetTimeCount(void);
void SD_SetTimeCount(longword count);

/* Force silent path (tests / missing device). */
void SD_SetSilent(boolean silent);

/* Smoke helpers */
int SD_NumDigi(void);
boolean SD_HasDigiFor(word sound);
boolean SD_AudioSmokeVerify(void);
