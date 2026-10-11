#pragma once

/*
 * Calls from the faithful engine into the hardware: sound, the clock, input,
 * the palette and showing the frame. The engine draws everything itself into
 * screenbuf (the 3D view, the status bar, the 2D screens: orig_vh.c); the
 * platform layer shows it and installs the real implementations. Every hook
 * is a no-op by default, so the engine runs headless; tests install
 * recorders.
 *
 * Sound numbers are the soundnames of orig_audio.h (WL6 numbering).
 */

#include <stddef.h>

#include "orig_def.h"

typedef struct {
    void (*play_sound)(int sound);
    void (*play_sound_loc)(int sound, fixed gx, fixed gy);

    /*
     * SD_SoundPlaying: the number of the sound still playing, 0 if none.
     * Game logic depends on it (UpdateFace skips its US_RndT calls while the
     * gatling pickup sound plays), so the platform must report real sound
     * lengths. HITWALLSND is sound 0 and so reads as "nothing playing".
     */
    int (*sound_playing)(void);
    void (*wait_sound_done)(void); /* SD_WaitSoundDone: block until it ends */

    void (*start_damage_flash)(int damage);
    void (*start_bonus_flash)(void);

    /*
     * A_StartDeathCam presentation: palette reset and a short wait (pause,
     * also when the cam is already over), red fizzle, "Let's see that
     * again!" and the wait for input (intro), then redrawing the play border
     * (outro). The camera placement between them is game logic.
     */
    void (*death_cam_pause)(void);
    void (*death_cam_intro)(void);
    void (*death_cam_outro)(void);

    /*
     * FizzleFade: dissolve the rectangle of screenbuf onto the screen over
     * frames frames (the level start, dying, the death cam).
     */
    void (*fizzle)(int x, int y, int width, int height, int frames);

    /*
     * The 70 Hz timer interrupt (S5). read_time returns TimeCount as the
     * clock has it now; write_time is called when the game sets TimeCount;
     * wait_time blocks until TimeCount reaches target (the original's busy
     * loop for demo pacing). Without them the engine runs headless: time
     * advances only as the game needs it.
     */
    int32_t (*read_time)(void);
    void (*write_time)(int32_t count);
    void (*wait_time)(int32_t target);

    /*
     * Input (S5). poll_input refreshes orig_keys / orig_lastscan (the keyboard
     * interrupt's table) before PollControls reads them. Mouse buttons: bit
     * 0-2; joystick buttons: bit 0-3; deltas as the drivers report them.
     */
    void (*poll_input)(void);
    int (*mouse_buttons)(void);
    void (*mouse_delta)(int *dx, int *dy);
    int (*joy_buttons)(void);
    void (*joy_delta)(int *dx, int *dy);
    boolean (*check_ack)(void); /* any key / button: ends demo playback */

    void (*play_music)(int song); /* musicnames */
    void (*stop_music)(void);     /* SD_MusicOff */
    void (*resume_music)(void);   /* SD_MusicOn, after the pause */

    /*
     * The control panel (P5b). which: 0 sound effects (0 off, 1 PC
     * speaker, 2 AdLib), 1 digitized sound (0 off, 1 Sound Source, 2 Sound
     * Blaster), 2 music (0 off, 1 AdLib). sound_available says whether a
     * mode can be chosen at all (the original greys out missing hardware).
     */
    int (*get_sound_mode)(int which);
    void (*set_sound_mode)(int which, int mode);
    boolean (*sound_available)(int which, int mode);

    /*
     * The save slots 0-9 of this pack: save_name fills name (32 chars) and
     * says whether the slot holds a game; write_game stores SaveTheGame's
     * stream under a name; read_game fills buf and returns the length, 0
     * when the slot cannot be read.
     */
    boolean (*save_name)(int slot, char *name);
    boolean (*write_game)(int slot, const char *name, const byte *data, size_t len);
    size_t (*read_game)(int slot, byte *buf, size_t cap);

    /*
     * Not in the original: a game pad in the menus, read like the joystick
     * (dx, dy -127..127; buttons: bit 0 select, bit 1 back). False when no
     * pad is connected. Confirm takes A / B as Y / N, US_LineInput as Enter
     * / Esc.
     */
    boolean (*pad_menu)(int *dx, int *dy, int *buttons);

    void (*idle)(void); /* a moment's rest in a 2D screen's wait loop */

    /*
     * Not in the original: the More options page's settings, OPT_* below.
     * The sliders go 0-9, the switches 0 / 1; the platform applies and
     * keeps them.
     */
    int (*get_option)(int which);
    void (*set_option)(int which, int value);

    /* palette: red shift 1-6, white shift 1-3, or 0 / 0 for the game palette */
    void (*set_palette_shift)(int red, int white);

    void (*present_frame)(void); /* show screenbuf after a 3D frame (the page flip) */

    /*
     * The 2D side (P5, orig_vh.c). set_palette shows a whole palette (768
     * bytes, 6-bit VGA) at once; wait_vbl waits vbls 70 Hz retraces (fades
     * step on them); update_screen shows screenbuf after 2D drawing
     * (VW_UpdateScreen, not a game frame); user_input waits up to tics
     * 70 Hz tics or until a key or button, and says whether one came
     * (IN_UserInput).
     */
    void (*set_palette)(const byte *palette);
    void (*wait_vbl)(int vbls);
    void (*update_screen)(void);
    boolean (*user_input)(int32_t tics);

    /*
     * The screens P5 still has to port: the intermission, game over (high
     * scores) and victory.
     */
    void (*level_completed)(void);
    void (*game_over)(void);
    void (*victory)(void);
    void (*sound_update)(void);  /* SD_Poll and UpdateSoundLoc, once a frame */
} orig_hooks_t;

enum {
    OPT_MUSICVOL,   /* music volume, 0 silent .. 9 full */
    OPT_EFFECTSVOL, /* sound effects volume (AdLib and digitized), 0 .. 9 */
    OPT_PADSENS,    /* the pad's turn speed, 0 .. 9 */
    OPT_FILLSCREEN, /* 1: fill the window at 4:3; 0: whole multiples only */
    OPT_FULLSCREEN, /* 1: fullscreen; 0: a window */
    NUMOPTIONS
};

extern orig_hooks_t orig_hooks;

/* Restore the no-op defaults. */
void OrigHooks_Reset(void);

/*
 * Original call names. SD_PlaySound, SD_SoundPlaying and SD_WaitSoundDone
 * already exist in the legacy id layer, so the engine-side versions carry an
 * Orig prefix.
 */
void OrigSD_PlaySound(int sound);
int OrigSD_SoundPlaying(void);
void OrigSD_WaitSoundDone(void);
void PlaySoundLocGlobal(int s, fixed gx, fixed gy);
void StartDamageFlash(int damage);
void StartBonusFlash(void);
void DeathCamPause(void);
void DeathCamIntro(void);
void DeathCamOutro(void);
void FizzleFade(int x, int y, int width, int height, int frames);
boolean IN_UserInput(int32_t tics);

/* JAB (WL_DEF.H) */
#define PlaySoundLocTile(s, tx, ty)                                                  \
    PlaySoundLocGlobal(s, (((fixed)(tx) << TILESHIFT) + (1L << (TILESHIFT - 1))),    \
                       (((fixed)(ty) << TILESHIFT) + (1L << (TILESHIFT - 1))))
#define PlaySoundLocActor(s, ob) PlaySoundLocGlobal(s, (ob)->x, (ob)->y)
