/*
 * Wolf3D x64 — application entry.
 * Interactive: the faithful engine (ADR-002) from its title page and control
 * panel. Any game option (--map, --demo, --load, ...) starts a game directly.
 * Use --smoke-only for harness checks.
 */

#include <stdio.h>
#include <string.h>

#include "id/audio_wl6.h"
#include "id/gfxv.h"
#include "id/id_ca.h"
#include "id/id_heads.h"
#include "id/id_sd.h"
#include "id/id_vh.h"
#include "id/id_vl.h"
#include "id/wl_data.h"
#include "platform/win_sdl/orig_runner.h"
#include "platform/win_sdl/wolf_pad.h"
#include "platform/win_sdl/wolf_present.h"
#include "platform/win_sdl/wolf_userdata.h"
#include "platform/win_sdl/wolf_folder.h"
#include "platform/win_sdl/wolf_message.h"
#include "platform/win_sdl/wolf_winutil.h"

static orig_run_options_t g_game_opts; /* the game options, when there are any */

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

/* a folder with a complete set of game files (the folder picker's check) */
static int folder_has_game_files(const char *folder)
{
    char *argv[4] = {(char *)"wolf3d", (char *)"--data", (char *)folder, NULL};
    wolf_data_dir_t dir;

    memset(&dir, 0, sizeof(dir));
    return Wolf_ResolveDataDir(3, argv, &dir) && Wolf_CheckRequiredFiles(&dir);
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

    /*
     * Nothing found and no --data given: ask for the folder (it is
     * remembered as last_data, so this comes once). A wrong --data still
     * just fails (FR-DATA-003).
     */
    if (!smoke_only && !has_data_cli_arg(argc, argv)) {
        char folder[WOLF_PATH_MAX];
        const int chosen = Wolf_ChooseDataFolder(folder_has_game_files, folder, sizeof(folder));

        if (chosen > 0) {
            char *pick_argv[4];

            pick_argv[0] = argv[0] ? argv[0] : (char *)"wolf3d";
            pick_argv[1] = (char *)"--data";
            pick_argv[2] = folder;
            pick_argv[3] = NULL;
            if (Wolf_ResolveDataDir(3, pick_argv, data_dir)) {
                printf("Using the chosen folder: %s\n", folder);
                return 1;
            }
        }
        if (chosen == 0) {
            return 0; /* the player chose Quit */
        }
    }

    if (!smoke_only) {
        Wolf_ShowDataError(
            "Could not find the Wolfenstein 3-D game files: the shareware "
            "(VSWAP.WL1 and the other .WL1 files) or the full version (.WL6).");
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
    /* Guards against a wrong chunk table for this pack. */
    if (w != 320 || h != 200 ||
        pictable[STATUSBARPIC - STARTPICS].width != 320 ||
        pictable[STATUSBARPIC - STARTPICS].height != STATUSLINES) {
        fprintf(stderr, "Graphics layout check failed (%s): TITLEPIC %dx%d, STATUSBARPIC %dx%d\n",
                g_gfx->name, w, h, pictable[STATUSBARPIC - STARTPICS].width,
                pictable[STATUSBARPIC - STARTPICS].height);
        return 9;
    }
    FB_Clear(0);
    FB_SetPalette(gamepal);
    FB_DrawPic(0, 0, TITLEPIC);
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

    /* the faithful engine: demo 0 with all hooks, then a save v2 roundtrip */
    if (!OrigRunner_SmokeVerify()) {
        fprintf(stderr, "Engine smoke verify failed\n");
        return 11;
    }
    if (!SD_AudioSmokeVerify()) {
        fprintf(stderr, "Audio smoke verify failed\n");
        return 16;
    }
    if (!Wolf_UserDataSmokeVerify()) {
        fprintf(stderr, "UserData smoke verify failed\n");
        return 17;
    }

    if (!Wolf_PresentCreate(&pr, "Wolf3D present smoke", g_config.scale, 0)) {
        fprintf(stderr, "Present create failed\n");
        return 18;
    }
    FB_Clear(0);
    FB_SetPalette(gamepal);
    FB_DrawPic(0, 0, TITLEPIC);
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

#ifdef _MSC_VER
    /*
     * The Microsoft CRT has no line buffering, and _IOLBF with size 0 is an
     * invalid parameter there: the default handler ends the program with
     * 0xC0000409. Unbuffered keeps logs complete if the game crashes.
     */
    setvbuf(stdout, NULL, _IONBF, 0);
#else
    setvbuf(stdout, NULL, _IOLBF, 0);
#endif
    Wolf_EnableDpiAwareness();
    Wolf_ConfigSetDefaults();
    (void)Wolf_ConfigLoad();

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            Wolf_PrintDataUsage();
            return 0;
        }
    }

    smoke_only = Wolf_ArgSmokeOnly(argc, argv);
    printf("wolf3d-x64 starting%s...\n",
           smoke_only ? " (smoke-only)" : " (interactive)");
    fflush(stdout);

    /* game options are checked before anything is loaded */
    if (!OrigRunner_ParseArgs(argc, argv, &g_game_opts)) {
        return 2;
    }

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
    /* the sound menu's choices from last time */
    if (!g_config.sfx_on) {
        SD_SetSoundMode(sdm_Off);
    }
    if (!g_config.digi_on) {
        SD_SetDigiDevice(sds_Off);
    }
    if (!g_config.music_on) {
        SD_SetMusicMode(smm_Off);
    }

    if (smoke_only) {
        rc = run_smoke_suite();
        ID_Engine_Shutdown();
        if (rc != 0) {
            fprintf(stderr, "Smoke suite failed (%d)\n", rc);
            return rc;
        }
        printf("Smoke-only mode: suite done.\n");
        printf("wolf3d-x64 smoke OK\n");
        return 0;
    }

    printf("Data ready: %s (%s, %s)\n", data_dir.path,
           strcmp(data_dir.extension, ".WL1") == 0 ? "the shareware" : "the full version",
           data_dir.extension);
    fflush(stdout);

    Wolf_PadInit(); /* XInput and other game pads, alongside keyboard and mouse */
    g_game_opts.menu = !OrigRunner_Direct(argc, argv);
    rc = OrigRunner_Run(&g_game_opts);
    Wolf_PadShutdown();
    ID_Engine_Shutdown();
    if (rc != 0) {
        fprintf(stderr, "wolf3d-x64 exited with error %d\n", rc);
        return rc;
    }

    printf("wolf3d-x64 clean quit\n");
    return 0;
}
