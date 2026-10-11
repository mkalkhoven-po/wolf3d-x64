#pragma once

#include <stddef.h>

#include "wl_types.h"

#ifndef WOLF_PATH_MAX
#define WOLF_PATH_MAX 512
#endif

#define WOLF_APPDATA_DIRNAME "Wolf3D-x64"
#define WOLF_APPDATA_OLDNAME "Wolf3DWin64" /* before R1: moved to the new name on first start */
#define WOLF_CONFIG_NAME "config.ini"
#define WOLF_SAVE_MAGIC "W64S"
#define WOLF_SAVE_VERSION 2 /* 1 was the legacy engine; see ADR-002 */
#define WOLF_MAX_SAVE_SLOTS 10 /* the control panel's ten */

#define WOLF_KEY_BUTTONS 8 /* NUMBUTTONS */

typedef struct {
    int scale;          /* integer window scale 1..8 (logical 320x200) */
    int fullscreen;     /* 0 windowed, 1 desktop fullscreen */
    int mouse_look;     /* the Control menu's Mouse Enabled */
    int mouse_adjust;   /* its Mouse Sensitivity, 0..9 */
    int music_on;       /* AdLib music */
    int sfx_on;         /* AdLib sound effects */
    int digi_on;        /* digitized sound (Sound Blaster) */
    int viewsize;       /* 4..20, the Change View setting */
    int joystick, joystick_port, joypad; /* the Control menu's joystick switches */
    /* Customize controls: -1 where the engine's defaults stay */
    int key_buttons[WOLF_KEY_BUTTONS]; /* buttonscan: the key for each action */
    int key_dirs[4];                   /* dirscan: forward, right, back, left */
    int mouse_buttons[4];              /* buttonmouse: the action of each button */
    int joy_buttons[4];                /* buttonjoy */
    /* the settings this port adds (not in the original) */
    int music_volume, effects_volume; /* 0..9 */
    int pad_sens;                      /* 0..9 */
    int fill_screen;
    char last_data[WOLF_PATH_MAX];
} wolf_config_t;

extern wolf_config_t g_config;
extern boolean g_config_found; /* a config.ini was there to load: not the first run */

/* Resolve %APPDATA%\Wolf3D-x64 or ~/.local/share/Wolf3D-x64 (creates it, moving an old Wolf3DWin64 over). */
boolean Wolf_GetAppDataDir(char *out, size_t out_sz);

/* Full path under AppData. */
boolean Wolf_GetAppDataPath(const char *filename, char *out, size_t out_sz);

void Wolf_ConfigSetDefaults(void);
boolean Wolf_ConfigLoad(void);
boolean Wolf_ConfigSave(void);

/* The high score table of a pack ("WL1" / "WL6"), as the runner lays it out. */
boolean Wolf_ScoresLoad(const char *pack, byte *data, size_t len);
boolean Wolf_ScoresSave(const char *pack, const byte *data, size_t len);

/*
 * Save slots (format v2): the engine's SaveTheGame stream in a file with a
 * header (pack, description, CRC-32). pack is "WL1" or "WL6"; each pack has
 * its own files (savegam0.WL6 ...), so its own ten slots.
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

/* User dirs smoke: write+read config in AppData without writing next to exe. */
boolean Wolf_UserDataSmokeVerify(void);
