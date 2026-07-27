/*
 * Wolf3D Win64 — application entry.
 * E3: data path. E4: title framebuffer + keyboard main menu.
 */

#include <stdio.h>
#include <string.h>

#include "game/wl_menu_simple.h"
#include "id/gfxv_wl6.h"
#include "id/id_ca.h"
#include "id/id_heads.h"
#include "id/id_vh.h"
#include "id/id_vl.h"
#include "id/wl_data.h"

int main(int argc, char *argv[])
{
    wolf_data_dir_t data_dir;
    boolean smoke_only;
    int i;

    setvbuf(stdout, NULL, _IOLBF, 0);

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            Wolf_PrintDataUsage();
            return 0;
        }
    }

    printf("wolf3d-win64 starting (E4 2D menus)...\n");
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
    if (!CA_OpenGraphFiles(&data_dir)) {
        fprintf(stderr, "Failed to open VGAGRAPH assets\n");
        ID_Engine_Shutdown();
        return 8;
    }

    PM_PrintSmoke();

    /* Cache first available map (keeps E3 acceptance green). */
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

    /* E4 graphics smoke: decompress title and verify pixels exist. */
    {
        int w, h;
        int pi, nz;

        CA_CacheGrChunk(TITLEPIC);
        if (!grsegs[TITLEPIC] || !pictable) {
            fprintf(stderr, "TITLEPIC load failed\n");
            ID_Engine_Shutdown();
            return 9;
        }

        w = pictable[TITLEPIC - STARTPICS].width;
        h = pictable[TITLEPIC - STARTPICS].height;
        printf("TITLEPIC loaded: %dx%d\n", w, h);
        VL_ClearScreen(0);
        VL_SetPalette(gamepal);
        VWB_DrawPic(0, 0, TITLEPIC);

        nz = 0;
        for (pi = 0; pi < SCREENPIXELS; ++pi) {
            if (framebuffer[pi]) {
                nz++;
            }
        }
        printf("Framebuffer non-zero pixels after title: %d\n", nz);
        if (nz < 1000) {
            fprintf(stderr, "Title paint looks empty\n");
            ID_Engine_Shutdown();
            return 10;
        }

        CA_CacheGrChunk(C_OPTIONSPIC);
        CA_CacheGrChunk(C_MOUSELBACKPIC);
        printf("Menu pics: options=%s mouseback=%s\n",
               grsegs[C_OPTIONSPIC] ? "ok" : "MISSING",
               grsegs[C_MOUSELBACKPIC] ? "ok" : "MISSING");
    }

    smoke_only = Wolf_ArgSmokeOnly(argc, argv);
    if (smoke_only) {
        printf("Smoke-only mode: skipping window.\n");
        ID_Engine_Shutdown();
        printf("wolf3d-win64 smoke OK\n");
        return 0;
    }

    {
        const int rc = Wolf_RunIntroMenu();
        ID_Engine_Shutdown();
        if (rc != 0) {
            fprintf(stderr, "wolf3d-win64 exited with error %d\n", rc);
            return rc;
        }
    }

    printf("wolf3d-win64 clean quit\n");
    return 0;
}
