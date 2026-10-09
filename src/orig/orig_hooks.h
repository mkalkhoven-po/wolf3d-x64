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
