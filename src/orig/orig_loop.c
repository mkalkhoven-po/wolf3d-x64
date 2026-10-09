/*
 * The play loop (WL_PLAY.C PollControls, CheckKeys, palette shifts, music,
 * PlayLoop; WL_DRAW.C CalcTics; WL_GAME.C PlayDemo).
 *
 * The original waited on the timer interrupt, read the keyboard interrupt's
 * key table and the mouse driver, and wrote the VGA palette. Those are the
 * platform layer's: the clock goes through orig_hooks read_time / write_time
 * / wait_time, input arrives in orig_keys / orig_lastscan and the mouse and
 * joystick hooks, and the screen work (palette, present, menus) through the
 * other hooks. Everything that decides game state stays here.
 */
#include "orig.h"

#include <string.h>

#include "id_us.h"

/* ------------------------------------------------------------ the clock */

/* TimeCount as the timer interrupt has it now */
static int32_t ReadTimeCount(void)
{
    if (orig_hooks.read_time) {
        TimeCount = orig_hooks.read_time();
    }
    return TimeCount;
}

/* the game setting TimeCount: the platform clock follows */
void SetTimeCount(int32_t count)
{
    TimeCount = count;
    if (orig_hooks.write_time) {
        orig_hooks.write_time(count);
    }
}

/*
 * The original's busy loop "while (TimeCount < target);". Without a clock
 * (headless tests) the time simply jumps to target.
 */
static void WaitTimeCount(int32_t target)
{
    if (orig_hooks.wait_time) {
        orig_hooks.wait_time(target);
        ReadTimeCount();
    }
    if (TimeCount < target) {
        TimeCount = target;
    }
}

/*
=====================
=
= CalcTics
=
= Without a clock (headless tests), a call that finds no tic passed
= advances the time by one, standing in for the timer interrupt.
=
=====================
*/

void CalcTics(void)
{
    int32_t newtime;

    /* calculate tics since last refresh for adaptive timing */
    if (lasttimecount > ReadTimeCount()) {
        SetTimeCount(lasttimecount); /* if the game was paused a LONG time */
    }

    do {
        newtime = ReadTimeCount();
        if (newtime == lasttimecount && !orig_hooks.read_time) {
            newtime = ++TimeCount;
        }
        tics = (uint16_t)(newtime - lasttimecount);
    } while (!tics); /* make sure at least one tic passes */

    lasttimecount = newtime;

    if (tics > MAXTICS) {
        SetTimeCount(TimeCount - (tics - MAXTICS));
        tics = MAXTICS;
    }
}

/* ------------------------------------------------------------- controls */

#define BASEMOVE 35
#define RUNMOVE  70

#define JOYSCALE 2

byte orig_keys[NUMCODES]; /* the keyboard interrupt's Keyboard[] */
byte orig_lastscan;        /* LastScan: the last key pressed */

boolean mouseenabled, joystickenabled, joypadenabled, joystickprogressive;
int16_t joystickport;
int16_t mouseadjustment = 5;

int16_t dirscan[4] = {sc_UpArrow, sc_RightArrow, sc_DownArrow, sc_LeftArrow};
int16_t buttonscan[NUMBUTTONS] = {sc_Control, sc_Alt, sc_RShift, sc_Space,
                                  sc_1,       sc_2,   sc_3,      sc_4};
int16_t buttonmouse[4] = {bt_attack, bt_strafe, bt_use, bt_nobutton};
int16_t buttonjoy[4] = {bt_attack, bt_strafe, bt_use, bt_run};

/* demo buffer: the command stream PollControls reads or writes */
const byte *demoptr;
const byte *lastdemoptr;
static byte *demorecordptr, *demorecordend;

static boolean KeyDown(int scan)
{
    return scan >= 0 && scan < NUMCODES && orig_keys[scan];
}

/* buttonmouse / buttonjoy hold bt_nobutton (-1) for an unused button */
static void PressButton(int button)
{
    if (button >= 0 && button < NUMBUTTONS) {
        buttonstate[button] = true;
    }
}

/*
===================
=
= PollKeyboardButtons
=
===================
*/

static void PollKeyboardButtons(void)
{
    int i;

    for (i = 0; i < NUMBUTTONS; i++) {
        if (KeyDown(buttonscan[i])) {
            buttonstate[i] = true;
        }
    }
}

