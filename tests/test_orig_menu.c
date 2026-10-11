/*
 * The control panel (P5b, orig_menu.c) on a real pack (--data DIR; without
 * one there is nothing to draw and the test passes). A script plays keys
 * into orig_keys / LastScan / LastASCII as the keyboard interrupt did, one
 * step per input poll, and the menus run as they do in the game: the main
 * menu and Quit, New Game (episodes, difficulty), Sound, Save and Load with
 * a typed name, the quick keys F8 / F9, Change View, the high scores, the
 * Control menu (mouse, sensitivity, a key customized, the controller's look
 * speed, a pad button held on a switch), the Sound and Video pages (volumes,
 * the mouse wheel, fill the screen, fullscreen, view size) and Read This!,
 * and the screens between floors: the intermission (with its tally, a key
 * skipping it, the secret floor), the victory and end text, a high score's
 * name typed in, "Let's see that again!".
 *
 * UI_DUMP=name writes the screens the script marks as name.<screen>.bmp.
 */

#include "orig_testkit.h"
#include "wl_data.h"

static boolean g_wl1;

/* ----------------------------------------------------------------- script */

typedef struct {
    byte scan;
    char ascii;        /* LastASCII for the key, 0 for none */
    const char *snap;  /* hash (and with UI_DUMP, save) the screen before the key */
} press_t;

#define PADA_SCAN 0xfe  /* not a key: the pad's A, held for PADHOLD polls */
#define WHEEL_UP 0xfd   /* not a key: a mouse wheel notch up */
#define WHEEL_DOWN 0xfc /* and down */
#define PADRIGHT_SCAN 0xfb /* not a key: the pad's D-pad right, held for PADHOLD polls */

#define HOLD 3  /* polls a key stays down */
#define GAP 40  /* polls before the next key */
#define RUNAWAY 200000

#define PADHOLD 30

static const press_t *g_script;
static int g_padheld, g_padright, g_wheelnotches;
static int g_nscript, g_step, g_phase, g_polls, g_pressed;
static boolean g_runaway;

#define MAXSNAPS 48
static const char *g_snapname[MAXSNAPS];
static uint32_t g_snaphash[MAXSNAPS];
static int g_numsnaps;

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

static void snap(const char *name)
{
    if (g_numsnaps < MAXSNAPS) {
        g_snapname[g_numsnaps] = name;
        g_snaphash[g_numsnaps++] = fnv(2166136261u, screenbuf, sizeof(screenbuf));
    }
    if (getenv("UI_DUMP")) {
        char path[512];

        snprintf(path, sizeof(path), "%s.%s.%s.bmp", getenv("UI_DUMP"), g_wl1 ? "wl1" : "wl6",
                 name);
        dump_bmp(path);
    }
}

static uint32_t snap_hash(const char *name)
{
    int i;

    for (i = 0; i < g_numsnaps; i++) {
        if (strcmp(g_snapname[i], name) == 0) {
            return g_snaphash[i];
        }
    }
    return 0;
}

/* the keyboard interrupt: a make code on the first poll of a step, the break code HOLD polls later */
static void script_poll(void)
{
    const press_t *p;

    if (++g_polls > RUNAWAY) {
        g_runaway = true;
        orig_quit = true;
        return;
    }
    if (g_step >= g_nscript) {
        return;
    }
    p = &g_script[g_step];
    if (g_phase < 0) { /* the lead-in: the screen comes up before the first key */
        g_phase++;
        return;
    }
    if (g_phase == 0 && p->scan >= PADRIGHT_SCAN) {
        if (p->snap) {
            snap(p->snap);
        }
        if (p->scan == PADA_SCAN) {
            g_padheld = PADHOLD;
            orig_lastscan = 0xff; /* as the platform: any pad button is a key for IN_Ack */
        } else if (p->scan == PADRIGHT_SCAN) {
            g_padright = PADHOLD;
            orig_lastscan = 0xff;
        } else {
            g_wheelnotches += p->scan == WHEEL_UP ? 1 : -1;
        }
        g_pressed++;
    } else if (g_phase == 0) {
        if (p->snap) {
            snap(p->snap);
        }
        orig_keys[p->scan] = 1;
        orig_lastscan = p->scan;
        orig_lastascii = (byte)p->ascii;
        g_pressed++;
    } else if (g_phase == HOLD && p->scan < PADRIGHT_SCAN) {
        orig_keys[p->scan] = 0;
    }
    if (++g_phase >= HOLD + GAP) {
        g_phase = 0;
        g_step++;
    }
}

