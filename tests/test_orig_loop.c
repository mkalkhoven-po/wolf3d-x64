/*
 * The play loop of the faithful engine (P4/S5a): CalcTics, PollControls
 * (keyboard, mouse, joystick, demo record and playback), palette shifts,
 * the MLI cheat, music, PlayLoop and PlayDemo.
 *
 * With --data DIR the original demos play headless. A demo recording stops
 * when the level ends, so a demo that plays back in sync ends (death or
 * completion) exactly on its last frame of input; that is checked for every
 * demo known to be in sync, and the end state of every demo is pinned.
 */
#include "orig_testkit.h"
#include "wl_data.h"

#include "id_pm.h"

/* ------------------------------------------------------------- the clock */

static int32_t g_clock;
static int g_writes;
static boolean g_ticking; /* the clock runs on by one each time it is read */

static int32_t clock_read(void)
{
    const int32_t now = g_clock;
    if (g_ticking) {
        g_clock++;
    }
    return now;
}

static void clock_write(int32_t count)
{
    g_clock = count;
    g_writes++;
}

static void test_calc_tics(void)
{
    empty_map();
    AT(plane1, 10, 10) = 20;
    load_level(gd_hard);
    orig_hooks.read_time = clock_read;
    orig_hooks.write_time = clock_write;

    g_clock = 3;
    lasttimecount = 0;
    CalcTics();
    CHECK(tics == 3 && lasttimecount == 3);

    /* more than MAXTICS: clamped, and the clock is set back */
    g_clock = 50;
    g_writes = 0;
    CalcTics();
    CHECK(tics == MAXTICS && g_clock == 50 - (47 - MAXTICS) && g_writes == 1);
    CHECK(lasttimecount == 50); /* as in the original: lasttimecount keeps newtime */

    /*
     * So the next frame finds the clock (13) behind lasttimecount (50), as
     * after a long pause: TimeCount is brought up to 50 and the loop waits
     * for the next tic.
     */
    g_ticking = true;
    CalcTics();
    g_ticking = false;
    CHECK(tics == 1 && lasttimecount == 51);

    /* headless (no clock): one tic per call */
    OrigHooks_Reset();
    TimeCount = lasttimecount = 7;
    CalcTics();
    CHECK(tics == 1 && TimeCount == 8);
}

/* -------------------------------------------------------------- controls */

static int g_mousebuttons, g_mousedx, g_mousedy, g_joybuttons, g_joyx, g_joyy;

static int mouse_buttons(void)
{
    return g_mousebuttons;
}

static void mouse_delta(int *dx, int *dy)
{
    *dx = g_mousedx;
    *dy = g_mousedy;
}

static int joy_buttons(void)
{
    return g_joybuttons;
}

static void joy_delta(int *x, int *y)
{
    *x = g_joyx;
    *y = g_joyy;
}

/* PollControls with t tics since the last frame (CalcTics reads TimeCount) */
static void poll_with_tics(int t)
{
    TimeCount = lasttimecount + t;
    PollControls();
}

static void test_keyboard(void)
{
    empty_map();
    AT(plane1, 10, 10) = 20;
    load_level(gd_hard);
    memset(orig_keys, 0, sizeof(orig_keys));

    orig_keys[sc_UpArrow] = 1;
    poll_with_tics(2);
    CHECK(tics == 2 && controly == -35 * 2 && controlx == 0);

    orig_keys[sc_RShift] = 1; /* run */
    orig_keys[sc_RightArrow] = 1;
    poll_with_tics(1);
    CHECK(buttonstate[bt_run] && controly == -70 && controlx == 70);

    /* running at two tics: 140, under the 200 limit */
    poll_with_tics(2);
    CHECK(controly == -140 && controlx == 140);
    mouseenabled = true;
    orig_hooks.mouse_delta = mouse_delta;
    orig_hooks.mouse_buttons = mouse_buttons;
    g_mousedx = 80;
    poll_with_tics(1);
    CHECK(controlx == 100); /* 70 + 80*10/8 = 170, clamped to 100 per tic */

    /* buttons, and last frame's in buttonheld */
    memset(orig_keys, 0, sizeof(orig_keys));
    g_mousedx = 0;
    orig_keys[sc_Control] = orig_keys[sc_Space] = orig_keys[sc_3] = 1;
    poll_with_tics(1);
    CHECK(buttonstate[bt_attack] && buttonstate[bt_use] && buttonstate[bt_readymachinegun]);
    CHECK(!buttonstate[bt_strafe] && buttonheld[bt_run]);
    memset(orig_keys, 0, sizeof(orig_keys));
    g_mousebuttons = 2; /* right button: strafe */
    poll_with_tics(1);
    CHECK(buttonstate[bt_strafe] && buttonheld[bt_attack] && !buttonstate[bt_attack]);

    /* mouse speed setting: 13 - mouseadjustment */
    g_mousebuttons = 0;
    g_mousedx = 13;
    g_mousedy = -13;
    mouseadjustment = 0;
    poll_with_tics(1);
    CHECK(controlx == 13 * 10 / 13 && controly == -13 * 20 / 13);
    mouseadjustment = 5;
    mouseenabled = false;

    /* joystick: the progressive mode adds a downward push to controlx */
    joystickenabled = true;
    joystickprogressive = true;
    orig_hooks.joy_delta = joy_delta;
    orig_hooks.joy_buttons = joy_buttons;
    g_joyx = 0;
    g_joyy = 74;
    poll_with_tics(1);
    CHECK(controlx == 20 && controly == 0);
    joystickprogressive = false;
    g_joyy = -100;
    g_joybuttons = 1;
    poll_with_tics(1);
    CHECK(controly == -35 && buttonstate[bt_attack]);
    joystickenabled = false;
    g_joybuttons = 0;
}

