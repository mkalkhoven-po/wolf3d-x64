#pragma once

#include "wl_types.h"

void VH_Startup(void);
void VH_Shutdown(void);
void VH_UpdateScreen(void);

void VWB_Bar(int x, int y, int width, int height, int color);
void VWB_DrawPic(int x, int y, int chunknum);
void VWB_DrawPropString(const char *string);

/* Simple text at pixel coords using 8x8 mono fallback (until font chunks). */
void VH_DrawString(int x, int y, const char *text, int color);
