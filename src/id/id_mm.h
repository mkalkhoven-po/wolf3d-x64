#pragma once

#include "wl_types.h"

/* Thin heap wrappers — full purge/EMS logic dropped (E2 stubs → grow later). */

void MM_Startup(void);
void MM_Shutdown(void);
void MM_GetPtr(memptr *baseptr, longword size);
void MM_FreePtr(memptr *baseptr);
void MM_SetPurge(memptr *baseptr, int purge);
void MM_SetLock(memptr *baseptr, boolean locked);
void MM_SortMem(void);
