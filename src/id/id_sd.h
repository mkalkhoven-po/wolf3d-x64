#pragma once

#include "wl_types.h"

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

void SD_Startup(void);
void SD_Shutdown(void);
void SD_PlaySound(word sound);
void SD_StopSound(void);
void SD_WaitSoundDone(void);
void SD_StartMusic(word chunk);
void SD_MusicOn(void);
void SD_MusicOff(void);
boolean SD_SoundPlaying(void);
longword SD_GetTimeCount(void);
void SD_SetTimeCount(longword count);
