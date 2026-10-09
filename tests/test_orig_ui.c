/*
 * The 2D toolkit (P5a, orig_vh.c, orig_menu.c, orig_inter.c) on a real pack
 * (--data DIR; without one there is nothing to draw and the test passes):
 * the proportional fonts, the print window, pictures on 8-pixel columns,
 * palette fades, "Get Psyched!", the status bar and Message().
 */

#include "orig_testkit.h"
#include "wl_data.h"

static boolean g_wl1;

static int count_colour(int x0, int y0, int x1, int y1, byte colour)
{
    int x, y, n = 0;

    for (y = y0; y < y1; y++) {
        for (x = x0; x < x1; x++) {
            n += screenbuf[y * SCREENWIDTH + x] == colour;
        }
    }
    return n;
}

/* the leftmost and rightmost columns holding colour */
static void colour_span(byte colour, int *left, int *right)
{
    int x, y;

    *left = SCREENWIDTH;
    *right = -1;
    for (y = 0; y < SCREENHEIGHT; y++) {
        for (x = 0; x < SCREENWIDTH; x++) {
            if (screenbuf[y * SCREENWIDTH + x] == colour) {
                if (x < *left) {
                    *left = x;
                }
                if (x > *right) {
                    *right = x;
                }
            }
        }
    }
}

static void test_fonts(void)
{
    uint16_t w0, h0, w1, h1, w;
    int n;

    fontnumber = 0;
    VW_MeasurePropString("Wolfenstein", &w0, &h0);
    fontnumber = 1;
    VW_MeasurePropString("Wolfenstein", &w1, &h1);
    printf("  fonts: small %ux%u, large %ux%u for \"Wolfenstein\"\n", w0, h0, w1, h1);
    CHECK(h0 > 0 && w0 > 0 && h1 > h0 && w1 > w0);

    /* a string draws only inside its measured box, in fontcolor */
    memset(screenbuf, 0, sizeof(screenbuf));
    fontcolor = 15;
    px = 20;
    py = 30;
    VW_DrawPropString("Wolfenstein");
    n = count_colour(20, 30, 20 + w1, 30 + h1, 15);
    CHECK(n > 50 && n == count_colour(0, 0, SCREENWIDTH, SCREENHEIGHT, 15));
    CHECK(px == 20 + w1); /* the pen moved on */

    /* US_Print: a newline goes back to the window's left edge, one line down */
    memset(screenbuf, 0, sizeof(screenbuf));
    WindowX = PrintX = 40;
    WindowY = PrintY = 50;
    WindowW = 200;
    WindowH = 100;
    VW_MeasurePropString("A", &w, &h1);
    US_Print("AAA\nA");
    CHECK(PrintX == 40 + w && PrintY == 50 + h1);

    /* US_CPrint centres each line in the window */
    {
        int left, right;

        memset(screenbuf, 0, sizeof(screenbuf));
        WindowX = 0;
        WindowW = 320;
        PrintY = 10;
        fontcolor = 14;
        US_CPrint("Get Psyched");
        colour_span(14, &left, &right);
        CHECK(abs((left + right) / 2 - 160) <= 2);
        CHECK(PrintY == 10 + h1);
    }
}

static void test_pictures(void)
{
    static byte at8[sizeof(screenbuf)];

    /* x is rounded down to a multiple of 8, as the original */
    memset(screenbuf, 0, sizeof(screenbuf));
    VWB_DrawPic(8, 40, GETPSYCHEDPIC);
    memcpy(at8, screenbuf, sizeof(at8));
    CHECK(count_colour(0, 0, SCREENWIDTH, SCREENHEIGHT, 0) < (int)sizeof(screenbuf));
    memset(screenbuf, 0, sizeof(screenbuf));
    VWB_DrawPic(13, 40, GETPSYCHEDPIC);
    CHECK(memcmp(at8, screenbuf, sizeof(at8)) == 0);
}

/* ------------------------------------------------------------------ fades */

static int g_waits, g_palettes, g_updates, g_inputs;
static byte g_lastpal[768];
static int32_t g_lastinput;
static uint32_t g_firstupdate;

static void rec_palette(const byte *palette)
{
    g_palettes++;
    memcpy(g_lastpal, palette, sizeof(g_lastpal));
}

static void rec_wait(int vbls)
{
    g_waits += vbls;
}

/* UI_DUMP=name: each screen as name.N.bmp, 8-bit with the game palette, to look at */
static void dump_bmp(const char *path)
{
    unsigned char hdr[54] = {'B', 'M'};
    FILE *f = fopen(path, "wb");
    int i, y;

    if (!f) {
        return;
    }
#define PUT32(o, v) (hdr[o] = (unsigned char)(v), hdr[(o) + 1] = (unsigned char)((v) >> 8), \
                     hdr[(o) + 2] = (unsigned char)((v) >> 16), hdr[(o) + 3] = (unsigned char)((v) >> 24))
    PUT32(2, 54 + 1024 + 64000);
    PUT32(10, 54 + 1024);
    PUT32(14, 40);
    PUT32(18, 320);
    PUT32(22, 200);
    hdr[26] = 1;
    hdr[28] = 8;
    PUT32(46, 256);
    fwrite(hdr, 1, sizeof(hdr), f);
    for (i = 0; i < 256; i++) {
        const unsigned char quad[4] = {(unsigned char)(gamepal[i * 3 + 2] * 255 / 63),
                                       (unsigned char)(gamepal[i * 3 + 1] * 255 / 63),
                                       (unsigned char)(gamepal[i * 3] * 255 / 63), 0};
        fwrite(quad, 1, 4, f);
    }
    for (y = 199; y >= 0; y--) {
        fwrite(screenbuf + y * SCREENWIDTH, 1, SCREENWIDTH, f);
    }
    fclose(f);
}