/* ------------------------------------------------------------------ demos */

static void test_demo_round_trip(void)
{
    static byte buf[3 * 40];
    static const int script[6][4] = {
        /* keys: up, left, ctrl, alt */
        {1, 0, 0, 0}, {1, 1, 0, 0}, {0, 0, 1, 0}, {0, 1, 0, 1}, {1, 0, 1, 1}, {0, 0, 0, 0}};
    int16_t recx[6], recy[6];
    boolean recattack[6], recstrafe[6];
    int i;

    empty_map();
    AT(plane1, 10, 10) = 20;
    load_level(gd_hard);
    memset(orig_keys, 0, sizeof(orig_keys));

    /* record six frames */
    demorecord = true;
    StartDemoRecording(buf, sizeof(buf));
    lasttimecount = TimeCount = 0;
    for (i = 0; i < 6; i++) {
        orig_keys[sc_UpArrow] = (byte)script[i][0];
        orig_keys[sc_LeftArrow] = (byte)script[i][1];
        orig_keys[sc_Control] = (byte)script[i][2];
        orig_keys[sc_Alt] = (byte)script[i][3];
        PollControls();
        CHECK(tics == DEMOTICS && TimeCount == 4 * (i + 1));
        recx[i] = controlx;
        recy[i] = controly;
        recattack[i] = buttonstate[bt_attack];
        recstrafe[i] = buttonstate[bt_strafe];
    }
    demorecord = false;
    CHECK(DemoRecordingEnd() == buf + 18);
    CHECK(buf[0] == 0 && (int8_t)buf[2] == -35); /* frame 1: forward */
    CHECK(buf[6] == 1 && buf[9] == 2);           /* attack, strafe bits */

    /* play them back */
    demoplayback = true;
    demoptr = buf;
    lastdemoptr = buf + 18;
    playstate = ex_stillplaying;
    memset(orig_keys, 0, sizeof(orig_keys));
    for (i = 0; i < 6; i++) {
        PollControls();
        CHECK(controlx == recx[i] && controly == recy[i]);
        CHECK(buttonstate[bt_attack] == recattack[i] && buttonstate[bt_strafe] == recstrafe[i]);
        CHECK(playstate == (i == 5 ? ex_completed : ex_stillplaying));
    }
    demoplayback = false;
}

/* --------------------------------------------------------- palette shifts */

static int g_red, g_white, g_shifts;

static void palette_shift(int red, int white)
{
    g_red = red;
    g_white = white;
    g_shifts++;
}

static void test_palette_shifts(void)
{
    int t;

    OrigHooks_Reset();
    orig_hooks.set_palette_shift = palette_shift;
    ClearPaletteShifts();
    palshifted = false;
    tics = 1;
    g_shifts = 0;

    StartDamageFlash(25);
    UpdatePaletteShifts();
    CHECK(g_red == 3 && g_white == 0 && palshifted && damagecount == 24); /* 25/10 + 1 */
    for (t = 0; t < 24; t++) {
        UpdatePaletteShifts();
    }
    CHECK(damagecount == 0 && g_red == 1);
    UpdatePaletteShifts();
    CHECK(g_red == 0 && g_white == 0 && !palshifted); /* back to normal, once */
    t = g_shifts;
    UpdatePaletteShifts();
    CHECK(g_shifts == t);

    /* bonus: white 3 for the first tics, counting down 18 */
    StartBonusFlash();
    CHECK(bonuscount == 18);
    UpdatePaletteShifts();
    CHECK(g_white == 3 && g_red == 0);
    /* damage wins over a bonus flash */
    StartDamageFlash(100);
    UpdatePaletteShifts();
    CHECK(g_red == 6 && g_white == 0);
    FinishPaletteShifts();
    CHECK(g_red == 0 && !palshifted);
}

