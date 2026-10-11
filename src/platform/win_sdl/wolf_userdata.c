#include "wolf_userdata.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>

#ifdef _WIN32
#include <direct.h>
#define WOLF_PATH_SEP '\\'
#ifndef S_IFMT
#define S_IFMT _S_IFMT
#define S_IFDIR _S_IFDIR
#endif
#include <process.h>
#define WOLF_GETPID() _getpid()
#define WOLF_MKDIR(p) _mkdir(p)
#else
#include <unistd.h>
#define WOLF_PATH_SEP '/'
#define WOLF_GETPID() getpid()
#define WOLF_MKDIR(p) mkdir((p), 0755)
#endif


wolf_config_t g_config;
boolean g_config_found;

/* where the per-user folders live: %APPDATA% (Windows), ~/.local/share (elsewhere) */
static boolean user_base_dir(char *out, size_t out_sz)
{
#ifdef _WIN32
    const char *appdata = getenv("APPDATA");
    const char *up;

    if (appdata && appdata[0]) {
        snprintf(out, out_sz, "%s", appdata);
        return true;
    }
    up = getenv("USERPROFILE");
    if (!up || !up[0]) {
        return false;
    }
    snprintf(out, out_sz, "%s\\AppData\\Roaming", up);
    return true;
#else
    /* the XDG base directory: $XDG_DATA_HOME, or ~/.local/share */
    const char *xdg = getenv("XDG_DATA_HOME");
    const char *home = getenv("HOME");

    if (xdg && xdg[0] == '/') {
        snprintf(out, out_sz, "%s", xdg);
        return true;
    }
    if (!home || !home[0]) {
        return false;
    }
    snprintf(out, out_sz, "%s/.local/share", home);
    return true;
#endif
}

/*
 * mkdir -p: every folder on the path. A new Linux account has no
 * ~/.local/share yet; %APPDATA% always exists on Windows.
 */
static void make_dirs(const char *path)
{
    char tmp[WOLF_PATH_MAX + 32];
    size_t i;

    snprintf(tmp, sizeof(tmp), "%s", path);
    for (i = 1; tmp[i]; i++) {
        if (tmp[i] == '/' || tmp[i] == '\\') {
            const char c = tmp[i];

            tmp[i] = '\0';
            if (!(i == 2 && tmp[1] == ':')) { /* not a bare drive (C:) */
                WOLF_MKDIR(tmp);
            }
            tmp[i] = c;
        }
    }
    WOLF_MKDIR(tmp);
}

static boolean dir_exists(const char *path)
{
    struct stat st;

    return stat(path, &st) == 0 && (st.st_mode & S_IFMT) == S_IFDIR;
}

boolean Wolf_GetAppDataDir(char *out, size_t out_sz)
{
    char base[WOLF_PATH_MAX];

    if (!out || out_sz < 16 || !user_base_dir(base, sizeof(base))) {
        return false;
    }
    snprintf(out, out_sz, "%s%c%s", base, WOLF_PATH_SEP, WOLF_APPDATA_DIRNAME);

    /* the folder before the x64 name: moved over once, with its config, saves and scores */
    if (!dir_exists(out)) {
        char old[WOLF_PATH_MAX + 32];

        snprintf(old, sizeof(old), "%s%c%s", base, WOLF_PATH_SEP, WOLF_APPDATA_OLDNAME);
        if (dir_exists(old)) {
            if (rename(old, out) == 0) {
                printf("Settings moved: %s -> %s\n", old, out);
            } else {
                fprintf(stderr, "Could not move %s to %s; starting with new settings\n", old, out);
            }
        }
    }
    make_dirs(out);
    return true;
}

