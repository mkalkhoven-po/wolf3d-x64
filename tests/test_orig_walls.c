/*
 * Renderer part S4b of the faithful engine: the raycaster (WL_DR_A.ASM), the
 * wall/door/pushwall posts and the column scaler, into the 320x200 frame.
 *
 * Synthetic texture pages make every pixel decodable: in "page" mode each
 * texel holds its page number, in "column" mode its column (0-63) plus 64 for
 * odd (vertical-wall) pages. Expected rows, heights and columns are worked out
 * by hand from the original formulas. Golden frame hashes pin whole frames.
 * With --data DIR: real start views of every map are rendered and checked,
 * and E1M1 / E2M1 frames are pinned for the supported packs.
 */
#include "orig_testkit.h"
#include "wl_data.h"

#include "id_pm.h"
#include "wl_fixed.h"

/* ------------------------------------------------------- synthetic pages */

static byte pages[128][PMPAGESIZE];
static int g_pagemode; /* 0: texel = page number, 1: texel = column (+64 if odd page) */

static void build_pages(void)
{
    int p, c, r;

    for (p = 0; p < 128; p++) {
        for (c = 0; c < 64; c++) {
            for (r = 0; r < 64; r++) {
                pages[p][c * 64 + r] =
                    (byte)(g_pagemode ? c + ((p & 1) ? 64 : 0) : p);
            }
        }
    }
}

static const byte *synthetic_page(int page)
{
    return page >= 0 && page < 128 ? pages[page] : NULL;
}

static void use_pages(int mode)
{
    g_pagemode = mode;
    build_pages();
    orig_getpage = synthetic_page;
    orig_spritestart = 106; /* WL6: DOORWALL is page 98 */
}

/* pixel at view row y, view column x */
static byte view_pixel(int x, int y)
{
    return screenbuf[screenofs + y * SCREENWIDTH + x];
}

static void render(void)
{
    memset(screenbuf, 0xff, sizeof(screenbuf));
    memset(spotvis, 0, sizeof(spotvis));
    VGAClearScreen();
    WallRefresh();
}

/* Player at (10.5, 10.5) facing east, view size 15, wall column at x. */
static void room_with_wall_at(int wallx)
{
    int y;

    empty_map();
    for (y = 1; y < MAPSIZE - 1; y++) {
        AT(plane0, wallx, y) = 1;
    }
    AT(plane1, 10, 10) = 20;
    load_level(gd_hard);
    NewViewSize(15);
}

/* ---------------------------------------------------------------- scalers */

static void test_scaler_table(void)
{
    NewViewSize(15);
    /* SetupScaling (240*1.5): 180 scalers of even heights, the last index 179 */
    CHECK(maxscale == 179 && maxscaleshl2 == 179 * 4 && stepbytwo == 60);
    CHECK(ScalerHeight(0) == 2 && ScalerHeight(1) == 2 && ScalerHeight(59) == 118);
    /* from stepbytwo on, one scaler serves three table entries */
    CHECK(ScalerHeight(60) == 120 && ScalerHeight(61) == 120 && ScalerHeight(62) == 120);
    CHECK(ScalerHeight(63) == 126 && ScalerHeight(179) == 354);

    NewViewSize(20);
    CHECK(maxscale == 239 && stepbytwo == 80);
    NewViewSize(15);
}

static void test_scale_column(void)
{
    byte src[64], col[120 * 4];
    int i;

    for (i = 0; i < 64; i++) {
        src[i] = (byte)(i + 1);
    }
    NewViewSize(15); /* viewheight 120 */

    /* height 64: one row per texel, centred: rows 28-91 */
    memset(col, 0, sizeof(col));
    ScaleColumn(src, 64, col, 1);
    CHECK(col[27] == 0 && col[28] == 1 && col[91] == 64 && col[92] == 0);

    /* height 2: texels 31 and 63 land on rows 59 and 60 */
    memset(col, 0, sizeof(col));
    ScaleColumn(src, 2, col, 1);
    CHECK(col[58] == 0 && col[59] == 32 && col[60] == 64 && col[61] == 0);

    /*
     * height 240, clipped to the view: texel s covers the full-column rows
     * [floor(s*3.75), floor((s+1)*3.75)), shifted up by 60
     */
    memset(col, 0, sizeof(col));
    ScaleColumn(src, 240, col, 1);
    for (i = 0; i < 64; i++) {
        int row;
        for (row = i * 240 / 64; row < (i + 1) * 240 / 64; row++) {
            if (row - 60 >= 0 && row - 60 < 120) {
                CHECK(col[row - 60] == i + 1);
            }
        }
    }

    /* the pitch steps rows */
    memset(col, 0, sizeof(col));
    ScaleColumn(src, 64, col, 4);
    CHECK(col[28 * 4] == 1 && col[28 * 4 + 1] == 0);
}