/* the pad: connected, still, A down while the script holds it */
static boolean pad_menu(int *dx, int *dy, int *buttons)
{
    *dx = *dy = 0;
    *buttons = 0;
    if (g_padheld > 0) {
        g_padheld--;
        *buttons = 1;
    }
    if (g_padright > 0) {
        g_padright--;
        *dx = 127;
    }
    return true;
}

static int mouse_wheel(void)
{
    const int n = g_wheelnotches;

    g_wheelnotches = 0;
    return n;
}

static boolean ack(void)
{
    return true;
}

/* the fake platform: sound modes and save slots */
static int g_soundmode[3];
static int g_modesets;

static int get_mode(int which)
{
    return g_soundmode[which];
}

static void set_mode(int which, int mode)
{
    g_soundmode[which] = mode;
    g_modesets++;
}

/* AdLib and the Sound Blaster only, as the port has */
static boolean available(int which, int mode)
{
    return (which == 0 && mode == 2) || (which == 1 && mode == 2) || (which == 2 && mode == 1);
}

static byte g_slotdata[10][64 * 1024];
static size_t g_slotlen[10];
static char g_slotname[10][32];
static int g_writes;

static boolean save_name(int slot, char *name)
{
    if (!g_slotlen[slot]) {
        return false;
    }
    memcpy(name, g_slotname[slot], 32);
    return true;
}

static boolean write_game(int slot, const char *name, const byte *data, size_t len)
{
    if (len > sizeof(g_slotdata[slot])) {
        return false;
    }
    memcpy(g_slotdata[slot], data, len);
    g_slotlen[slot] = len;
    snprintf(g_slotname[slot], sizeof(g_slotname[slot]), "%s", name);
    g_writes++;
    return true;
}

static size_t read_game(int slot, byte *buf, size_t cap)
{
    if (!g_slotlen[slot] || g_slotlen[slot] > cap) {
        return 0;
    }
    memcpy(buf, g_slotdata[slot], g_slotlen[slot]);
    return g_slotlen[slot];
}

/* the mouse is there and does not move; the settings this port adds */
static void still_mouse(int *dx, int *dy)
{
    *dx = *dy = 0;
}

static int g_option[NUMOPTIONS];

static int get_option(int which)
{
    return g_option[which];
}

static void set_option(int which, int value)
{
    g_option[which] = value;
}

static void run_script(const press_t *script, int n)
{
    install_hooks();
    orig_hooks.poll_input = script_poll;
    orig_hooks.check_ack = ack;
    orig_hooks.get_sound_mode = get_mode;
    orig_hooks.set_sound_mode = set_mode;
    orig_hooks.sound_available = available;
    orig_hooks.save_name = save_name;
    orig_hooks.write_game = write_game;
    orig_hooks.read_game = read_game;
    orig_hooks.mouse_delta = still_mouse;
    orig_hooks.get_option = get_option;
    orig_hooks.set_option = set_option;
    orig_hooks.pad_menu = pad_menu;
    orig_hooks.mouse_wheel = mouse_wheel;
    g_padheld = g_padright = g_wheelnotches = 0;
    memset(orig_keys, 0, sizeof(orig_keys));
    g_script = script;
    g_nscript = n;
    g_step = g_polls = g_pressed = 0;
    g_phase = -GAP;
    g_runaway = false;
    g_numsounds = 0;
    orig_quit = false;
    abortgame = false;
}

