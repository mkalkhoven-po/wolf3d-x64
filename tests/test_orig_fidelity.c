/*
 * The P4/S7 fidelity gate, on a real pack (--data DIR; without one the test
 * has nothing to check and passes):
 *
 *  1. Demos at every view size. The original plays (and records) demos at
 *     whatever view size is set, and the size changes gameplay: the
 *     player's hit test and which tiles the rays reach (actor activation,
 *     bonus pickup). A demo is only certain to stay in sync at the size it
 *     was recorded at. All eight original demos end exactly on their last
 *     input frame at view sizes 7, 12, 14, 17 and 19; at the default 15
 *     the shareware's demo 0 dies 97 frames early, as the original would.
 *  2. Golden frames: hashes of the rendered frame at fixed points of every
 *     demo, so renderer changes show up.
 *  3. The L1.5 matrix on E1M1: secrets, a pushwall that moves and one that
 *     cannot, the vacated cell, ceiling and floor colours.
 *  4. E1M1 to the elevator: a bot drives the real controls (arrow keys,
 *     Space, Ctrl as scan codes) while the run is recorded as a demo; the
 *     demo then replays to the same frame.
 */

#include <math.h>

#include "orig_testkit.h"
#include "wl_data.h"

#include "id_pm.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static int g_frames;
static boolean g_wl1;

static const byte *pack_page(int page)
{
    return (const byte *)PM_GetPage(page);
}

static void count_frame(void)
{
    g_frames++;
}

static const byte *demo_lump(int d, unsigned *length, unsigned *inputframes)
{
    const byte *lump;

    CA_CacheGrChunk(T_DEMO0 + d);
    lump = grsegs[T_DEMO0 + d];
    if (!lump) {
        return NULL;
    }
    *length = (unsigned)lump[1] | ((unsigned)lump[2] << 8);
    *inputframes = (*length - 4) / 3;
    return lump;
}

/* Plays demo d at the current view size; returns the frames shown. */
static int play_demo(int d, void (*frame)(void))
{
    unsigned length, inputframes;
    const byte *lump = demo_lump(d, &length, &inputframes);

    install_hooks();
    orig_hooks.present_frame = frame;
    g_frames = 0;
    CHECK(lump && PlayDemo(lump, length));
    return g_frames;
}

/* ------------------------------------------------ 1. demos x view sizes */

#define MINVIEW 4
#define MAXVIEW 20

static void test_demo_view_sizes(void)
{
    static const int gate[] = {7, 12, 14, 17, 19};
    boolean sync[4][MAXVIEW + 1];
    int frames[4][MAXVIEW + 1];
    int v, d, g;

    printf("demo end frames by view size (* = ends on its last input frame):\n  view  ");
    for (v = MINVIEW; v <= MAXVIEW; v++) {
        printf("%6d", v);
    }
    printf("\n");
    for (d = 0; d < 4; d++) {
        unsigned length = 0, inputframes = 0;

        (void)demo_lump(d, &length, &inputframes);
        printf("  demo %d", d);
        for (v = MINVIEW; v <= MAXVIEW; v++) {
            NewViewSize(v);
            frames[d][v] = play_demo(d, count_frame);
            sync[d][v] = (unsigned)frames[d][v] == inputframes &&
                         (playstate == ex_died || playstate == ex_completed);
            printf("%5d%c", frames[d][v], sync[d][v] ? '*' : ' ');
        }
        printf("\n");
    }

    for (g = 0; g < (int)(sizeof(gate) / sizeof(gate[0])); g++) {
        for (d = 0; d < 4; d++) {
            if (!sync[d][gate[g]]) {
                fprintf(stderr, "FAIL: demo %d out of sync at view size %d (%d frames)\n", d,
                        gate[g], frames[d][gate[g]]);
                g_failures++;
            }
        }
    }
    /* the default size: all in sync but the shareware's demo 0 */
    for (d = 0; d < 4; d++) {
        if (g_wl1 && d == 0) {
            CHECK(!sync[0][15] && frames[0][15] == 1055);
        } else {
            CHECK(sync[d][15]);
        }
    }
    NewViewSize(15);
}

/* ------------------------------------------------------- 2. golden frames */

static const int goldenframes[] = {1, 100, 300, 600};
static uint32_t g_golden;