boolean Wolf_GetAppDataPath(const char *filename, char *out, size_t out_sz)
{
    char dir[WOLF_PATH_MAX];
    size_t dir_len;
    size_t name_len;
    if (!Wolf_GetAppDataDir(dir, sizeof(dir))) {
        return false;
    }
    dir_len = strlen(dir);
    name_len = strlen(filename);
    if (dir_len + 1 + name_len + 1 > out_sz) {
        return false;
    }
    memcpy(out, dir, dir_len);
#ifdef _WIN32
    out[dir_len] = '\\';
#else
    out[dir_len] = '/';
#endif
    memcpy(out + dir_len + 1, filename, name_len + 1);
    return true;
}

void Wolf_ConfigSetDefaults(void)
{
    memset(&g_config, 0, sizeof(g_config));
    g_config.scale = 3;
    g_config.fullscreen = 0;
    g_config.mouse_look = 1;
    g_config.mouse_adjust = 5;
    g_config.music_on = 1;
    g_config.sfx_on = 1;
    g_config.digi_on = 1;
    g_config.viewsize = 15;
    memset(g_config.key_buttons, 0xff, sizeof(g_config.key_buttons));
    memset(g_config.key_dirs, 0xff, sizeof(g_config.key_dirs));
    memset(g_config.mouse_buttons, 0xff, sizeof(g_config.mouse_buttons));
    memset(g_config.joy_buttons, 0xff, sizeof(g_config.joy_buttons));
    g_config.music_volume = 9;
    g_config.effects_volume = 9;
    g_config.pad_sens = 5;
    g_config.last_data[0] = '\0';
}

static int clamp_int(int v, int lo, int hi)
{
    return v < lo ? lo : v > hi ? hi : v;
}

/* "29,56,54" into n ints; false (and nothing changed) unless all n are there */
static boolean parse_list(const char *val, int *out, int n)
{
    int tmp[16], i;
    char *end;

    for (i = 0; i < n && i < 16; i++) {
        tmp[i] = (int)strtol(val, &end, 10);
        if (end == val) {
            return false;
        }
        val = end;
        if (*val == ',') {
            val++;
        }
    }
    memcpy(out, tmp, (size_t)n * sizeof(int));
    return true;
}

static void print_list(FILE *f, const char *key, const int *v, int n)
{
    int i;

    if (v[0] < 0 && n) { /* the engine's defaults */
        return;
    }
    fprintf(f, "%s=", key);
    for (i = 0; i < n; i++) {
        fprintf(f, i ? ",%d" : "%d", v[i]);
    }
    fprintf(f, "\n");
}

static void clamp_config(void)
{
    if (g_config.scale < 1) {
        g_config.scale = 1;
    }
    if (g_config.scale > 8) {
        g_config.scale = 8;
    }
    g_config.fullscreen = g_config.fullscreen ? 1 : 0;
    g_config.mouse_look = g_config.mouse_look ? 1 : 0;
    g_config.mouse_adjust = clamp_int(g_config.mouse_adjust, 0, 9);
    g_config.music_volume = clamp_int(g_config.music_volume, 0, 9);
    g_config.effects_volume = clamp_int(g_config.effects_volume, 0, 9);
    g_config.pad_sens = clamp_int(g_config.pad_sens, 0, 9);
    g_config.fill_screen = g_config.fill_screen ? 1 : 0;
    g_config.joystick = g_config.joystick ? 1 : 0;
    g_config.joystick_port = g_config.joystick_port ? 1 : 0;
    g_config.joypad = g_config.joypad ? 1 : 0;
    g_config.music_on = g_config.music_on ? 1 : 0;
    g_config.sfx_on = g_config.sfx_on ? 1 : 0;
    g_config.digi_on = g_config.digi_on ? 1 : 0;
    if (g_config.viewsize < 4 || g_config.viewsize > 20) {
        g_config.viewsize = 15;
    }
}