/* every key of the script was pressed and nothing waited for ever */
static boolean script_done(void)
{
    return !g_runaway && g_pressed == g_nscript;
}

#define KEY(sc) {sc, 0, NULL}
#define PADA {PADA_SCAN, 0, NULL}
#define PADRIGHT(name) {PADRIGHT_SCAN, 0, name}
#define WHEEL(n) {(n) > 0 ? WHEEL_UP : WHEEL_DOWN, 0, NULL}
#define SNAPKEY(sc, name) {sc, 0, name}
#define N(a) ((int)(sizeof(a) / sizeof((a)[0])))

/*
 * The screens, checked by eye against the original (UI_DUMP) and pinned.
 * WL1 differs in the main menu (WL6 has no Read This!, as the registered 1.4),
 * the locked episodes (READCOLOR) and the high score levels (no episode:
 * UPLOAD).
 */
static void check_screens(void)
{
    static const struct {
        const char *name;
        uint32_t wl6, wl1;
    } pins[] = {
        {"mainmenu", 0xd49ec5f2u, 0x7d198dd6u},
        {"episodes", 0xdb1e2c11u, 0xa2b07c15u},
        {"difficulty", 0x0df46d54u, 0x0df46d54u},
        {"sound", 0xb1a9424eu, 0xb1a9424eu},
        {"savegame", 0xe873848du, 0xe873848du},
        {"typed", 0xf59a9da6u, 0xf59a9da6u},
        {"changeview", 0x6521edadu, 0x6521edadu},
        {"scores", 0x66da3e84u, 0x4b5e6d16u},
        {"control", 0x02938571u, 0x02938571u},
        {"mousesens", 0x7641c575u, 0x7641c575u},
        {"customize", 0xb6374824u, 0xb6374824u},
        {"padspeed", 0x8fb3eebcu, 0x8fb3eebcu},
        {"soundpage", 0x5a62485du, 0x5a62485du},
        {"videopage", 0xa26fd24cu, 0xa26fd24cu},
        {"help1", 0, 0x174b84c4u}, /* WL6: no help shown (the registered 1.4 had none) */
        {"help2", 0, 0xd152ab99u},
        {"intermission", 0x5a3458c0u, 0x2a14cd96u},
        {"secretfloor", 0x8d3d6521u, 0xa8e14743u},
        {"victory", 0x67dbb236u, 0xf55df129u}, /* WL6 has the time code, the shareware not */
        {"endtext", 0xe60bfeacu, 0xe60bfeacu},
        {"scorename", 0x9e60ec9au, 0x833153a4u},
        {"deathcam", 0x3865f1dfu, 0x24af147du},
    };
    size_t i;

    for (i = 0; i < sizeof(pins) / sizeof(pins[0]); i++) {
        const uint32_t h = snap_hash(pins[i].name);

        printf("  screen %-10s %08x\n", pins[i].name, h);
        CHECK(h == (g_wl1 ? pins[i].wl1 : pins[i].wl6));
    }
}

/* ------------------------------------------------------------------- tests */

static void test_main_menu_and_quit(void)
{
    /* a first run: the cursor on "Read This!" (WL1); WL6 has no Read This!, it starts on New Game */
    static const press_t script[] = {
        SNAPKEY(sc_Escape, "mainmenu"), /* Esc: Quit? */
        SNAPKEY(sc_N, "quit"),          /* no */
        KEY(sc_Escape),
        KEY(sc_Y), /* yes */
    };

    ingame = false;
    run_script(script, N(script));
    US_ControlPanel(0);
    printf("  main menu %08x, quit %08x\n", snap_hash("mainmenu"), snap_hash("quit"));
    CHECK(script_done() && orig_quit && abortgame);
    CHECK(snap_hash("mainmenu") != snap_hash("quit"));
    CHECK(count_sound(ESCPRESSEDSND) == 3 && count_sound(SHOOTSND) == 2);
    orig_quit = false;
    abortgame = false;
}

