/*
 * Wolf3D Win64 — application entry.
 * E8: Win11 productization — AppData config/saves, integer scale, fullscreen.
 */

#include <stdio.h>
#include <string.h>

#include "game/wl_draw.h"
#include "game/wl_menu_simple.h"
#include "game/wl_play.h"
#include "id/audio_wl6.h"
#include "id/gfxv_wl6.h"
#include "id/id_ca.h"
#include "id/id_heads.h"
#include "id/id_sd.h"
#include "id/id_vh.h"
#include "id/id_vl.h"
#include "id/wl_data.h"
#include "platform/win_sdl/wolf_present.h"
#include "platform/win_sdl/wolf_userdata.h"

int main(int argc, char *argv[])
{
    wolf_data_dir_t data_dir;
    boolean smoke_only;
    int i;

    setvbuf(stdout, NULL, _IOLBF, 0);
    Wolf_EnableDpiAwareness();
    Wolf_ConfigSetDefaults();
    (void)Wolf_ConfigLoad();
    (void)Wolf_HighScoresLoad();

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            Wolf_PrintDataUsage();
            return 0;
        }
    }

    printf("wolf3d-win64 starting (E8 Win11 productization)...\n");
    fflush(stdout);

    if (!Wolf_ResolveDataDir(argc, argv, &data_dir)) {
            /* Only fall back to last_data when the user did not pass --data. */
            int has_data_arg = 0;
            for (i = 1; i < argc; ++i) {
                if (strcmp(argv[i], "--data") == 0 || strcmp(argv[i], "-data") == 0 ||
                    strncmp(argv[i], "--data=", 7) == 0) {
                    has_data_arg = 1;
                    break;
                }
            }
            if (!has_data_arg && g_config.last_data[0]) {
                char *fallback_argv[4];
                fallback_argv[0] = argv[0] ? argv[0] : (char *)"wolf3d";
                fallback_argv[1] = (char *)"--data";
                fallback_argv[2] = g_config.last_data;
                fallback_argv[3] = NULL;
                if (Wolf_ResolveDataDir(3, fallback_argv, &data_dir)) {
                    printf("Using last_data from AppData config: %s\n",
                           g_config.last_data);
                } else {
                    return 2;
                }
            } else {
                return 2;
            }
        }
    if (!Wolf_CheckRequiredFiles(&data_dir)) {
        return 3;
    }
    snprintf(g_config.last_data, sizeof(g_config.last_data), "%s", data_dir.path);
    (void)Wolf_ConfigSave();

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
    if (!SD_SetupFromData(&data_dir)) {
        fprintf(stderr, "Audio setup failed\n");
        ID_Engine_Shutdown();
        return 15;
    }
    if (!g_config.sfx_on) {
        SD_SetSilent(true);
    }
    if (!g_config.music_on) {
        SD_MusicOff();
        MusicMode = smm_Off;
    }

    PM_PrintSmoke();

    /* Title paint sanity. */
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

    Wolf_ResetPlayerStats();
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
    if (!Wolf_CombatSmokeVerify()) {
        fprintf(stderr, "Combat smoke verify failed\n");
        ID_Engine_Shutdown();
        return 13;
    }
    if (!SD_AudioSmokeVerify()) {
        fprintf(stderr, "Audio smoke verify failed\n");
        ID_Engine_Shutdown();
        return 16;
    }
    if (!Wolf_UserDataSmokeVerify()) {
        fprintf(stderr, "UserData smoke verify failed\n");
        ID_Engine_Shutdown();
        return 17;
    }

    /* Present path smoke without holding a window open long. */
    {
        wolf_present_t pr;
        if (!Wolf_PresentCreate(&pr, "Wolf3D present smoke", g_config.scale, 0)) {
            fprintf(stderr, "Present create failed\n");
            ID_Engine_Shutdown();
            return 18;
        }
        VL_ClearScreen(0);
        VL_SetPalette(gamepal);
        VWB_DrawPic(0, 0, TITLEPIC);
        Wolf_PresentFrame(&pr);
        if (pr.scale < 1 || pr.logical_w != 320 || pr.logical_h != 200) {
            fprintf(stderr, "Present logical size wrong\n");
            Wolf_PresentDestroy(&pr);
            ID_Engine_Shutdown();
            return 19;
        }
        printf("Present smoke: scale=%d logical=%dx%d fullscreen=%d\n", pr.scale,
               pr.logical_w, pr.logical_h, pr.fullscreen);
        (void)Wolf_PresentSetScale(&pr, pr.scale >= 8 ? 2 : pr.scale + 1);
        printf("Present smoke: scale after bump=%d\n", pr.scale);
        Wolf_PresentDestroy(&pr);
        (void)Wolf_ConfigLoad();
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