/* ------------------------------------------------------------------ walls */

static void test_wall_straight_ahead(void)
{
    int x;

    use_pages(1);
    room_with_wall_at(12);
    render();

    /*
     * CalcHeight: heightnumerator / (120573 >> 8) = 357 -> scaler index
     * 357 >> 3 = 44 -> 88 pixels, rows 16-103 of the 120-row view.
     */
    CHECK(wallheight[centerx] == 357);
    CHECK(view_pixel(centerx, 15) == 0x1d && view_pixel(centerx, 16) != 0x1d);
    CHECK(view_pixel(centerx, 103) != 0x19 && view_pixel(centerx, 104) == 0x19);

    /*
     * The two centre columns (pixelangle 0) cast at angle 0, which the
     * 0-89 degree code steps with ystep = -finetangent[0] = -57 (the half
     * pixel bias): the hit is at y = 10.498, texture column 31 of
     * vertwall[1], an odd page.
     */
    CHECK(pixelangle[centerx] == 0 && pixelangle[centerx + 1] == 0);
    CHECK(view_pixel(centerx, 60) == 64 + 31 && view_pixel(centerx + 1, 60) == 64 + 31);

    /*
     * The wall is flat: every column the same height. The texture column
     * grows left to right and wraps from 63 to 0 at each tile edge.
     */
    for (x = 1; x < viewwidth; x++) {
        const int prev = view_pixel(x - 1, 60), cur = view_pixel(x, 60);
        CHECK(wallheight[x] >= 355 && wallheight[x] <= 357);
        CHECK(cur >= prev || (prev >= 64 + 60 && cur <= 64 + 3));
    }

    /* ceiling 0x1d on E1 maps, floor 0x19; outside the window untouched */
    CHECK(view_pixel(0, 0) == 0x1d && view_pixel(0, viewheight - 1) == 0x19);
    CHECK(screenbuf[0] == 0xff);

    /*
     * Rays mark the tiles they enter: the next one is, the wall and beyond
     * are not. The player's own tile is only marked when a ray re-enters it.
     */
    CHECK(spotvis[11][10] && !spotvis[12][10] && !spotvis[13][10]);
}

static void test_wall_texture_direction(void)
{
    use_pages(1);
    room_with_wall_at(12);

    /* looking west at a wall on the other side: the texture runs mirrored */
    AT(plane0, 7, 10) = 1;
    player->angle = 180;
    render();
    /* hit at y = 10.502: column 32 from the west side reads 0xfc0 - 32*64 = column 31 */
    CHECK(view_pixel(centerx, 60) == 64 + 31);

    /* horizontal walls use the even horizwall pages */
    use_pages(0);
    empty_map();
    AT(plane1, 10, 10) = 19; /* facing north */
    load_level(gd_hard);
    NewViewSize(15);
    render();
    CHECK(view_pixel(centerx, 60) == horizwall[1] && horizwall[1] == 0);
    player->angle = 0;
    render();
    CHECK(view_pixel(centerx, 60) == vertwall[1] && vertwall[1] == 1);
}