static void test_new_game(void)
{
    /* WL6: episode 2, the hardest; WL1 has only episode 1 and says so */
    static const press_t wl6[] = {
        KEY(sc_Return),                       /* New Game */
        SNAPKEY(sc_DownArrow, "episodes"),    /* episode 2 */
        KEY(sc_Return),
        SNAPKEY(sc_DownArrow, "difficulty"),  /* from "Bring 'em on!" down */
        KEY(sc_Return),
    };
    static const press_t wl1[] = {
        KEY(sc_Return),
        SNAPKEY(sc_DownArrow, "episodes"),
        KEY(sc_Return), /* "Please select Read This!" */
        KEY(sc_UpArrow),
        KEY(sc_Return),
        SNAPKEY(sc_DownArrow, "difficulty"),
        KEY(sc_Return),
    };

    ingame = false;
    MainItems.curpos = newgame;
    startgame = false;
    if (g_wl1) {
        run_script(wl1, N(wl1));
    } else {
        run_script(wl6, N(wl6));
    }
    US_ControlPanel(0);
    printf("  new game: episode %d, difficulty %d\n", gamestate.episode, gamestate.difficulty);
    CHECK(script_done() && !orig_quit && startgame);
    CHECK(gamestate.difficulty == gd_hard && gamestate.episode == (g_wl1 ? 0 : 1));
    CHECK(count_sound(NOWAYSND) == (g_wl1 ? 1 : 0));
    /* a game is on: "View Scores" became "End Game" */
    CHECK(strcmp(MainMenu[viewscores].string, "End Game") == 0 && !MainMenu[viewscores].routine);
    CHECK(MainMenu[readthis].active == 1);
    startgame = false;
}

static void test_sound_menu(void)
{
    static const press_t script[] = {
        SNAPKEY(sc_Return, "sound"), /* effects: None */
        KEY(sc_DownArrow),           /* past the PC speaker, greyed */
        KEY(sc_Return),              /* AdLib */
        KEY(sc_Escape),
    };

    g_soundmode[0] = 2;
    g_soundmode[1] = 2;
    g_soundmode[2] = 1;
    g_modesets = 0;
    run_script(script, N(script));
    CP_Sound(0);
    CHECK(script_done());
    CHECK(g_modesets == 2 && g_soundmode[0] == 2 && g_soundmode[1] == 2 && g_soundmode[2] == 1);
}

static void test_save_and_load(void)
{
    /* F2: slot 1 (the cursor moves down), a typed name */
    static const press_t save[] = {
        SNAPKEY(sc_DownArrow, "savegame"),
        KEY(sc_Return),
        {sc_T, 'T', NULL},
        {sc_E, 'e', NULL},
        {sc_S, 's', NULL},
        {sc_BackSpace, 0, NULL},
        {sc_S, 's', NULL},
        {sc_T, 't', "typed"},
        KEY(sc_Return),
    };
    /* F3: the cursor is on slot 1 */
    static const press_t load[] = {
        SNAPKEY(sc_Return, "loadgame"),
    };
    /* F9 before a slot was picked from the menus (pickquick): the load menu */
    static const press_t quickload[] = {
        SNAPKEY(sc_Escape, "loadmenu"),
    };
    int16_t health;

    memset(g_slotlen, 0, sizeof(g_slotlen));
    g_writes = 0;
    NewViewSize(15);
    NewGame(gd_hard, 0);
    gamestate.mapon = 0;
    CHECK(SetupGameLevel());
    ingame = true;
    gamestate.health = health = 57;

    run_script(save, N(save));
    US_ControlPanel(sc_F2);
    printf("  saved \"%s\", %u bytes\n", g_slotname[1], (unsigned)g_slotlen[1]);
    CHECK(script_done() && g_writes == 1 && g_slotlen[1] > 0 && !g_slotlen[0]);
    CHECK(strcmp(g_slotname[1], "Test") == 0);

    gamestate.health = 1;
    run_script(load, N(load));
    US_ControlPanel(sc_F3);
    CHECK(script_done() && loadedgame && gamestate.health == health);
    loadedgame = false;

    /* the quick keys need a slot picked from their own menus first (pickquick) */
    gamestate.health = 2;
    run_script(quickload, N(quickload));
    US_ControlPanel(sc_F9);
    CHECK(script_done() && gamestate.health == 2 && !loadedgame);

    ingame = false;
    loadedgame = false;
    orig_quit = false;
    abortgame = false;
    playstate = ex_stillplaying;
}

