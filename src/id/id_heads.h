#pragma once

/*
 * Umbrella header for modern ID engine stubs (E2).
 * Game code will include this instead of Borland ID_HEADS.H.
 */

#include "wl_types.h"
#include "id_us.h"
#include "id_mm.h"
#include "id_pm.h"
#include "id_ca.h"
#include "id_vl.h"
#include "id_vh.h"
#include "id_in.h"
#include "id_sd.h"

void ID_Engine_Startup(void);
void ID_Engine_Shutdown(void);
