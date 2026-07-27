#pragma once

#include "wl_types.h"

/* Video high-level / pics — stubs until E4. */

void VH_Startup(void);
void VH_Shutdown(void);
void VH_UpdateScreen(void);
void VWB_Bar(int x, int y, int width, int height, int color);
void VWB_DrawPic(int x, int y, int chunknum);
void VWB_DrawPropString(const char *string);
