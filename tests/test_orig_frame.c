/*
 * Renderer part S4c of the faithful engine: sprite scaling (WL_SCALE.C
 * ScaleShape / SimpleScaleShape), the player's weapon and ThreeDRefresh.
 *
 * Synthetic pages: walls hold their page number; every sprite is a
 * compiled shape covering columns 16-47 with one post per column (rows
 * 8-55), whose texels hold 0x80 | (shapenum & 0x7f), so a pixel tells which
 * shape drew it. With --data DIR: full frames of every map, and pinned
 * frames of E1M1 for the supported packs.
 */
#include "orig_testkit.h"
#include "wl_data.h"

#include "id_pm.h"

#define SPRITESTART 106
#define SHAPE_LEFT 16
#define SHAPE_RIGHT 47
#define POST_TOP 8
#define POST_END 56

static byte wallpage[PMPAGESIZE];
static byte shapepage[PMPAGESIZE];
static int shape_left = SHAPE_LEFT, shape_right = SHAPE_RIGHT;

static void put_word(byte *p, unsigned ofs, unsigned v)
{
    p[ofs] = (byte)(v & 0xff);
    p[ofs + 1] = (byte)(v >> 8);
}

/* A compiled shape: header, dataofs[], a pixel pool, then the post lists. */
static void build_shape(int shapenum)
{
    const int columns = shape_right - shape_left + 1;
    const unsigned pool = 4 + 2u * (unsigned)columns;
    unsigned cmds = pool + 64u * (unsigned)columns;
    int c, y;

    memset(shapepage, 0, sizeof(shapepage));
    put_word(shapepage, 0, (unsigned)shape_left);
    put_word(shapepage, 2, (unsigned)shape_right);
    for (c = 0; c < columns; c++) {
        const unsigned pixels = pool + 64u * (unsigned)c; /* texel y at pixels + y */
        for (y = 0; y < 64; y++) {
            shapepage[pixels + (unsigned)y] = (byte)(0x80 | (shapenum & 0x7f));
        }
        put_word(shapepage, 4 + 2u * (unsigned)c, cmds);
        put_word(shapepage, cmds, POST_END * 2);
        put_word(shapepage, cmds + 2, pixels); /* corrected top: texel y at top + y */
        put_word(shapepage, cmds + 4, POST_TOP * 2);
        put_word(shapepage, cmds + 6, 0);
        cmds += 8;
    }
}

static const byte *synthetic_page(int page)
{
    if (page < SPRITESTART) {
        memset(wallpage, page, sizeof(wallpage));
        return wallpage;
    }
    build_shape(page - SPRITESTART);
    return shapepage;
}

static void use_synthetic_pages(void)
{
    orig_getpage = synthetic_page;
    orig_spritestart = SPRITESTART;
}

static byte view_pixel(int x, int y)
{
    return screenbuf[screenofs + y * SCREENWIDTH + x];
}

static boolean is_shape_pixel(byte b)
{
    return (b & 0x80) != 0 && b != 0xff;
}

static void clear_frame(void)
{
    memset(screenbuf, 0xff, sizeof(screenbuf));
    numscaledraws = 0;
}

/* -------------------------------------------------------------- scalers */

static void test_texel_width_and_masks(void)
{
    int s, sum, cols[12], n;

    for (s = 0; s < 64; s++) {
        CHECK(TexelWidth(64, s) == 1 && TexelWidth(128, s) == 2);
    }
    for (s = 0, sum = 0; s < 64; s++) {
        sum += TexelWidth(90, s); /* widths add up to the height */
    }
    CHECK(sum == 90);

    NewViewSize(15);
    n = MaskedColumns(5, 3, cols);
    CHECK(n == 3 && cols[0] == 5 && cols[1] == 6 && cols[2] == 7);
    n = MaskedColumns(3, 8, cols);
    CHECK(n == 8 && cols[0] == 3 && cols[7] == 10);
    n = MaskedColumns(7, 2, cols);
    CHECK(n == 2 && cols[0] == 7 && cols[1] == 8);
    /*
     * Width 9 at x 0 reads past the 8-entry row into mapmasks1[1][0] (plane 1)
     * and mapmasks2[1][0] (0): one pixel at x 1, as the original's tables give.
     */
    n = MaskedColumns(0, 9, cols);
    CHECK(n == 1 && cols[0] == 1);
    n = MaskedColumns(viewwidth - 1, 3, cols); /* clipped at the view's right edge */
    CHECK(n == 1 && cols[0] == viewwidth - 1);
}

