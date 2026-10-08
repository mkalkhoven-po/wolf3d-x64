#pragma once

#include "wl_types.h"

/*
 * John Carmack's table-driven random number generator (ID_US_A.ASM).
 * Demos depend on the exact sequence: playback and recording seed it with
 * US_InitRndT(false), normal play with US_InitRndT(true).
 */
extern const byte rndtable[256];

/* randomize false: index 0; true: hundredths of the current second (0-99). */
void US_InitRndT(boolean randomize);

/* Next value 0-255. The index is advanced before the read. */
int US_RndT(void);
