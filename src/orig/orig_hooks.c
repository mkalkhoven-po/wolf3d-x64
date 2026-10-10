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

void FizzleFade(int x, int y, int width, int height, int frames)
{
    if (orig_hooks.fizzle) {
        orig_hooks.fizzle(x, y, width, height, frames);
    }
}

boolean IN_UserInput(int32_t tics)
{
    return orig_hooks.user_input ? orig_hooks.user_input(tics) : false;
}