/* --------------------------------------------------------- cheats / music */

static int g_song = -1, g_acks;

static boolean count_ack(void)
{
    g_acks++;
    return true;
}

static void play_music(int song)
{
    g_song = song;
}

static void test_cheat_and_music(void)
{
    empty_map();
    AT(plane1, 10, 10) = 20;
    load_level(gd_hard);
    orig_hooks.check_ack = count_ack;
    orig_hooks.play_music = play_music;
    gamestate.health = 20;
    gamestate.score = 1234;
    gamestate.TimeCount = 100;

    memset(orig_keys, 0, sizeof(orig_keys));
    orig_keys[sc_M] = orig_keys[sc_L] = orig_keys[sc_I] = 1;
    orig_lastscan = sc_I;
    g_acks = 0;
    CheckKeys();
    CHECK(gamestate.health == 100 && gamestate.ammo == 99 && gamestate.keys == 3);
    CHECK(gamestate.score == 0 && gamestate.TimeCount == 42100);
    CHECK(gamestate.bestweapon == wp_chaingun && gamestate.weapon == wp_chaingun);
    CHECK(!orig_keys[sc_M] && !orig_lastscan); /* keys cleared */
    CHECK(g_acks == 1);                         /* the message waits for a key */

    demoplayback = true; /* no keys during demos */
    orig_keys[sc_M] = orig_keys[sc_L] = orig_keys[sc_I] = 1;
    gamestate.health = 20;
    CheckKeys();
    demoplayback = false;
    CHECK(g_acks == 1 && gamestate.health == 20);
    memset(orig_keys, 0, sizeof(orig_keys));

    gamestate.episode = 0;
    gamestate.mapon = 0;
    StartMusic();
    CHECK(g_song == GETTHEM_MUS);
    gamestate.mapon = 8; /* boss level */
    StartMusic();
    CHECK(g_song == WARMARCH_MUS);
    gamestate.episode = 5;
    gamestate.mapon = 9;
    StartMusic();
    CHECK(g_song == FUNKYOU_MUS);
}

/* --------------------------------------------------------------- PlayLoop */

static int g_presented;

static void present(void)
{
    g_presented++;
}

/* SetSoundLoc: 0 is full volume, 8 the quietest, per ear */
static void test_sound_loc(void)
{
    const fixed cx = (10l << TILESHIFT) + TILEGLOBAL / 2, cy = cx;

    viewx = cx;
    viewy = cy;
    viewcos = costable[0]; /* facing east: south is to the right */
    viewsin = sintable[0];

    SetSoundLoc(cx, cy);
    CHECK(leftchannel == 0 && rightchannel == 0);
    SetSoundLoc(cx, cy + 3 * TILEGLOBAL);
    CHECK(leftchannel == 7 && rightchannel == 0);
    SetSoundLoc(cx, cy - 3 * TILEGLOBAL);
    CHECK(leftchannel == 0 && rightchannel == 7);
    SetSoundLoc(cx + 20 * TILEGLOBAL, cy); /* far ahead: quiet on both sides */
    CHECK(leftchannel == 8 && rightchannel == 8);
    /* behind and to the right is still on the right (the tables have no back) */
    SetSoundLoc(cx - 2 * TILEGLOBAL, cy + TILEGLOBAL);
    CHECK(leftchannel > rightchannel);

    viewcos = costable[ANGLES / 4]; /* facing north: east is to the right */
    viewsin = sintable[ANGLES / 4];
    SetSoundLoc(cx + 3 * TILEGLOBAL, cy);
    CHECK(leftchannel == 7 && rightchannel == 0);
}

/*
 * controlstrafe (not in the original, for twin-stick pads): strafes while
 * controlx turns, bounded like the others, never recorded in a demo.
 */
static void pad_strafe(void)
{
    controlstrafe = 32000; /* far past the bound */
}

