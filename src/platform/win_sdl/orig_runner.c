/*
 * The faithful engine in an SDL window (P4/S5b). See orig_runner.h.
 *
 * Everything here is what the original did with hardware or with screens
 * that are not game logic: the timer interrupt, the keyboard interrupt's
 * scan-code table, the mouse driver, VGA palette writes, FizzleFade, the
 * status bar pictures, sound and music. The engine calls it through
 * orig_hooks.
 */
#include "orig_runner.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif
#include <SDL.h>

#include "orig.h"
#include "orig_bridge.h"
#include "wolf_pad.h"
#include "wolf_userdata.h"
#include "wl_tics.h"

static tic_clock_t g_clock;
static boolean g_fast;      /* no clock: smoke tests */
static boolean g_quit;      /* window closed or Esc */
static boolean g_paused;    /* the Pause key */
static int g_mousedx, g_mousedy;
static int g_frames;

/* -------------------------------------------------------------- arguments */

static const char *ArgValue(int argc, char **argv, const char *name)
{
    const size_t len = strlen(name);
    int i;

    for (i = 1; i < argc; i++) {
        if (strncmp(argv[i], name, len) == 0 && argv[i][len] == '=') {
            return argv[i] + len + 1;
        }
        if (strcmp(argv[i], name) == 0 && i + 1 < argc) {
            return argv[i + 1];
        }
    }
    return NULL;
}

boolean OrigRunner_Direct(int argc, char **argv)
{
    static const char *const flags[] = {"--engine", "--map",  "--skill",  "--viewsize",  "--demo",
                                        "--load",   "--fast", "--frames", "--screenshot"};
    size_t f;
    int i;

    for (i = 1; i < argc; i++) {
        for (f = 0; f < sizeof(flags) / sizeof(flags[0]); f++) {
            const size_t len = strlen(flags[f]);
            if (strncmp(argv[i], flags[f], len) == 0 && (!argv[i][len] || argv[i][len] == '=')) {
                return true;
            }
        }
    }
    return false;
}

void OrigRunner_Defaults(orig_run_options_t *opts)
{
    opts->map = 0;
    opts->skill = gd_medium; /* "Bring 'em on" */
    opts->viewsize = 15;
    opts->demo = -1;
    opts->load = -1;
    opts->fast = 0;
    opts->maxframes = 0;
    opts->shotframe = 0;
    opts->shotpath = NULL;
}

static boolean IntArg(int argc, char **argv, const char *name, int lo, int hi, int *out)
{
    const char *v = ArgValue(argc, argv, name);
    char *end;
    long n;

    if (!v) {
        return true;
    }
    n = strtol(v, &end, 10);
    if (*end || n < lo || n > hi) {
        fprintf(stderr, "%s must be %d-%d\n", name, lo, hi);
        return false;
    }
    *out = (int)n;
    return true;
}

boolean OrigRunner_ParseArgs(int argc, char **argv, orig_run_options_t *opts)
{
    const char *engine = ArgValue(argc, argv, "--engine");
    int i;

    OrigRunner_Defaults(opts);
    if (engine && strcmp(engine, "orig") != 0) {
        fprintf(stderr, "--engine=%s: the legacy engine was removed in P4/S6; "
                        "the faithful engine is the only one\n", engine);
        return false;
    }
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--fast") == 0) {
            opts->fast = 1;
        }
        if (strcmp(argv[i], "--screenshot") == 0 && i + 2 < argc) {
            opts->shotframe = atoi(argv[i + 1]);
            opts->shotpath = argv[i + 2];
            if (opts->shotframe < 1) {
                fprintf(stderr, "--screenshot needs a frame number of 1 or more\n");
                return false;
            }
        }
    }
    if (!IntArg(argc, argv, "--frames", 1, 1000000, &opts->maxframes)) {
        return false;
    }
    return IntArg(argc, argv, "--map", 0, 59, &opts->map) &&
           IntArg(argc, argv, "--skill", gd_baby, gd_hard, &opts->skill) &&
           IntArg(argc, argv, "--viewsize", 4, 20, &opts->viewsize) &&
           IntArg(argc, argv, "--demo", 0, 3, &opts->demo) && /* T_DEMO0..3 */
           IntArg(argc, argv, "--load", 0, WOLF_MAX_SAVE_SLOTS - 1, &opts->load);
}

/* ------------------------------------------------------------------ clock */

static int32_t ReadClock(void)
{
    return (int32_t)TicClock_Read(&g_clock, SDL_GetPerformanceCounter());
}

static void WriteClock(int32_t count)
{
    TicClock_Set(&g_clock, (longword)count, SDL_GetPerformanceCounter());
}

static void PumpEvents(void);

