#pragma once

#include "wl_types.h"

void VH_Startup(void);
void VH_Shutdown(void);
void VH_UpdateScreen(void);

/* Into the present buffer (framebuffer): the smoke suite's title check. */
void FB_DrawPic(int x, int y, int chunknum);
