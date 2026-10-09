/*
 * Renderer part S4a of the faithful engine (WL_MAIN.C view size, WL_DRAW.C
 * projection and visibility).
 *
 * Without arguments: projection constants per view size, view variables,
 * TransformActor / TransformTile / CalcHeight results worked out by hand,
 * sprite rotation, and DrawScaleds (activation, FL_VISABLE, item pickup,
 * back-to-front order) including its effect on the player's hit test.
 * With --data DIR: DrawScaleds runs on every map with everything visible.
 */
#include "orig_testkit.h"
#include "wl_data.h"

#include "wl_fixed.h"

static const fixed CENTER = TILEGLOBAL / 2;

static fixed tile_center(int t)
{
    return ((fixed)t << TILESHIFT) + CENTER;
}

/* ------------------------------------------------------------- projection */

static void test_projection_constants(void)
{
    int i;

    NewViewSize(15); /* the default */
    CHECK(viewwidth == 240 && viewheight == 120);
    CHECK(centerx == 119 && shootdelta == 24);
    /* 120 * (0x5700 + 0x5800) / 0x8000 = 164.06, truncated */
    CHECK(scale == 164 && heightnumerator == 167936 && minheightdiv == 6);
    CHECK(focallength == 0x5700);

    for (i = 0; i < viewwidth / 2; i++) {
        CHECK(pixelangle[viewwidth / 2 - 1 - i] == -pixelangle[viewwidth / 2 + i]);
        if (i > 0) {
            CHECK(pixelangle[viewwidth / 2 + i] <= pixelangle[viewwidth / 2 + i - 1]);
        }
    }
    CHECK(maxslope == finetangent[pixelangle[0]] >> 8);
    printf("viewsize 15: pixelangle[0] = %d, maxslope = %d\n", pixelangle[0], maxslope);

    NewViewSize(20); /* full width */
    CHECK(viewwidth == 320 && viewheight == 160 && centerx == 159 && shootdelta == 32);
    CHECK(scale == 218 && heightnumerator == 223232 && minheightdiv == 7);
    printf("viewsize 20: pixelangle[0] = %d, maxslope = %d\n", pixelangle[0], maxslope);

    NewViewSize(15);
}

/*
 * Pixel angles for every menu view size. The closest any entry gets to an
 * integer boundary is 5e-4 of a fine angle, far above float rounding, so a
 * second compiler (MSVC in CI) must produce the same tables.
 */
static void test_pixelangle_tables(void)
{
    static const uint32_t expected[17] = {
        0xff80873du, 0xf23a9ebcu, 0xbfbd83bcu, 0x5010ba96u, 0x09b6d5cdu, 0xa72f08dau,
        0x024ffb76u, 0x4b410b88u, 0x0dace5b1u, 0xcbc3bd6eu, 0x0e2c5708u, 0x4679dce6u,
        0x039c4111u, 0xf38fb5e2u, 0x7beb759au, 0x771cdda3u, 0x0d7a92e7u};
    int size;

    for (size = 4; size <= 20; size++) {
        NewViewSize(size);
        CHECK(fnv(2166136261u, pixelangle, sizeof(int16_t) * (size_t)viewwidth) ==
              expected[size - 4]);
    }
    NewViewSize(15);
}

/* Player at (10.5, 10.5) facing east, view size 15. */
static void setup_east_view(void)
{
    empty_map();
    AT(plane1, 10, 10) = 20;
    load_level(gd_hard);
    NewViewSize(15);
    CalcViewVariables();
}

static void test_view_variables(void)
{
    setup_east_view();

    CHECK(viewangle == 0 && midangle == 0);
    CHECK(viewcos == 0xffff && viewsin == 0);
    /* the focal point sits FixedByFrac (0x5700, 0xffff) = 22271 behind the player */
    CHECK(viewx == tile_center(10) - 22271 && viewy == tile_center(10));
    CHECK(focaltx == 10 && focalty == 10 && viewtx == 10 && viewty == 10);
    CHECK(xpartialdown == 10497 && xpartialup == 55039);
    CHECK(ypartialdown == 0x8000 && ypartialup == 0x8000);

    player->angle = 90; /* facing north: the focal point moves south */
    CalcViewVariables();
    CHECK(midangle == 900 && viewy == tile_center(10) + 22271);
}

static objtype *spawn_guard_at(int x, int y)
{
    objtype *ob;

    for (ob = player->next; ob; ob = ob->next) {
        if (ob->tilex == x && ob->tiley == y) {
            return ob;
        }
    }
    return NULL;
}