static void WaitClock(int32_t target)
{
    while (!g_quit && ReadClock() < target) {
        PumpEvents();
        SDL_Delay(1);
    }
}

/* Wait up to tics, or until a key or button (IN_UserInput). */
static void UserInput(int32_t waittics)
{
    const int32_t end = ReadClock() + waittics;

    orig_lastscan = 0;
    while (!g_quit && ReadClock() < end && !orig_lastscan) {
        PumpEvents();
        SDL_Delay(1);
    }
}

/* ------------------------------------------------------------------ input */

/* SDL scancode -> DOS set-1 scan code, as the original's Keyboard[] has it */
static byte ScanCode(SDL_Scancode sc)
{
    static const struct {
        SDL_Scancode sdl;
        byte dos;
    } map[] = {
        {SDL_SCANCODE_ESCAPE, 0x01}, {SDL_SCANCODE_1, 0x02}, {SDL_SCANCODE_2, 0x03},
        {SDL_SCANCODE_3, 0x04}, {SDL_SCANCODE_4, 0x05}, {SDL_SCANCODE_5, 0x06},
        {SDL_SCANCODE_6, 0x07}, {SDL_SCANCODE_7, 0x08}, {SDL_SCANCODE_8, 0x09},
        {SDL_SCANCODE_9, 0x0a}, {SDL_SCANCODE_0, 0x0b}, {SDL_SCANCODE_MINUS, 0x0c},
        {SDL_SCANCODE_EQUALS, 0x0d}, {SDL_SCANCODE_BACKSPACE, 0x0e}, {SDL_SCANCODE_TAB, 0x0f},
        {SDL_SCANCODE_Q, 0x10}, {SDL_SCANCODE_W, 0x11}, {SDL_SCANCODE_E, 0x12},
        {SDL_SCANCODE_R, 0x13}, {SDL_SCANCODE_T, 0x14}, {SDL_SCANCODE_Y, 0x15},
        {SDL_SCANCODE_U, 0x16}, {SDL_SCANCODE_I, 0x17}, {SDL_SCANCODE_O, 0x18},
        {SDL_SCANCODE_P, 0x19}, {SDL_SCANCODE_LEFTBRACKET, 0x1a},
        {SDL_SCANCODE_RIGHTBRACKET, 0x1b}, {SDL_SCANCODE_RETURN, 0x1c},
        {SDL_SCANCODE_LCTRL, 0x1d}, {SDL_SCANCODE_RCTRL, 0x1d}, {SDL_SCANCODE_A, 0x1e},
        {SDL_SCANCODE_S, 0x1f}, {SDL_SCANCODE_D, 0x20}, {SDL_SCANCODE_F, 0x21},
        {SDL_SCANCODE_G, 0x22}, {SDL_SCANCODE_H, 0x23}, {SDL_SCANCODE_J, 0x24},
        {SDL_SCANCODE_K, 0x25}, {SDL_SCANCODE_L, 0x26}, {SDL_SCANCODE_SEMICOLON, 0x27},
        {SDL_SCANCODE_APOSTROPHE, 0x28}, {SDL_SCANCODE_GRAVE, 0x29}, {SDL_SCANCODE_LSHIFT, 0x2a},
        {SDL_SCANCODE_BACKSLASH, 0x2b}, {SDL_SCANCODE_Z, 0x2c}, {SDL_SCANCODE_X, 0x2d},
        {SDL_SCANCODE_C, 0x2e}, {SDL_SCANCODE_V, 0x2f}, {SDL_SCANCODE_B, 0x30},
        {SDL_SCANCODE_N, 0x31}, {SDL_SCANCODE_M, 0x32}, {SDL_SCANCODE_COMMA, 0x33},
        {SDL_SCANCODE_PERIOD, 0x34}, {SDL_SCANCODE_SLASH, 0x35}, {SDL_SCANCODE_RSHIFT, 0x36},
        {SDL_SCANCODE_KP_MULTIPLY, 0x37}, {SDL_SCANCODE_LALT, 0x38}, {SDL_SCANCODE_RALT, 0x38},
        {SDL_SCANCODE_SPACE, 0x39}, {SDL_SCANCODE_CAPSLOCK, 0x3a}, {SDL_SCANCODE_F1, 0x3b},
        {SDL_SCANCODE_F2, 0x3c}, {SDL_SCANCODE_F3, 0x3d}, {SDL_SCANCODE_F4, 0x3e},
        {SDL_SCANCODE_F5, 0x3f}, {SDL_SCANCODE_F6, 0x40}, {SDL_SCANCODE_F7, 0x41},
        {SDL_SCANCODE_F8, 0x42}, {SDL_SCANCODE_F9, 0x43}, {SDL_SCANCODE_F10, 0x44},
        {SDL_SCANCODE_HOME, 0x47}, {SDL_SCANCODE_UP, 0x48}, {SDL_SCANCODE_PAGEUP, 0x49},
        {SDL_SCANCODE_KP_MINUS, 0x4a}, {SDL_SCANCODE_LEFT, 0x4b}, {SDL_SCANCODE_KP_5, 0x4c},
        {SDL_SCANCODE_RIGHT, 0x4d}, {SDL_SCANCODE_KP_PLUS, 0x4e}, {SDL_SCANCODE_END, 0x4f},
        {SDL_SCANCODE_DOWN, 0x50}, {SDL_SCANCODE_PAGEDOWN, 0x51}, {SDL_SCANCODE_INSERT, 0x52},
        {SDL_SCANCODE_DELETE, 0x53}, {SDL_SCANCODE_KP_8, 0x48}, {SDL_SCANCODE_KP_2, 0x50},
        {SDL_SCANCODE_KP_4, 0x4b}, {SDL_SCANCODE_KP_6, 0x4d}, {SDL_SCANCODE_KP_ENTER, 0x1c},
    };
    size_t i;

    for (i = 0; i < sizeof(map) / sizeof(map[0]); i++) {
        if (map[i].sdl == sc) {
            return map[i].dos;
        }
    }
    return 0;
}

