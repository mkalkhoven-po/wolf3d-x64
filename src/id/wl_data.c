#include "wl_data.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#define WOLF_STAT _stat
#define WOLF_ISDIR(m) (((m) & _S_IFDIR) != 0)
#define WOLF_SEP '\\'
#else
#include <dirent.h>
#include <strings.h>
#define WOLF_STAT stat
#define WOLF_ISDIR(m) S_ISDIR(m)
#define WOLF_SEP '/'
#endif

#include "wolf_winutil.h"

static const char *const k_required_basenames[] = {
    "VSWAP",
    "VGAGRAPH",
    "VGAHEAD",
    "VGADICT",
    "GAMEMAPS",
    "MAPHEAD",
    "AUDIOHED",
    "AUDIOT",
};

static boolean path_is_dir(const char *path)
{
    struct WOLF_STAT st;
    if (WOLF_STAT(path, &st) != 0) {
        return false;
    }
    return WOLF_ISDIR(st.st_mode) ? true : false;
}

static boolean path_is_file(const char *path)
{
    struct WOLF_STAT st;
    if (WOLF_STAT(path, &st) != 0) {
        return false;
    }
#ifdef _WIN32
    return ((st.st_mode & _S_IFREG) != 0) ? true : false;
#else
    return S_ISREG(st.st_mode) ? true : false;
#endif
}

static void trim_trailing_slash(char *path)
{
    size_t n = strlen(path);
    while (n > 0 && (path[n - 1] == '\\' || path[n - 1] == '/')) {
        path[--n] = '\0';
    }
}

/*
 * dirpath/name, found whatever the case of its letters (vswap.wl6 for
 * VSWAP.WL6): Linux file systems tell them apart, and the game files come
 * both ways. False (dst as the exact name) when there is none.
 */
static boolean find_file(const char *dirpath, const char *name, char *dst, size_t dst_sz)
{
    int n = snprintf(dst, dst_sz, "%s%c%s", dirpath, WOLF_SEP, name);

    /* An empty path fails to open instead of opening a truncated one. */
    if (n < 0 || (size_t)n >= dst_sz) {
        if (dst_sz > 0) {
            dst[0] = '\0';
        }
        return false;
    }
    if (path_is_file(dst)) {
        return true;
    }
#ifndef _WIN32
    {
        DIR *d = opendir(dirpath);
        struct dirent *e;
        boolean found = false;

        while (d && !found && (e = readdir(d)) != NULL) {
            if (strcasecmp(e->d_name, name) == 0) {
                n = snprintf(dst, dst_sz, "%s%c%s", dirpath, WOLF_SEP, e->d_name);
                found = n > 0 && (size_t)n < dst_sz && path_is_file(dst);
            }
        }
        if (d) {
            closedir(d);
        }
        if (found) {
            return true;
        }
        snprintf(dst, dst_sz, "%s%c%s", dirpath, WOLF_SEP, name);
    }
#endif
    return false;
}

void Wolf_MakeDataPath(const wolf_data_dir_t *dir, const char *basename,
                       char *dst, size_t dst_sz)
{
    char name[64];

    snprintf(name, sizeof(name), "%s%s", basename, dir->extension);
    (void)find_file(dir->path, name, dst, dst_sz);
}

static boolean probe_extension(const char *dirpath, const char *ext_with_dot,
                               char *tmp, size_t tmp_sz)
{
    char name[32];

    snprintf(name, sizeof(name), "VSWAP%s", ext_with_dot);
    return find_file(dirpath, name, tmp, tmp_sz);
}

static boolean detect_extension(const char *dirpath, char *ext_out, size_t ext_sz)
{
    char tmp[WOLF_PATH_MAX];
    static const char *candidates[] = { ".WL6", ".WL1", ".SOD", ".SDM", ".WL3" };
    size_t i;
    for (i = 0; i < sizeof(candidates) / sizeof(candidates[0]); ++i) {
        if (probe_extension(dirpath, candidates[i], tmp, sizeof(tmp))) {
            snprintf(ext_out, ext_sz, "%s", candidates[i]);
            return true;
        }
    }
    return false;
}

boolean Wolf_CheckRequiredFiles(const wolf_data_dir_t *dir)
{
    size_t i;
    int missing = 0;
    char path[WOLF_PATH_MAX];

    printf("Data directory: %s\n", dir->path);
    printf("Detected extension: %s\n", dir->extension);

    for (i = 0; i < sizeof(k_required_basenames) / sizeof(k_required_basenames[0]); ++i) {
        Wolf_MakeDataPath(dir, k_required_basenames[i], path, sizeof(path));
        if (!path_is_file(path)) {
            fprintf(stderr, "MISSING: %s\n", path);
            missing++;
        } else {
            printf("OK: %s\n", path);
        }
    }

    if (missing) {
        fprintf(stderr, "Required data files missing: %d (FR-DATA-003)\n", missing);
        return false;
    }
    return true;
}