boolean Wolf_ConfigLoad(void)
{
    char path[WOLF_PATH_MAX];
    FILE *f;
    char line[512];

    Wolf_ConfigSetDefaults();
    if (!Wolf_GetAppDataPath(WOLF_CONFIG_NAME, path, sizeof(path))) {
        return false;
    }
    f = fopen(path, "r");
    if (!f) {
        return false;
    }
    while (fgets(line, sizeof(line), f)) {
        char key[64], val[WOLF_PATH_MAX];
        if (line[0] == '#' || line[0] == ';' || line[0] == '\n' || line[0] == '[') {
            continue;
        }
        if (sscanf(line, "%63[^=]=%511[^\r\n]", key, val) != 2) {
            continue;
        }
        if (strcmp(key, "scale") == 0) {
            g_config.scale = atoi(val);
        } else if (strcmp(key, "fullscreen") == 0) {
            g_config.fullscreen = atoi(val);
        } else if (strcmp(key, "mouse_look") == 0) {
            g_config.mouse_look = atoi(val);
        } else if (strcmp(key, "mouse_adjust") == 0) {
            g_config.mouse_adjust = atoi(val);
        } else if (strcmp(key, "joystick") == 0) {
            g_config.joystick = atoi(val);
        } else if (strcmp(key, "joystick_port") == 0) {
            g_config.joystick_port = atoi(val);
        } else if (strcmp(key, "joypad") == 0) {
            g_config.joypad = atoi(val);
        } else if (strcmp(key, "key_buttons") == 0) {
            (void)parse_list(val, g_config.key_buttons, WOLF_KEY_BUTTONS);
        } else if (strcmp(key, "key_dirs") == 0) {
            (void)parse_list(val, g_config.key_dirs, 4);
        } else if (strcmp(key, "mouse_buttons") == 0) {
            (void)parse_list(val, g_config.mouse_buttons, 4);
        } else if (strcmp(key, "joy_buttons") == 0) {
            (void)parse_list(val, g_config.joy_buttons, 4);
        } else if (strcmp(key, "music_volume") == 0) {
            g_config.music_volume = atoi(val);
        } else if (strcmp(key, "effects_volume") == 0) {
            g_config.effects_volume = atoi(val);
        } else if (strcmp(key, "pad_sens") == 0) {
            g_config.pad_sens = atoi(val);
        } else if (strcmp(key, "fill_screen") == 0) {
            g_config.fill_screen = atoi(val);
        } else if (strcmp(key, "music_on") == 0) {
            g_config.music_on = atoi(val);
        } else if (strcmp(key, "sfx_on") == 0) {
            g_config.sfx_on = atoi(val);
        } else if (strcmp(key, "digi_on") == 0) {
            g_config.digi_on = atoi(val);
        } else if (strcmp(key, "viewsize") == 0) {
            g_config.viewsize = atoi(val);
        } else if (strcmp(key, "last_data") == 0) {
            snprintf(g_config.last_data, sizeof(g_config.last_data), "%s", val);
        }
    }
    fclose(f);
    clamp_config();
    g_config_found = true;
    return true;
}

boolean Wolf_ConfigSave(void)
{
    char path[WOLF_PATH_MAX];
    FILE *f;

    clamp_config();
    if (!Wolf_GetAppDataPath(WOLF_CONFIG_NAME, path, sizeof(path))) {
        return false;
    }
    f = fopen(path, "w");
    if (!f) {
        return false;
    }
    fprintf(f, "# Wolf3D x64 config (do not put next to Program Files)\n");
    fprintf(f, "scale=%d\n", g_config.scale);
    fprintf(f, "fullscreen=%d\n", g_config.fullscreen);
    fprintf(f, "mouse_look=%d\n", g_config.mouse_look);
    fprintf(f, "mouse_adjust=%d\n", g_config.mouse_adjust);
    fprintf(f, "joystick=%d\n", g_config.joystick);
    fprintf(f, "joystick_port=%d\n", g_config.joystick_port);
    fprintf(f, "joypad=%d\n", g_config.joypad);
    print_list(f, "key_buttons", g_config.key_buttons, WOLF_KEY_BUTTONS);
    print_list(f, "key_dirs", g_config.key_dirs, 4);
    print_list(f, "mouse_buttons", g_config.mouse_buttons, 4);
    print_list(f, "joy_buttons", g_config.joy_buttons, 4);
    fprintf(f, "music_volume=%d\n", g_config.music_volume);
    fprintf(f, "effects_volume=%d\n", g_config.effects_volume);
    fprintf(f, "pad_sens=%d\n", g_config.pad_sens);
    fprintf(f, "fill_screen=%d\n", g_config.fill_screen);
    fprintf(f, "music_on=%d\n", g_config.music_on);
    fprintf(f, "sfx_on=%d\n", g_config.sfx_on);
    fprintf(f, "digi_on=%d\n", g_config.digi_on);
    fprintf(f, "viewsize=%d\n", g_config.viewsize);
    fprintf(f, "last_data=%s\n", g_config.last_data);
    fclose(f);
    return true;
}