static void hash_frame(void)
{
    size_t i;

    g_frames++;
    for (i = 0; i < sizeof(goldenframes) / sizeof(goldenframes[0]); i++) {
        if (g_frames == goldenframes[i]) {
            g_golden = fnv(g_golden, screenbuf, sizeof(screenbuf));
        }
    }
}

static void test_golden_frames(void)
{
    /* view size 19, where every demo is in sync; frames 1, 100, 300, 600 (status bar included since P5a) */
    static const uint32_t wl1_golden[4] = {0xffe47c0au, 0x03b80decu, 0x79994107u, 0x4523c9adu};
    static const uint32_t wl6_golden[4] = {0x7cb2d84du, 0xc35b1952u, 0x78911785u, 0x84dd59ccu};
    int d;

    NewViewSize(19);
    for (d = 0; d < 4; d++) {
        g_golden = 2166136261u;
        play_demo(d, hash_frame);
        printf("  golden frames demo %d: %08x\n", d, g_golden);
        CHECK(g_golden == (g_wl1 ? wl1_golden : wl6_golden)[d]);
    }
    NewViewSize(15);
}

/* ------------------------------------------------------ 3. L1.5 on E1M1 */

static const int dx[4] = {1, 0, -1, 0}; /* east, north, west, south */
static const int dy[4] = {0, -1, 0, 1};

static void start_level(int map)
{
    NewGame(gd_hard, 0);
    gamestate.mapon = (int16_t)map;
    demoplayback = demorecord = loadedgame = false;
    godmode = noclip = false;
    CHECK(SetupGameLevel());
    install_hooks();
    clear_input();
    fizzlein = false;
    playstate = ex_stillplaying;
    pwallstate = 0;
    tics = 1;
}

static boolean floor_free(int x, int y)
{
    return x > 0 && y > 0 && x < MAPSIZE - 1 && y < MAPSIZE - 1 && !tilemap[x][y] &&
           !actorat[x][y];
}

static void place_player(int tx, int ty, int dir)
{
    player->tilex = (uint16_t)tx;
    player->tiley = (uint16_t)ty;
    player->x = ((fixed)tx << TILESHIFT) + TILEGLOBAL / 2;
    player->y = ((fixed)ty << TILESHIFT) + TILEGLOBAL / 2;
    player->angle = (int16_t)(dir * ANGLES / 4);
    player->areanumber = (byte)(*(mapsegs[0] + farmapylookup[ty] + tx) - AREATILE);
    update_plu();
}

/* A pushwall and a side to push it from, with the cell behind free or not. */
static boolean find_pushwall(boolean behindfree, int *wx, int *wy, int *wdir)
{
    int x, y, dir;

    for (y = 1; y < MAPSIZE - 1; y++) {
        for (x = 1; x < MAPSIZE - 1; x++) {
            if (*(mapsegs[1] + farmapylookup[y] + x) != PUSHABLETILE) {
                continue;
            }
            for (dir = 0; dir < 4; dir++) {
                const int bx = x + dx[dir], by = y + dy[dir];
                if (!floor_free(x - dx[dir], y - dy[dir])) {
                    continue;
                }
                if (behindfree ? floor_free(bx, by) && floor_free(bx + dx[dir], by + dy[dir])
                               : tilemap[bx][by] != 0 || actorat[bx][by] != NULL) {
                    *wx = x;
                    *wy = y;
                    *wdir = dir;
                    return true;
                }
            }
        }
    }
    return false;
}

static int count_row(int y, byte colour)
{
    const int x0 = screenofs % SCREENWIDTH, y0 = screenofs / SCREENWIDTH;
    int x, n = 0;

    for (x = 0; x < viewwidth; x++) {
        n += screenbuf[(y0 + y) * SCREENWIDTH + x0 + x] == colour;
    }
    return n;
}

