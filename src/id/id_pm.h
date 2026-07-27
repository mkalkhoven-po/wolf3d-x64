#pragma once

#include "wl_types.h"

/* VSWAP page manager — simplified stubs until E3. */

void PM_Startup(void);
void PM_Shutdown(void);
void PM_Reset(void);
void PM_NextFrame(void);
memptr PM_GetPage(int pagenum);
memptr PM_GetPageAddress(int pagenum);
