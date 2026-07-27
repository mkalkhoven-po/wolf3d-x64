/*
 * Wolf3D Win64 — application entry.
 * E1: SDL shell. E2: ID engine stubs start up before the window loop.
 */

#include <stdio.h>

#include "id/id_heads.h"
#include "platform/win_sdl/platform_sdl.h"

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    printf("wolf3d-win64 starting (E2 platform contracts)...\n");

    ID_Engine_Startup();

    /* Smoke the contracts briefly so link/use is real, not dead code. */
    {
        memptr block = NULL;
        byte pal[768];
        MM_GetPtr(&block, 64);
        VL_ClearScreen(0);
        VL_GetPalette(pal);
        VH_UpdateScreen();
        (void)SD_GetTimeCount();
        (void)IN_GetTimeCount();
        MM_FreePtr(&block);
    }

    const int rc = wolf_platform_run();

    ID_Engine_Shutdown();

    if (rc != 0) {
        fprintf(stderr, "wolf3d-win64 exited with error %d\n", rc);
        return rc;
    }
    printf("wolf3d-win64 clean quit\n");
    return 0;
}