static void test_l15_e1m1(void)
{
    int x, y, dir, t, tilevalue;

    start_level(0);
    printf("  E1M1: %d secrets, %d treasures, %d enemies\n", gamestate.secrettotal,
           gamestate.treasuretotal, gamestate.killtotal);
    CHECK(gamestate.secrettotal == 5);

    /* ceiling 0x1d (vgaCeiling for map 0) at the top of the view, floor 0x19 at the bottom */
    ThreeDRefresh();
    CHECK(count_row(0, 0x1d) > viewwidth / 4);
    CHECK(count_row(viewheight - 1, 0x19) > viewwidth / 4);

    /* a pushwall with something in the way (a wall or a blocking static) does not move */
    if (!find_pushwall(false, &x, &y, &dir)) {
        fprintf(stderr, "FAIL: no blocked pushwall on E1M1\n");
        g_failures++;
        return;
    }
    tilevalue = tilemap[x][y];
    place_player(x - dx[dir], y - dy[dir], dir);
    g_numsounds = 0;
    buttonstate[bt_use] = true;
    Cmd_Use();
    CHECK(!pwallstate && count_sound(NOWAYSND) == 1 && tilemap[x][y] == tilevalue);
    CHECK(gamestate.secretcount == 0);

    /* one with room behind slides, counts as a secret, frees its cell */
    if (!find_pushwall(true, &x, &y, &dir)) {
        fprintf(stderr, "FAIL: no free pushwall on E1M1\n");
        g_failures++;
        return;
    }
    tilevalue = tilemap[x][y];
    place_player(x - dx[dir], y - dy[dir], dir);
    g_numsounds = 0;
    Cmd_Use();
    CHECK(pwallstate && count_sound(PUSHWALLSND) == 1 && gamestate.secretcount == 1);
    for (t = 0; t < 400 && pwallstate; t++) {
        MovePWalls();
        if (t == 40) {
            ThreeDRefresh(); /* mid-slide: drawn from tile & 63, no 0xc0 page */
        }
    }
    CHECK(!pwallstate);
    CHECK(!tilemap[x][y] && !actorat[x][y]); /* walkable now */
    CHECK(tilemap[x + 2 * dx[dir]][y + 2 * dy[dir]] == tilevalue ||
          tilemap[x + 3 * dx[dir]][y + 3 * dy[dir]] == tilevalue);
    printf("  E1M1 pushwalls: blocked one stays, free one slid %s\n",
           tilemap[x + 3 * dx[dir]][y + 3 * dy[dir]] == tilevalue ? "3 tiles" : "2 tiles");
}

/* ------------------------------------------------- 4. E1M1 to the elevator */

static int g_goalx, g_goaly, g_goaldir; /* stand here, face the switch */
static int g_botframe, g_stillframes, g_lastx, g_lasty;

#define MAXBOTFRAMES 4000 /* about 4 minutes of play */

static boolean door_open(int door)
{
    return doorobjlist[door].action == dr_open;
}

static boolean door_passable(int door)
{
    const int lock = doorobjlist[door].lock;
    return lock == 0 || lock == 5 || (gamestate.keys & (1 << (lock - 1)));
}

/* The first step of a shortest path to the goal, through floor and doors. */
static boolean next_step(int *nx, int *ny)
{
    static short from[MAPSIZE][MAPSIZE];
    static int queue[MAPSIZE * MAPSIZE];
    int head = 0, tail = 0, x, y, dir;
    const int sx = player->tilex, sy = player->tiley;

    memset(from, -1, sizeof(from));
    from[sx][sy] = (short)(sy * MAPSIZE + sx);
    queue[tail++] = sy * MAPSIZE + sx;
    while (head < tail) {
        const int cur = queue[head++];
        x = cur % MAPSIZE;
        y = cur / MAPSIZE;
        if (x == g_goalx && y == g_goaly) {
            while (from[x][y] != sy * MAPSIZE + sx) {
                const int prev = from[x][y];
                x = prev % MAPSIZE;
                y = prev / MAPSIZE;
            }
            *nx = x;
            *ny = y;
            return true;
        }
        for (dir = 0; dir < 4; dir++) {
            const int ax = x + dx[dir], ay = y + dy[dir];
            const int tile = tilemap[ax][ay];
            if (from[ax][ay] != -1 || *(mapsegs[1] + farmapylookup[ay] + ax) == PUSHABLETILE) {
                continue;
            }
            if (tile && !((tile & 0xc0) == 0x80 && door_passable(tile & 0x3f))) {
                continue;
            }
            if (!tile && actorat[ax][ay] && !ISPOINTER(actorat[ax][ay])) {
                continue; /* a blocking static (table, barrel...) */
            }
            from[ax][ay] = (short)cur;
            queue[tail++] = ay * MAPSIZE + ax;
        }
    }
    return false;
}

