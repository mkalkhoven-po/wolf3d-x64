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

#include "wl_play.h"

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

#pragma pack(push, 1)
typedef struct {
    char magic[4];
    int32_t version;
    int32_t mapnum;
    int32_t episode_map;
    float px, py, pangle;
    int32_t health, lives, ammo, score, keys, weapon, best_weapon;
} wolf_save_hdr_t;
#pragma pack(pop)

static void save_path(int slot, char *out, size_t out_sz)
{
    char name[32];
    snprintf(name, sizeof(name), "save%d.w64", slot);
    (void)Wolf_GetAppDataPath(name, out, out_sz);
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

boolean Wolf_SaveGame(int slot)
{
    char path[WOLF_PATH_MAX];
    FILE *f;
    wolf_save_hdr_t hdr;
    player_t *p;

    if (slot < 0 || slot >= WOLF_MAX_SAVE_SLOTS || !g_level.active) {
        return false;
    }
    save_path(slot, path, sizeof(path));
    f = fopen(path, "wb");
    if (!f) {
        return false;
    }
    memset(&hdr, 0, sizeof(hdr));
    memcpy(hdr.magic, WOLF_SAVE_MAGIC, 4);
    hdr.version = WOLF_SAVE_VERSION;
    hdr.mapnum = g_level.mapnum;
    hdr.episode_map = g_level.episode_map;
    p = &g_level.player;
    hdr.px = p->x;
    hdr.py = p->y;
    hdr.pangle = p->angle;
    hdr.health = p->health;
    hdr.lives = p->lives;
    hdr.ammo = p->ammo;
    hdr.score = p->score;
    hdr.keys = p->keys;
    hdr.weapon = p->weapon;
    hdr.best_weapon = p->best_weapon;
    if (fwrite(&hdr, sizeof(hdr), 1, f) != 1) {
        fclose(f);
        return false;
    }
    fclose(f);
    printf("Saved game slot %d -> %s\n", slot, path);
    return true;
}

boolean Wolf_LoadGame(int slot)
{
    char path[WOLF_PATH_MAX];
    FILE *f;
    wolf_save_hdr_t hdr;
    player_t *p;

    if (slot < 0 || slot >= WOLF_MAX_SAVE_SLOTS) {
        return false;
    }
    save_path(slot, path, sizeof(path));
    f = fopen(path, "rb");
    if (!f) {
        return false;
    }
    if (fread(&hdr, sizeof(hdr), 1, f) != 1) {
        fclose(f);
        return false;
    }
    fclose(f);
    if (memcmp(hdr.magic, WOLF_SAVE_MAGIC, 4) != 0 || hdr.version != WOLF_SAVE_VERSION) {
        fprintf(stderr, "Save slot %d: bad magic/version\n", slot);
        return false;
    }
    if (!Wolf_SetupLevel(hdr.mapnum)) {
        return false;
    }
    p = &g_level.player;
    p->x = hdr.px;
    p->y = hdr.py;
    p->angle = hdr.pangle;
    p->health = hdr.health;
    p->lives = hdr.lives;
    p->ammo = hdr.ammo;
    p->score = hdr.score;
    p->keys = hdr.keys;
    p->weapon = hdr.weapon;
    p->best_weapon = hdr.best_weapon;
    p->dead = false;
    g_level.episode_map = hdr.episode_map;
    printf("Loaded game slot %d map=%d score=%d\n", slot, hdr.mapnum, hdr.score);
    return true;
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

    /* Save/load current level snapshot if active, else setup map0 temporarily. */
    {
        boolean had = g_level.active;
        if (!had) {
            Wolf_ResetPlayerStats();
            if (!Wolf_SetupLevel(0)) {
                fprintf(stderr, "UserData smoke: cannot setup level for save test\n");
                return false;
            }
        }
        g_level.player.score = 12345;
        if (!Wolf_SaveGame(0)) {
            fprintf(stderr, "UserData smoke: SaveGame failed\n");
            ok = false;
        }
        g_level.player.score = 0;
        if (!Wolf_LoadGame(0) || g_level.player.score != 12345) {
            fprintf(stderr, "UserData smoke: LoadGame failed score=%d\n",
                    g_level.player.score);
            ok = false;
        } else {
            printf("UserData smoke: save/load OK score=%d\n", g_level.player.score);
        }
        save_path(0, savep, sizeof(savep));
        printf("UserData smoke: save path=%s\n", savep);
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
