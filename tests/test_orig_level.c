/*
 * Level setup of the faithful engine (P4/S1).
 *
 * Without arguments: a synthetic 64x64 map exercises SetupLoadedLevel and the
 * spawn functions. With --data DIR: every map of the pack is set up on all
 * four difficulties and checked against counts taken from the raw planes.
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

#define CHECK_MAP(cond, mapnum, diff)                                                  \
    do {                                                                               \
        if (!(cond)) {                                                                 \
            fprintf(stderr, "FAIL %s:%d: map %d difficulty %d: %s\n", __FILE__, __LINE__, \
                    (mapnum), (diff), #cond);                                          \
            g_failures++;                                                              \
        }                                                                              \
    } while (0)

/* ---------------------------------------------------------------- synthetic */

static uint16_t plane0[MAPAREA], plane1[MAPAREA];

#define AT(plane, x, y) ((plane)[(y) * MAPSIZE + (x)])

static void build_test_map(void)
{
    int x, y;

    for (y = 0; y < MAPSIZE; y++) {
        for (x = 0; x < MAPSIZE; x++) {
            const int edge = x == 0 || y == 0 || x == MAPSIZE - 1 || y == MAPSIZE - 1;
            AT(plane0, x, y) = edge ? 1 : AREATILE;
            AT(plane1, x, y) = 0;
        }
    }

    /* vertical door between area 1 (west) and area 2 (east), walls north/south */
    AT(plane0, 10, 9) = 1;
    AT(plane0, 10, 11) = 1;
    AT(plane0, 9, 10) = AREATILE + 1;
    AT(plane0, 11, 10) = AREATILE + 2;
    AT(plane0, 10, 10) = 90;

    AT(plane1, 5, 5) = 19;      /* player, facing north */
    AT(plane1, 6, 5) = 23 + 1;  /* green barrel: block */
    AT(plane1, 7, 5) = 23 + 29; /* cross: treasure */

    AT(plane1, 20, 20) = 108; /* guard, all difficulties, east */
    AT(plane1, 21, 20) = 144; /* guard, medium and up */
    AT(plane1, 22, 20) = 180; /* guard, hard only */

    AT(plane1, 30, 30) = 112 + 1; /* patrolling guard heading north */

    /*
     * Standing guard on an ambush tile. SpawnStand checks east, north, south,
     * west and the last area tile found wins, so west (area 4) beats east (3).
     */
    AT(plane0, 40, 40) = AMBUSHTILE;
    AT(plane0, 40, 39) = 1;
    AT(plane0, 40, 41) = 1;
    AT(plane0, 41, 40) = AREATILE + 3;
    AT(plane0, 39, 40) = AREATILE + 4;
    AT(plane1, 40, 40) = 108;

    AT(plane1, 45, 45) = 134; /* standing dog: spawns nothing (original bug) */

    AT(plane1, 50, 50) = 98; /* pushwall */
}

static objtype *find_actor_at(int tilex, int tiley)
{
    objtype *ob;

    for (ob = player->next; ob; ob = ob->next) {
        if (ob->tilex == tilex && ob->tiley == tiley) {
            return ob;
        }
    }
    return NULL;
}

static int count_actors(void)
{
    int n = 0;
    objtype *ob;

    for (ob = player->next; ob; ob = ob->next) {
        n++;
    }
    return n;
}

static void setup_test_level(int difficulty)
{
    build_test_map();
    mapsegs[0] = plane0;
    mapsegs[1] = plane1;
    mapwidth = mapheight = 64;

    NewGame(difficulty, 0);
    US_InitRndT(false);
    SetupLoadedLevel();
}