static void test_doors(void)
{
    use_pages(0);
    room_with_wall_at(15);
    /* a vertical door at x=12 between wall tiles, open floor beyond */
    AT(plane0, 12, 9) = 1;
    AT(plane0, 12, 11) = 1;
    AT(plane0, 12, 10) = 90;
    load_level(gd_hard);
    NewViewSize(15);

    /* closed: the vertical door page DOORWALL+1 = 99, drawn at the tile centre */
    render();
    CHECK(view_pixel(centerx, 60) == 99);
    CHECK(wallheight[centerx] == heightnumerator / ((FixedByFrac(
                                     ((12l << TILESHIFT) + 0x8000) - viewx, viewcos)) >> 8));
    /* oblique rays that meet the door tile's side pass on and mark it */
    CHECK(spotvis[11][10] && !spotvis[14][10]);

    /* fully open: the rays pass the door plane and hit the wall at x=15 */
    doorposition[0] = 0xffff;
    render();
    CHECK(view_pixel(centerx, 60) == vertwall[1] && spotvis[12][10] && spotvis[14][10]);

    /*
     * Half open: rays crossing the door plane at a y fraction below
     * doorposition go through, so the open half is the north (low y) side,
     * left of centre when facing east.
     */
    doorposition[0] = 0x8000;
    render();
    CHECK(view_pixel(centerx - 10, 60) == vertwall[1]); /* through the gap */
    CHECK(view_pixel(centerx + 10, 60) == 99);           /* the door's closed half */

    /* locked and elevator doors */
    doorposition[0] = 0;
    doorobjlist[0].lock = dr_lock1;
    render();
    CHECK(view_pixel(centerx, 60) == 98 + 6 + 1);
    doorobjlist[0].lock = dr_elevator;
    render();
    CHECK(view_pixel(centerx, 60) == 98 + 4 + 1);
}

static void test_door_jambs(void)
{
    int x, jambs = 0;

    use_pages(0);
    room_with_wall_at(15);
    AT(plane0, 12, 9) = 1;
    AT(plane0, 12, 11) = 1;
    AT(plane0, 12, 10) = 90;
    load_level(gd_hard);
    NewViewSize(15);

    /* in the open doorway, the side walls are the door frame (DOORWALL+2) */
    doorposition[0] = 0xffff;
    player->x = (12l << TILESHIFT) + 0x4000;
    player->tilex = 12;
    player->angle = 90; /* look north at the frame */
    render();
    for (x = 0; x < viewwidth; x++) {
        jambs += view_pixel(x, 60) == 98 + 2;
    }
    CHECK(jambs > 0);
}

static void test_pushwall(void)
{
    int16_t at12, at13;

    use_pages(0);
    room_with_wall_at(20);
    AT(plane0, 12, 10) = 5;
    load_level(gd_hard);
    NewViewSize(15);

    render();
    at12 = (int16_t)wallheight[centerx];
    CHECK(view_pixel(centerx, 60) == vertwall[5]);

    /* sliding east, halfway: the face is drawn at x = 12.5 */
    tilemap[12][10] |= 0xc0;
    tilemap[13][10] = 5;
    pwallpos = 32;
    render();
    CHECK(view_pixel(centerx, 60) == vertwall[5]);
    CHECK(wallheight[centerx] < (uint16_t)at12);

    tilemap[12][10] = 0;
    tilemap[13][10] = 5;
    render();
    at13 = (int16_t)wallheight[centerx];
    pwallpos = 32;
    tilemap[12][10] = 5 | 0xc0;
    render();
    CHECK(wallheight[centerx] > (uint16_t)at13);
}

static void test_open_area_and_angles(void)
{
    int angle;

    use_pages(1);
    empty_map();
    AT(plane1, 32, 32) = 19;
    load_level(gd_hard);

    for (angle = 0; angle < ANGLES; angle += 15) {
        int x, filled = 0;
        player->angle = (int16_t)angle;
        render();
        for (x = 0; x < viewwidth; x++) {
            filled += wallheight[x] > 0;
        }
        CHECK(filled == viewwidth); /* the border is always hit */
        CHECK(view_pixel(0, 0) == 0x1d && view_pixel(0, viewheight - 1) == 0x19);
    }
}

/* ----------------------------------------------------------- golden frames */

static uint32_t frame_hash(void)
{
    uint32_t h = fnv(2166136261u, screenbuf, sizeof(screenbuf));
    h = fnv(h, wallheight, sizeof(wallheight));
    return fnv(h, spotvis, sizeof(spotvis));
}

