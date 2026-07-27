#include "id_pm.h"

#include <stdio.h>

void PM_Startup(void)
{
    fprintf(stderr, "PM_Startup: stub (no VSWAP yet)\n");
}

void PM_Shutdown(void)
{
}

void PM_Reset(void)
{
}

void PM_NextFrame(void)
{
}

memptr PM_GetPage(int pagenum)
{
    (void)pagenum;
    return NULL;
}

memptr PM_GetPageAddress(int pagenum)
{
    return PM_GetPage(pagenum);
}
