/*
 * Actor AI, doors and pushwalls of the faithful engine (P4/S2a).
 *
 * Without arguments: synthetic scenarios with hand-derived expectations
 * (reaction delay from the RNG table, death/pain state chains, door and
 * pushwall timing, sight lines) plus a golden trace hash that pins the whole
 * simulation. With --data DIR: every map whose actors are all ported runs for
 * 2100 tics on all four difficulties with the player standing still.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "id_ca.h"
#include "id_us_rnd.h"
#include "orig.h"
#include "wl_data.h"

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

static void record_sound(int sound)
{
    if (g_numsounds < MAXSOUNDS) {
        g_sounds[g_numsounds++] = sound;
    }
}

static void record_sound_loc(int sound, fixed gx, fixed gy)
{
    (void)gx;
    (void)gy;
    record_sound(sound);
}

static int count_sound(int sound)
{
    int i, n = 0;

    for (i = 0; i < g_numsounds; i++) {
        n += g_sounds[i] == sound;
    }
    return n;
}

static void install_hooks(void)
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
static void empty_map(void)
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

/* Wall column at x from y0 to y1, everything east of it becomes area 1. */
static void split_areas(int wallx)
{
    int x, y;

    for (y = 1; y < MAPSIZE - 1; y++) {
        AT(plane0, wallx, y) = 1;
        for (x = wallx + 1; x < MAPSIZE - 1; x++) {
            AT(plane0, x, y) = AREATILE + 1;
        }
    }
}

static void update_plu(void)
{
    plux = (uint16_t)(player->x >> UNSIGNEDSHIFT);
    pluy = (uint16_t)(player->y >> UNSIGNEDSHIFT);
}

static void load_level(int difficulty)
{
    mapsegs[0] = plane0;
    mapsegs[1] = plane1;
    mapwidth = mapheight = 64;

    NewGame(difficulty, 0);
    US_InitRndT(false);
    SetupLoadedLevel();
    update_plu();
    pwallstate = 0;
    playstate = ex_stillplaying;
    godmode = false;
    tics = 1;
    install_hooks();
}

/* One PlayLoop iteration without input, refresh or palette work. */
static void run_tic(void)
{
    madenoise = false;
    MoveDoors();
    MovePWalls();
    for (obj = player; obj; obj = obj->next) {
        DoActor(obj);
    }
    gamestate.TimeCount += tics;
}

static objtype *first_actor(void)
{
    return player->next;
}

static int state_index(const statetype *s)
{
    int i;

    for (i = 0; i < orig_numstates; i++) {
        if (orig_statelist[i] == s) {
            return i;
        }
    }
    return -1;
}

/* ------------------------------------------------------------ state tables */

static void test_state_tables(void)
{
    int i, null_next = 0;

    CHECK(orig_numstates == 303);
    for (i = 0; i < orig_numstates; i++) {
        const statetype *s = orig_statelist[i];
        CHECK(s->tictime >= 0);
        CHECK(s->rotate >= 0 && s->rotate <= 2);
        CHECK(s->shapenum >= 0 && s->shapenum <= SPR_CHAINATK4);
        if (s->next) {
            CHECK(state_index(s->next) >= 0);
        } else {
            null_next++;
        }
    }
    /* s_smoke4, s_boom3 and s_deathcam end their chains (object removed) */
    CHECK(null_next == 3);

    CHECK(s_grdstand.think == T_Stand && s_grdstand.next == &s_grdstand);
    CHECK(s_grdpath4.next == &s_grdpath1 && s_grdpath1s.think == NULL);
    CHECK(s_grdpain.rotate == 2 && s_grdpain1.rotate == 2);
    CHECK(s_grdshoot2.action == T_Shoot && s_grddie1.action == A_DeathScream);
    CHECK(s_dogjump2.action == T_Bite && s_dogchase1.think == T_DogChase);
    CHECK(s_rocket.think == T_Projectile && s_rocket.action == A_Smoke);
    CHECK(s_ssshoot1.tictime == 20 && s_ofcshoot1.tictime == 6);
}