static void test_quick_keys(void)
{
    /* F8 without a picked slot: the save menu, slot 1 is used, overwrite it */
    static const press_t pick[] = {
        KEY(sc_Return),
        SNAPKEY(sc_Y, "overwrite"),
        KEY(sc_Return), /* the name as it was */
    };
    static const press_t yes[] = {
        KEY(sc_Y),
    };
    int writes;

    ingame = true;
    gamestate.health = 66;
    run_script(pick, N(pick));
    US_ControlPanel(sc_F8);
    CHECK(script_done() && g_writes == 2 && strcmp(g_slotname[1], "Test") == 0);

    /* now F8 saves to it at once, and F9 asks, then loads it */
    writes = g_writes;
    run_script(NULL, 0);
    US_ControlPanel(sc_F8);
    CHECK(g_writes == writes + 1);

    gamestate.health = 3;
    run_script(yes, N(yes));
    US_ControlPanel(sc_F9);
    CHECK(script_done() && gamestate.health == 66 && !loadedgame);

    ingame = false;
}

static void test_change_view(void)
{
    static const press_t bigger[] = {
        SNAPKEY(sc_UpArrow, "changeview"),
        KEY(sc_UpArrow),
        KEY(sc_Return),
    };
    static const press_t cancel[] = {
        KEY(sc_DownArrow),
        KEY(sc_Escape),
    };

    NewViewSize(15);
    run_script(bigger, N(bigger));
    CP_ChangeView(0);
    CHECK(script_done() && viewsize == 17 && viewwidth == 17 * 16);

    run_script(cancel, N(cancel));
    CP_ChangeView(0);
    CHECK(script_done() && viewsize == 17 && viewwidth == 17 * 16);
    NewViewSize(15);
}

static void test_control_menu(void)
{
    static const press_t script[] = {
        SNAPKEY(sc_Return, "control"), /* Mouse Enabled: off */
        KEY(sc_Return),                /* and on again */
        KEY(sc_DownArrow),             /* past the joystick lines, greyed: Mouse Sensitivity */
        KEY(sc_Return),
        SNAPKEY(sc_RightArrow, "mousesens"),
        KEY(sc_RightArrow),
        KEY(sc_Return),
        KEY(sc_DownArrow), /* Customize controls */
        KEY(sc_Return),
        SNAPKEY(sc_DownArrow, "customize"), /* past the joystick: the keyboard buttons */
        KEY(sc_Return),
        KEY(sc_Return),    /* Run: pick a key */
        {sc_Q, 'q', NULL}, /* Q runs */
        KEY(sc_Escape),
        KEY(sc_Escape),
        KEY(sc_DownArrow), /* Controller look speed */
        KEY(sc_Return),
        SNAPKEY(sc_LeftArrow, "padspeed"),
        KEY(sc_Return),
        KEY(sc_UpArrow), /* up to Mouse Enabled, past the greyed lines */
        KEY(sc_UpArrow),
        KEY(sc_UpArrow),
        PADA,              /* Mouse Enabled with the pad's A held down: toggles once */
        KEY(sc_Escape),
    };
    const int16_t run = buttonscan[bt_run];

    mouseenabled = true;
    mouseadjustment = 5;
    joystickenabled = false;
    g_option[OPT_PADSENS] = 5;
    run_script(script, N(script));
    CP_Control(0);
    printf("  control: mouse %d, sensitivity %d, run key %02x, look speed %d\n", mouseenabled,
           mouseadjustment, buttonscan[bt_run], g_option[OPT_PADSENS]);
    CHECK(script_done());
    CHECK(!mouseenabled && mouseadjustment == 7); /* off: one toggle for the held button */
    CHECK(buttonscan[bt_run] == sc_Q);
    CHECK(g_option[OPT_PADSENS] == 4);
    buttonscan[bt_run] = run;
    mouseenabled = true;
}

