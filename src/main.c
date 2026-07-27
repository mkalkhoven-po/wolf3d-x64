/*
 * Wolf3D Win64 — application entry.
 * E3: resolve user data dir, verify pack, smoke-load maps + VSWAP.
 */

#include <stdio.h>
#include <string.h>

#include "id/id_heads.h"
#include "id/wl_data.h"
#include "platform/win_sdl/platform_sdl.h"

int main(int argc, char *argv[])
{
    wolf_data_dir_t data_dir;
    boolean smoke_only;
    int i;

    /* Line-buffer stdout so logs stay ordered with stderr when redirected. */
    setvbuf(stdout, NULL, _IOLBF, 0);

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            Wolf_PrintDataUsage();
            return 0;
        }
    }

    printf("wolf3d-win64 starting (E3 data path)...\n");
        fflush(stdout);

        if (!Wolf_ResolveDataDir(argc, argv, &data_dir)) {
            return 2;
        }
    if (!Wolf_CheckRequiredFiles(&data_dir)) {
        return 3;
    }

    ID_Engine_Startup();

    if (!CA_OpenMapFiles(&data_dir)) {
        fprintf(stderr, "Failed to open map files\n");
        ID_Engine_Shutdown();
        return 4;
    }
    if (!PM_OpenPageFile(&data_dir)) {
        fprintf(stderr, "Failed to open VSWAP\n");
        ID_Engine_Shutdown();
        return 5;
    }

    PM_PrintSmoke();

    /* Cache first available map (usually 0). */
    {
        int map = 0;
        while (map < NUMMAPS && !mapheaderseg[map]) {
            ++map;
        }
        if (map >= NUMMAPS) {
            fprintf(stderr, "No valid maps in MAPHEAD\n");
            ID_Engine_Shutdown();
            return 6;
        }
        printf("Caching map index %d...\n", map);
        if (!CA_CacheMap(map)) {
            fprintf(stderr, "CA_CacheMap(%d) failed\n", map);
            ID_Engine_Shutdown();
            return 7;
        }
        CA_PrintMapSmoke(map);
    }

    smoke_only = Wolf_ArgSmokeOnly(argc, argv);
    if (smoke_only) {
        printf("Smoke-only mode: skipping window.\n");
        ID_Engine_Shutdown();
        printf("wolf3d-win64 smoke OK\n");
        return 0;
    }

    {
        const int rc = wolf_platform_run();
        ID_Engine_Shutdown();
        if (rc != 0) {
            fprintf(stderr, "wolf3d-win64 exited with error %d\n", rc);
            return rc;
        }
    }

    printf("wolf3d-win64 clean quit\n");
    return 0;
}