/* ------------------------------------------------------------- sight lines */

static void test_checkline(void)
{
    objtype *guard;

    empty_map();
    AT(plane1, 10, 10) = 19;  /* player, north */
    AT(plane1, 15, 10) = 108; /* guard */
    load_level(gd_hard);
    guard = first_actor();

    CHECK(CheckLine(guard));

    tilemap[12][10] = 1; /* wall in between */
    CHECK(!CheckLine(guard));

    doornum = 1; /* closed door in between */
    tilemap[12][10] = 0x80;
    doorposition[0] = 0;
    CHECK(!CheckLine(guard));
    /*
     * The intercept is the absolute y of the line in 1/256 tiles (10.5 tiles
     * = 2688 here), compared with the door position: the door stops blocking
     * sight once it is open at least that far, not when the line clears it.
     */
    doorposition[0] = 2687;
    CHECK(!CheckLine(guard));
    doorposition[0] = 2688;
    CHECK(CheckLine(guard));
    doorposition[0] = 0xffff;
    CHECK(CheckLine(guard));

    tilemap[12][10] = 0;
    guard->dir = east; /* looking away, player more than MINSIGHT behind */
    CHECK(!CheckSight(guard));
    guard->dir = west;
    CHECK(CheckSight(guard));
}

/* -------------------------------------------------------- reaction / chase */

static void test_guard_reaction(void)
{
    objtype *guard;
    int tic, reacted = 0, shot_tic = 0;

    empty_map();
    AT(plane1, 10, 10) = 19;      /* player */
    AT(plane1, 15, 10) = 108 + 2; /* guard facing west, towards the player */
    load_level(gd_hard);
    guard = first_actor();
    CHECK(guard->state == &s_grdstand && guard->dir == west);

    /* no RNG used while spawning: SightPlayer draws rndtable[1] = 8 -> 1 + 8/4 */
    for (tic = 1; tic <= 10 && !reacted; tic++) {
        run_tic();
        if (tic == 1) {
            CHECK(guard->temp2 == 3);
        }
        if (guard->flags & FL_ATTACKMODE) {
            reacted = tic;
        }
    }
    CHECK(reacted == 4);
    CHECK(guard->state == &s_grdchase1 && guard->speed == SPDPATROL * 3);
    CHECK(guard->flags & FL_FIRSTATTACK);
    CHECK(count_sound(HALTSND) == 1);

    for (tic = 0; tic < 700 && !shot_tic; tic++) {
        run_tic();
        if (count_sound(NAZIFIRESND)) {
            shot_tic = tic + 1;
        }
    }
    CHECK(shot_tic > 0);
    CHECK(gamestate.health <= 100);
}

/* ----------------------------------------------------------- pain / death */