static void test_twin_stick_strafe(void)
{
    fixed x, y;
    int angle;
    static byte buf[64];

    empty_map();
    AT(plane1, 10, 10) = 20; /* the player, facing east */
    load_level(gd_hard);
    x = player->x;
    y = player->y;
    angle = player->angle;

    /* strafing right while facing east moves south, no turn */
    controlx = controly = 0;
    controlstrafe = 50;
    ControlMovement(player);
    CHECK(player->x == x && player->y > y && player->angle == angle);

    /* and at the same time as a turn */
    y = player->y;
    controlx = 100;
    controlstrafe = -50;
    ControlMovement(player);
    CHECK(player->y < y && player->angle != angle);
    controlx = controlstrafe = 0;

    /* PollControls clears it, bounds what the input hook adds */
    orig_hooks.poll_input = pad_strafe;
    memset(orig_keys, 0, sizeof(orig_keys));
    lasttimecount = TimeCount = 0;
    demorecord = true; /* fixed tics */
    StartDemoRecording(buf, sizeof(buf));
    PollControls();
    CHECK(controlstrafe == 0); /* recording: a demo cannot hold it */
    demorecord = false;
    PollControls();
    CHECK(controlstrafe == 100 * (int)tics);
    orig_hooks.poll_input = NULL;
    PollControls();
    CHECK(controlstrafe == 0);
}

static void test_play_loop(void)
{
    /* a synthetic demo on map 0 of the synthetic map: 30 frames forward */
    static byte demo[4 + 3 * 30];
    int i;

    empty_map();
    AT(plane1, 10, 10) = 20;
    mapsegs[0] = plane0;
    mapsegs[1] = plane1;

    demo[0] = 0;
    demo[1] = (byte)sizeof(demo);
    demo[2] = 0;
    for (i = 0; i < 30; i++) {
        demo[4 + 3 * i] = 0;
        demo[5 + 3 * i] = 0;
        demo[6 + 3 * i] = (byte)(int8_t)-25;
    }

    /*
     * PlayDemo loads the map through SetupGameLevel, which needs map files;
     * run its loop part by hand on the synthetic map instead.
     */
    load_level(gd_hard);
    orig_hooks.present_frame = present;
    g_presented = 0;
    demoplayback = true;
    demoptr = demo + 4;
    lastdemoptr = demo + sizeof(demo);
    fizzlein = true;
    startgame = false; /* as PlayDemo after NewGame */
    PlayLoop();
    demoplayback = false;

    CHECK(playstate == ex_completed && g_presented == 30);
    CHECK(gamestate.TimeCount == 30 * DEMOTICS);
    /* 30 frames of -25*4 forward: 100 per frame -> FixedByFrac (15000, 0xffff) each */
    CHECK(player->x == (10l << TILESHIFT) + TILEGLOBAL / 2 + 30 * 14999);
}

/* -------------------------------------------------------------- real data */

static int g_frames;

static const byte *pack_page(int page)
{
    return (const byte *)PM_GetPage(page);
}

static void count_frame(void)
{
    g_frames++;
}

static uint32_t end_state(void)
{
    const int32_t v[12] = {playstate,        gamestate.health,  gamestate.ammo,
                           gamestate.score,  gamestate.killcount, player->x,
                           player->y,        player->angle,     gamestate.mapon,
                           gamestate.weapon, g_frames,          gamestate.treasurecount};
    return fnv(2166136261u, v, sizeof(v));
}

/*
 * GameLoop over a scripted game: each level run ends on its first frame with
 * the next playstate. Completed, secret elevator, back via ElevatorBackTo,
 * then four deaths: three restarts and game over.
 */
static const exit_t g_script[] = {ex_completed, ex_secretlevel, ex_completed,
                                  ex_died,      ex_died,        ex_died,
                                  ex_died};
#define SCRIPTRUNS ((int)(sizeof(g_script) / sizeof(g_script[0])))
static int g_runs, g_runmap[16], g_runlives[16];
static int32_t g_runscore[16];
static boolean g_inrun;
static int g_psyched, g_completed, g_gameover, g_diedfade, g_victory;

/* the fizzle-in that starts every level run (Died's red fizzle is 70 frames) */
static void script_level(int x, int y, int width, int height, int frames)
{
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    if (frames != 20) {
        return;
    }
    if (g_runs < 16) {
        g_runmap[g_runs] = gamestate.mapon;
        g_runlives[g_runs] = gamestate.lives;
        g_runscore[g_runs] = gamestate.score;
    }
    g_runs++;
    g_inrun = true;
}