/* the high score table, per pack: scores.WL1 / scores.WL6 */
static void scores_path(const char *pack, char *out, size_t out_sz)
{
    char name[48];
    snprintf(name, sizeof(name), "scores.%s", pack);
    (void)Wolf_GetAppDataPath(name, out, out_sz);
}

boolean Wolf_ScoresLoad(const char *pack, byte *data, size_t len)
{
    char path[WOLF_PATH_MAX];
    FILE *f;
    boolean ok;

    scores_path(pack, path, sizeof(path));
    f = fopen(path, "rb");
    if (!f) {
        return false;
    }
    ok = fread(data, 1, len, f) == len;
    fclose(f);
    return ok;
}

boolean Wolf_ScoresSave(const char *pack, const byte *data, size_t len)
{
    char path[WOLF_PATH_MAX];
    FILE *f;
    boolean ok;

    scores_path(pack, path, sizeof(path));
    f = fopen(path, "wb");
    if (!f) {
        return false;
    }
    ok = fwrite(data, 1, len, f) == len;
    ok = (fclose(f) == 0) && ok;
    return ok;
}

/*
 * Save files, format v2 (ADR-002): a header, then the engine's stream
 * (SaveTheGame in src/orig/orig_save.c). All numbers little-endian.
 *
 *   0  "W64S"
 *   4  u16 version (2; 1 was the legacy engine's player snapshot)
 *   6  u16 reserved
 *   8  char pack[4]   "WL1" / "WL6": a save only loads into its own pack
 *  12  char desc[32]  shown when choosing a save
 *  44  u32 length of the stream
 *  48  u32 CRC-32 of the stream
 *  52  the stream
 */
#define SAVE_HEADER 52

/* savegam0.WL6 and on: each pack has its own slots */
static void save_path(int slot, const char *pack, char *out, size_t out_sz)
{
    char name[32];
    snprintf(name, sizeof(name), "savegam%d.%s", slot, pack);
    (void)Wolf_GetAppDataPath(name, out, out_sz);
}

static uint32_t crc32_of(const byte *data, size_t len)
{
    uint32_t crc = 0xffffffffu;
    size_t i;
    int bit;

    for (i = 0; i < len; i++) {
        crc ^= data[i];
        for (bit = 0; bit < 8; bit++) {
            crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1u)));
        }
    }
    return ~crc;
}

static void put_u32(byte *p, uint32_t v)
{
    p[0] = (byte)v;
    p[1] = (byte)(v >> 8);
    p[2] = (byte)(v >> 16);
    p[3] = (byte)(v >> 24);
}