static void test_synthetic_level(void)
{
    objtype *ob;

    setup_test_level(gd_baby);

    /* player */
    CHECK(player == &objlist[0]);
    CHECK(player->obclass == playerobj && player->active == ac_yes);
    CHECK(player->tilex == 5 && player->tiley == 5);
    CHECK(player->x == (5l << TILESHIFT) + TILEGLOBAL / 2);
    CHECK(player->angle == 90); /* (1 - NORTH) * 90 */
    CHECK(player->areanumber == 0);
    CHECK(player->flags == FL_NEVERMARK);
    CHECK(player->state == &s_player);
    CHECK(areabyplayer[0] && !areabyplayer[1]);

    /* door */
    CHECK(doornum == 1);
    CHECK(doorobjlist[0].vertical && doorobjlist[0].lock == 0 && doorobjlist[0].action == dr_closed);
    CHECK(tilemap[10][10] == 0x80);
    CHECK(tilemap[10][9] == (1 | 0x40) && tilemap[10][11] == (1 | 0x40));
    CHECK(ACTORAT_VALUE(actorat[10][10]) == 0x80);
    CHECK(AT(plane0, 10, 10) == AREATILE + 1); /* area taken from the west tile */

    /* walls */
    CHECK(tilemap[0][0] == 1 && ACTORAT_VALUE(actorat[0][0]) == 1);
    CHECK(tilemap[5][5] == 0 && actorat[5][5] == NULL);

    /* statics */
    CHECK(laststatobj - statobjlist == 2);
    CHECK(statobjlist[0].shapenum == SPR_STAT_1 && statobjlist[0].flags == 0);
    CHECK(ACTORAT_VALUE(actorat[6][5]) == 1);
    CHECK(statobjlist[1].shapenum == SPR_STAT_29 && statobjlist[1].flags == FL_BONUS &&
          statobjlist[1].itemnumber == bo_cross);
    CHECK(statobjlist[1].visspot == &spotvis[7][5]);
    CHECK(gamestate.treasuretotal == 1);
    CHECK(gamestate.secrettotal == 1);

    /* baby: guard at 20,20, the patrol and the ambush guard */
    CHECK(count_actors() == 3);
    CHECK(gamestate.killtotal == 3);

    ob = find_actor_at(20, 20);
    CHECK(ob && ob->obclass == guardobj && ob->state == &s_grdstand);
    CHECK(ob && ob->hitpoints == 25 && ob->dir == east && ob->speed == SPDPATROL);
    CHECK(ob && ob->flags == FL_SHOOTABLE && ob->active == ac_no && ob->ticcount == 0);
    CHECK(ob && actorat[20][20] == ob);

    /* the patrol moved one tile north and took the only US_RndT call */
    ob = find_actor_at(30, 29);
    CHECK(ob && ob->state == &s_grdpath1 && ob->dir == north);
    CHECK(ob && ob->active == ac_yes && ob->distance == TILEGLOBAL);
    CHECK(ob && ob->ticcount == rndtable[1] % 20);
    CHECK(ob && actorat[30][29] == ob && actorat[30][30] == NULL);
    CHECK(ob && ob->x == (30l << TILESHIFT) + TILEGLOBAL / 2); /* only the tile moved */
    CHECK(US_RndT() == rndtable[2]);

    /* ambush guard; the standing dog's fields landed on it */
    ob = find_actor_at(40, 40);
    CHECK(ob && (ob->flags & FL_AMBUSH));
    CHECK(ob && ob->areanumber == 4);
    CHECK(ob && ob->obclass == dogobj && ob->hitpoints == 1);
    CHECK(tilemap[40][40] == 0 && AT(plane0, 40, 40) == AREATILE + 4);
    CHECK(find_actor_at(45, 45) == NULL);

    setup_test_level(gd_medium);
    CHECK(count_actors() == 4 && find_actor_at(21, 20) && !find_actor_at(22, 20));
    CHECK(find_actor_at(21, 20)->dir == east);

    setup_test_level(gd_hard);
    CHECK(count_actors() == 5 && find_actor_at(22, 20));
    CHECK(gamestate.killtotal == 5);

    mapsegs[0] = mapsegs[1] = NULL;
}

static void test_actor_list(void)
{
    objtype *a, *b, *c;

    InitActorList();
    CHECK(player == &objlist[0] && objcount == 1 && lastobj == player);

    GetNewActor();
    a = new;
    GetNewActor();
    b = new;
    GetNewActor();
    c = new;
    CHECK(a == &objlist[1] && b == &objlist[2] && c == &objlist[3] && objcount == 4);
    CHECK(player->next == a && a->next == b && b->next == c && c->next == NULL);
    CHECK(c->prev == b && lastobj == c);

    RemoveObj(b);
    CHECK(a->next == c && c->prev == a && objcount == 3 && b->state == NULL);

    /* freed slot is reused first */
    GetNewActor();
    CHECK(new == b && c->next == b && lastobj == b);

    RemoveObj(b);
    CHECK(lastobj == c && c->next == NULL);
}

static void test_place_item(void)
{
    build_test_map();
    InitStaticList();
    SpawnStatic(3, 3, 3); /* floor lamp */
    statobjlist[0].shapenum = -1;            /* picked up */

    PlaceItemType(bo_clip2, 4, 4);
    CHECK(laststatobj - statobjlist == 1); /* reused the free slot */
    CHECK(statobjlist[0].shapenum == SPR_STAT_26 && statobjlist[0].itemnumber == bo_clip2);
    CHECK(statobjlist[0].tilex == 4 && statobjlist[0].flags == FL_BONUS);

    PlaceItemType(bo_key1, 5, 5);
    CHECK(laststatobj - statobjlist == 2 && statobjlist[1].shapenum == SPR_STAT_20);
}

static void test_sprite_numbers(void)
{
    /* spot checks against the non-SPEAR enum in WL_DEF.H */
    CHECK(SPR_STAT_0 == 2 && SPR_STAT_47 == 49);
    CHECK(SPR_GRD_S_1 == 50 && SPR_GRD_W1_1 == 58);
    CHECK(SPR_DOG_W1_1 == SPR_GRD_SHOOT3 + 1);
    CHECK(SPR_BJ_JUMP4 + 1 == SPR_KNIFEREADY);
    CHECK(SPR_CHAINATK4 == 435);
}

