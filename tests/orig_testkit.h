#pragma once

/*
 * Shared helpers for the faithful-engine tests: a CHECK macro, sound
 * recording hooks, synthetic 64x64 maps and a PlayLoop-like tic. Header-only;
 * each test executable gets its own copy of the state.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "id_ca.h"
#include "id_us_rnd.h"
#include "orig.h"

static int g_failures;

#define CHECK(cond)                                                          \
    do {                                                                     \
        if (!(cond)) {                                                       \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);  \
            g_failures++;                                                    \
        }                                                                    \
    } while (0)

/* ------------------------------------------------------------------ hooks */

#define MAXSOUNDS 4096
static int g_sounds[MAXSOUNDS];
static int g_numsounds;

static inline void record_sound(int sound)
{
    if (g_numsounds < MAXSOUNDS) {
        g_sounds[g_numsounds++] = sound;
    }
}

static inline void record_sound_loc(int sound, fixed gx, fixed gy)
{
    (void)gx;
    (void)gy;
    record_sound(sound);
}

static inline int count_sound(int sound)
{
    int i, n = 0;

    for (i = 0; i < g_numsounds; i++) {
        n += g_sounds[i] == sound;
    }
    return n;
}

static inline void install_hooks(void)
{
    OrigHooks_Reset();
    orig_hooks.play_sound = record_sound;
    orig_hooks.play_sound_loc = record_sound_loc;
    g_numsounds = 0;
}

/* ------------------------------------------------------------- map helpers */

static uint16_t plane0[MAPAREA], plane1[MAPAREA];

#define AT(plane, x, y) ((plane)[(y) * MAPSIZE + (x)])

/* 64x64 floor of area 0 inside a wall border. */
static inline void empty_map(void)
{
    int x, y;

    for (y = 0; y < MAPSIZE; y++) {
        for (x = 0; x < MAPSIZE; x++) {
            const int edge = x == 0 || y == 0 || x == MAPSIZE - 1 || y == MAPSIZE - 1;
            AT(plane0, x, y) = edge ? 1 : AREATILE;
            AT(plane1, x, y) = 0;
        }
    }
}

/* Wall column at wallx; everything east of it becomes area 1. */
static inline void split_areas(int wallx)
{
    int x, y;

    for (y = 1; y < MAPSIZE - 1; y++) {
        AT(plane0, wallx, y) = 1;
        for (x = wallx + 1; x < MAPSIZE - 1; x++) {
            AT(plane0, x, y) = AREATILE + 1;
        }
    }
}

static inline void update_plu(void)
{
    plux = (uint16_t)(player->x >> UNSIGNEDSHIFT);
    pluy = (uint16_t)(player->y >> UNSIGNEDSHIFT);
}

static inline void clear_input(void)
{
    controlx = controly = 0;
    memset(buttonstate, 0, sizeof(buttonstate));
    memset(buttonheld, 0, sizeof(buttonheld));
}

static inline void load_level(int difficulty)
{
    mapsegs[0] = plane0;
    mapsegs[1] = plane1;
    mapwidth = mapheight = 64;

    NewGame(difficulty, 0);
    US_InitRndT(false);
    clear_input();
    anglefrac = 0;
    facecount = 0;
    SetupLoadedLevel();
    update_plu();
    pwallstate = 0;
    fizzlein = false;
    playstate = ex_stillplaying;
    godmode = false;
    noclip = false;
    tics = 1;
    install_hooks();
}

/* One PlayLoop iteration without input polling, refresh or palette work. */
static inline void run_tic(void)
{
    madenoise = false;
    MoveDoors();
    MovePWalls();
    for (obj = player; obj; obj = obj->next) {
        DoActor(obj);
    }
    gamestate.TimeCount += tics;
}

static inline objtype *first_actor(void)
{
    return player->next;
}

static inline int state_index(const statetype *s)
{
    int i;

    for (i = 0; i < orig_numstates; i++) {
        if (orig_statelist[i] == s) {
            return i;
        }
    }
    return -1;
}

static inline uint32_t fnv(uint32_t h, const void *data, size_t n)
{
    const unsigned char *p = (const unsigned char *)data;
    size_t i;

    for (i = 0; i < n; i++) {
        h ^= p[i];
        h *= 16777619u;
    }
    return h;
}