/* Sound (F4): the sound devices screen behind the first line, then the volumes */
static void test_sound_page(void)
{
    static const press_t script[] = {
        SNAPKEY(sc_DownArrow, "soundpage"), /* Music */
        KEY(sc_Return),
        KEY(sc_LeftArrow),
        WHEEL(-1), /* the wheel down: one less */
        KEY(sc_Return),
        KEY(sc_DownArrow), /* Effects: less, then Esc puts it back */
        KEY(sc_Return),
        KEY(sc_LeftArrow),
        KEY(sc_Escape),
        KEY(sc_UpArrow), /* the wheel and the arrows move the gun: up twice to Sound devices */
        WHEEL(1),
        KEY(sc_Return),
        SNAPKEY(sc_Escape, "sounddevices"),
        KEY(sc_Escape),
    };

    g_option[OPT_MUSICVOL] = g_option[OPT_EFFECTSVOL] = 9;
    g_soundmode[0] = g_soundmode[1] = 2;
    g_soundmode[2] = 1;
    run_script(script, N(script));
    US_ControlPanel(sc_F4);
    printf("  sound: music %d, effects %d\n", g_option[OPT_MUSICVOL], g_option[OPT_EFFECTSVOL]);
    CHECK(script_done());
    CHECK(g_option[OPT_MUSICVOL] == 7 && g_option[OPT_EFFECTSVOL] == 9);
}

/* Video (the main menu's line for Change View): the view size, fill the screen, fullscreen */
static void test_video_page(void)
{
    static const press_t script[] = {
        KEY(sc_Return), /* Video */
        SNAPKEY(sc_DownArrow, "videopage"),
        PADA,           /* Fill the screen, the pad's A held down: on once */
        KEY(sc_DownArrow),
        KEY(sc_Return), /* Fullscreen: on */
        KEY(sc_UpArrow),
        KEY(sc_UpArrow),
        KEY(sc_Return), /* View size: the original Change View */
        KEY(sc_UpArrow),
        KEY(sc_Return),
        KEY(sc_Escape),
        KEY(sc_Escape), /* the main menu: Quit? yes */
        KEY(sc_Y),
    };

    NewViewSize(15);
    ingame = false;
    MainItems.curpos = changeview;
    g_option[OPT_FILLSCREEN] = g_option[OPT_FULLSCREEN] = 0;
    run_script(script, N(script));
    US_ControlPanel(0);
    printf("  video: fill %d, fullscreen %d, view size %d\n", g_option[OPT_FILLSCREEN],
           g_option[OPT_FULLSCREEN], viewsize);
    CHECK(script_done() && orig_quit);
    CHECK(g_option[OPT_FILLSCREEN] == 1 && g_option[OPT_FULLSCREEN] == 1);
    CHECK(viewsize == 16);
    CHECK(strcmp(MainMenu[changeview].string, "Video") == 0);
    orig_quit = false;
    abortgame = false;
    NewViewSize(15);
}

/* Read This!: the help text, a page on and back, the frame around it */
static void test_help(void)
{
    static const press_t script[] = {
        SNAPKEY(sc_RightArrow, "help1"),
        SNAPKEY(sc_LeftArrow, "help2"),
        PADRIGHT("help1again"), /* the D-pad turns the page too */
        SNAPKEY(sc_Escape, "help2again"),
    };
    const struct { /* the chunk numbers depend on the pack */
        int chunk, width, height;
    } frame[] = {{H_TOPWINDOWPIC, 320, 8},
                 {H_LEFTWINDOWPIC, 8, 192},
                 {H_RIGHTWINDOWPIC, 8, 192},
                 {H_BOTTOMINFOPIC, 304, 24}};
    size_t i;

    for (i = 0; i < sizeof(frame) / sizeof(frame[0]); i++) {
        CHECK(pictable[frame[i].chunk - STARTPICS].width == frame[i].width &&
              pictable[frame[i].chunk - STARTPICS].height == frame[i].height);
    }

    if (!g_wl1) {
        /* the registered data's help text is the shareware's: not shown, as the registered 1.4 */
        CHECK(!HelpAvailable());
        run_script(NULL, 0);
        HelpScreens();
        CHECK(g_polls == 0);
        return;
    }
    CHECK(HelpAvailable());
    run_script(script, N(script));
    HelpScreens();
    CHECK(script_done());
    CHECK(snap_hash("help1") != snap_hash("help2"));
    CHECK(snap_hash("help1") == snap_hash("help1again"));
    CHECK(snap_hash("help2") == snap_hash("help2again"));
}