static void PumpEvents(void)
{
    SDL_Event ev;

    while (SDL_PollEvent(&ev)) {
        switch (ev.type) {
        case SDL_QUIT:
            g_quit = true;
            break;
        case SDL_KEYDOWN:
            if (ev.key.keysym.scancode == SDL_SCANCODE_F11 && !ev.key.repeat) {
                (void)Bridge_ToggleFullscreen();
                break;
            }
            if (ev.key.keysym.scancode == SDL_SCANCODE_PAUSE) {
                g_paused = true;
                break;
            }
            {
                const byte sc = ScanCode(ev.key.keysym.scancode);
                if (sc) {
                    orig_keys[sc] = 1;
                    orig_lastscan = sc;
                }
            }
            break;
        case SDL_KEYUP: {
            const byte sc = ScanCode(ev.key.keysym.scancode);
            if (sc) {
                orig_keys[sc] = 0;
            }
        } break;
        case SDL_MOUSEMOTION:
            g_mousedx += ev.motion.xrel;
            g_mousedy += ev.motion.yrel;
            break;
        case SDL_MOUSEBUTTONDOWN:
            orig_lastscan = 0xff; /* any button counts as an acknowledge */
            break;
        case SDL_CONTROLLERDEVICEADDED:
        case SDL_CONTROLLERDEVICEREMOVED:
            Wolf_PadEvent(&ev);
            break;
        case SDL_CONTROLLERBUTTONDOWN:
            if (ev.cbutton.button == SDL_CONTROLLER_BUTTON_BACK) {
                g_paused = true;
                break;
            }
            /* Start is Esc; any other button acknowledges, as a key would */
            orig_lastscan = ev.cbutton.button == SDL_CONTROLLER_BUTTON_START ? 0x01 : 0xff;
            break;
        default:
            break;
        }
    }
}

/* The pad, twin-stick (pad_map.h), added to what the keyboard and mouse give. */
static pad_state_t g_padbefore;

static void PollInput(void)
{
    pad_state_t pad;
    pad_controls_t c;

    PumpEvents();
    if (!Wolf_PadRead(&pad)) {
        memset(&g_padbefore, 0, sizeof(g_padbefore));
        return;
    }
    Pad_Map(&pad, &g_padbefore, tics, &c);
    g_padbefore = pad;

    controlx = (int16_t)(controlx + c.controlx);
    controly = (int16_t)(controly + c.controly);
    controlstrafe = (int16_t)(controlstrafe + c.controlstrafe);
    if (c.attack) {
        buttonstate[bt_attack] = true;
    }
    if (c.use) {
        buttonstate[bt_use] = true;
    }
    if (c.run) {
        buttonstate[bt_run] = true;
    }
    if (c.weaponstep && (int)gamestate.weapon >= 0) {
        /* as pressing 1-4: the engine still decides (no bullets, no change) */
        buttonstate[bt_readyknife + Pad_NextWeapon(gamestate.weapon, gamestate.bestweapon,
                                                   c.weaponstep)] = true;
    }
}

static int MouseButtons(void)
{
    const Uint32 b = SDL_GetMouseState(NULL, NULL);
    return ((b & SDL_BUTTON_LMASK) ? 1 : 0) | ((b & SDL_BUTTON_RMASK) ? 2 : 0) |
           ((b & SDL_BUTTON_MMASK) ? 4 : 0);
}

