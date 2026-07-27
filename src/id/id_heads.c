#include "id_heads.h"

#include <stdio.h>

void ID_Engine_Startup(void)
{
    MM_Startup();
    PM_Startup();
    CA_Startup();
    VL_Startup();
    VH_Startup();
    IN_Startup();
    SD_Startup();
    fprintf(stderr, "ID_Engine_Startup: ready (E4 2D menus)\n");
}

void ID_Engine_Shutdown(void)
{
    SD_Shutdown();
    IN_Shutdown();
    VH_Shutdown();
    VL_Shutdown();
    CA_Shutdown();
    PM_Shutdown();
    MM_Shutdown();
}