/*
===================
=
= PollMouseButtons
=
===================
*/

static void PollMouseButtons(void)
{
    const int buttons = orig_hooks.mouse_buttons ? orig_hooks.mouse_buttons() : 0;

    if (buttons & 1) {
        PressButton(buttonmouse[0]);
    }
    if (buttons & 2) {
        PressButton(buttonmouse[1]);
    }
    if (buttons & 4) {
        PressButton(buttonmouse[2]);
    }
}

/*
===================
=
= PollJoystickButtons
=
===================
*/

static void PollJoystickButtons(void)
{
    const int buttons = orig_hooks.joy_buttons ? orig_hooks.joy_buttons() : 0;

    if (joystickport && !joypadenabled) {
        if (buttons & 4) {
            PressButton(buttonjoy[0]);
        }
        if (buttons & 8) {
            PressButton(buttonjoy[1]);
        }
    } else {
        if (buttons & 1) {
            PressButton(buttonjoy[0]);
        }
        if (buttons & 2) {
            PressButton(buttonjoy[1]);
        }
        if (joypadenabled) {
            if (buttons & 4) {
                PressButton(buttonjoy[2]);
            }
            if (buttons & 8) {
                PressButton(buttonjoy[3]);
            }
        }
    }
}

/*
===================
=
= PollKeyboardMove
=
===================
*/

static void PollKeyboardMove(void)
{
    const int move = buttonstate[bt_run] ? RUNMOVE : BASEMOVE;

    if (KeyDown(dirscan[di_north])) {
        controly = (int16_t)(controly - move * tics);
    }
    if (KeyDown(dirscan[di_south])) {
        controly = (int16_t)(controly + move * tics);
    }
    if (KeyDown(dirscan[di_west])) {
        controlx = (int16_t)(controlx - move * tics);
    }
    if (KeyDown(dirscan[di_east])) {
        controlx = (int16_t)(controlx + move * tics);
    }
}

/*
===================
=
= PollMouseMove
=
===================
*/

static void PollMouseMove(void)
{
    int mousexmove = 0, mouseymove = 0;

    if (orig_hooks.mouse_delta) {
        orig_hooks.mouse_delta(&mousexmove, &mouseymove);
    }

    controlx = (int16_t)(controlx + mousexmove * 10 / (13 - mouseadjustment));
    controly = (int16_t)(controly + mouseymove * 20 / (13 - mouseadjustment));
}

/*
===================
=
= PollJoystickMove
=
= The progressive branch adds the y deflection to controlx when pushed
= down, as in the original.
=
===================
*/

static void PollJoystickMove(void)
{
    int joyx = 0, joyy = 0;

    if (orig_hooks.joy_delta) {
        orig_hooks.joy_delta(&joyx, &joyy);
    }

    if (joystickprogressive) {
        if (joyx > 64) {
            controlx = (int16_t)(controlx + (joyx - 64) * JOYSCALE * tics);
        } else if (joyx < -64) {
            controlx = (int16_t)(controlx - (-joyx - 64) * JOYSCALE * tics);
        }
        if (joyy > 64) {
            controlx = (int16_t)(controlx + (joyy - 64) * JOYSCALE * tics);
        } else if (joyy < -64) {
            controly = (int16_t)(controly - (-joyy - 64) * JOYSCALE * tics);
        }
    } else {
        const int move = buttonstate[bt_run] ? RUNMOVE : BASEMOVE;

        if (joyx > 64) {
            controlx = (int16_t)(controlx + move * tics);
        } else if (joyx < -64) {
            controlx = (int16_t)(controlx - move * tics);
        }
        if (joyy > 64) {
            controly = (int16_t)(controly + move * tics);
        } else if (joyy < -64) {
            controly = (int16_t)(controly - move * tics);
        }
    }
}

/*
===================
=
= PollControls
=
= Gets user or demo input, call once each frame
=
= controlx      set between -100 and 100 per tic
= controly
= buttonheld[]  the state of the buttons LAST frame
= buttonstate[] the state of the buttons THIS frame
=
===================
*/