static void MouseDelta(int *dx, int *dy)
{
    *dx = g_mousedx;
    *dy = g_mousedy;
    g_mousedx = g_mousedy = 0;
}

static boolean CheckAck(void)
{
    PumpEvents();
    if (orig_lastscan) {
        orig_lastscan = 0;
        return true;
    }
    return g_quit;
}

/* --------------------------------------------------------- the 320x200 frame */

static const orig_run_options_t *g_opts;
static const byte *g_palette; /* the palette on screen now */
static boolean g_shotsaved;

static void PutLE(byte *p, uint32_t v, int bytes)
{
    int i;

    for (i = 0; i < bytes; i++) {
        p[i] = (byte)(v >> (8 * i));
    }
}

/* The frame as an 8-bit BMP with the palette on screen. */
static void SaveScreenshot(const char *path)
{
    const byte *fb = Bridge_Framebuffer();
    byte header[54], pal[1024];
    FILE *f;
    int i, y;

    memset(header, 0, sizeof(header));
    header[0] = 'B';
    header[1] = 'M';
    PutLE(header + 2, 54 + 1024 + SCREENWIDTH * SCREENHEIGHT, 4);
    PutLE(header + 10, 54 + 1024, 4);
    PutLE(header + 14, 40, 4);
    PutLE(header + 18, SCREENWIDTH, 4);
    PutLE(header + 22, SCREENHEIGHT, 4);
    PutLE(header + 26, 1, 2);
    PutLE(header + 28, 8, 2);
    PutLE(header + 46, 256, 4);
    for (i = 0; i < 256; i++) {
        pal[i * 4 + 0] = (byte)(g_palette[i * 3 + 2] * 255 / 63);
        pal[i * 4 + 1] = (byte)(g_palette[i * 3 + 1] * 255 / 63);
        pal[i * 4 + 2] = (byte)(g_palette[i * 3 + 0] * 255 / 63);
        pal[i * 4 + 3] = 0;
    }

    f = fopen(path, "wb");
    if (!f) {
        fprintf(stderr, "Could not write %s\n", path);
        return;
    }
    fwrite(header, 1, sizeof(header), f);
    fwrite(pal, 1, sizeof(pal), f);
    for (y = SCREENHEIGHT - 1; y >= 0; y--) { /* bottom-up rows */
        fwrite(fb + y * SCREENWIDTH, 1, SCREENWIDTH, f);
    }
    g_shotsaved = fclose(f) == 0;
    printf("Screenshot of frame %d: %s\n", g_frames, path);
}

static void Present(void)
{
    byte *fb = Bridge_Framebuffer();

    /* the play area comes from the engine's frame, the status bar is drawn in place */
    memcpy(fb, screenbuf, (size_t)SCREENWIDTH * (SCREENHEIGHT - STATUSLINES));
    Bridge_Present();
    PumpEvents();
    g_frames++;

    if (g_opts->shotframe == g_frames && g_opts->shotpath) {
        SaveScreenshot(g_opts->shotpath);
    }
    if (g_opts->maxframes && g_frames >= g_opts->maxframes) {
        abortgame = true;
        playstate = ex_abort;
    }
}

/* FizzleFade (ID_VH.C): the view window of screenbuf appears pixel by pixel
 * in the order of a 17-bit LFSR, 64000/frames pixels per tic. */
static void FizzleFade(int frames)
{
    byte *fb = Bridge_Framebuffer();
    const int x0 = screenofs % SCREENWIDTH, y0 = screenofs / SCREENWIDTH;
    const int pixperframe = 64000 / frames;
    uint32_t rndval = 1;
    int32_t start = g_fast ? 0 : ReadClock();
    int frame = 0, p;

    for (;;) {
        for (p = 0; p < pixperframe; p++) {
            const int y = (int)((rndval & 0xff) - 1) & 0xff; /* low 8 bits - 1 */
            const int x = (int)((rndval >> 8) & 0x1ff);       /* next 9 bits */
            const uint32_t carry = rndval & 1;

            rndval >>= 1;
            if (carry) {
                rndval ^= 0x00012000u;
            }

            if (x < viewwidth && y < viewheight) {
                const int at = (y0 + y) * SCREENWIDTH + x0 + x;
                fb[at] = screenbuf[at];
            }
            if (rndval == 1) { /* entire sequence has been completed */
                Bridge_Present();
                return;
            }
        }
        Bridge_Present();
        frame++;
        if (!g_fast) {
            WaitClock(start + frame); /* don't go too fast */
        }
        PumpEvents();
        if (g_quit) {
            return;
        }
    }
}