static void test_damage_and_death(void)
{
    objtype *guard;
    statobj_t *clip;
    int tic;

    empty_map();
    AT(plane1, 10, 10) = 19;
    AT(plane1, 15, 10) = 108; /* facing east, away from the player */
    load_level(gd_hard);
    guard = first_actor();

    DamageActor(guard, 10); /* doubled: not in attack mode */
    CHECK(guard->hitpoints == 5);
    CHECK(guard->state == &s_grdpain); /* odd hitpoints */
    CHECK(guard->flags & FL_ATTACKMODE);
    CHECK(madenoise);

    DamageActor(guard, 10);
    CHECK(guard->hitpoints == -5);
    CHECK(guard->state == &s_grddie1 && guard->ticcount == 15);
    CHECK(gamestate.score == 100 && gamestate.killcount == 1);
    CHECK(!(guard->flags & FL_SHOOTABLE) && (guard->flags & FL_NONMARK));
    clip = laststatobj - 1;
    CHECK(clip->itemnumber == bo_clip2 && clip->tilex == 15 && clip->tiley == 10);

    for (tic = 1; tic <= 45; tic++) {
        run_tic();
        if (tic == 44) {
            CHECK(guard->state == &s_grddie3);
        }
    }
    CHECK(guard->state == &s_grddie4);
    CHECK(count_sound(DEATHSCREAM1SND) + count_sound(DEATHSCREAM2SND) +
              count_sound(DEATHSCREAM3SND) + count_sound(DEATHSCREAM4SND) +
              count_sound(DEATHSCREAM5SND) + count_sound(DEATHSCREAM7SND) +
              count_sound(DEATHSCREAM8SND) + count_sound(DEATHSCREAM9SND) ==
          1);

    /* SS drop a machine gun while the player only has the pistol */
    empty_map();
    AT(plane1, 10, 10) = 19;
    AT(plane1, 15, 10) = 126;
    load_level(gd_hard);
    KillActor(first_actor());
    CHECK((laststatobj - 1)->itemnumber == bo_machinegun);

    /* extra life every 40000 points */
    gamestate.score = 39950;
    GivePoints(100);
    CHECK(gamestate.lives == 4 && gamestate.nextextra == 80000);
    CHECK(count_sound(BONUS1UPSND) == 1);

    /* baby mode quarters damage; death sets playstate */
    gamestate.difficulty = gd_baby;
    gamestate.health = 100;
    TakeDamage(40, NULL);
    CHECK(gamestate.health == 90);
    gamestate.difficulty = gd_hard;
    TakeDamage(200, first_actor());
    CHECK(gamestate.health == 0 && playstate == ex_died && killerobj == first_actor());
}

/* ------------------------------------------------------------------ doors */

static void test_door_cycle(void)
{
    int tic;

    empty_map();
    split_areas(20);
    AT(plane0, 20, 10) = 90; /* vertical door between area 0 and 1 */
    AT(plane1, 10, 10) = 19;
    load_level(gd_hard);

    CHECK(doornum == 1 && !areabyplayer[1]);
    OpenDoor(0);

    for (tic = 1; tic <= 64; tic++) {
        MoveDoors();
        if (tic == 1) {
            CHECK(doorposition[0] == 1024 && areaconnect[0][1] == 1 && areabyplayer[1]);
            CHECK(count_sound(OPENDOORSND) == 1);
        }
    }
    CHECK(doorobjlist[0].action == dr_open && doorposition[0] == 0xffff);
    CHECK(actorat[20][10] == NULL);

    for (tic = 1; tic <= 300; tic++) {
        MoveDoors();
        if (tic == 299) {
            CHECK(doorobjlist[0].action == dr_open);
        }
    }
    CHECK(doorobjlist[0].action == dr_closing);
    CHECK(ACTORAT_VALUE(actorat[20][10]) == 0x80);
    CHECK(count_sound(CLOSEDOORSND) == 1);

    for (tic = 1; tic <= 64; tic++) {
        MoveDoors();
    }
    CHECK(doorobjlist[0].action == dr_closed && doorposition[0] == 0);
    CHECK(areaconnect[0][1] == 0 && !areabyplayer[1]);
}

static void test_guard_opens_door(void)
{
    objtype *guard;
    int tic;

    empty_map();
    split_areas(20);
    AT(plane0, 20, 10) = 90;
    AT(plane1, 10, 10) = 19;
    AT(plane1, 18, 10) = 112; /* patrol heading east, through the door */
    load_level(gd_hard);
    guard = first_actor();
    CHECK(guard->tilex == 19);

    for (tic = 0; tic < 400 && doorobjlist[0].action == dr_closed; tic++) {
        run_tic();
    }
    CHECK(doorobjlist[0].action != dr_closed); /* TryWalk asked for it */
}

/* --------------------------------------------------------------- pushwalls */

static int pushwall_distance(int tics_per_frame)
{
    int frames, x;

    empty_map();
    AT(plane0, 30, 30) = 5;
    AT(plane1, 30, 30) = PUSHABLETILE;
    AT(plane1, 29, 30) = 19;
    load_level(gd_hard);
    CHECK(gamestate.secrettotal == 1);

    PushWall(30, 30, di_east);
    CHECK(gamestate.secretcount == 1 && pwallstate == 1);
    CHECK(tilemap[30][30] == (5 | 0xc0) && tilemap[31][30] == 5);
    CHECK(AT(plane1, 30, 30) == 0 && count_sound(PUSHWALLSND) == 1);

    tics = (uint16_t)tics_per_frame;
    for (frames = 0; frames < 1000 && pwallstate; frames++) {
        MovePWalls();
    }
    for (x = 30; x < 40; x++) {
        if (tilemap[x][30]) {
            return x - 30;
        }
    }
    return -1;
}

