#include "orig_bridge.h"

#include <string.h>

#include "id/gfxv.h"
#include "id/id_ca.h"
#include "id/id_pm.h"
#include "id/id_sd.h"
#include "id/id_vh.h"
#include "id/id_vl.h"
#include "platform/win_sdl/wolf_present.h"
#include "platform/win_sdl/wolf_userdata.h"

static wolf_present_t g_present;

boolean Bridge_OpenWindow(const char *title)
{
    return Wolf_PresentCreate(&g_present, title, g_config.scale, g_config.fullscreen);
}

void Bridge_CloseWindow(void)
{
    Wolf_PresentDestroy(&g_present);
}

boolean Bridge_ToggleFullscreen(void)
{
    return Wolf_PresentToggleFullscreen(&g_present);
}

byte *Bridge_Framebuffer(void)
{
    return framebuffer;
}

void Bridge_Present(void)
{
    Wolf_PresentFrame(&g_present);
}

void Bridge_SetPalette(const byte *palette)
{
    FB_SetPalette(palette);
}

const byte *Bridge_GamePalette(void)
{
    return gamepal;
}




boolean Bridge_PlaySound(int sound)
{
    return SD_PlaySound((word)sound);
}

int Bridge_SoundPlaying(void)
{
    return SD_SoundPlaying();
}

void Bridge_PositionSound(int left, int right)
{
    SD_PositionSound(left, right);
}

void Bridge_SetSoundPosition(int left, int right)
{
    SD_SetPosition(left, right);
}

boolean Bridge_SoundPositioned(void)
{
    return SD_SoundPositioned();
}

void Bridge_WaitSoundDone(void)
{
    SD_WaitSoundDone();
}

void Bridge_StartMusic(int song)
{
    SD_StartMusic((word)song);
}

void Bridge_MusicOff(void)
{
    SD_MusicOff();
}

const byte *Bridge_Page(int page)
{
    return (const byte *)PM_GetPage(page);
}

int Bridge_SpriteStart(void)
{
    return PMSpriteStart;
}

const byte *Bridge_DemoLump(int demo, size_t *length)
{
    const byte *lump;

    if (demo < 0 || demo >= NUMDEMOS) {
        return NULL;
    }
    CA_CacheGrChunk(T_DEMO0 + demo);
    lump = grsegs[T_DEMO0 + demo];
    if (!lump) {
        return NULL;
    }
    *length = (size_t)lump[1] | ((size_t)lump[2] << 8);
    return lump;
}

boolean Bridge_IsShareware(void)
{
    return strcmp(g_gfx->name, "WL1") == 0;
}

boolean Bridge_MapExists(int map)
{
    return map >= 0 && map < NUMMAPS && mapheaderseg[map] != NULL;
}

int Bridge_MouseLook(void)
{
    return g_config.mouse_look;
}

/* the control panel's sound modes: which 0 effects, 1 digitized, 2 music (orig_hooks.h) */
int Bridge_GetSoundMode(int which)
{
    switch (which) {
    case 0:
        return SoundMode == sdm_AdLib ? 2 : 0;
    case 1:
        return DigiMode == sds_SoundBlaster ? 2 : 0;
    default:
        return MusicMode == smm_AdLib ? 1 : 0;
    }
}

void Bridge_SetSoundMode(int which, int mode)
{
    switch (which) {
    case 0:
        SD_SetSoundMode(mode == 2 ? sdm_AdLib : sdm_Off);
        break;
    case 1:
        SD_SetDigiDevice(mode == 2 ? sds_SoundBlaster : sds_Off);
        break;
    default:
        SD_SetMusicMode(mode == 1 ? smm_AdLib : smm_Off);
        break;
    }
}

/* AdLib effects and music, and the Sound Blaster's digitized sound; no PC speaker or Sound Source */
boolean Bridge_SoundAvailable(int which, int mode)
{
    switch (which) {
    case 0:
        return mode == 2 && SD_AdLibPresent();
    case 1:
        return mode == 2 && SD_DigiPresent();
    default:
        return mode == 1 && SD_AdLibPresent();
    }
}

void Bridge_MusicOn(void)
{
    SD_MusicOn();
}

void Bridge_StopSound(void)
{
    SD_StopSound();
}