static void HookFizzleIn(void)
{
    FizzleFade(20);
}

/* -------------------------------------------------------------- status bar */

static void StatusDrawPic(int x, int y, int chunk)
{
    Bridge_DrawPic(x * 8, 200 - STATUSLINES + y, chunk);
}

/* right justifies and pads with blanks */
static void LatchNumber(int x, int y, int width, long number)
{
    char str[20];
    int length, c;

    snprintf(str, sizeof(str), "%ld", number);
    length = (int)strlen(str);

    while (length < width) {
        StatusDrawPic(x, y, Bridge_Pic(BRIDGE_N_BLANKPIC));
        x++;
        width--;
    }

    c = length <= width ? 0 : length - width;

    while (c < length) {
        StatusDrawPic(x, y, str[c] - '0' + Bridge_Pic(BRIDGE_N_0PIC));
        x++;
        c++;
    }
}

static void HookStatusBar(void)
{
    Bridge_DrawPic(0, 200 - STATUSLINES, Bridge_Pic(BRIDGE_STATUSBARPIC));
}

static void HookDrawFace(void)
{
    if (gamestate.health) {
        StatusDrawPic(17, 4,
                      Bridge_Pic(BRIDGE_FACE1APIC) + 3 * ((100 - gamestate.health) / 16) +
                          gamestate.faceframe);
    } else if (LastAttacker && LastAttacker->obclass == needleobj) {
        StatusDrawPic(17, 4, Bridge_Pic(BRIDGE_MUTANTBJPIC));
    } else {
        StatusDrawPic(17, 4, Bridge_Pic(BRIDGE_FACE8APIC));
    }
}

static void HookDrawHealth(void)
{
    LatchNumber(21, 16, 3, gamestate.health);
}

static void HookDrawLevel(void)
{
    LatchNumber(2, 16, 2, gamestate.mapon + 1);
}

static void HookDrawLives(void)
{
    LatchNumber(14, 16, 1, gamestate.lives);
}

static void HookDrawScore(void)
{
    LatchNumber(6, 16, 6, gamestate.score);
}

static void HookDrawWeapon(void)
{
    if ((int)gamestate.weapon >= 0) {
        StatusDrawPic(32, 8, Bridge_Pic(BRIDGE_KNIFEPIC) + gamestate.weapon);
    }
}

static void HookDrawKeys(void)
{
    StatusDrawPic(30, 4,
                  Bridge_Pic((gamestate.keys & 1) ? BRIDGE_GOLDKEYPIC : BRIDGE_NOKEYPIC));
    StatusDrawPic(30, 20,
                  Bridge_Pic((gamestate.keys & 2) ? BRIDGE_SILVERKEYPIC : BRIDGE_NOKEYPIC));
}

static void HookDrawAmmo(void)
{
    LatchNumber(27, 16, 2, gamestate.ammo);
}

static void HookDrawGotGatling(void)
{
    StatusDrawPic(17, 4, Bridge_Pic(BRIDGE_GOTGATLINGPIC));
}

/* ----------------------------------------------------------------- palette */

#define NUMREDSHIFTS   6
#define REDSTEPS       8
#define NUMWHITESHIFTS 3
#define WHITESTEPS     20

static byte redshifts[NUMREDSHIFTS][768];
static byte whiteshifts[NUMWHITESHIFTS][768];

/* InitRedShifts (WL_PLAY.C) */
static void InitRedShifts(void)
{
    const byte *pal = Bridge_GamePalette();
    int i, j;

    for (i = 1; i <= NUMREDSHIFTS; i++) {
        for (j = 0; j < 256; j++) {
            redshifts[i - 1][j * 3] = (byte)(pal[j * 3] + (64 - pal[j * 3]) * i / REDSTEPS);
            redshifts[i - 1][j * 3 + 1] = (byte)(pal[j * 3 + 1] - pal[j * 3 + 1] * i / REDSTEPS);
            redshifts[i - 1][j * 3 + 2] = (byte)(pal[j * 3 + 2] - pal[j * 3 + 2] * i / REDSTEPS);
        }
    }
    for (i = 1; i <= NUMWHITESHIFTS; i++) {
        for (j = 0; j < 256; j++) {
            whiteshifts[i - 1][j * 3] =
                (byte)(pal[j * 3] + (64 - pal[j * 3]) * i / WHITESTEPS);
            whiteshifts[i - 1][j * 3 + 1] =
                (byte)(pal[j * 3 + 1] + (62 - pal[j * 3 + 1]) * i / WHITESTEPS);
            whiteshifts[i - 1][j * 3 + 2] =
                (byte)(pal[j * 3 + 2] + (0 - pal[j * 3 + 2]) * i / WHITESTEPS);
        }
    }
}