/* --------------------------------------------------------------- sprites */

/* An empty view: walls far away, nothing drawn yet. */
static void open_view(void)
{
    int x;

    use_synthetic_pages();
    empty_map();
    AT(plane1, 32, 32) = 20;
    load_level(gd_hard);
    NewViewSize(15);
    for (x = 0; x < viewwidth; x++) {
        wallheight[x] = 0;
    }
    clear_frame();
}

static void test_scale_shape_placement(void)
{
    const int xcenter = 100;
    const int toppix = (120 - 64) / 2; /* scaler of height 64 at view height 120 */
    int x, y;

    open_view();
    /* height 32 << 3: scaler index 32 -> 64 pixels, one pixel per texel */
    ScaleShape(xcenter, 5, 32 << 3);
    CHECK(numscaledraws == 1 && scaledraws[0].shapenum == 5);

    /* columns 16..47 land on xcenter-16 .. xcenter+15 (column 32 at xcenter) */
    for (x = 0; x < viewwidth; x++) {
        const int col = x - xcenter + 32;
        const boolean inside = col >= SHAPE_LEFT && col <= SHAPE_RIGHT;
        CHECK(is_shape_pixel(view_pixel(x, toppix + 30)) == inside);
    }
    /* the post covers texel rows 8..55 */
    for (y = 0; y < viewheight; y++) {
        const boolean inside = y >= toppix + POST_TOP && y < toppix + POST_END;
        CHECK(is_shape_pixel(view_pixel(xcenter, y)) == inside);
        if (inside) {
            CHECK(view_pixel(xcenter, y) == (0x80 | 5));
        }
    }

    /* twice the height: two pixels per texel (index 64 is below stepbytwo 80 at view size 20) */
    NewViewSize(20);
    clear_frame();
    ScaleShape(xcenter, 6, 64 << 3);
    CHECK(ScalerHeight(64) == 128);
    CHECK(is_shape_pixel(view_pixel(xcenter - 32, (160 - 128) / 2 + 2 * POST_TOP)));
    CHECK(!is_shape_pixel(view_pixel(xcenter - 33, 60)));
    CHECK(is_shape_pixel(view_pixel(xcenter + 31, 60)) && !is_shape_pixel(view_pixel(xcenter + 32, 60)));
}

static void test_scale_shape_limits(void)
{
    open_view();
    ScaleShape(100, 1, 7); /* scale 0: too far */
    ScaleShape(100, 1, (unsigned)(maxscale + 1) << 3); /* too close */
    CHECK(numscaledraws == 2);
    CHECK(screenbuf[screenofs + 60 * SCREENWIDTH + 100] == 0xff);
    ScaleShape(100, 1, (unsigned)maxscale << 3); /* the largest still drawn */
    CHECK(is_shape_pixel(view_pixel(100, 60)));

    /* partly off the left and right edges */
    clear_frame();
    ScaleShape(5, 2, 32 << 3);
    CHECK(is_shape_pixel(view_pixel(0, 60)) && is_shape_pixel(view_pixel(20, 60)));
    clear_frame();
    ScaleShape(viewwidth - 5, 2, 32 << 3);
    CHECK(is_shape_pixel(view_pixel(viewwidth - 1, 60)));
}