static void test_pushwall(void)
{
    /*
     * MovePWalls stops once pwallstate > 256. With one tic per frame it lands
     * on exactly 256 first and moves once more, so the wall ends three tiles
     * away; with two tics per frame it skips 256 and ends two tiles away.
     * The original behaves the same, so the distance depends on frame rate.
     */
    CHECK(pushwall_distance(1) == 3);
    CHECK(pushwall_distance(2) == 2);
}

/* ---------------------------------------------------------------- patrols */

static void test_patrol_loop(void)
{
    objtype *guard;
    int tic, turns = 0;
    dirtype lastdir;
    static const dirtype order[4] = {north, west, south, east};

    empty_map();
    split_areas(40); /* player out of sight in area 1 */
    AT(plane1, 50, 50) = 19;
    AT(plane1, 11, 20) = 112;            /* patrol heading east */
    AT(plane1, 14, 20) = ICONARROWS + 2; /* north */
    AT(plane1, 14, 16) = ICONARROWS + 4; /* west */
    AT(plane1, 10, 16) = ICONARROWS + 6; /* south */
    AT(plane1, 10, 20) = ICONARROWS + 0; /* east */
    load_level(gd_hard);
    guard = first_actor();
    CHECK(guard->ticcount == rndtable[1] % 20);

    lastdir = guard->dir;
    CHECK(lastdir == east);
    for (tic = 0; tic < 6000 && turns < 8; tic++) {
        run_tic();
        if (guard->dir != lastdir) {
            CHECK(guard->dir == order[turns % 4]); /* turns follow the arrows */
            lastdir = guard->dir;
            turns++;
        }
        CHECK(guard->tiley >= 16 && guard->tiley <= 20 && guard->tilex >= 10 &&
              guard->tilex <= 14);
    }
    CHECK(turns == 8); /* two laps */
    CHECK(!(guard->flags & FL_ATTACKMODE));
    CHECK(US_RndT() == rndtable[2]); /* patrolling used no randomness */
}

/* ------------------------------------------------------------ golden trace */

static uint32_t fnv(uint32_t h, const void *data, size_t n)
{
    const unsigned char *p = (const unsigned char *)data;
    size_t i;

    for (i = 0; i < n; i++) {
        h ^= p[i];
        h *= 16777619u;
    }
    return h;
}

static uint32_t hash_world(uint32_t h)
{
    objtype *ob;

    for (ob = player; ob; ob = ob->next) {
        const int32_t v[10] = {ob->x, ob->y, ob->tilex, ob->tiley, ob->dir,
                               state_index(ob->state), ob->ticcount, ob->hitpoints, ob->flags,
                               ob->distance};
        h = fnv(h, v, sizeof(v));
    }
    h = fnv(h, &gamestate.health, sizeof(gamestate.health));
    h = fnv(h, doorposition, sizeof(doorposition));
    return h;
}

static uint32_t run_trace(void)
{
    uint32_t h = 2166136261u;
    int tic;

    empty_map();
    split_areas(40);
    AT(plane0, 40, 12) = 90;      /* door to area 1 */
    AT(plane1, 8, 12) = 19;       /* player */
    AT(plane1, 30, 12) = 108 + 2; /* guard facing the player */
    AT(plane1, 30, 20) = 134 + 0; /* standing dog: rewrites the SS patrol (quirk) */
    AT(plane1, 25, 30) = 138;     /* dog patrol east */
    AT(plane1, 35, 30) = ICONARROWS + 4;
    AT(plane1, 20, 30) = ICONARROWS + 0;
    AT(plane1, 50, 12) = 130 + 2; /* SS patrol west, through the door */
    AT(plane1, 28, 6) = 126 + 3;  /* SS standing, facing south */
    load_level(gd_hard);
    godmode = true; /* keep the player alive for the whole trace */

    for (tic = 0; tic < 1500; tic++) {
        run_tic();
        h = hash_world(h);
    }
    return h;
}