static void SetPalette(const byte *palette)
{
    g_palette = palette;
    Bridge_SetPalette(palette);
}

static void HookPaletteShift(int red, int white)
{
    if (red) {
        SetPalette(redshifts[red - 1]);
    } else if (white) {
        SetPalette(whiteshifts[white - 1]);
    } else {
        SetPalette(Bridge_GamePalette());
    }
}

/* ------------------------------------------------------------ sound, music */

static void HookPlaySound(int sound)
{
    (void)Bridge_PlaySound(sound);
}

/* PlaySoundLocGlobal (WL_GAME.C): place the sound, remember where it is */
static void HookPlaySoundLoc(int sound, fixed gx, fixed gy)
{
    SetSoundLoc(gx, gy);
    Bridge_PositionSound(leftchannel, rightchannel);
    if (Bridge_PlaySound(sound)) {
        globalsoundx = gx;
        globalsoundy = gy;
    }
}

/* UpdateSoundLoc (WL_GAME.C), once a frame: keep a placed sound where it was */
static void HookSoundUpdate(void)
{
    if (Bridge_SoundPositioned()) {
        SetSoundLoc(globalsoundx, globalsoundy);
        Bridge_SetSoundPosition(leftchannel, rightchannel);
    }
}

static int HookSoundPlaying(void)
{
    return Bridge_SoundPlaying();
}

static void HookWaitSoundDone(void)
{
    if (!g_fast) {
        Bridge_WaitSoundDone();
    }
}

static void HookPlayMusic(int song)
{
    Bridge_StartMusic(song);
}

static void HookStopMusic(void)
{
    Bridge_MusicOff();
}

/* ------------------------------------------------- screens (until P5 does them) */

static void Message(const char *line1, const char *line2)
{
    byte *fb = Bridge_Framebuffer();
    int y;

    for (y = 60; y < 100; y++) {
        memset(&fb[y * SCREENWIDTH + 40], 0x29, 240); /* window, menu border colour */
    }
    Bridge_DrawString(52, 68, line1, 15);
    if (line2) {
        Bridge_DrawString(52, 82, line2, 15);
    }
    Bridge_Present();
}

static void HookGetPsyched(void)
{
    byte *fb = Bridge_Framebuffer();

    memset(fb, 127, (size_t)SCREENWIDTH * (SCREENHEIGHT - STATUSLINES));
    Bridge_DrawPic((20 - 14) * 8, 80 - 3 * 8, Bridge_Pic(BRIDGE_GETPSYCHEDPIC));
    Bridge_Present();
    if (!g_fast) {
        UserInput(70);
    }
}

static void HookDiedFade(void)
{
    const int x0 = screenofs % SCREENWIDTH, y0 = screenofs / SCREENWIDTH;
    int y;

    for (y = 0; y < viewheight; y++) {
        memset(&screenbuf[(y0 + y) * SCREENWIDTH + x0], 4, (size_t)viewwidth);
    }
    FizzleFade(70);
    if (!g_fast) {
        UserInput(100);
    }
}

static void HookDeathCamPause(void)
{
    if (!g_fast) {
        UserInput(100 * TICRATE / 70); /* VW_WaitVBL (100) */
    }
}

static void HookDeathCamIntro(void)
{
    memset(screenbuf, 127, (size_t)SCREENWIDTH * (SCREENHEIGHT - STATUSLINES));
    FizzleFade(70);
    Message("Let's see that again!", NULL);
    if (!g_fast) {
        UserInput(300);
    }
}

static void HookDeathCamOutro(void)
{
    DrawPlayBorder();
}

static void HookLevelCompleted(void)
{
    Message("Floor completed!", "(intermission: P5)");
    if (!g_fast) {
        UserInput(140);
    }
}

static void HookGameOver(void)
{
    Message("Game over", NULL);
    if (!g_fast) {
        UserInput(210);
    }
}

static void HookVictory(void)
{
    Message("You win!", "(victory screen: P5)");
    if (!g_fast) {
        UserInput(280);
    }
}

static void HookCheatMessage(void)
{
    Message("You now have 100% Health,", "99 Ammo and both Keys!");
    if (!g_fast) {
        UserInput(140);
    }
}

/* ------------------------------------------------------- saving (F8, F9) */

/*
 * The original's quick save (F8) and quick load (F9) use the slot last
 * chosen in the save menu; the menus are P5, so they use slot 0 until then.
 */
#define QUICKSLOT 0

static byte g_savebuf[64 * 1024];

static const char *Pack(void)
{
    return Bridge_IsShareware() ? "WL1" : "WL6";
}

