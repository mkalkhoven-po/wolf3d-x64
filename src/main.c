/*
 * Wolf3D Win64 — application entry.
 * Interactive: title/menu/game. Use --smoke-only for harness checks.
 */

#include <stdio.h>
#include <string.h>

#include "game/wl_draw.h"
#include "game/wl_menu_simple.h"
#include "game/wl_play.h"
#include "game/wl_status.h"
#include "game/wl_weapon.h"
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
#include "platform/win_sdl/wolf_winutil.h"

static int has_data_cli_arg(int argc, char **argv)
{
    int i;
    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--data") == 0 || strcmp(argv[i], "-data") == 0 ||
            strncmp(argv[i], "--data=", 7) == 0) {
            return 1;
        }
    }
    return 0;
}

static int resolve_data_or_die(int argc, char **argv, wolf_data_dir_t *data_dir,
                               boolean smoke_only)
{
    if (Wolf_ResolveDataDir(argc, argv, data_dir)) {
        return 1;
    }

    /* Only fall back to last_data when the user did not pass --data. */
    if (!has_data_cli_arg(argc, argv) && g_config.last_data[0]) {
        char *fallback_argv[4];
        fallback_argv[0] = argv[0] ? argv[0] : (char *)"wolf3d";
        fallback_argv[1] = (char *)"--data";
        fallback_argv[2] = g_config.last_data;
        fallback_argv[3] = NULL;
        if (Wolf_ResolveDataDir(3, fallback_argv, data_dir)) {
            printf("Using last_data from AppData config: %s\n", g_config.last_data);
            return 1;
        }
    }

    if (!smoke_only) {
        Wolf_ShowDataError(
            "Could not find a Wolfenstein 3D data folder "
            "(VSWAP.WL6 / .WL1, etc.).");
    }
    return 0;
}

static int run_smoke_suite(void)
{
    int w, h, pi, nz;
    wolf_present_t pr;

    PM_PrintSmoke();

    CA_CacheGrChunk(TITLEPIC);
    if (!grsegs[TITLEPIC] || !pictable) {
        fprintf(stderr, "TITLEPIC load failed\n");
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
        return 10;
    }

    Wolf_ResetPlayerStats();
    if (!Wolf_SetupLevel(0)) {
        fprintf(stderr, "Level setup failed\n");
        return 11;
    }
    if (!Wolf_RaycastSmokeVerify()) {
        fprintf(stderr, "Raycast smoke verify failed\n");
        return 12;
    }
    if (!Wolf_CombatSmokeVerify()) {
        fprintf(stderr, "Combat smoke verify failed\n");
        return 13;
    }
    if (!SD_AudioSmokeVerify()) {
        fprintf(stderr, "Audio smoke verify failed\n");
        return 16;
    }
    if (!Wolf_UserDataSmokeVerify()) {
        fprintf(stderr, "UserData smoke verify failed\n");
        return 17;
    }
    if (!Wolf_StatusSmokeVerify()) {
        fprintf(stderr, "Status HUD smoke verify failed\n");
        return 20;
    }
    if (!Wolf_WeaponSmokeVerify()) {
        fprintf(stderr, "Weapon smoke verify failed\n");
        return 21;
    }

    if (!Wolf_PresentCreate(&pr, "Wolf3D present smoke", g_config.scale, 0)) {
        fprintf(stderr, "Present create failed\n");
        return 18;
    }
    VL_ClearScreen(0);
    VL_SetPalette(gamepal);
    VWB_DrawPic(0, 0, TITLEPIC);
    Wolf_PresentFrame(&pr);
    /* Brief Get Psyched paint path (short_mode skips long wait). */
    (void)Wolf_ShowGetPsyched(&pr, true);
    Wolf_DrawStatusBar();
    g_level.active = true;
    Wolf_DrawPlayerWeapon();
    Wolf_DrawStatusBar();
    Wolf_PresentFrame(&pr);
    if (pr.scale < 1 || pr.logical_w != 320 || pr.logical_h != 200) {
        fprintf(stderr, "Present logical size wrong\n");
        Wolf_PresentDestroy(&pr);
        return 19;
    }
    printf("Present smoke: scale=%d logical=%dx%d fullscreen=%d windowed_client=%dx%d\n",
           pr.scale, pr.logical_w, pr.logical_h, pr.fullscreen, 320 * pr.scale,
           240 * pr.scale);
    (void)Wolf_PresentSetScale(&pr, pr.scale >= 8 ? 2 : pr.scale + 1);
    printf("Present smoke: scale after bump=%d\n", pr.scale);
    Wolf_PresentDestroy(&pr);
    (void)Wolf_ConfigLoad();
    return 0;
}

int main(int argc, char *argv[])
{
    wolf_data_dir_t data_dir;
    boolean smoke_only;
    int i;
    int rc;

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

    smoke_only = Wolf_ArgSmokeOnly(argc, argv);
    printf("wolf3d-win64 starting%s...\n",
           smoke_only ? " (smoke-only)" : " (interactive)");
    fflush(stdout);

    if (!resolve_data_or_die(argc, argv, &data_dir, smoke_only)) {
            return 2;
        }
        if (!Wolf_CheckRequiredFiles(&data_dir)) {
            if (!smoke_only) {
                Wolf_ShowDataError(
                    "Data folder was found, but required pack files are missing.\n"
                    "Check the console/log for the MISSING: list.");
            }
            return 3;
        }
    snprintf(g_config.last_data, sizeof(g_config.last_data), "%s", data_dir.path);
    (void)Wolf_ConfigSave();

    ID_Engine_Startup();

    if (!CA_OpenMapFiles(&data_dir)) {
            fprintf(stderr, "Failed to open map files\n");
            if (!smoke_only) {
                Wolf_ShowDataError("Failed to open map files (MAPHEAD/GAMEMAPS).");
            }
            ID_Engine_Shutdown();
            return 4;
        }
        if (!PM_OpenPageFile(&data_dir)) {
            fprintf(stderr, "Failed to open VSWAP\n");
            if (!smoke_only) {
                Wolf_ShowDataError("Failed to open VSWAP page file.");
            }
            ID_Engine_Shutdown();
            return 5;
        }
        if (!CA_OpenGraphFiles(&data_dir)) {
            fprintf(stderr, "Failed to open VGAGRAPH assets\n");
            if (!smoke_only) {
                Wolf_ShowDataError("Failed to open VGAGRAPH graphics.");
            }
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

    if (smoke_only) {
        rc = run_smoke_suite();
        ID_Engine_Shutdown();
        if (rc != 0) {
            fprintf(stderr, "Smoke suite failed (%d)\n", rc);
            return rc;
        }
        printf("Smoke-only mode: suite done.\n");
        printf("wolf3d-win64 smoke OK\n");
        return 0;
    }

    /* Interactive: go straight to title/menu/game. */
    printf("Data ready: %s (%s)\n", data_dir.path, data_dir.extension);
    fflush(stdout);
    rc = Wolf_RunIntroMenu();
    ID_Engine_Shutdown();
    if (rc != 0) {
        fprintf(stderr, "wolf3d-win64 exited with error %d\n", rc);
        return rc;
    }

    printf("wolf3d-win64 clean quit\n");
    return 0;
}