void PollControls(void)
{
    int max, min, i;
    byte buttonbits;

    /* get timing info for last frame */
    if (demoplayback || demorecord) { /* demo recording and playback needs to be constant */
        WaitTimeCount(lasttimecount + DEMOTICS);
        SetTimeCount(lasttimecount + DEMOTICS);
        lasttimecount += DEMOTICS;
        tics = DEMOTICS;
    } else {
        CalcTics();
    }

    controlx = 0;
    controly = 0;
    memcpy(buttonheld, buttonstate, sizeof(buttonstate));
    memset(buttonstate, 0, sizeof(buttonstate));

    if (demoplayback) {
        /* read commands from demo buffer */
        buttonbits = *demoptr++;
        for (i = 0; i < NUMBUTTONS; i++) {
            buttonstate[i] = buttonbits & 1;
            buttonbits >>= 1;
        }

        controlx = (int8_t)*demoptr++; /* the demo buffer was signed char */
        controly = (int8_t)*demoptr++;

        if (demoptr >= lastdemoptr) { /* == in the original */
            playstate = ex_completed; /* demo is done */
        }

        controlx = (int16_t)(controlx * (int)tics);
        controly = (int16_t)(controly * (int)tics);

        return;
    }

    if (orig_hooks.poll_input) {
        orig_hooks.poll_input(); /* the interrupts' key table and mouse state */
    }

    /* get button states */
    PollKeyboardButtons();

    if (mouseenabled) {
        PollMouseButtons();
    }

    if (joystickenabled) {
        PollJoystickButtons();
    }

    /* get movements */
    PollKeyboardMove();

    if (mouseenabled) {
        PollMouseMove();
    }

    if (joystickenabled) {
        PollJoystickMove();
    }

    /* bound movement to a maximum */
    max = 100 * tics;
    min = -max;
    if (controlx > max) {
        controlx = (int16_t)max;
    } else if (controlx < min) {
        controlx = (int16_t)min;
    }

    if (controly > max) {
        controly = (int16_t)max;
    } else if (controly < min) {
        controly = (int16_t)min;
    }

    if (demorecord) {
        /* save info out to demo buffer */
        controlx = (int16_t)(controlx / (int)tics);
        controly = (int16_t)(controly / (int)tics);

        buttonbits = 0;

        for (i = NUMBUTTONS - 1; i >= 0; i--) {
            buttonbits <<= 1;
            if (buttonstate[i]) {
                buttonbits |= 1;
            }
        }

        if (!demorecordptr || demorecordptr + 3 > demorecordend) {
            Quit("Demo buffer overflowed!");
        }
        *demorecordptr++ = buttonbits;
        *demorecordptr++ = (byte)(int8_t)controlx;
        *demorecordptr++ = (byte)(int8_t)controly;

        controlx = (int16_t)(controlx * (int)tics);
        controly = (int16_t)(controly * (int)tics);
    }
}

/* Record into buf (len bytes) from the next PollControls on. */
void StartDemoRecording(byte *buf, size_t len)
{
    demorecordptr = buf;
    demorecordend = buf + len;
}

byte *DemoRecordingEnd(void)
{
    return demorecordptr;
}

/*
=====================
=
= CheckKeys
=
= The game-state part: the MLI cheat. Pause, the control panel, the other
= cheats, their messages and the debug keys are the platform layer's
= (check_keys hook); it may end the level by setting playstate.
=
=====================
*/

boolean screenfaded;

void CheckKeys(void)
{
    if (screenfaded || demoplayback) { /* don't do anything with a faded screen */
        return;
    }

    /* SECRET CHEAT CODE: MLI */
    if (KeyDown(sc_M) && KeyDown(sc_L) && KeyDown(sc_I)) {
        gamestate.health = 100;
        gamestate.ammo = 99;
        gamestate.keys = 3;
        gamestate.score = 0;
        gamestate.TimeCount += 42000L;
        GiveWeapon(wp_chaingun);

        DrawWeapon();
        DrawHealth();
        DrawKeys();
        DrawAmmo();
        DrawScore();

        if (orig_hooks.cheat_message) {
            orig_hooks.cheat_message(); /* "You now have 100% Health..." */
        }
        memset(orig_keys, 0, sizeof(orig_keys)); /* IN_ClearKeysDown */
    }

    if (orig_hooks.check_keys) {
        orig_hooks.check_keys(orig_lastscan);
    }
}

