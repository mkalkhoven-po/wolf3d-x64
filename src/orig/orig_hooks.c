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

void OrigSD_StopSound(void)
{
    CALL_HOOK(stop_sound);
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
