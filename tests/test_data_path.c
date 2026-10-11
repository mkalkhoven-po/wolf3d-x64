/*
 * Finding the game files (wl_data.c): a folder whose files are named in
 * lower or mixed case (vswap.wl1, VgaGraph.Wl1) is found and every file
 * opens, as on a case-sensitive Linux file system. No game data needed: the
 * test makes its own little folder of empty files.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#define MKDIR(p) _mkdir(p)
#else
#include <sys/stat.h>
#define MKDIR(p) mkdir((p), 0755)
#endif

#include "wl_data.h"

static int failures;

#define CHECK(cond)                                                                    \
    do {                                                                               \
        if (!(cond)) {                                                                 \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);            \
            failures++;                                                                \
        }                                                                              \
    } while (0)

static void touch(const char *dir, const char *name)
{
    char path[512];
    FILE *f;

    snprintf(path, sizeof(path), "%s/%s", dir, name);
    f = fopen(path, "wb");
    if (f) {
        fputs("x", f);
        fclose(f);
    }
}

int main(void)
{
    static const char *const files[] = {"vswap.wl1",   "VgaGraph.Wl1", "vgahead.wl1", "VGADICT.wl1",
                                        "gamemaps.WL1", "maphead.wl1",  "audiohed.wl1", "Audiot.Wl1"};
    static const char *const stems[] = {"VSWAP",    "VGAGRAPH", "VGAHEAD",  "VGADICT",
                                        "GAMEMAPS", "MAPHEAD",  "AUDIOHED", "AUDIOT"};
    const char *dirname = "data_path_case";
    char *argv[] = {"test", "--data", (char *)dirname, NULL};
    wolf_data_dir_t dir;
    size_t i;

    MKDIR(dirname);
    for (i = 0; i < sizeof(files) / sizeof(files[0]); i++) {
        touch(dirname, files[i]);
    }

    memset(&dir, 0, sizeof(dir));
    CHECK(Wolf_ResolveDataDir(3, argv, &dir));
    CHECK(strcmp(dir.extension, ".WL1") == 0);
    CHECK(Wolf_CheckRequiredFiles(&dir));

    /* each file opens under the name it really has */
    for (i = 0; i < sizeof(stems) / sizeof(stems[0]); i++) {
        char path[WOLF_PATH_MAX];
        FILE *f;

        Wolf_MakeDataPath(&dir, stems[i], path, sizeof(path));
        f = fopen(path, "rb");
        CHECK(f != NULL);
        if (f) {
            fclose(f);
        }
    }

    /* a file that is not there stays missing */
    {
        char path[WOLF_PATH_MAX];

        Wolf_MakeDataPath(&dir, "NOSUCH", path, sizeof(path));
        CHECK(fopen(path, "rb") == NULL);
    }

    if (failures) {
        fprintf(stderr, "%d check(s) failed\n", failures);
        return 1;
    }
    printf("test_data_path: all checks passed\n");
    return 0;
}
