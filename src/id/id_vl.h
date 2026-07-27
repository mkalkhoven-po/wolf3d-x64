#pragma once

#include "wl_types.h"

/* VGA low-level — RAM buffer later; E2 is contract only. */

void VL_Startup(void);
void VL_Shutdown(void);
void VL_SetPalette(const byte *palette);
void VL_GetPalette(byte *palette);
void VL_FadeIn(int start, int end, const byte *palette, int steps);
void VL_FadeOut(int start, int end, int red, int green, int blue, int steps);
void VL_WaitVBL(int vbls);
void VL_ClearScreen(int color);