static boolean fill_from_path(const char *in_path, wolf_data_dir_t *out)
{
    char ext[WOLF_EXT_MAX];

    if (!in_path || !in_path[0]) {
        return false;
    }
    snprintf(out->path, sizeof(out->path), "%s", in_path);
    trim_trailing_slash(out->path);

    if (!path_is_dir(out->path)) {
        fprintf(stderr, "Not a directory: %s\n", out->path);
        return false;
    }
    if (!detect_extension(out->path, ext, sizeof(ext))) {
        fprintf(stderr, "No VSWAP.* found in: %s\n", out->path);
        fprintf(stderr, "Expected the shareware (VSWAP.WL1) or the full version (VSWAP.WL6)\n");
        return false;
    }
    snprintf(out->extension, sizeof(out->extension), "%s", ext);
    return true;
}

boolean Wolf_ResolveDataDir(int argc, char **argv, wolf_data_dir_t *out)
{
    const char *from_arg = NULL;
    const char *env;
    int i;

    memset(out, 0, sizeof(*out));

    for (i = 1; i < argc; ++i) {
        if ((strcmp(argv[i], "--data") == 0 || strcmp(argv[i], "-data") == 0) &&
            i + 1 < argc) {
            from_arg = argv[++i];
        } else if (strncmp(argv[i], "--data=", 7) == 0) {
            from_arg = argv[i] + 7;
        }
    }

    if (from_arg) {
        if (!fill_from_path(from_arg, out)) {
            return false;
        }
        return true;
    }

    env = getenv("WOLF3D_DATA");
    if (env && env[0]) {
        if (!fill_from_path(env, out)) {
            return false;
        }
        return true;
    }

    /* Convenience defaults: cwd, then next to the executable (double-click
         * from build\ must still find repo\Wolf3dgame or sibling packs). */
        {
            static const char *cwd_defaults[] = {
                "Wolf3dgame",
                ".\\Wolf3dgame",
                "./Wolf3dgame",
                "data",
                ".\\data",
                "./data",
            };
            char exe_dir[WOLF_PATH_MAX];
            char try_path[WOLF_PATH_MAX];
            size_t d;

            for (d = 0; d < sizeof(cwd_defaults) / sizeof(cwd_defaults[0]); ++d) {
                if (fill_from_path(cwd_defaults[d], out)) {
                    printf("Using default data path: %s\n", out->path);
                    return true;
                }
            }

            if (Wolf_GetExecutableDir(exe_dir, sizeof(exe_dir))) {
                static const char *rel[] = {
                    "Wolf3dgame",
                    "data",
                    "..\\Wolf3dgame",
                    "../Wolf3dgame",
                    "..\\data",
                    "../data",
                };
                for (d = 0; d < sizeof(rel) / sizeof(rel[0]); ++d) {
                    int n;
    #ifdef _WIN32
                    n = snprintf(try_path, sizeof(try_path), "%s\\%s", exe_dir, rel[d]);
    #else
                    n = snprintf(try_path, sizeof(try_path), "%s/%s", exe_dir, rel[d]);
    #endif
                    if (n < 0 || (size_t)n >= sizeof(try_path)) {
                        continue;
                    }
                    if (fill_from_path(try_path, out)) {
                        printf("Using data path near exe: %s\n", out->path);
                        return true;
                    }
                }
            }
        }

        fprintf(stderr, "No game data directory found.\n");
        Wolf_PrintDataUsage();
        return false;
    }

boolean Wolf_ArgSmokeOnly(int argc, char **argv)
{
    int i;
    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--smoke-only") == 0 ||
            strcmp(argv[i], "--smoke") == 0) {
            return true;
        }
    }
    return false;
}

void Wolf_PrintDataUsage(void)
{
    fprintf(stderr,
            "Usage:\n"
            "  wolf3d.exe --data <dir> [--smoke-only]\n"
            "  set WOLF3D_DATA=<dir> && wolf3d.exe [--smoke-only]\n"
            "  wolf3d.exe --data <dir> [--map N] [--skill 0-3] [--viewsize 4-20]\n"
            "             [--demo 0-3] [--load 0-9] [--screenshot N file.bmp]\n"
            "    (any of these starts the game directly instead of the menu;\n"
            "     in the game Esc opens the menu, F8 / F9 quick save and load)\n"
            "\n"
            "Also tries (no flags): ./Wolf3dgame, ./data, then those folders\n"
            "next to the .exe and one level up (so double-click from build\\ works\n"
            "when the pack lives at repo\\Wolf3dgame).\n"
            "\n"
            "dir must hold the game files: the shareware (VSWAP.WL1, VGAGRAPH.WL1, ...)\n"
            "or the full version (the same names ending .WL6). See docs/product/GAME_DATA.md\n");
}