/* turn towards angle: the keys to press now; true once facing it */
/* the angle from the player to a point, 0 east, counterclockwise */
static int angle_to(fixed x, fixed y)
{
    const double ddx = (double)(x - player->x);
    const double ddy = (double)(player->y - y);
    int angle = (int)lround(atan2(ddy, ddx) * 180.0 / M_PI);

    return angle < 0 ? angle + ANGLES : angle;
}

/* the closest living enemy on screen within two tiles */
static const objtype *nearest_enemy(void)
{
    const objtype *ob, *best = NULL;
    fixed bestdist = 2 * TILEGLOBAL;

    for (ob = player->next; ob; ob = ob->next) {
        const fixed dist = labs(ob->x - player->x) > labs(ob->y - player->y)
                               ? labs(ob->x - player->x)
                               : labs(ob->y - player->y);
        if ((ob->flags & (FL_SHOOTABLE | FL_VISABLE)) == (FL_SHOOTABLE | FL_VISABLE) &&
            dist < bestdist) {
            best = ob;
            bestdist = dist;
        }
    }
    return best;
}

static boolean steer(int angle, int tolerance)
{
    int err = angle - player->angle;

    while (err > ANGLES / 2) {
        err -= ANGLES;
    }
    while (err < -ANGLES / 2) {
        err += ANGLES;
    }
    if (err > tolerance) {
        orig_keys[sc_LeftArrow] = 1; /* angles grow counterclockwise */
    } else if (err < -tolerance) {
        orig_keys[sc_RightArrow] = 1;
    }
    return abs(err) <= tolerance;
}

static void bot_input(void)
{
    int nx, ny, tile;

    memset(orig_keys, 0, sizeof(orig_keys));
    g_botframe++;
    if (g_botframe > MAXBOTFRAMES) {
        playstate = ex_abort; /* lost: stop before the demo buffer runs out */
        return;
    }
    if (getenv("BOT_TRACE") && g_botframe % 50 == 0) {
        printf("    bot %4d: tile %2d,%2d angle %3d health %3d goal %d,%d\n", g_botframe,
               player->tilex, player->tiley, player->angle, gamestate.health, g_goalx, g_goaly);
    }

    if (player->tilex == g_lastx && player->tiley == g_lasty) {
        g_stillframes++;
    } else {
        g_stillframes = 0;
        g_lastx = player->tilex;
        g_lasty = player->tiley;
    }

    if (player->tilex == g_goalx && player->tiley == g_goaly) {
        if (steer(g_goaldir * ANGLES / 4, 10) && (g_botframe & 1)) {
            orig_keys[sc_Space] = 1; /* the switch */
        }
        return;
    }
    if (!next_step(&nx, &ny)) {
        return;
    }
    tile = tilemap[nx][ny];
    if (getenv("BOT_TRACE") && g_botframe % 50 == 0) {
        printf("      next %d,%d tile %d actorat %lu pos %.2f,%.2f ammo %d weapon %d still %d\n",
               nx, ny, tile,
               (unsigned long)(ISPOINTER(actorat[nx][ny]) ? 9999 : ACTORAT_VALUE(actorat[nx][ny])),
               player->x / 65536.0, player->y / 65536.0, gamestate.ammo, (int)gamestate.weapon,
               g_stillframes);
    }
    if ((tile & 0xc0) == 0x80 && !door_open(tile & 0x3f)) {
        /* a closed door: face it squarely and press Space (on and off for the edge) */
        const int dir = nx > player->tilex ? 0 : ny < player->tiley ? 1 : nx < player->tilex ? 2 : 3;
        if (steer(dir * ANGLES / 4, 10) && (g_botframe & 1) &&
            doorobjlist[tile & 0x3f].action == dr_closed) {
            orig_keys[sc_Space] = 1;
        }
        return;
    }
    {
        const double ddx = ((nx << TILESHIFT) + TILEGLOBAL / 2) - player->x;
        const double ddy = player->y - ((ny << TILESHIFT) + TILEGLOBAL / 2);
        int angle = (int)lround(atan2(ddy, ddx) * 180.0 / M_PI);

        if (angle < 0) {
            angle += ANGLES;
        }
        if (steer(angle, 6) || g_stillframes < 30) {
            orig_keys[sc_UpArrow] = 1;
        }
    }
    if (g_stillframes > 40) {
        /* something in the way: an actor within a tile blocks, diagonals too */
        const objtype *ob = nearest_enemy();

        if (ob) {
            /* a live one on screen: stop, turn to it, fight */
            memset(orig_keys, 0, sizeof(orig_keys));
            if (steer(angle_to(ob->x, ob->y), 4) && gamestate.weapon == wp_knife) {
                orig_keys[sc_UpArrow] = 1; /* out of bullets: close in for the knife */
            }
        }
        orig_keys[sc_Control] = (byte)(g_botframe & 1); /* a fresh press each time */
    }
}

