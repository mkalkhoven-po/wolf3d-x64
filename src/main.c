/*
 * Wolf3D Win64 — application entry (E1 runnable shell).
 * Game bring-up starts in later epics; this only proves platform + build.
 */

#include <stdio.h>

#include "platform/win_sdl/platform_sdl.h"

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    printf("wolf3d-win64 E1 shell starting...\n");
    const int rc = wolf_platform_run();
    if (rc != 0) {
        fprintf(stderr, "wolf3d-win64 exited with error %d\n", rc);
        return rc;
    }
    printf("wolf3d-win64 clean quit\n");
    return 0;
}
