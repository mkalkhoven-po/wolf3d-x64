#include "orig_hooks.h"

orig_hooks_t orig_hooks;

void OrigHooks_Reset(void)
{
    orig_hooks_t none = {0};
    orig_hooks = none;
}

#define CALL_HOOK(name)                                                        \
    do {                                                                       \
        if (orig_hooks.name) {                                                 \
            orig_hooks.name();                                                 \
        }                                                                      \
    } while (0)

void OrigSD_PlaySound(int sound)
{
    if (orig_hooks.play_sound) {
        orig_hooks.play_sound(sound);
    }
}

int OrigSD_SoundPlaying(void)
{
    return orig_hooks.sound_playing ? orig_hooks.sound_playing() : 0;
}

void OrigSD_WaitSoundDone(void)
{
    CALL_HOOK(wait_sound_done);
}

void PlaySoundLocGlobal(int s, fixed gx, fixed gy)
{
    if (orig_hooks.play_sound_loc) {
        orig_hooks.play_sound_loc(s, gx, gy);
    }
}

void DrawHealth(void)
{
    CALL_HOOK(draw_health);
}

void DrawFace(void)
{
    CALL_HOOK(draw_face);
}

void DrawScore(void)
{
    CALL_HOOK(draw_score);
}

void DrawLives(void)
{
    CALL_HOOK(draw_lives);
}

void DrawWeapon(void)
{
    CALL_HOOK(draw_weapon);
}

void DrawKeys(void)
{
    CALL_HOOK(draw_keys);
}

void DrawAmmo(void)
{
    CALL_HOOK(draw_ammo);
}

void DrawGotGatling(void)
{
    CALL_HOOK(draw_got_gatling);
}

void StartDamageFlash(int damage)
{
    if (orig_hooks.start_damage_flash) {
        orig_hooks.start_damage_flash(damage);
    }
}

void StartBonusFlash(void)
{
    CALL_HOOK(start_bonus_flash);
}

void DeathCamPause(void)
{
    CALL_HOOK(death_cam_pause);
}

void DeathCamIntro(void)
{
    CALL_HOOK(death_cam_intro);
}

void DeathCamOutro(void)
{
    CALL_HOOK(death_cam_outro);
}

void FizzleIn(void)
{
    CALL_HOOK(fizzle_in);
}
