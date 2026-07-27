#include "id_sd.h"

#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif
#include <SDL.h>

#include "id_in.h"

SDMode SoundMode = sdm_Off;
SMMode MusicMode = smm_Off;
SDSMode DigiMode = sds_Off;

void SD_Startup(void)
{
    SoundMode = sdm_Off;
    MusicMode = smm_Off;
    DigiMode = sds_Off;
}

void SD_Shutdown(void)
{
}

void SD_PlaySound(word sound)
{
    (void)sound;
}

void SD_StopSound(void)
{
}

void SD_WaitSoundDone(void)
{
}

void SD_StartMusic(word chunk)
{
    (void)chunk;
}

void SD_MusicOn(void)
{
}

void SD_MusicOff(void)
{
}

boolean SD_SoundPlaying(void)
{
    return false;
}

longword SD_GetTimeCount(void)
{
    return IN_GetTimeCount();
}

void SD_SetTimeCount(longword count)
{
    (void)count;
}