static void test_high_scores(void)
{
    run_script(NULL, 0);
    CP_ViewScores(0);
    snap("scores");
}

/* ---------------------------------------------- the screens between floors */

/* IN_CheckAck as the platform: a key since IN_StartAck, read as it comes (not at once) */
static boolean key_ack(void)
{
    IN_Pump();
    if (orig_lastscan) {
        orig_lastscan = 0;
        return true;
    }
    return false;
}

/* E1M1 at its end: every kill, no secret, half the treasure, a minute (par 1:30) */
static void finish_floor(int mapon)
{
    NewViewSize(15);
    NewGame(gd_hard, 0);
    gamestate.mapon = (int16_t)mapon;
    CHECK(SetupGameLevel());
    gamestate.killcount = gamestate.killtotal;
    gamestate.secretcount = 0;
    gamestate.treasurecount = gamestate.treasuretotal / 2;
    gamestate.TimeCount = 60 * 70;
    gamestate.score = gamestate.oldscore = 0;
    gamestate.nextextra = EXTRAPOINTS;
    DrawPlayScreen(); /* the status bar under the intermission */
}

static void test_intermission(void)
{
    static const press_t keep[] = {
        SNAPKEY(sc_Return, "intermission"), /* the tally done: on */
    };
    int tr;

    finish_floor(0);
    tr = gamestate.treasuretotal ? (gamestate.treasurecount * 100) / gamestate.treasuretotal : 0;
    run_script(keep, N(keep));
    orig_hooks.check_ack = key_ack;
    g_phase = -1000; /* the tally reads the keys at every step: no key until it is done */
    LevelCompleted();
    printf("  intermission: kills %d/%d, treasure %d%%, score %ld, ratios %d/%d/%d %lds\n",
           gamestate.killcount, gamestate.killtotal, tr, (long)gamestate.score,
           LevelRatios[0].kill, LevelRatios[0].secret, LevelRatios[0].treasure,
           (long)LevelRatios[0].time);
    CHECK(script_done());
    /* 30 seconds under par at 500 each, and 10000 for every kill */
    CHECK(gamestate.score == 30 * 500 + 10000);
    CHECK(LevelRatios[0].kill == 100 && LevelRatios[0].secret == 0 &&
          LevelRatios[0].treasure == tr && LevelRatios[0].time == 60);
    CHECK(count_sound(PERCENT100SND) == 1 && count_sound(NOBONUSSND) == 1);

    /*
     * BJ breathes while the screen waits: L_GUY2PIC every 35 tics. The
     * shareware has two different pictures; in the registered data both
     * are the same, so there he stands still (as in the original).
     */
    {
        static const press_t wait[] = {
            {sc_None, 0, "breathe1"}, /* no key: just a look */
            {sc_None, 0, NULL},       /* 43 tics later, past one breath (every 35) */
            SNAPKEY(sc_Return, "breathe2"),
        };
        uint32_t first, second;

        finish_floor(0);
        run_script(wait, N(wait));
        orig_hooks.check_ack = key_ack;
        g_phase = -1000;
        LevelCompleted();
        first = snap_hash("breathe1");
        second = snap_hash("breathe2");
        printf("  breathing: %s\n", first != second ? "yes" : "no (the same picture twice)");
        CHECK(g_wl1 ? first != second : first == second);
        CHECK(memcmp(grsegs[L_GUYPIC], grsegs[L_GUY2PIC], 104 * 88) == 0 ? !g_wl1 : g_wl1);
    }

    /* the secret floor: no tally, 15000 */
    {
        static const press_t secret[] = {
            SNAPKEY(sc_Return, "secretfloor"),
        };
        finish_floor(9);
        run_script(secret, N(secret));
        orig_hooks.check_ack = key_ack;
        LevelCompleted();
        CHECK(script_done() && gamestate.score == 15000);
    }

    /* a key during the tally jumps to its end, with the same bonus */
    finish_floor(0);
    {
        static const press_t skip[] = {
            KEY(sc_Space),  /* during the count: straight to the end */
            KEY(sc_Return), /* and on */
        };
        run_script(skip, N(skip));
        orig_hooks.check_ack = key_ack;
        g_phase = 0; /* no lead-in: the key comes at once */
        LevelCompleted();
        CHECK(script_done() && gamestate.score == 30 * 500 + 10000);
    }
}

