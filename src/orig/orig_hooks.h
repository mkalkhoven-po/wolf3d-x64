#pragma once

/*
 * Calls from the faithful engine's game logic into sound and the status bar.
 * The logic only decides *that* a sound plays or the HUD changes; the platform
 * layer installs the real implementations when the engine is wired up (S5/S6).
 * Until then every hook is a no-op. Tests install recorders.
 *
 * Sound numbers are the soundnames of orig_audio.h (WL6 numbering).
 */

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

    /* status bar (WL_AGENT.C draw functions); state is read from gamestate */
    void (*draw_health)(void);
    void (*draw_face)(void); /* also reads LastAttacker when health is 0 */
    void (*draw_score)(void);
    void (*draw_lives)(void);
    void (*draw_weapon)(void);
    void (*draw_keys)(void);
    void (*draw_ammo)(void);
    void (*draw_got_gatling)(void); /* StatusDrawPic (17,4,GOTGATLINGPIC) */

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

    /* ThreeDRefresh after a level start or the death cam: fizzle the new view in */
    void (*fizzle_in)(void);

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

    /*
     * The non-game part of CheckKeys: pause, the control panel (F1-F10,
     * Esc), the other cheat keys, debug keys. It may set playstate.
     */
    void (*check_keys)(byte lastscan);
    void (*cheat_message)(void); /* after the MLI cheat */

    void (*play_music)(int song); /* musicnames */
    void (*stop_music)(void);

    /* palette: red shift 1-6, white shift 1-3, or 0 / 0 for the game palette */
    void (*set_palette_shift)(int red, int white);

    void (*present_frame)(void); /* show screenbuf (the page flip) */
    void (*sound_update)(void);  /* SD_Poll and UpdateSoundLoc, once a frame */
} orig_hooks_t;

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
void DrawHealth(void);
void DrawFace(void);
void DrawScore(void);
void DrawLives(void);
void DrawWeapon(void);
void DrawKeys(void);
void DrawAmmo(void);
void DrawGotGatling(void);
void StartDamageFlash(int damage);
void StartBonusFlash(void);
void DeathCamPause(void);
void DeathCamIntro(void);
void DeathCamOutro(void);
void FizzleIn(void);

/* JAB (WL_DEF.H) */
#define PlaySoundLocTile(s, tx, ty)                                                  \
    PlaySoundLocGlobal(s, (((fixed)(tx) << TILESHIFT) + (1L << (TILESHIFT - 1))),    \
                       (((fixed)(ty) << TILESHIFT) + (1L << (TILESHIFT - 1))))
#define PlaySoundLocActor(s, ob) PlaySoundLocGlobal(s, (ob)->x, (ob)->y)
