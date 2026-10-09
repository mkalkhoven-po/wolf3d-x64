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
    VL_SetPalette(palette);
}

const byte *Bridge_GamePalette(void)
{
    return gamepal;
}

int Bridge_Pic(bridge_pic_t pic)
{
    switch (pic) {
    case BRIDGE_STATUSBARPIC:
        return STATUSBARPIC;
    case BRIDGE_FACE1APIC:
        return FACE1APIC;
    case BRIDGE_FACE8APIC:
        return FACE8APIC;
    case BRIDGE_MUTANTBJPIC:
        return MUTANTBJPIC;
    case BRIDGE_GOTGATLINGPIC:
        return GOTGATLINGPIC;
    case BRIDGE_N_BLANKPIC:
        return N_BLANKPIC;
    case BRIDGE_N_0PIC:
        return N_0PIC;
    case BRIDGE_KNIFEPIC:
        return KNIFEPIC;
    case BRIDGE_NOKEYPIC:
        return NOKEYPIC;
    case BRIDGE_GOLDKEYPIC:
        return GOLDKEYPIC;
    case BRIDGE_SILVERKEYPIC:
        return SILVERKEYPIC;
    case BRIDGE_GETPSYCHEDPIC:
        return GETPSYCHEDPIC;
    case BRIDGE_PAUSEDPIC:
        return PAUSEDPIC;
    }
    return -1;
}

void Bridge_DrawPic(int x, int y, int chunk)
{
    VWB_DrawPic(x, y, chunk);
}

void Bridge_DrawString(int x, int y, const char *text, int color)
{
    VH_DrawString(x, y, text, color);
}

void Bridge_PlaySound(int sound)
{
    SD_PlaySound((word)sound);
}

boolean Bridge_SoundPlaying(void)
{
    return SD_SoundPlaying();
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