static void test_transform_actor(void)
{
    objtype *ob;

    empty_map();
    AT(plane1, 10, 10) = 20;
    AT(plane1, 14, 10) = 108;     /* straight ahead */
    AT(plane1, 14, 8) = 108;      /* ahead and to the left (north) */
    AT(plane1, 10, 11) = 108;     /* beside the player */
    load_level(gd_hard);
    NewViewSize(15);
    CalcViewVariables();

    ob = spawn_guard_at(14, 10);
    TransformActor(ob);
    /* gx = 284415: nx = 284410 - ACTORSIZE, ny = 0 */
    CHECK(ob->transx == 268026 && ob->transy == 0);
    CHECK(ob->viewx == centerx);
    CHECK(ob->viewheight == 167936 / (268026 >> 8)); /* 160 */

    ob = spawn_guard_at(14, 8);
    TransformActor(ob);
    CHECK(ob->transy < 0 && ob->viewx < centerx && ob->viewheight > 0);

    /* nx below mindist: no height, viewx left as it was */
    ob = spawn_guard_at(10, 11);
    ob->viewx = 1234;
    TransformActor(ob);
    CHECK(ob->viewheight == 0 && ob->viewx == 1234);
}

static void test_transform_tile(void)
{
    int16_t x = 0, h = 0;

    setup_east_view();

    /*
     * Grab range: the tile centre has to be 0.13-0.79 tiles ahead of the
     * player (between mindist and TILEGLOBAL from the focal point, less the
     * 0x2000 object size) and within half a tile sideways.
     */
    CHECK(!TransformTile(11, 10, &x, &h) && h > 0); /* a full tile ahead: too far */
    CHECK(!TransformTile(10, 10, &x, &h) && h == 0); /* under the player: too close */

    player->x = 11l << TILESHIFT; /* half a tile from the next centre */
    CalcViewVariables();
    CHECK(TransformTile(11, 10, &x, &h) && x == centerx);
    CHECK(!TransformTile(11, 11, &x, &h)); /* a tile to the side */
}

static void test_calc_height(void)
{
    setup_east_view();

    /* a wall two tiles ahead: nx = FixedByFrac (120575, 0xffff) = 120573 */
    xintercept = 12l << TILESHIFT;
    yintercept = tile_center(10);
    CHECK(CalcHeight() == 167936 / (120573 >> 8)); /* 357 */

    /* closer than mindist is clamped */
    xintercept = viewx + 0x1000;
    CHECK(CalcHeight() == 167936 / (MINDIST >> 8));
}

static void test_calc_rotate(void)
{
    objtype *ob;

    setup_east_view();
    AT(plane1, 14, 10) = 108;
    load_level(gd_hard);
    CalcViewVariables();
    ob = first_actor();
    ob->viewx = centerx;

    ob->dir = west; /* facing the player */
    CHECK(CalcRotate(ob) == 0);
    ob->dir = east; /* back to the player */
    CHECK(CalcRotate(ob) == 4);
    ob->dir = north; /* walking to the player's left */
    CHECK(CalcRotate(ob) == 2);
    ob->dir = south;
    CHECK(CalcRotate(ob) == 6);

    ob->state = &s_grdpain; /* two-sided pain frames */
    ob->dir = west;
    CHECK(CalcRotate(ob) == 0);
    ob->dir = east;
    CHECK(CalcRotate(ob) == 4);
}

/* ------------------------------------------------------------ DrawScaleds */

static void mark_visible(int x0, int y0, int x1, int y1)
{
    int x, y;

    memset(spotvis, 0, sizeof(spotvis));
    for (x = x0; x <= x1; x++) {
        for (y = y0; y <= y1; y++) {
            spotvis[x][y] = 1;
        }
    }
}

static void test_draw_scaleds(void)
{
    objtype *a, *b, *c, *hidden;
    int i;

    empty_map();
    AT(plane1, 10, 10) = 20;
    AT(plane1, 13, 10) = 108; /* near */
    AT(plane1, 18, 10) = 108; /* far */
    AT(plane1, 15, 11) = 108; /* middle, slightly right */
    AT(plane1, 30, 30) = 108; /* elsewhere */
    AT(plane1, 16, 10) = 23 + 1; /* barrel (static) */
    load_level(gd_hard);
    NewViewSize(15);
    CalcViewVariables();
    a = spawn_guard_at(13, 10);
    b = spawn_guard_at(18, 10);
    c = spawn_guard_at(15, 11);
    hidden = spawn_guard_at(30, 30);

    mark_visible(11, 9, 20, 12);
    numscaledraws = 0;
    DrawScaleds();

    CHECK(a->active == ac_yes && (a->flags & FL_VISABLE));
    CHECK(b->active == ac_yes && (c->flags & FL_VISABLE));
    CHECK(hidden->active == ac_no && !(hidden->flags & FL_VISABLE));

    /* back to front: smallest height first */
    CHECK(numscaledraws == 4);
    for (i = 1; i < numscaledraws; i++) {
        CHECK(scaledraws[i - 1].viewheight <= scaledraws[i].viewheight);
    }
    CHECK(scaledraws[0].viewheight == (int16_t)b->viewheight);
    CHECK(scaledraws[3].viewheight == (int16_t)a->viewheight);
    /* standing guards facing east: back view, rotation 4 */
    CHECK(scaledraws[3].shapenum == SPR_GRD_S_1 + 4);

    /* seen through a neighbouring open tile, not through a wall */
    mark_visible(17, 10, 17, 10);
    tilemap[17][10] = 0;
    b->flags &= ~FL_VISABLE;
    DrawScaleds();
    CHECK(b->flags & FL_VISABLE);
    tilemap[17][10] = 1;
    DrawScaleds();
    CHECK(!(b->flags & FL_VISABLE));
    tilemap[17][10] = 0;

    /* shapenum 0 means no shape: not placed */
    a->state = &s_deathcam; /* shapenum 0: no shape, skipped */
    mark_visible(11, 9, 20, 12);
    numscaledraws = 0;
    DrawScaleds();
    CHECK(numscaledraws == 3);
}