static void test_scale_shape_occlusion(void)
{
    const int xcenter = 100;
    int x;

    /* a closer wall covers the left half: those columns stay unpainted */
    open_view();
    for (x = 0; x < xcenter; x++) {
        wallheight[x] = 40 << 3;
    }
    ScaleShape(xcenter, 3, 32 << 3);
    CHECK(!is_shape_pixel(view_pixel(xcenter - 1, 60)));
    CHECK(is_shape_pixel(view_pixel(xcenter, 60)) && is_shape_pixel(view_pixel(xcenter + 15, 60)));

    /* a wall equally close hides it too (>=) */
    clear_frame();
    for (x = 0; x < viewwidth; x++) {
        wallheight[x] = 32 << 3;
    }
    ScaleShape(xcenter, 3, 32 << 3);
    CHECK(!is_shape_pixel(view_pixel(xcenter, 60)));

    /* a wall slice in the middle of the shape: both sides drawn around it */
    NewViewSize(20);
    clear_frame();
    for (x = 0; x < viewwidth; x++) {
        wallheight[x] = (x >= xcenter - 4 && x < xcenter + 4) ? 80 << 3 : 0; /* closer */
    }
    ScaleShape(xcenter, 3, 64 << 3); /* two pixels per texel */
    CHECK(is_shape_pixel(view_pixel(xcenter - 10, 60)) && is_shape_pixel(view_pixel(xcenter + 10, 60)));
    CHECK(!is_shape_pixel(view_pixel(xcenter, 60)) && !is_shape_pixel(view_pixel(xcenter - 4, 60)));
}

static void test_weapon(void)
{
    const int xcenter = 120; /* viewwidth / 2 */
    int x, drawn = 0, drawn_expected;

    open_view();
    for (x = 0; x < viewwidth; x++) {
        wallheight[x] = 0x7fff; /* walls everywhere: SimpleScaleShape ignores them */
    }
    DrawPlayerWeapon();
    /* pistol ready: height viewheight+1 -> scaler index 60 -> 120 pixels */
    CHECK(view_pixel(xcenter, 60) == (0x80 | ((SPR_PISTOLREADY) & 0x7f)));
    for (x = 0; x < viewwidth; x++) {
        drawn += is_shape_pixel(view_pixel(x, 60));
    }
    for (x = SHAPE_LEFT, drawn_expected = 0; x <= SHAPE_RIGHT; x++) {
        drawn_expected += TexelWidth(120, x);
    }
    CHECK(drawn == drawn_expected); /* each column TexelWidth (120, c) wide */

    gamestate.weaponframe = 2;
    clear_frame();
    DrawPlayerWeapon();
    CHECK(view_pixel(xcenter, 60) == (0x80 | ((SPR_PISTOLREADY + 2) & 0x7f)));

    /* demo playback adds the DEMO sign over the weapon */
    demoplayback = true;
    clear_frame();
    DrawPlayerWeapon();
    demoplayback = false;
    CHECK(view_pixel(xcenter, 60) == (0x80 | (SPR_DEMO & 0x7f)));

    /* victory: no weapon; the death cam shows its sign every other 32 tics */
    gamestate.victoryflag = true;
    clear_frame();
    DrawPlayerWeapon();
    CHECK(!is_shape_pixel(view_pixel(xcenter, 60)));
    player->state = &s_deathcam;
    TimeCount = 32;
    DrawPlayerWeapon();
    CHECK(view_pixel(xcenter, 60) == (0x80 | (SPR_DEATHCAM & 0x7f)));
    gamestate.victoryflag = false;
    TimeCount = 0;
}

/* ---------------------------------------------------------- ThreeDRefresh */

static int g_fizzles;

static void count_fizzle(void)
{
    g_fizzles++;
}

