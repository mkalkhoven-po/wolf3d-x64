/*
 * Wolf3D Win64 — application entry.
 * E5: move & walls — raycast + doors from menu New Game.
 */

#include <stdio.h>
#include <string.h>

#include "game/wl_draw.h"
#include "game/wl_menu_simple.h"
#include "game/wl_play.h"
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

    printf("wolf3d-win64 starting (E5 move & walls)...\n");
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

    /* E4 graphics smoke retained. */
    {
        int w, h, pi, nz;

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
    }

    /* E5: setup E1M1 (map 0), raycast smoke. */
    if (!Wolf_SetupLevel(0)) {
        fprintf(stderr, "Level setup failed\n");
        ID_Engine_Shutdown();
        return 11;
    }
    if (!Wolf_RaycastSmokeVerify()) {
        fprintf(stderr, "Raycast smoke verify failed\n");
        ID_Engine_Shutdown();
        return 12;
    }
    {
        /* Open first door if any, re-draw — proves door path. */
        if (g_level.door_count > 0) {
            g_level.doors[0].open = 1.0f;
            printf("Smoke: forced door 0 open at %d,%d\n", g_level.doors[0].tilex,
                   g_level.doors[0].tiley);
            (void)Wolf_RaycastSmokeVerify();
        } else {
            printf("Smoke: no doors on map 0\n");
        }
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
