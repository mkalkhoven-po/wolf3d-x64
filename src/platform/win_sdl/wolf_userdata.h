#pragma once

#include <stddef.h>

#include "wl_types.h"

#ifndef WOLF_PATH_MAX
#define WOLF_PATH_MAX 512
#endif

#define WOLF_APPDATA_DIRNAME "Wolf3DWin64"
#define WOLF_CONFIG_NAME "config.ini"
#define WOLF_HIGHSCORE_NAME "highscores.dat"
#define WOLF_SAVE_MAGIC "W64S"
#define WOLF_SAVE_VERSION 2 /* 1 was the legacy engine; see ADR-002 */
#define WOLF_MAX_SAVE_SLOTS 6
#define WOLF_MAX_HIGH_SCORES 7

typedef struct {
    int scale;          /* integer window scale 1..8 (logical 320x200) */
    int fullscreen;     /* 0 windowed, 1 desktop fullscreen */
    int mouse_look;     /* mouse X turns */
    int mouse_sens;     /* 1..20 */
    int music_on;
    int sfx_on;
    char last_data[WOLF_PATH_MAX];
} wolf_config_t;

typedef struct {
    char name[16];
    int score;
    int episode;
    int level;
} wolf_highscore_t;

extern wolf_config_t g_config;
extern wolf_highscore_t g_highscores[WOLF_MAX_HIGH_SCORES];

/* Resolve %APPDATA%\Wolf3DWin64 (creates directory). */
boolean Wolf_GetAppDataDir(char *out, size_t out_sz);

/* Full path under AppData. */
boolean Wolf_GetAppDataPath(const char *filename, char *out, size_t out_sz);

void Wolf_ConfigSetDefaults(void);
boolean Wolf_ConfigLoad(void);
boolean Wolf_ConfigSave(void);

boolean Wolf_HighScoresLoad(void);
boolean Wolf_HighScoresSave(void);
void Wolf_HighScoresTryInsert(int score, int episode, int level, const char *name);

/*
 * Save slots (format v2): the engine's SaveTheGame stream in a file with a
 * header (pack, description, CRC-32). pack is "WL1" or "WL6".
 */
#define WOLF_SAVE_DESC 32

typedef enum {
    WOLF_SAVE_OK,
    WOLF_SAVE_MISSING,
    WOLF_SAVE_OLD,        /* version 1: the legacy engine's saves, not loadable */
    WOLF_SAVE_OTHER_PACK, /* saved with WL1 data, loading into WL6 or back */
    WOLF_SAVE_DAMAGED
} wolf_save_result_t;

boolean Wolf_WriteSave(int slot, const char *pack, const char *desc, const byte *data,
                       size_t len);
/* Fills buf (cap bytes) and len; desc, when not NULL, gets WOLF_SAVE_DESC chars. */
wolf_save_result_t Wolf_ReadSave(int slot, const char *pack, byte *buf, size_t cap, size_t *len,
                                 char *desc);
const char *Wolf_SaveResultText(wolf_save_result_t result); /* for a message on screen */
boolean Wolf_SaveExists(int slot);

/* User dirs smoke: write+read config in AppData without writing next to exe. */
boolean Wolf_UserDataSmokeVerify(void);