static void test_victory(void)
{
    static const press_t script[] = {
        SNAPKEY(sc_Return, "victory"),
        SNAPKEY(sc_Escape, "endtext"), /* the episode's end text: out */
    };
    int i;

    NewGame(gd_hard, 0);
    for (i = 0; i < 8; i++) {
        LevelRatios[i].kill = 100;
        LevelRatios[i].secret = (int16_t)(i * 10);
        LevelRatios[i].treasure = 50;
        LevelRatios[i].time = 90;
    }
    run_script(script, N(script));
    orig_hooks.check_ack = key_ack;
    Victory();
    CHECK(script_done());
    CHECK(snap_hash("victory") != snap_hash("endtext"));
}

static void test_high_score_entry(void)
{
    static const press_t script[] = {
        {sc_B, 'B', NULL},
        {sc_J, 'J', NULL},
        SNAPKEY(sc_Return, "scorename"),
    };
    HighScore saved[MaxScores];

    memcpy(saved, Scores, sizeof(saved));
    gamestate.episode = 0;
    run_script(script, N(script));
    CheckHighScore(20000, 3);
    printf("  high score: \"%s\" %ld, floor %u\n", Scores[0].name, (long)Scores[0].score,
           Scores[0].completed);
    CHECK(script_done());
    CHECK(strcmp(Scores[0].name, "BJ") == 0 && Scores[0].score == 20000 && Scores[0].completed == 3);
    CHECK(strcmp(Scores[1].name, "id software-'92") == 0); /* the rest moved down */
    CHECK(strcmp(Scores[MaxScores - 1].name, "John Romero") == 0);

    /* not good enough: no name asked */
    run_script(NULL, 0);
    CheckHighScore(100, 1);
    CHECK(Scores[MaxScores - 1].score == 10000);
    memcpy(Scores, saved, sizeof(saved));
}

static void test_death_cam_text(void)
{
    run_script(NULL, 0);
    memset(screenbuf, 0, sizeof(screenbuf));
    DeathCamIntro();
    snap("deathcam");
}

int main(int argc, char **argv)
{
    wolf_data_dir_t dir;

    InitGame();
    if (argc < 2) {
        printf("test_orig_menu: needs --data DIR; nothing to check\n");
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
    upload = g_wl1;
    printf("%s:\n", g_gfx->name);

    test_main_menu_and_quit();
    test_new_game();
    test_sound_menu();
    test_save_and_load();
    test_quick_keys();
    test_change_view();
    test_high_scores();
    test_control_menu();
    test_sound_page();
    test_video_page();
    test_help();
    test_intermission();
    test_victory();
    test_high_score_entry();
    test_death_cam_text();

    check_screens();

    if (g_failures) {
        fprintf(stderr, "%d check(s) failed\n", g_failures);
        return 1;
    }
    printf("test_orig_menu: all checks passed\n");
    return 0;
}