/* ---------------------------------------------------------------- music */

static const byte songs[] = {
    /* Episode One */
    GETTHEM_MUS,
    SEARCHN_MUS,
    POW_MUS,
    SUSPENSE_MUS,
    GETTHEM_MUS,
    SEARCHN_MUS,
    POW_MUS,
    SUSPENSE_MUS,
    WARMARCH_MUS, /* Boss level */
    CORNER_MUS, /* Secret level */
    /* Episode Two */
    NAZI_OMI_MUS,
    PREGNANT_MUS,
    GOINGAFT_MUS,
    HEADACHE_MUS,
    NAZI_OMI_MUS,
    PREGNANT_MUS,
    HEADACHE_MUS,
    GOINGAFT_MUS,
    WARMARCH_MUS, /* Boss level */
    DUNGEON_MUS, /* Secret level */
    /* Episode Three */
    INTROCW3_MUS,
    NAZI_RAP_MUS,
    TWELFTH_MUS,
    ZEROHOUR_MUS,
    INTROCW3_MUS,
    NAZI_RAP_MUS,
    TWELFTH_MUS,
    ZEROHOUR_MUS,
    ULTIMATE_MUS, /* Boss level */
    PACMAN_MUS, /* Secret level */
    /* Episode Four */
    GETTHEM_MUS,
    SEARCHN_MUS,
    POW_MUS,
    SUSPENSE_MUS,
    GETTHEM_MUS,
    SEARCHN_MUS,
    POW_MUS,
    SUSPENSE_MUS,
    WARMARCH_MUS, /* Boss level */
    CORNER_MUS, /* Secret level */
    /* Episode Five */
    NAZI_OMI_MUS,
    PREGNANT_MUS,
    GOINGAFT_MUS,
    HEADACHE_MUS,
    NAZI_OMI_MUS,
    PREGNANT_MUS,
    HEADACHE_MUS,
    GOINGAFT_MUS,
    WARMARCH_MUS, /* Boss level */
    DUNGEON_MUS, /* Secret level */
    /* Episode Six */
    INTROCW3_MUS,
    NAZI_RAP_MUS,
    TWELFTH_MUS,
    ZEROHOUR_MUS,
    INTROCW3_MUS,
    NAZI_RAP_MUS,
    TWELFTH_MUS,
    ZEROHOUR_MUS,
    ULTIMATE_MUS, /* Boss level */
    FUNKYOU_MUS  /* Secret level */
};

/*
=================
=
= StopMusic / StartMusic
=
=================
*/

void StopMusic(void)
{
    if (orig_hooks.stop_music) {
        orig_hooks.stop_music();
    }
}

void StartMusic(void)
{
    int mapindex = gamestate.mapon + gamestate.episode * 10;

    if (mapindex < 0 || mapindex >= (int)sizeof(songs)) {
        mapindex = 0; /* not in the original: never index past the table */
    }
    if (orig_hooks.play_music) {
        orig_hooks.play_music(songs[mapindex]);
    }
}

/*
=============================================================================

                    PALETTE SHIFTING STUFF

The shift tables are palettes, so the platform layer builds them; the
engine counts the flashes down and asks for red shift 1-6, white shift 1-3,
or 0 / 0 for the normal palette.

=============================================================================
*/

#define NUMREDSHIFTS   6
#define NUMWHITESHIFTS 3
#define WHITETICS      6

int16_t damagecount, bonuscount;
boolean palshifted;

static void SetPaletteShift(int red, int white)
{
    if (orig_hooks.set_palette_shift) {
        orig_hooks.set_palette_shift(red, white); /* after VW_WaitVBL(1) */
    }
}

/*
=====================
=
= ClearPaletteShifts
=
=====================
*/

void ClearPaletteShifts(void)
{
    bonuscount = damagecount = 0;
}

/*
=====================
=
= StartBonusFlash
=
=====================
*/

void StartBonusFlash(void)
{
    bonuscount = NUMWHITESHIFTS * WHITETICS; /* white shift palette */
    if (orig_hooks.start_bonus_flash) {
        orig_hooks.start_bonus_flash();
    }
}

