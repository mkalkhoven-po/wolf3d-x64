#pragma once

#include "wl_types.h"

void VH_Startup(void);
void VH_Shutdown(void);
void VH_UpdateScreen(void);

/* Into the present buffer (framebuffer): the stand-in menu until P5d. */
void FB_DrawPic(int x, int y, int chunknum);

/* Simple text at pixel coords using 8x8 mono fallback (until font chunks). */
void FB_DrawString(int x, int y, const char *text, int color);
