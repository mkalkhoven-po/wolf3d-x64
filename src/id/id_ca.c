#include "id_ca.h"

#include <stdio.h>

#include "id_mm.h"

void CA_Startup(void)
{
    fprintf(stderr, "CA_Startup: stub (no assets yet)\n");
}

void CA_Shutdown(void)
{
}

void CA_CacheScreen(int chunk)
{
    (void)chunk;
}

void CA_CacheGrChunk(int chunk)
{
    (void)chunk;
}

void CA_UnCacheGrChunk(int chunk)
{
    (void)chunk;
}

boolean CA_LoadFile(const char *filename, memptr *ptr)
{
    (void)filename;
    if (ptr) {
        *ptr = NULL;
    }
    return false;
}