static void script_frame(void)
{
    if (!g_inrun) {
        return; /* Died's turn towards the killer */
    }
    g_inrun = false;
    if (g_runs > SCRIPTRUNS) {
        abortgame = true; /* the script ran out: GameLoop did not stop */
        playstate = ex_abort;
        return;
    }
    gamestate.score += g_runs == 1 ? 100 : 500; /* kept at level end, lost on death */
    playstate = g_script[g_runs - 1];
}

/* PreloadGraphics waits 70 tics on "Get Psyched!", Died 100 after the red fizzle */
static boolean count_waits(int32_t tics)
{
    g_psyched += tics == 70;
    g_diedfade += tics == 100;
    return false;
}
static void count_completed(void) { g_completed++; }
static void count_gameover(void) { g_gameover++; }
static void count_victory(void) { g_victory++; }

static void test_game_loop(void)
{
    static const int maps[SCRIPTRUNS] = {0, 1, 9, 1, 1, 1, 1};
    static const int lives[SCRIPTRUNS] = {3, 3, 3, 3, 2, 1, 0};
    int i;

    install_hooks();
    orig_hooks.fizzle = script_level;
    orig_hooks.present_frame = script_frame;
    orig_hooks.user_input = count_waits;
    orig_hooks.level_completed = count_completed;
    orig_hooks.game_over = count_gameover;
    orig_hooks.victory = count_victory;
    g_runs = g_psyched = g_completed = g_gameover = g_diedfade = g_victory = 0;
    g_inrun = false;
    abortgame = false;
    killerobj = NULL;

    NewGame(gd_medium, 0);
    GameLoop();

    printf("GameLoop: %d level runs, %d completed, %d died, lives %d, score %ld\n", g_runs,
           g_completed, g_diedfade, gamestate.lives, (long)gamestate.score);
    CHECK(g_runs == SCRIPTRUNS && !abortgame);
    for (i = 0; i < SCRIPTRUNS && i < g_runs; i++) {
        CHECK(g_runmap[i] == maps[i]);
        CHECK(g_runlives[i] == lives[i]);
    }
    /* the score at a level's end carries over; points from a lost life do not */
    CHECK(g_runscore[0] == 0 && g_runscore[1] == 100 && g_runscore[2] == 600);
    CHECK(g_runscore[3] == 1100 && g_runscore[4] == 1100 && g_runscore[6] == 1100);
    CHECK(gamestate.lives == -1);
    CHECK(g_psyched == 4); /* no "get psyched" after a death */
    CHECK(g_completed == 3 && g_diedfade == 4 && g_gameover == 1 && g_victory == 0);

    /* Esc (abortgame) leaves GameLoop at once */
    install_hooks();
    orig_hooks.fizzle = script_level;
    orig_hooks.present_frame = script_frame;
    g_runs = SCRIPTRUNS; /* the next frame aborts */
    g_inrun = false;
    abortgame = false;
    NewGame(gd_medium, 0);
    GameLoop();
    CHECK(abortgame && g_runs == SCRIPTRUNS + 1);
    abortgame = false;
}

/*
 * Save format v2: stop a demo halfway and save. Run A plays on from there;
 * run B first loads another map, then loads the save and plays on. Loading
 * restarts PlayLoop in both, as after F9 in the game, so they must end the
 * same. Also: the stream survives a load byte for byte, and a damaged one
 * is refused without touching the engine.
 */
static int g_stopat;
static byte g_save[64 * 1024], g_save2[64 * 1024];

static void stop_frame(void)
{
    if (++g_frames == g_stopat) {
        playstate = ex_abort;
    }
}