static uint32_t get_u32(const byte *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static boolean write_save(const char *path, const char *pack, const char *desc,
                          const byte *data, size_t len)
{
    byte hdr[SAVE_HEADER];
    FILE *f;
    boolean ok;

    memset(hdr, 0, sizeof(hdr));
    memcpy(hdr, WOLF_SAVE_MAGIC, 4);
    hdr[4] = WOLF_SAVE_VERSION;
    memcpy(hdr + 8, pack, strlen(pack) < 3 ? strlen(pack) : 3);
    if (desc) {
        memcpy(hdr + 12, desc, strlen(desc) < WOLF_SAVE_DESC - 1 ? strlen(desc) : WOLF_SAVE_DESC - 1);
    }
    put_u32(hdr + 44, (uint32_t)len);
    put_u32(hdr + 48, crc32_of(data, len));

    f = fopen(path, "wb");
    if (!f) {
        return false;
    }
    ok = fwrite(hdr, 1, sizeof(hdr), f) == sizeof(hdr) && fwrite(data, 1, len, f) == len;
    ok = (fclose(f) == 0) && ok;
    return ok;
}

static wolf_save_result_t read_save(const char *path, const char *pack, byte *buf, size_t cap,
                                    size_t *len, char *desc)
{
    byte hdr[SAVE_HEADER];
    uint32_t n;
    FILE *f = fopen(path, "rb");

    if (!f) {
        return WOLF_SAVE_MISSING;
    }
    if (fread(hdr, 1, sizeof(hdr), f) != sizeof(hdr) || memcmp(hdr, WOLF_SAVE_MAGIC, 4) != 0) {
        fclose(f);
        return WOLF_SAVE_DAMAGED;
    }
    if ((hdr[4] | (hdr[5] << 8)) != WOLF_SAVE_VERSION) {
        fclose(f);
        return WOLF_SAVE_OLD;
    }
    if (strncmp((const char *)hdr + 8, pack, 4) != 0) {
        fclose(f);
        return WOLF_SAVE_OTHER_PACK;
    }
    n = get_u32(hdr + 44);
    if (n > cap || fread(buf, 1, n, f) != n || crc32_of(buf, n) != get_u32(hdr + 48)) {
        fclose(f);
        return WOLF_SAVE_DAMAGED;
    }
    fclose(f);
    *len = n;
    if (desc) {
        memcpy(desc, hdr + 12, WOLF_SAVE_DESC);
        desc[WOLF_SAVE_DESC - 1] = '\0';
    }
    return WOLF_SAVE_OK;
}

boolean Wolf_WriteSave(int slot, const char *pack, const char *desc, const byte *data,
                       size_t len)
{
    char path[WOLF_PATH_MAX];

    if (slot < 0 || slot >= WOLF_MAX_SAVE_SLOTS || !data || !len) {
        return false;
    }
    save_path(slot, pack, path, sizeof(path));
    if (!write_save(path, pack, desc, data, len)) {
        fprintf(stderr, "Could not write %s\n", path);
        return false;
    }
    printf("Saved game slot %d -> %s\n", slot, path);
    return true;
}

wolf_save_result_t Wolf_ReadSave(int slot, const char *pack, byte *buf, size_t cap, size_t *len,
                                 char *desc)
{
    char path[WOLF_PATH_MAX];

    if (slot < 0 || slot >= WOLF_MAX_SAVE_SLOTS) {
        return WOLF_SAVE_MISSING;
    }
    save_path(slot, pack, path, sizeof(path));
    return read_save(path, pack, buf, cap, len, desc);
}

const char *Wolf_SaveResultText(wolf_save_result_t result)
{
    switch (result) {
    case WOLF_SAVE_OK:
        return "OK";
    case WOLF_SAVE_MISSING:
        return "NO SAVED GAME";
    case WOLF_SAVE_OLD:
        return "SAVED BY AN OLDER VERSION";
    case WOLF_SAVE_OTHER_PACK:
        return "SAVED WITH OTHER GAME DATA";
    default:
        return "SAVED GAME IS DAMAGED";
    }
}

boolean Wolf_UserDataSmokeVerify(void)
{
    char dir[WOLF_PATH_MAX];
    char cfg[WOLF_PATH_MAX];
    char savep[WOLF_PATH_MAX];
    char smokename[32], savename[48];
    int old_scale;
    boolean ok = true;

    if (!Wolf_GetAppDataDir(dir, sizeof(dir))) {
        fprintf(stderr, "UserData smoke: AppData dir failed\n");
        return false;
    }
    printf("UserData smoke: AppData=%s\n", dir);

    /* Primary store must be under APPDATA, not beside the build exe. */
    {
#ifdef _WIN32
        const char *appdata = getenv("APPDATA");
        if (!appdata || !appdata[0] ||
            _strnicmp(dir, appdata, strlen(appdata)) != 0) {
            fprintf(stderr, "UserData smoke: AppData path not under %%APPDATA%%\n");
            ok = false;
        } else {
            printf("UserData smoke: path under %%APPDATA%% (good)\n");
        }
#else
        printf("UserData smoke: non-Windows path isolation assumed\n");
#endif
    }

    old_scale = g_config.scale;
    g_config.scale = 4;
    g_config.fullscreen = 0;
    snprintf(g_config.last_data, sizeof(g_config.last_data), "%s", "Wolf3dgame");
    if (!Wolf_ConfigSave()) {
        fprintf(stderr, "UserData smoke: ConfigSave failed\n");
        ok = false;
    }
    g_config.scale = 1;
    if (!Wolf_ConfigLoad() || g_config.scale != 4) {
        fprintf(stderr, "UserData smoke: ConfigLoad roundtrip failed (scale=%d)\n",
                g_config.scale);
        ok = false;
    } else {
        printf("UserData smoke: config roundtrip OK (scale=%d)\n", g_config.scale);
    }
    g_config.scale = old_scale ? old_scale : 3;
    (void)Wolf_ConfigSave();

    if (!Wolf_GetAppDataPath(WOLF_CONFIG_NAME, cfg, sizeof(cfg))) {
        ok = false;
    } else {
        printf("UserData smoke: config path=%s\n", cfg);
    }

    /* the save file format, on a file of its own: the player's slots stay untouched */
    /* files of this process's own: the WL1 and WL6 smoke tests run at the same time */
    snprintf(smokename, sizeof(smokename), "smoke%d", (int)WOLF_GETPID());
    snprintf(savename, sizeof(savename), "%s_save.w64", smokename);
    if (!Wolf_GetAppDataPath(savename, savep, sizeof(savep))) {
        ok = false;
    } else {
        static byte data[1000], back[1000];
        size_t len = 0;
        char desc[WOLF_SAVE_DESC];
        int i;

        for (i = 0; i < (int)sizeof(data); i++) {
            data[i] = (byte)(i * 7);
        }
        if (!write_save(savep, "WL6", "SMOKE", data, sizeof(data)) ||
            read_save(savep, "WL6", back, sizeof(back), &len, desc) != WOLF_SAVE_OK ||
            len != sizeof(data) || memcmp(data, back, len) != 0 || strcmp(desc, "SMOKE") != 0) {
            fprintf(stderr, "UserData smoke: save file roundtrip failed\n");
            ok = false;
        } else if (read_save(savep, "WL1", back, sizeof(back), &len, NULL) !=
                   WOLF_SAVE_OTHER_PACK) {
            fprintf(stderr, "UserData smoke: save file loaded into the wrong pack\n");
            ok = false;
        } else {
            printf("UserData smoke: save/load OK (format v%d, %u bytes)\n", WOLF_SAVE_VERSION,
                   (unsigned)len);
        }
        remove(savep);
    }

    /* the high score file, on a pack name of its own */
    {
        static const byte scores[66] = {'S', 'M', 'O', 'K', 'E'};
        byte back[66];
        char path[WOLF_PATH_MAX];

        if (!Wolf_ScoresSave(smokename, scores, sizeof(scores)) ||
            !Wolf_ScoresLoad(smokename, back, sizeof(back)) ||
            memcmp(scores, back, sizeof(back)) != 0) {
            fprintf(stderr, "UserData smoke: high score file roundtrip failed\n");
            ok = false;
        } else {
            printf("UserData smoke: high score file OK\n");
        }
        scores_path(smokename, path, sizeof(path));
        remove(path);
    }

    return ok;
}