static void rec_update(void)
{
    if (!g_updates++) {
        g_firstupdate = fnv(2166136261u, screenbuf, sizeof(screenbuf));
        if (getenv("UI_DUMP")) {
            static int dumps;
            char name[512];

            snprintf(name, sizeof(name), "%s.%d.bmp", getenv("UI_DUMP"), dumps++);
            dump_bmp(name);
        }
    }
}

static boolean rec_input(int32_t tics)
{
    g_inputs++;
    g_lastinput = tics;
    return false;
}

static void ui_hooks(void)
{
    OrigHooks_Reset();
    orig_hooks.set_palette = rec_palette;
    orig_hooks.wait_vbl = rec_wait;
    orig_hooks.update_screen = rec_update;
    orig_hooks.user_input = rec_input;
    g_waits = g_palettes = g_updates = g_inputs = 0;
}

static void test_fades(void)
{
    static const byte black[768];
    int i;

    ui_hooks();
    VL_SetPalette(gamepal);
    g_palettes = 0;
    VW_FadeOut();
    CHECK(g_waits == 31 && g_palettes == 31); /* a retrace, 30 steps, the final colour */
    CHECK(memcmp(g_lastpal, black, sizeof(black)) == 0 && screenfaded);

    g_waits = g_palettes = 0;
    VW_FadeIn();
    CHECK(g_waits == 31 && g_palettes == 31);
    CHECK(memcmp(g_lastpal, gamepal, sizeof(g_lastpal)) == 0 && !screenfaded);

    /* half way through a fade every colour is between its ends */
    VL_FadeOut(0, 255, 63, 0, 0, 2); /* to red in two steps: 0, then half */
    VL_GetPalette(g_lastpal);
    for (i = 0; i < 256; i++) {
        CHECK(g_lastpal[i * 3] == 63 && g_lastpal[i * 3 + 1] == 0);
    }
    VL_SetPalette(gamepal);
    screenfaded = false;
}

/* --------------------------------------------------- screens on real data */

static void test_get_psyched_and_status_bar(void)
{
    /* view size 15, a new game on E1M1 */
    static const uint32_t psyched_pins[2] = {0x37ef0d0bu, 0x37ef0d0bu}; /* WL6, WL1: the same art */
    static const uint32_t status_pins[2] = {0x07f6f332u, 0x07f6f332u};
    uint32_t status;

    NewViewSize(15);
    NewGame(gd_hard, 0);
    gamestate.mapon = 0;
    CHECK(SetupGameLevel());
    ui_hooks();
    memset(screenbuf, 0, sizeof(screenbuf));
    DrawPlayScreen();
    status = fnv(2166136261u, screenbuf + (200 - STATUSLINES) * SCREENWIDTH,
                 (size_t)STATUSLINES * SCREENWIDTH);
    CHECK(screenfaded); /* DrawPlayScreen fades out first */

    g_updates = g_inputs = 0;
    PreloadGraphics();
    printf("  get psyched %08x, status bar %08x\n", g_firstupdate, status);
    CHECK(g_inputs == 1 && g_lastinput == 70); /* the original's IN_UserInput (70) */
    CHECK(g_updates == 3 && screenfaded);       /* drawn, bar, border; faded out after */
    CHECK(g_firstupdate == psyched_pins[g_wl1]);
    CHECK(status == status_pins[g_wl1]);
    /* the view is cleared to the border colour after */
    CHECK(count_colour(0, 0, 320, 160, 127) > 320 * 160 / 4);
}

static void test_message(void)
{
    int left, right;

    ui_hooks();
    memset(screenbuf, 0, sizeof(screenbuf));
    ClearSplitVWB();
    Message("You now have 100% Health,\n99 Ammo and both Keys!");
    CHECK(g_updates == 1);
    colour_span(0x17, &left, &right); /* the window (TEXTCOLOR) */
    CHECK(left > 10 && right < 310 && abs((left + right) / 2 - 160) <= 3);
    CHECK(count_colour(0, 0, 320, 200, 0x13) > 0); /* its outline (HIGHLIGHT) */
}

int main(int argc, char **argv)
{
    wolf_data_dir_t dir;

    InitGame();
    if (argc < 2) {
        printf("test_orig_ui: needs --data DIR; nothing to check\n");
        return 0;
    }
    memset(&dir, 0, sizeof(dir));
    if (!Wolf_ResolveDataDir(argc, argv, &dir)) {
        return 1;
    }
    CA_Startup();
    if (!CA_OpenMapFiles(&dir) || !CA_OpenGraphFiles(&dir)) {
        fprintf(stderr, "FAIL: could not open %s\n", dir.path);
        return 1;
    }
    g_wl1 = strcmp(g_gfx->name, "WL1") == 0;
    printf("%s:\n", g_gfx->name);

    test_fonts();
    test_pictures();
    test_fades();
    test_get_psyched_and_status_bar();
    test_message();

    if (g_failures) {
        fprintf(stderr, "%d check(s) failed\n", g_failures);
        return 1;
    }
    printf("test_orig_ui: all checks passed\n");
    return 0;
}