static void test_e1m1_route(void)
{
    static byte demo[64 * 1024];
    int x, y, recorded;
    unsigned length;
    boolean found = false;

    start_level(0);
    for (y = 1; y < MAPSIZE - 1 && !found; y++) {
        for (x = 1; x < MAPSIZE - 1 && !found; x++) {
            if (tilemap[x][y] != ELEVATORTILE) {
                continue;
            }
            /* the switch is used facing east or west */
            if (floor_free(x - 1, y) || !tilemap[x - 1][y]) {
                g_goalx = x - 1, g_goaly = y, g_goaldir = 0, found = true;
            } else if (floor_free(x + 1, y) || !tilemap[x + 1][y]) {
                g_goalx = x + 1, g_goaly = y, g_goaldir = 2, found = true;
            }
        }
    }
    CHECK(found);
    if (!found) {
        return;
    }

    /* record, as RecordDemo: hard, demo timing, the random table from 0 */
    NewViewSize(15);
    NewGame(gd_hard, 0);
    gamestate.mapon = 0;
    godmode = true; /* a bot does not dodge: this checks the way through, not survival */
    demo[0] = 0;
    demorecord = true;
    StartDemoRecording(demo + 4, sizeof(demo) - 4);
    CHECK(SetupGameLevel());
    install_hooks();
    orig_hooks.poll_input = bot_input;
    orig_hooks.present_frame = count_frame;
    memset(orig_keys, 0, sizeof(orig_keys));
    g_frames = g_botframe = g_stillframes = 0;
    g_lastx = g_lasty = -1;
    fizzlein = true;
    startgame = false; /* as RecordDemo after NewGame */
    PlayLoop();
    demorecord = false;
    recorded = g_frames;
    length = (unsigned)(DemoRecordingEnd() - demo);
    demo[1] = (byte)length;
    demo[2] = (byte)(length >> 8);
    demo[3] = 0;
    printf("  E1M1 route: %s after %d frames (%.1f s), %d kills, health %d, ammo %d\n",
           playstate == ex_completed ? "elevator" : "stopped", recorded, recorded * 4 / 70.0,
           gamestate.killcount, gamestate.health, gamestate.ammo);
    CHECK(playstate == ex_completed);
    CHECK(recorded == 1381); /* the same E1M1 in both packs: any change in play shows here */

    /* the recording plays back to the same end */
    install_hooks();
    orig_hooks.present_frame = count_frame;
    g_frames = 0;
    CHECK(PlayDemo(demo, length));
    CHECK(g_frames == recorded && playstate == ex_completed);
    godmode = false;
    memset(orig_keys, 0, sizeof(orig_keys));
}

/* ---------------------------------------------------------------- main */

int main(int argc, char **argv)
{
    wolf_data_dir_t dir;

    InitGame();
    if (argc < 2) {
        printf("test_orig_fidelity: needs --data DIR; nothing to check\n");
        return 0;
    }
    memset(&dir, 0, sizeof(dir));
    if (!Wolf_ResolveDataDir(argc, argv, &dir)) {
        return 1;
    }
    CA_Startup();
    if (!CA_OpenMapFiles(&dir) || !PM_OpenPageFile(&dir) || !CA_OpenGraphFiles(&dir)) {
        fprintf(stderr, "FAIL: could not open %s\n", dir.path);
        return 1;
    }
    g_wl1 = strcmp(g_gfx->name, "WL1") == 0;
    upload = g_wl1;
    orig_getpage = pack_page;
    orig_spritestart = (int16_t)PMSpriteStart;
    printf("%s:\n", g_gfx->name);

    test_demo_view_sizes();
    test_golden_frames();
    test_l15_e1m1();
    test_e1m1_route();

    if (g_failures) {
        fprintf(stderr, "%d check(s) failed\n", g_failures);
        return 1;
    }
    printf("test_orig_fidelity: all checks passed\n");
    return 0;
}
