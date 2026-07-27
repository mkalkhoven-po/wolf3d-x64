#include "id_vl.h"

#include <string.h>

static byte g_palette[768];

void VL_Startup(void)
{
    memset(g_palette, 0, sizeof(g_palette));
}

void VL_Shutdown(void)
{
}

void VL_SetPalette(const byte *palette)
{
    if (palette) {
        memcpy(g_palette, palette, sizeof(g_palette));
    }
}

void VL_GetPalette(byte *palette)
{
    if (palette) {
        memcpy(palette, g_palette, sizeof(g_palette));
    }
}

void VL_FadeIn(int start, int end, const byte *palette, int steps)
{
    (void)start;
    (void)end;
    (void)steps;
    VL_SetPalette(palette);
}

void VL_FadeOut(int start, int end, int red, int green, int blue, int steps)
{
    (void)start;
    (void)end;
    (void)red;
    (void)green;
    (void)blue;
    (void)steps;
}

void VL_WaitVBL(int vbls)
{
    (void)vbls;
}

void VL_ClearScreen(int color)
{
    (void)color;
}