/*
=====================
=
= StartDamageFlash
=
=====================
*/

void StartDamageFlash(int damage)
{
    damagecount = (int16_t)(damagecount + damage);
    if (orig_hooks.start_damage_flash) {
        orig_hooks.start_damage_flash(damage);
    }
}

/*
=====================
=
= UpdatePaletteShifts
=
=====================
*/

void UpdatePaletteShifts(void)
{
    int red, white;

    if (bonuscount) {
        white = bonuscount / WHITETICS + 1;
        if (white > NUMWHITESHIFTS) {
            white = NUMWHITESHIFTS;
        }
        bonuscount = (int16_t)(bonuscount - tics);
        if (bonuscount < 0) {
            bonuscount = 0;
        }
    } else {
        white = 0;
    }

    if (damagecount) {
        red = damagecount / 10 + 1;
        if (red > NUMREDSHIFTS) {
            red = NUMREDSHIFTS;
        }

        damagecount = (int16_t)(damagecount - tics);
        if (damagecount < 0) {
            damagecount = 0;
        }
    } else {
        red = 0;
    }

    if (red) {
        SetPaletteShift(red, 0);
        palshifted = true;
    } else if (white) {
        SetPaletteShift(0, white);
        palshifted = true;
    } else if (palshifted) {
        SetPaletteShift(0, 0); /* back to normal */
        palshifted = false;
    }
}

/*
=====================
=
= FinishPaletteShifts
=
= Resets palette to normal if needed
=
=====================
*/

void FinishPaletteShifts(void)
{
    if (palshifted) {
        palshifted = 0;
        SetPaletteShift(0, 0);
    }
}

/*
=============================================================================

                            CORE PLAYLOOP

=============================================================================
*/

boolean startgame;

/*
===================
=
= PlayLoop
=
===================
*/

void PlayLoop(void)
{
    playstate = ex_stillplaying;
    SetTimeCount(0);
    lasttimecount = 0;
    frameon = 0;
    running = false;
    anglefrac = 0;
    facecount = 0;
    memset(buttonstate, 0, sizeof(buttonstate));
    ClearPaletteShifts();

    do {
        PollControls();

        /* actor thinking */
        madenoise = false;

        MoveDoors();
        MovePWalls();

        for (obj = player; obj; obj = obj->next) {
            DoActor(obj);
        }

        UpdatePaletteShifts();

        ThreeDRefresh();
        if (orig_hooks.present_frame) {
            orig_hooks.present_frame(); /* the page flip in ThreeDRefresh */
        }

        gamestate.TimeCount += tics;

        if (orig_hooks.sound_update) {
            orig_hooks.sound_update(); /* SD_Poll, UpdateSoundLoc */
        }

        CheckKeys();

        /* a key press ends demo playback */
        if (demoplayback && orig_hooks.check_ack && orig_hooks.check_ack()) {
            playstate = ex_abort;
        }

    } while (!playstate && !startgame);

    if (playstate != ex_died) {
        FinishPaletteShifts();
    }
}

/*
=================
=
= PlayDemo (WL_GAME.C)
=
= The game part: the demo lump is a map byte, a little-endian length that
= counts the 4-byte header, a pad byte and three bytes per frame (buttons,
= signed controlx, signed controly). Demos always play on hard. Returns
= false if the lump is unusable.
=
=================
*/

boolean PlayDemo(const byte *demo, size_t demosize)
{
    unsigned length;

    if (!demo || demosize < 4) {
        return false;
    }

    NewGame(1, 0);
    gamestate.mapon = demo[0];
    gamestate.difficulty = gd_hard;
    length = (unsigned)demo[1] | ((unsigned)demo[2] << 8);
    if (length < 4 || length > demosize || (length - 4) % 3) {
        return false; /* not in the original: a bad length ran off the buffer */
    }
    demoptr = demo + 4;
    lastdemoptr = demo + length;

    DrawPlayScreen(); /* after NewGame, so the status bar shows the new game */

    startgame = false;
    demoplayback = true;

    if (!SetupGameLevel()) {
        demoplayback = false;
        return false;
    }
    StartMusic();
    fizzlein = true;

    PlayLoop();

    demoplayback = false;

    StopMusic();
    return true;
}
