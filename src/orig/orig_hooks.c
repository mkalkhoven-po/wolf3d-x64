#include "orig_hooks.h"

orig_hooks_t orig_hooks;

void OrigHooks_Reset(void)
{
    orig_hooks_t none = {0};
    orig_hooks = none;
}

void OrigSD_PlaySound(int sound)
{
    if (orig_hooks.play_sound) {
        orig_hooks.play_sound(sound);
    }
}

boolean OrigSD_SoundPlaying(void)
{
    return orig_hooks.sound_playing ? orig_hooks.sound_playing() : false;
}

void PlaySoundLocGlobal(int s, fixed gx, fixed gy)
{
    if (orig_hooks.play_sound_loc) {
        orig_hooks.play_sound_loc(s, gx, gy);
    }
}

void DrawHealth(void)
{
    if (orig_hooks.draw_health) {
        orig_hooks.draw_health();
    }
}

void DrawFace(void)
{
    if (orig_hooks.draw_face) {
        orig_hooks.draw_face();
    }
}

void DrawScore(void)
{
    if (orig_hooks.draw_score) {
        orig_hooks.draw_score();
    }
}

void DrawLives(void)
{
    if (orig_hooks.draw_lives) {
        orig_hooks.draw_lives();
    }
}

void StartDamageFlash(int damage)
{
    if (orig_hooks.start_damage_flash) {
        orig_hooks.start_damage_flash(damage);
    }
}

void DeathCamIntro(void)
{
    if (orig_hooks.death_cam_intro) {
        orig_hooks.death_cam_intro();
    }
}

void DeathCamOutro(void)
{
    if (orig_hooks.death_cam_outro) {
        orig_hooks.death_cam_outro();
    }
}

void DeathCamPause(void)
{
    if (orig_hooks.death_cam_pause) {
        orig_hooks.death_cam_pause();
    }
}