static void test_golden_frames(void)
{
    uint32_t h = 2166136261u;
    int angle, size;

    use_pages(1);
    empty_map();
    AT(plane0, 20, 20) = 3;
    AT(plane0, 21, 20) = 4 | 0; /* plain walls in the room */
    AT(plane0, 25, 18) = 90;    /* a door */
    AT(plane0, 25, 17) = 1;
    AT(plane0, 25, 19) = 1;
    AT(plane1, 16, 20) = 20;
    load_level(gd_hard);
    doorposition[0] = 0x6000;

    for (size = 4; size <= 20; size += 4) {
        NewViewSize(size);
        for (angle = 0; angle < ANGLES; angle += 30) {
            player->angle = (int16_t)angle;
            render();
            h = fnv(h, &(uint32_t){frame_hash()}, sizeof(uint32_t));
        }
    }
    printf("golden frames: %08x\n", h);
    CHECK(h == 0x4965275au); /* regression pin: update only for an intended change */
    NewViewSize(15);
}

/* -------------------------------------------------------------- real data */

static const byte *pack_page(int page)
{
    return (const byte *)PM_GetPage(page);
}

static int run_pack(int argc, char **argv)
{
    wolf_data_dir_t dir;
    int mapnum, maps = 0;
    uint32_t e1m1 = 0, e2m1 = 0;

    memset(&dir, 0, sizeof(dir));
    if (!Wolf_ResolveDataDir(argc, argv, &dir)) {
        return 1;
    }
    CA_Startup();
    if (!CA_OpenMapFiles(&dir) || !PM_OpenPageFile(&dir)) {
        fprintf(stderr, "FAIL: could not open %s\n", dir.path);
        return 1;
    }
    orig_getpage = pack_page;
    orig_spritestart = (int16_t)PMSpriteStart;

    for (mapnum = 0; mapnum < NUMMAPS; mapnum++) {
        int angle, views_seeing = 0;
        uint32_t h = 2166136261u;

        if (!mapheaderseg[mapnum]) {
            continue;
        }
        NewGame(gd_hard, mapnum / 10);
        gamestate.mapon = (int16_t)(mapnum % 10);
        demoplayback = true;
        CHECK(SetupGameLevel());
        demoplayback = false;
        NewViewSize(20);

        for (angle = 0; angle < ANGLES; angle += 45) {
            int x, seen = 0;
            player->angle = (int16_t)angle;
            render();
            for (x = 0; x < viewwidth; x++) {
                CHECK(wallheight[x] > 0);
            }
            for (x = 0; x < MAPSIZE * MAPSIZE; x++) {
                seen += spotvis[x / MAPSIZE][x % MAPSIZE];
            }
            views_seeing += seen > 0; /* facing an adjacent wall, no ray enters a tile */
            h = fnv(h, &(uint32_t){frame_hash()}, sizeof(uint32_t));
        }
        CHECK(views_seeing > 0);
        if (mapnum == 0) {
            e1m1 = h;
        }
        if (mapnum == 10) {
            e2m1 = h;
        }
        maps++;
    }
    printf("%d maps rendered at 8 angles; E1M1 %08x, E2M1 %08x\n", maps, e1m1, e2m1);
    CHECK(maps > 0);
    /* pins for the supported packs (WL1 Apogee 1.4, WL6 1.4): E1M1 is the same level in both */
    CHECK(e1m1 == 0xdd560acdu);
    if (mapheaderseg[10]) {
        CHECK(e2m1 == 0xa8d91d3au);
    }
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
        test_scaler_table();
        test_scale_column();
        test_wall_straight_ahead();
        test_wall_texture_direction();
        test_doors();
        test_door_jambs();
        test_pushwall();
        test_open_area_and_angles();
        test_golden_frames();
    }

    mapsegs[0] = mapsegs[1] = NULL;
    if (g_failures) {
        fprintf(stderr, "%d check(s) failed\n", g_failures);
        return 1;
    }
    printf("test_orig_walls: all checks passed\n");
    return 0;
}
