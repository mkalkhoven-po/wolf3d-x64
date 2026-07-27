#include "id_mm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "id_us.h"

void MM_Startup(void)
{
}

void MM_Shutdown(void)
{
}

void MM_GetPtr(memptr *baseptr, longword size)
{
    if (!baseptr) {
        Quit("MM_GetPtr: null baseptr");
    }
    *baseptr = calloc(1, size ? size : 1);
    if (!*baseptr) {
        Quit("MM_GetPtr: out of memory");
    }
}

void MM_FreePtr(memptr *baseptr)
{
    if (!baseptr || !*baseptr) {
        return;
    }
    free(*baseptr);
    *baseptr = NULL;
}

void MM_SetPurge(memptr *baseptr, int purge)
{
    (void)baseptr;
    (void)purge;
}

void MM_SetLock(memptr *baseptr, boolean locked)
{
    (void)baseptr;
    (void)locked;
}

void MM_SortMem(void)
{
}