static void QuickSave(void)
{
    char desc[WOLF_SAVE_DESC];
    const size_t len = SaveTheGame(g_savebuf, sizeof(g_savebuf));

    snprintf(desc, sizeof(desc), "Episode %d, floor %d", gamestate.episode + 1,
             gamestate.mapon + 1);
    if (len && len <= sizeof(g_savebuf) &&
        Wolf_WriteSave(QUICKSLOT, Pack(), desc, g_savebuf, len)) {
        Message("Game saved", desc);
    } else {
        Message("Could not save the game", NULL);
    }
    if (!g_fast) {
        UserInput(70);
    }
}

/* True when the saved game is loaded: GameLoop then restarts on it (loadedgame). */
static boolean LoadSlot(int slot)
{
    size_t len = 0;
    const wolf_save_result_t result =
        Wolf_ReadSave(slot, Pack(), g_savebuf, sizeof(g_savebuf), &len, NULL);

    if (result == WOLF_SAVE_OK && LoadTheGame(g_savebuf, len)) {
        return true;
    }
    if (result == WOLF_SAVE_OK && loadedgame) {
        /* the stream was fine but its level would not load: the game is gone */
        loadedgame = false;
        abortgame = true;
        playstate = ex_abort;
    }
    Message(result == WOLF_SAVE_OK ? Wolf_SaveResultText(WOLF_SAVE_DAMAGED)
                                   : Wolf_SaveResultText(result),
            NULL);
    if (!g_fast) {
        UserInput(105);
    }
    return false;
}

/* Pause and Esc. The control panel (F1-F10, Esc) is P5; Esc quits until then. */
static void HookCheckKeys(byte scan)
{
    if (g_paused) {
        Bridge_DrawPic((20 - 4) * 8, 80 - 2 * 8, Bridge_Pic(BRIDGE_PAUSEDPIC));
        Bridge_Present();
        orig_lastscan = 0;
        while (!g_quit && !orig_lastscan) {
            PumpEvents();
            SDL_Delay(5);
        }
        g_paused = false;
        g_mousedx = g_mousedy = 0;
        orig_lastscan = 0;
        return;
    }
    if (scan == 0x42) { /* sc_F8 */
        orig_lastscan = 0;
        QuickSave();
        return;
    }
    if (scan == 0x43) { /* sc_F9 */
        orig_lastscan = 0;
        if (LoadSlot(QUICKSLOT)) {
            playstate = ex_abort; /* as CheckKeys after a load from the control panel */
        }
        return;
    }
    if (scan == 0x01 || g_quit) { /* sc_Escape */
        abortgame = true;
        playstate = ex_abort;
    }
}

/* ------------------------------------------------------------------- run */

static const byte *PageForEngine(int page)
{
    return Bridge_Page(page);
}

static void InstallHooks(void)
{
    OrigHooks_Reset();

    orig_hooks.play_sound = HookPlaySound;
    orig_hooks.play_sound_loc = HookPlaySoundLoc;
    orig_hooks.sound_playing = HookSoundPlaying;
    orig_hooks.sound_update = HookSoundUpdate;
    orig_hooks.wait_sound_done = HookWaitSoundDone;
    orig_hooks.play_music = HookPlayMusic;
    orig_hooks.stop_music = HookStopMusic;

    orig_hooks.status_bar = HookStatusBar;
    orig_hooks.draw_health = HookDrawHealth;
    orig_hooks.draw_face = HookDrawFace;
    orig_hooks.draw_score = HookDrawScore;
    orig_hooks.draw_lives = HookDrawLives;
    orig_hooks.draw_level = HookDrawLevel;
    orig_hooks.draw_weapon = HookDrawWeapon;
    orig_hooks.draw_keys = HookDrawKeys;
    orig_hooks.draw_ammo = HookDrawAmmo;
    orig_hooks.draw_got_gatling = HookDrawGotGatling;

    orig_hooks.set_palette_shift = HookPaletteShift;
    orig_hooks.present_frame = Present;
    orig_hooks.fizzle_in = HookFizzleIn;

    orig_hooks.death_cam_pause = HookDeathCamPause;
    orig_hooks.death_cam_intro = HookDeathCamIntro;
    orig_hooks.death_cam_outro = HookDeathCamOutro;
    orig_hooks.get_psyched = HookGetPsyched;
    orig_hooks.died_fade = HookDiedFade;
    orig_hooks.level_completed = HookLevelCompleted;
    orig_hooks.game_over = HookGameOver;
    orig_hooks.victory = HookVictory;
    orig_hooks.cheat_message = HookCheatMessage;

    orig_hooks.poll_input = PollInput;
    orig_hooks.mouse_buttons = MouseButtons;
    orig_hooks.mouse_delta = MouseDelta;
    orig_hooks.check_keys = HookCheckKeys;
    orig_hooks.check_ack = CheckAck;

    if (!g_fast) {
        orig_hooks.read_time = ReadClock;
        orig_hooks.write_time = WriteClock;
        orig_hooks.wait_time = WaitClock;
    }
}