static void test_save_load(int d, const byte *lump, unsigned length, unsigned inputframes)
{
    const byte *resume;
    size_t size, size2;
    uint32_t a, b;
    gametype before;
    unsigned other;

    install_hooks();
    orig_hooks.present_frame = stop_frame;
    g_frames = 0;
    g_stopat = (int)inputframes / 2;
    CHECK(PlayDemo(lump, length));
    CHECK(g_frames == g_stopat && playstate == ex_abort);
    resume = demoptr;
    size = SaveTheGame(g_save, sizeof(g_save));
    CHECK(size > 0 && size <= sizeof(g_save));

    /* A: on from the stop */
    orig_hooks.present_frame = count_frame;
    g_frames = 0;
    demoplayback = true;
    PlayLoop();
    demoplayback = false;
    a = end_state();

    /* B: another map first, then the save */
    other = (lump[0] + 1) % 10;
    gamestate.mapon = (int16_t)other;
    CHECK(SetupGameLevel());
    CHECK(LoadTheGame(g_save, size));
    size2 = SaveTheGame(g_save2, sizeof(g_save2));
    CHECK(size2 == size && memcmp(g_save, g_save2, size) == 0);
    g_frames = 0;
    demoptr = resume;
    lastdemoptr = lump + length;
    demoplayback = true;
    loadedgame = false;
    PlayLoop();
    demoplayback = false;
    b = end_state();
    printf("  demo %d saved at frame %d (%u bytes): on %08x, loaded %08x\n", d, g_stopat,
           (unsigned)size, a, b);
    CHECK(a == b);

    /* damaged saves are refused and leave the game alone */
    before = gamestate;
    CHECK(!LoadTheGame(g_save, size - 1)); /* truncated */
    CHECK(!LoadTheGame(NULL, 0));
    g_save[0] = 1; /* version 1: the legacy format */
    CHECK(!LoadTheGame(g_save, size));
    g_save[0] = 2;
    g_save[4] = 60; /* gamestate.mapon (after version and difficulty): past the last map */
    CHECK(!LoadTheGame(g_save, size));
    CHECK(memcmp(&before, &gamestate, sizeof(before)) == 0 && !loadedgame);
}

static int run_pack(int argc, char **argv)
{
    wolf_data_dir_t dir;
    int d;
    boolean wl1;
    /*
     * End states pinned per pack, at view size 15 (the original's default).
     * At that size WL1 demo 0 ends 97 frames before its input does: the
     * view size changes play (hit test, which tiles the rays reach) and the
     * demo was recorded at another size. All eight demos are in sync at
     * view sizes 7, 12, 14, 17 and 19 (test_orig_fidelity).
     */
    static const uint32_t wl1_pins[4] = {0xe30e88cau, 0x23f1465fu, 0xa2a105b1u, 0x4bfec1c1u};
    static const uint32_t wl6_pins[4] = {0x4d19d1c2u, 0xc7717cf7u, 0x57bf2a35u, 0xd4a674a1u};

    memset(&dir, 0, sizeof(dir));
    if (!Wolf_ResolveDataDir(argc, argv, &dir)) {
        return 1;
    }
    CA_Startup();
    if (!CA_OpenMapFiles(&dir) || !PM_OpenPageFile(&dir) || !CA_OpenGraphFiles(&dir)) {
        fprintf(stderr, "FAIL: could not open %s\n", dir.path);
        return 1;
    }
    wl1 = strcmp(g_gfx->name, "WL1") == 0;
    upload = wl1;
    orig_getpage = pack_page;
    orig_spritestart = (int16_t)PMSpriteStart;
    NewViewSize(15);

    for (d = 0; d < NUMDEMOS; d++) {
        const byte *lump;
        unsigned length, inputframes;
        uint32_t h;

        CA_CacheGrChunk(T_DEMO0 + d);
        lump = grsegs[T_DEMO0 + d];
        CHECK(lump != NULL);
        if (!lump) {
            continue;
        }
        length = (unsigned)lump[1] | ((unsigned)lump[2] << 8);
        inputframes = (length - 4) / 3;

        install_hooks();
        orig_hooks.present_frame = count_frame;
        g_frames = 0;
        CHECK(PlayDemo(lump, length));
        h = end_state();

        printf("%s demo %d (map %d): %d of %u frames, playstate %d, health %d, kills %d, "
               "end state %08x\n",
               g_gfx->name, d, lump[0], g_frames, inputframes, playstate, gamestate.health,
               gamestate.killcount, h);

        if (!(wl1 && d == 0)) {
            /* in sync: the level ends exactly when the recorded input does */
            CHECK((unsigned)g_frames == inputframes);
            CHECK(playstate == ex_died || playstate == ex_completed);
        }
        CHECK(h == (wl1 ? wl1_pins : wl6_pins)[d]);
        test_save_load(d, lump, length, inputframes);
    }

    test_game_loop(); /* SetupGameLevel needs the pack's maps */
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
        test_calc_tics();
        test_keyboard();
        test_demo_round_trip();
        test_palette_shifts();
        test_cheat_and_music();
        test_play_loop();
        test_twin_stick_strafe();
        test_sound_loc();
    }

    mapsegs[0] = mapsegs[1] = NULL;
    if (g_failures) {
        fprintf(stderr, "%d check(s) failed\n", g_failures);
        return 1;
    }
    printf("test_orig_loop: all checks passed\n");
    return 0;
}
