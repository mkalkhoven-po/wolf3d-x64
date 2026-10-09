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
/* The present buffer (framebuffer) and its palette; the engine draws into screenbuf (orig_vh.c). */
void FB_SetPalette(const byte *palette);
void FB_GetPalette(byte *palette);
void FB_Clear(int color);
void FB_Bar(int x, int y, int width, int height, int color);
void FB_Plot(int x, int y, int color);

/* Convert VGA 6-bit palette entry to 8-bit channel. */
static inline byte VL_VGA6to8(byte v)
{
    return (byte)((v & 63) * 255 / 63);
}
