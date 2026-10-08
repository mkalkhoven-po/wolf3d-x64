#pragma once

/*
 * Constants for the faithful engine (P4, ADR-002), taken from WL_DEF.H.
 * The legacy float engine in src/game does not use this header.
 */

#include "wl_types.h"

#define PI          3.141592657

#define GLOBAL1     (1l << 16)
#define TILEGLOBAL  GLOBAL1
#define PIXGLOBAL   (GLOBAL1 / 64)
#define TILESHIFT   16l
#define UNSIGNEDSHIFT 8

#define ANGLES      360     /* must be divisable by 4 */
#define ANGLEQUAD   (ANGLES / 4)
#define FINEANGLES  3600
#define ANG90       (FINEANGLES / 4)
#define ANG180      (ANG90 * 2)
#define ANG270      (ANG90 * 3)
#define ANG360      (ANG90 * 4)
#define VANG90      (ANGLES / 4)
#define VANG180     (VANG90 * 2)
#define VANG270     (VANG90 * 3)
#define VANG360     (VANG90 * 4)

#define MINDIST     (0x5800l)

/* Rate of TimeCount (ID_SD.C runs it from the PIT at ~70 Hz). */
#define TICRATE     70