static void test_bonus_pickup(void)
{
    statobj_t *cross;

    empty_map();
    AT(plane1, 10, 10) = 20;
    AT(plane1, 11, 10) = 23 + 29; /* cross */
    load_level(gd_hard);
    cross = &statobjlist[0];
    mark_visible(10, 10, 11, 10);

    CalcViewVariables();
    numscaledraws = 0;
    DrawScaleds();
    CHECK(cross->shapenum == SPR_STAT_29 && numscaledraws == 1); /* drawn, too far */

    player->x = 11l << TILESHIFT;
    CalcViewVariables();
    numscaledraws = 0;
    DrawScaleds();
    CHECK(cross->shapenum == -1 && gamestate.score == 100 && numscaledraws == 0);

    /* not picked up when its tile isn't visible */
    empty_map();
    AT(plane1, 10, 10) = 20;
    AT(plane1, 11, 10) = 23 + 29;
    load_level(gd_hard);
    player->x = 11l << TILESHIFT;
    CalcViewVariables();
    memset(spotvis, 0, sizeof(spotvis));
    DrawScaleds();
    CHECK(statobjlist[0].shapenum == SPR_STAT_29);
}

static void test_visibility_feeds_hit_test(void)
{
    objtype *guard;

    empty_map();
    AT(plane1, 10, 10) = 20;
    AT(plane1, 12, 10) = 108;
    load_level(gd_hard);
    NewViewSize(15);
    guard = first_actor();

    /* not drawn yet: GunAttack finds no target */
    GunAttack(player);
    CHECK(guard->hitpoints == 25);

    CalcViewVariables();
    mark_visible(10, 9, 13, 11);
    DrawScaleds();
    CHECK((guard->flags & FL_VISABLE) && guard->viewx == centerx);
    GunAttack(player);
    CHECK(guard->hitpoints == 23); /* rndtable[1] / 6, doubled */
}

static void test_vislist_overflow(void)
{
    int x, y, n = 0;

    empty_map();
    AT(plane1, 2, 32) = 20;
    for (x = 10; x < 60; x += 4) {
        for (y = 20; y < 46; y += 4) {
            AT(plane1, x, y) = 108 + 2;
            n++;
        }
    }
    load_level(gd_hard);
    CHECK(n > MAXVISABLE);
    CalcViewVariables();
    mark_visible(0, 0, MAPSIZE - 1, MAPSIZE - 1);
    numscaledraws = 0;
    DrawScaleds();
    CHECK(numscaledraws <= MAXVISABLE);
}

/* -------------------------------------------------------------- real data */

static int run_pack(int argc, char **argv)
{
    wolf_data_dir_t dir;
    int mapnum, maps = 0, i;
    long drawn = 0;

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
        int angle;

        if (!mapheaderseg[mapnum]) {
            continue;
        }
        NewGame(gd_hard, mapnum / 10);
        gamestate.mapon = (int16_t)(mapnum % 10);
        demoplayback = true;
        CHECK(SetupGameLevel());
        demoplayback = false;
        install_hooks();
        memset(spotvis, 1, sizeof(spotvis));

        for (angle = 0; angle < ANGLES; angle += 45) {
            player->angle = (int16_t)angle;
            CalcViewVariables();
            numscaledraws = 0;
            DrawScaleds();
            CHECK(numscaledraws <= MAXVISABLE);
            for (i = 1; i < numscaledraws; i++) {
                CHECK(scaledraws[i - 1].viewheight <= scaledraws[i].viewheight);
            }
            for (i = 0; i < numscaledraws; i++) {
                CHECK(scaledraws[i].viewheight > 0);
            }
            drawn += numscaledraws;
        }
        maps++;
    }
    printf("%d maps x 8 view angles: %ld sprites placed\n", maps, drawn);
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
        test_projection_constants();
        test_pixelangle_tables();
        test_view_variables();
        test_transform_actor();
        test_transform_tile();
        test_calc_height();
        test_calc_rotate();
        test_draw_scaleds();
        test_bonus_pickup();
        test_visibility_feeds_hit_test();
        test_vislist_overflow();
    }

    mapsegs[0] = mapsegs[1] = NULL;
    if (g_failures) {
        fprintf(stderr, "%d check(s) failed\n", g_failures);
        return 1;
    }
    printf("test_orig_draw: all checks passed\n");
    return 0;
}