static void test_three_d_refresh(void)
{
    objtype *guard;
    int32_t frames;

    use_synthetic_pages();
    empty_map();
    AT(plane1, 10, 10) = 20;
    AT(plane1, 13, 10) = 108 + 2; /* guard facing the player */
    AT(plane0, 20, 9) = 1;
    load_level(gd_hard);
    NewViewSize(15);
    orig_hooks.fizzle_in = count_fizzle;
    g_fizzles = 0;
    guard = first_actor();
    frames = frameon;
    gamestate.weapon = (weapontype)-1; /* no hands over the guard */

    memset(spotvis, 1, sizeof(spotvis)); /* stale marks from a previous frame */
    clear_frame();
    ThreeDRefresh();
    CHECK(frameon == frames + 1);
    CHECK(!spotvis[40][40]); /* cleared and traced again */
    CHECK(guard->active == ac_yes && (guard->flags & FL_VISABLE));
    /* the guard's front frame stands over the floor at the view centre */
    CHECK(view_pixel(centerx, 60) == (0x80 | (SPR_GRD_S_1 & 0x7f)) ||
          view_pixel(centerx + 1, 60) == (0x80 | (SPR_GRD_S_1 & 0x7f)));
    CHECK(numscaledraws == 1);

    /* fizzle after a level start resets the clock */
    fizzlein = true;
    TimeCount = 500;
    lasttimecount = 400;
    ThreeDRefresh();
    CHECK(!fizzlein && g_fizzles == 1 && TimeCount == 0 && lasttimecount == 0);

    /* a wall between hides the guard and its neighbour tiles: not visible, not drawn */
    tilemap[12][9] = tilemap[12][10] = tilemap[12][11] = 1;
    actorat[12][10] = ACTORAT_TILE(1);
    clear_frame();
    ThreeDRefresh();
    CHECK(!(guard->flags & FL_VISABLE) && numscaledraws == 0);
}

/* ---------------------------------------------------------- golden frames */

static uint32_t run_frames(void)
{
    uint32_t h = 2166136261u;
    int angle, size;

    use_synthetic_pages();
    empty_map();
    AT(plane1, 16, 20) = 20;
    AT(plane1, 20, 20) = 108 + 2;
    AT(plane1, 22, 18) = 126;
    AT(plane1, 19, 23) = 23 + 3;  /* floor lamp */
    AT(plane1, 24, 21) = 23 + 26; /* clip */
    AT(plane0, 21, 19) = 2;
    AT(plane0, 26, 20) = 90;
    AT(plane0, 26, 19) = 1;
    AT(plane0, 26, 21) = 1;
    load_level(gd_hard);
    doorposition[0] = 0x5000;

    for (size = 4; size <= 20; size += 4) {
        NewViewSize(size);
        for (angle = 0; angle < ANGLES; angle += 30) {
            uint32_t fh;
            player->angle = (int16_t)angle;
            clear_frame();
            ThreeDRefresh();
            fh = fnv(2166136261u, screenbuf, sizeof(screenbuf));
            h = fnv(h, &fh, sizeof(fh));
        }
    }
    return h;
}

static void test_golden_frames(void)
{
    const uint32_t a = run_frames();
    const uint32_t b = run_frames();

    printf("golden frames: %08x\n", a);
    CHECK(a == b);
    CHECK(a == 0x3fd1af7du); /* regression pin: update only for an intended change */
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
    long sprites = 0;
    uint32_t e1m1 = 0;

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
        int angle;
        uint32_t h = 2166136261u;

        if (!mapheaderseg[mapnum]) {
            continue;
        }
        NewGame(gd_hard, mapnum / 10);
        gamestate.mapon = (int16_t)(mapnum % 10);
        demoplayback = false;
        CHECK(SetupGameLevel());
        install_hooks();
        NewViewSize(20);

        for (angle = 0; angle < ANGLES; angle += 45) {
            uint32_t fh;
            player->angle = (int16_t)angle;
            clear_frame();
            ThreeDRefresh();
            sprites += numscaledraws;
            fh = fnv(2166136261u, screenbuf, sizeof(screenbuf));
            h = fnv(h, &fh, sizeof(fh));
        }
        if (mapnum == 0) {
            e1m1 = h;
        }
        maps++;
    }
    printf("%d maps: full frames at 8 angles, %ld sprites drawn; E1M1 %08x\n", maps, sprites,
           e1m1);
    CHECK(maps > 0);
    CHECK(e1m1 == 0x06ae13b7u); /* E1M1 is the same level in WL1 and WL6 */
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
        test_texel_width_and_masks();
        test_scale_shape_placement();
        test_scale_shape_limits();
        test_scale_shape_occlusion();
        test_weapon();
        test_three_d_refresh();
        test_golden_frames();
    }

    mapsegs[0] = mapsegs[1] = NULL;
    if (g_failures) {
        fprintf(stderr, "%d check(s) failed\n", g_failures);
        return 1;
    }
    printf("test_orig_frame: all checks passed\n");
    return 0;
}
