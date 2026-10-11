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
extern boolean SD_Silent; /* no sound effects: no device, or switched off */
extern int DigiMap[LASTSOUND];

void SD_Startup(void);
void SD_Shutdown(void);

/* Load digi table + optional AUDIOT music rectangle. Call after PM_OpenPageFile. */
boolean SD_SetupFromData(const wolf_data_dir_t *dir);

/* True when a digitized sound started (SD_PlaySound in ID_SD.C). */
boolean SD_PlaySound(word sound);
void SD_StopSound(void);
void SD_WaitSoundDone(void);
void SD_StartMusic(word music_index); /* musicnames index into AUDIOT STARTMUSIC+ */
void SD_MusicOn(void);
void SD_MusicOff(void);
word SD_SoundPlaying(void); /* the AdLib effect playing, 0 when none */
boolean SD_MusicPlaying(void);
longword SD_GetTimeCount(void);
void SD_SetTimeCount(longword count);

/* Stereo placement, 0 (loud) to 15 per side, as the original SB Pro code. */
void SD_PositionSound(int leftvol, int rightvol); /* for the next SD_PlaySound */
void SD_SetPosition(int leftpos, int rightpos);   /* for the sound playing */
boolean SD_SoundPositioned(void);                 /* a placed digitized sound plays */

/* Sound effects off (music has MusicMode). */
void SD_SetSilent(boolean silent);

/* The control panel's sound menu (ID_SD.C). Modes this machine cannot play become off. */
boolean SD_AdLibPresent(void); /* an audio device and the AdLib sounds (AUDIOT) */
boolean SD_DigiPresent(void);  /* an audio device and digitized sounds (VSWAP) */
void SD_SetSoundMode(SDMode mode);
void SD_SetDigiDevice(SDSMode mode);
void SD_SetMusicMode(SMMode mode);

/* Not in the original: music and effects volume, 0 (silent) to SD_MAXVOLUME (full). */
#define SD_MAXVOLUME 9
void SD_SetVolumes(int music, int effects);

/* Smoke helpers */
int SD_NumDigi(void);
boolean SD_HasDigiFor(word sound);
boolean SD_AudioSmokeVerify(void);