static void test_golden_trace(void)
{
    const uint32_t a = run_trace();
    const uint32_t b = run_trace();

    printf("golden trace: %08x\n", a);
    CHECK(a == b);
    CHECK(a == 0x45b140d7u); /* regression pin: update only for an intended change */
}

/* -------------------------------------------------------------- real data */

static boolean actors_all_ported(void)
{
    objtype *ob;

    for (ob = player->next; ob; ob = ob->next) {
        switch (ob->obclass) {
        case schabbobj:
        case giftobj:
        case fatobj:
        case fakeobj:
        case mechahitlerobj:
            return false; /* special attacks arrive in S2b */
        default:
            break;
        }
    }
    return true;
}

static int run_pack(int argc, char **argv)
{
    wolf_data_dir_t dir;
    int mapnum, difficulty, runs = 0, skipped = 0;
    long sightings = 0, shots = 0;

    memset(&dir, 0, sizeof(dir));
    if (!Wolf_ResolveDataDir(argc, argv, &dir)) {
        return 1;
    }
    CA_Startup();
    if (!CA_OpenMapFiles(&dir)) {
        fprintf(stderr, "FAIL: could not open maps in %s\n", dir.path);
        return 1;
    }

    for (mapnum = 0; mapnum < NUMMAPS; mapnum++) {
        if (!mapheaderseg[mapnum]) {
            continue;
        }
        for (difficulty = gd_baby; difficulty <= gd_hard; difficulty++) {
            int tic;
            objtype *ob;

            NewGame(difficulty, mapnum / 10);
            gamestate.mapon = (int16_t)(mapnum % 10);
            demoplayback = true;
            CHECK(SetupGameLevel());
            demoplayback = false;
            if (!actors_all_ported()) {
                skipped++;
                continue;
            }
            update_plu();
            pwallstate = 0;
            playstate = ex_stillplaying;
            godmode = true;
            tics = 1;
            install_hooks();

            for (tic = 0; tic < 2100; tic++) {
                run_tic();
                for (ob = player->next; ob; ob = ob->next) {
                    if (ob->tilex >= MAPSIZE || ob->tiley >= MAPSIZE || ob->x < 0 ||
                        ob->y < 0 || ob->x >= (64l << TILESHIFT) ||
                        ob->y >= (64l << TILESHIFT)) {
                        fprintf(stderr, "FAIL: map %d diff %d tic %d: actor left the map\n",
                                mapnum, difficulty, tic);
                        g_failures++;
                        return 0;
                    }
                }
            }
            for (ob = player->next; ob; ob = ob->next) {
                sightings += (ob->flags & FL_ATTACKMODE) != 0;
            }
            shots += count_sound(NAZIFIRESND) + count_sound(SSFIRESND) +
                     count_sound(BOSSFIRESND);
            runs++;
        }
    }
    printf("%d map/difficulty runs of 2100 tics (%d skipped: S2b bosses); "
           "%ld actors alerted, %ld shots fired\n",
           runs, skipped, sightings, shots);
    CHECK(runs > 0);
    return 0;
}

int main(int argc, char **argv)
{
    InitGame();

    if (argc > 1) {
        if (run_pack(argc, argv)) {
            return 1;
        }
    } else {
        test_state_tables();
        test_checkline();
        test_guard_reaction();
        test_damage_and_death();
        test_door_cycle();
        test_guard_opens_door();
        test_pushwall();
        test_patrol_loop();
        test_golden_trace();
    }

    mapsegs[0] = mapsegs[1] = NULL;
    if (g_failures) {
        fprintf(stderr, "%d check(s) failed\n", g_failures);
        return 1;
    }
    printf("test_orig_ai: all checks passed\n");
    return 0;
}
