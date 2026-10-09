#include "wolf_userdata.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#define WOLF_MKDIR(p) _mkdir(p)
#else
#include <sys/stat.h>
#include <sys/types.h>
#define WOLF_MKDIR(p) mkdir((p), 0755)
#endif


wolf_config_t g_config;
wolf_highscore_t g_highscores[WOLF_MAX_HIGH_SCORES];

boolean Wolf_GetAppDataDir(char *out, size_t out_sz)
{
    if (!out || out_sz < 16) {
        return false;
    }
#ifdef _WIN32
    {
        const char *appdata = getenv("APPDATA");
        const char *up;
        if (appdata && appdata[0]) {
            snprintf(out, out_sz, "%s\\%s", appdata, WOLF_APPDATA_DIRNAME);
        } else {
            up = getenv("USERPROFILE");
            if (!up || !up[0]) {
                return false;
            }
            snprintf(out, out_sz, "%s\\AppData\\Roaming\\%s", up, WOLF_APPDATA_DIRNAME);
        }
        WOLF_MKDIR(out);
        return true;
    }
#else
    {
        const char *home = getenv("HOME");
        if (!home) {
            return false;
        }
        snprintf(out, out_sz, "%s/.local/share/%s", home, WOLF_APPDATA_DIRNAME);
        WOLF_MKDIR(out);
        return true;
    }
#endif
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
    g_config.mouse_sens = 8;
    g_config.music_on = 1;
    g_config.sfx_on = 1;
    g_config.last_data[0] = '\0';
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
    if (g_config.mouse_sens < 1) {
        g_config.mouse_sens = 1;
    }
    if (g_config.mouse_sens > 20) {
        g_config.mouse_sens = 20;
    }
    g_config.music_on = g_config.music_on ? 1 : 0;
    g_config.sfx_on = g_config.sfx_on ? 1 : 0;
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
        } else if (strcmp(key, "mouse_sens") == 0) {
            g_config.mouse_sens = atoi(val);
        } else if (strcmp(key, "music_on") == 0) {
            g_config.music_on = atoi(val);
        } else if (strcmp(key, "sfx_on") == 0) {
            g_config.sfx_on = atoi(val);
        } else if (strcmp(key, "last_data") == 0) {
            snprintf(g_config.last_data, sizeof(g_config.last_data), "%s", val);
        }
    }
    fclose(f);
    clamp_config();
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
    fprintf(f, "# Wolf3D Win64 config (do not put next to Program Files)\n");
    fprintf(f, "scale=%d\n", g_config.scale);
    fprintf(f, "fullscreen=%d\n", g_config.fullscreen);
    fprintf(f, "mouse_look=%d\n", g_config.mouse_look);
    fprintf(f, "mouse_sens=%d\n", g_config.mouse_sens);
    fprintf(f, "music_on=%d\n", g_config.music_on);
    fprintf(f, "sfx_on=%d\n", g_config.sfx_on);
    fprintf(f, "last_data=%s\n", g_config.last_data);
    fclose(f);
    return true;
}

boolean Wolf_HighScoresLoad(void)
{
    char path[WOLF_PATH_MAX];
    FILE *f;
    int i;

    memset(g_highscores, 0, sizeof(g_highscores));
    for (i = 0; i < WOLF_MAX_HIGH_SCORES; ++i) {
        snprintf(g_highscores[i].name, sizeof(g_highscores[i].name), "BJ");
    }
    if (!Wolf_GetAppDataPath(WOLF_HIGHSCORE_NAME, path, sizeof(path))) {
        return false;
    }
    f = fopen(path, "rb");
    if (!f) {
        return false;
    }
    if (fread(g_highscores, sizeof(g_highscores), 1, f) != 1) {
        fclose(f);
        return false;
    }
    fclose(f);
    return true;
}

boolean Wolf_HighScoresSave(void)
{
    char path[WOLF_PATH_MAX];
    FILE *f;
    if (!Wolf_GetAppDataPath(WOLF_HIGHSCORE_NAME, path, sizeof(path))) {
        return false;
    }
    f = fopen(path, "wb");
    if (!f) {
        return false;
    }
    if (fwrite(g_highscores, sizeof(g_highscores), 1, f) != 1) {
        fclose(f);
        return false;
    }
    fclose(f);
    return true;
}

void Wolf_HighScoresTryInsert(int score, int episode, int level, const char *name)
{
    int i, j;
    if (score <= g_highscores[WOLF_MAX_HIGH_SCORES - 1].score) {
        return;
    }
    for (i = 0; i < WOLF_MAX_HIGH_SCORES; ++i) {
        if (score > g_highscores[i].score) {
            for (j = WOLF_MAX_HIGH_SCORES - 1; j > i; --j) {
                g_highscores[j] = g_highscores[j - 1];
            }
            memset(&g_highscores[i], 0, sizeof(g_highscores[i]));
            g_highscores[i].score = score;
            g_highscores[i].episode = episode;
            g_highscores[i].level = level;
            snprintf(g_highscores[i].name, sizeof(g_highscores[i].name), "%s",
                     name && name[0] ? name : "PLAYER");
            (void)Wolf_HighScoresSave();
            return;
        }
    }
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

static void save_path(int slot, char *out, size_t out_sz)
{
    char name[32];
    snprintf(name, sizeof(name), "save%d.w64", slot);
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

boolean Wolf_SaveExists(int slot)
{
    char path[WOLF_PATH_MAX];
    FILE *f;
    if (slot < 0 || slot >= WOLF_MAX_SAVE_SLOTS) {
        return false;
    }
    save_path(slot, path, sizeof(path));
    f = fopen(path, "rb");
    if (!f) {
        return false;
    }
    fclose(f);
    return true;
}

boolean Wolf_WriteSave(int slot, const char *pack, const char *desc, const byte *data,
                       size_t len)
{
    char path[WOLF_PATH_MAX];

    if (slot < 0 || slot >= WOLF_MAX_SAVE_SLOTS || !data || !len) {
        return false;
    }
    save_path(slot, path, sizeof(path));
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
    save_path(slot, path, sizeof(path));
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
    if (!Wolf_GetAppDataPath("smoke_save.w64", savep, sizeof(savep))) {
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

    Wolf_HighScoresTryInsert(999, 0, 0, "SMOKE");
    if (g_highscores[0].score < 999) {
        fprintf(stderr, "UserData smoke: highscore insert failed\n");
        ok = false;
    } else {
        printf("UserData smoke: highscore OK top=%d\n", g_highscores[0].score);
    }

    return ok;
}