int OrigRunner_Run(const orig_run_options_t *opts)
{
    g_opts = opts;
    g_fast = opts->fast != 0;
    g_quit = false;
    g_frames = 0;
    g_shotsaved = false;

    if (opts->demo < 0 && opts->load < 0 && !Bridge_MapExists(opts->map)) {
        fprintf(stderr, "Map %d is not in this pack\n", opts->map);
        return 34;
    }
    if (!Bridge_OpenWindow("Wolfenstein 3-D (faithful engine)")) {
        fprintf(stderr, "Could not open the window\n");
        return 30;
    }
    if (!g_fast) {
        SDL_SetRelativeMouseMode(Bridge_MouseLook() ? SDL_TRUE : SDL_FALSE);
    }

    TicClock_Init(&g_clock, SDL_GetPerformanceFrequency(), SDL_GetPerformanceCounter());
    InitGame();
    NewViewSize(opts->viewsize);
    orig_getpage = PageForEngine;
    orig_spritestart = (int16_t)Bridge_SpriteStart();
    upload = Bridge_IsShareware();
    mouseenabled = Bridge_MouseLook() != 0;
    InitRedShifts();
    SetPalette(Bridge_GamePalette());
    InstallHooks();
    memset(orig_keys, 0, sizeof(orig_keys));
    abortgame = false;

    if (opts->demo >= 0) {
        size_t length = 0;
        const byte *lump = Bridge_DemoLump(opts->demo, &length);

        if (!lump) {
            fprintf(stderr, "Demo %d not found in this pack\n", opts->demo);
            Bridge_CloseWindow();
            return 31;
        }
        if (!PlayDemo(lump, length)) {
            fprintf(stderr, "Demo %d could not be played\n", opts->demo);
            Bridge_CloseWindow();
            return 32;
        }
        printf("orig demo %d: %d frames, playstate %d, health %d, kills %d\n", opts->demo,
               g_frames, playstate, gamestate.health, gamestate.killcount);
    } else {
        if (opts->load >= 0) {
            if (!LoadSlot(opts->load)) {
                Bridge_CloseWindow();
                return 35;
            }
        } else {
            NewGame(opts->skill, opts->map / 10);
            gamestate.mapon = (int16_t)(opts->map % 10);
        }
        GameLoop();
        printf("orig game over: map %d, score %ld, lives %d\n",
               gamestate.episode * 10 + gamestate.mapon, (long)gamestate.score, gamestate.lives);
    }

    SDL_SetRelativeMouseMode(SDL_FALSE);
    Bridge_MusicOff();
    Bridge_CloseWindow();
    if (opts->shotpath && !g_shotsaved) {
        fprintf(stderr, "No screenshot saved (the run ended before frame %d?)\n", opts->shotframe);
        return 33;
    }
    return 0;
}

boolean OrigRunner_SmokeVerify(void)
{
    static byte first[64 * 1024], again[64 * 1024];
    orig_run_options_t opts;
    size_t n1, n2;
    boolean ok = true;

    /* demo 0 with everything wired up, unclocked, for 300 frames */
    OrigRunner_Defaults(&opts);
    opts.demo = 0;
    opts.fast = 1;
    opts.maxframes = 300;
    if (OrigRunner_Run(&opts) != 0 || g_frames != opts.maxframes) {
        fprintf(stderr, "Engine smoke: demo 0 did not run %d frames\n", opts.maxframes);
        return false;
    }
    printf("Engine smoke: demo 0 on map %d, %d frames, %d actors, %d doors\n",
           gamestate.mapon + 10 * gamestate.episode, g_frames, objcount, doornum);

    /* save format v2: the state at frame 300 survives a load byte for byte */
    n1 = SaveTheGame(first, sizeof(first));
    if (!n1 || n1 > sizeof(first) || !LoadTheGame(first, n1)) {
        fprintf(stderr, "Engine smoke: save/load failed\n");
        ok = false;
    } else {
        n2 = SaveTheGame(again, sizeof(again));
        if (n2 != n1 || memcmp(first, again, n1) != 0) {
            fprintf(stderr, "Engine smoke: save changed after a load\n");
            ok = false;
        } else {
            printf("Engine smoke: save v2 roundtrip OK (%u bytes)\n", (unsigned)n1);
        }
    }
    loadedgame = false;
    return ok;
}

boolean OrigRunner_QuitRequested(void)
{
    return g_quit;
}
