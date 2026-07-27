#pragma once

#include "wl_types.h"

/* Cache manager — real codecs in E3. */

void CA_Startup(void);
void CA_Shutdown(void);
void CA_CacheScreen(int chunk);
void CA_CacheGrChunk(int chunk);
void CA_UnCacheGrChunk(int chunk);
boolean CA_LoadFile(const char *filename, memptr *ptr);
