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
    boolean (*sound_playing)(void);
    void (*draw_health)(void);
    void (*draw_face)(void);
    void (*draw_score)(void);
    void (*draw_lives)(void);
    void (*start_damage_flash)(int damage);

    /*
     * A_StartDeathCam presentation: palette reset and a short wait (pause,
     * also when the cam is already over), red fizzle, "Let's see that
     * again!" and the wait for input (intro), then redrawing the play border
     * (outro). The camera placement between them is game logic.
     */
    void (*death_cam_pause)(void);
    void (*death_cam_intro)(void);
    void (*death_cam_outro)(void);
} orig_hooks_t;

extern orig_hooks_t orig_hooks;

/* Restore the no-op defaults. */
void OrigHooks_Reset(void);

/*
 * Original call names. SD_PlaySound and SD_SoundPlaying already exist in the
 * legacy id layer, so the engine-side versions carry an Orig prefix.
 */
void OrigSD_PlaySound(int sound);
boolean OrigSD_SoundPlaying(void);
void PlaySoundLocGlobal(int s, fixed gx, fixed gy);
void DrawHealth(void);
void DrawFace(void);
void DrawScore(void);
void DrawLives(void);
void StartDamageFlash(int damage);
void DeathCamPause(void);
void DeathCamIntro(void);
void DeathCamOutro(void);

/* JAB (WL_DEF.H) */
#define PlaySoundLocTile(s, tx, ty)                                                  \
    PlaySoundLocGlobal(s, (((fixed)(tx) << TILESHIFT) + (1L << (TILESHIFT - 1))),    \
                       (((fixed)(ty) << TILESHIFT) + (1L << (TILESHIFT - 1))))
#define PlaySoundLocActor(s, ob) PlaySoundLocGlobal(s, (ob)->x, (ob)->y)