/* ---------------------------------------------------------------- real data */

static int g_shared_patrol_spots;

static int count_tiles(const uint16_t *plane, int lo, int hi)
{
    int i, n = 0;

    for (i = 0; i < MAPAREA; i++) {
        if (plane[i] >= lo && plane[i] <= hi) {
            n++;
        }
    }
    return n;
}

static void check_pack_level(int mapnum, int difficulty)
{
    int doors, statics, pushwalls, starts;
    int x, y, n = 0;
    objtype *ob;

    if (!CA_CacheMap(mapnum)) {
        CHECK_MAP(false, mapnum, difficulty);
        return;
    }
    doors = count_tiles(mapsegs[0], 90, 101);
    statics = count_tiles(mapsegs[1], 23, 74);
    pushwalls = count_tiles(mapsegs[1], PUSHABLETILE, PUSHABLETILE);
    starts = count_tiles(mapsegs[1], 19, 22);

    NewGame(difficulty, mapnum / 10);
    gamestate.mapon = (int16_t)(mapnum % 10);
    demoplayback = true;
    CHECK_MAP(SetupGameLevel(), mapnum, difficulty);
    demoplayback = false;

    CHECK_MAP(starts == 1, mapnum, difficulty);
    CHECK_MAP(player->obclass == playerobj, mapnum, difficulty);
    CHECK_MAP(player->areanumber < NUMAREAS, mapnum, difficulty);
    CHECK_MAP(doornum == doors, mapnum, difficulty);
    CHECK_MAP(laststatobj - statobjlist == statics, mapnum, difficulty);
    CHECK_MAP(gamestate.secrettotal == pushwalls, mapnum, difficulty);
    CHECK_MAP(count_tiles(mapsegs[0], AMBUSHTILE, AMBUSHTILE) == 0, mapnum, difficulty);

    for (ob = player->next; ob; ob = ob->next) {
        n++;
        CHECK_MAP(ob->tilex < MAPSIZE && ob->tiley < MAPSIZE, mapnum, difficulty);
        /*
         * A patrol steps one tile off its spawn spot and clears actorat there.
         * That can wipe an earlier patrol that already stepped onto the tile,
         * or two patrols can end up on one tile. The original does the same;
         * anything else must own its tile.
         */
        if (actorat[ob->tilex][ob->tiley] != ob) {
            const objtype *owner = actorat[ob->tilex][ob->tiley];
            CHECK_MAP(ob->active == ac_yes &&
                          (owner == NULL || (ISPOINTER(owner) && owner->active == ac_yes)),
                      mapnum, difficulty);
            g_shared_patrol_spots++;
        }
        CHECK_MAP(ob->state != NULL, mapnum, difficulty);
    }
    CHECK_MAP(objcount == n + 1, mapnum, difficulty);

    for (y = 0; y < MAPSIZE; y++) {
        for (x = 0; x < MAPSIZE; x++) {
            if (tilemap[x][y] & 0x80) {
                const int door = tilemap[x][y] & 0x3f;
                CHECK_MAP(door < doornum && doorobjlist[door].tilex == x &&
                              doorobjlist[door].tiley == y,
                          mapnum, difficulty);
            }
        }
    }

    if (difficulty == gd_hard && (mapnum % 10) == 0) {
        printf("map %2d (hard): %3d actors, %2d doors, %3d statics, kills %3d, "
               "treasure %3d, secrets %2d, player %2u,%2u\n",
               mapnum, n, doornum, statics, gamestate.killtotal, gamestate.treasuretotal,
               gamestate.secrettotal, player->tilex, player->tiley);
    }
}

static int run_pack(int argc, char **argv)
{
    wolf_data_dir_t dir;
    int mapnum, difficulty, maps = 0, standing_dogs = 0;

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
        maps++;
        if (CA_CacheMap(mapnum)) {
            standing_dogs += count_tiles(mapsegs[1], 134, 137) +
                             count_tiles(mapsegs[1], 170, 173) +
                             count_tiles(mapsegs[1], 206, 209);
        }
        for (difficulty = gd_baby; difficulty <= gd_hard; difficulty++) {
            check_pack_level(mapnum, difficulty);
        }
    }
    printf("%d maps checked on 4 difficulties; standing-dog tiles in pack: %d; "
           "patrols not owning their actorat spot: %d\n",
           maps, standing_dogs, g_shared_patrol_spots);
    CHECK(maps > 0);
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
        test_sprite_numbers();
        test_actor_list();
        test_place_item();
        test_synthetic_level();
    }

    if (g_failures) {
        fprintf(stderr, "%d check(s) failed\n", g_failures);
        return 1;
    }
    printf("test_orig_level: all checks passed\n");
    return 0;
}
