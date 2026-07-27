#pragma once

#include "wl_types.h"

#define SCREENWIDTH  320
#define SCREENHEIGHT 200
#define SCREENPIXELS (SCREENWIDTH * SCREENHEIGHT)

extern byte *framebuffer;     /* 320*200 chunky 8-bit */
extern byte g_palette[768]; /* 6-bit VGA components, 0..63 */
extern const byte gamepal[768];

void VL_Startup(void);
void VL_Shutdown(void);
void VL_SetPalette(const byte *palette);
void VL_GetPalette(byte *palette);
void VL_FadeIn(int start, int end, const byte *palette, int steps);
void VL_FadeOut(int start, int end, int red, int green, int blue, int steps);
void VL_WaitVBL(int vbls);
void VL_ClearScreen(int color);
void VL_Bar(int x, int y, int width, int height, int color);
void VL_Plot(int x, int y, int color);
void VL_MemToScreen(const byte *source, int width, int height, int x, int y);

/* Convert VGA 6-bit palette entry to 8-bit channel. */
static inline byte VL_VGA6to8(byte v)
{
    return (byte)((v & 63) * 255 / 63);
}
