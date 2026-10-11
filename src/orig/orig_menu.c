/*
 * WL_MENU.C: the control panel (P5b). The main menu, New Game, Sound,
 * Load / Save Game, Change View, Control (mouse, joystick, Customize),
 * Read This! (orig_text.c), View Scores, Quit and the quick keys F7-F10,
 * drawn into screenbuf as the original drew them. Not in the original:
 * Sound opens a page with the sound screen and the music and effects
 * volume, Change View became Video (the view size, fill the screen,
 * fullscreen), and the Control menu's last line sets the pad's look speed.
 *
 * Hardware the menus read directly in the original comes through hooks:
 * the keyboard table (orig_keys, as KbdDefs reads it), the mouse driver,
 * the joystick, the clock, sound modes and the save files. Not in the
 * original: a game pad (pad_menu) works the menus like a joystick, and
 * orig_quit (the window closed) backs out of every menu.
 */

#include <stdio.h>
#include <string.h>

#include "id_ca.h"
#include "id_us_rnd.h"
#include "orig.h"

/* WL_MENU.H */
#define BORDCOLOR 0x29
#define BORD2COLOR 0x23
#define DEACTIVE 0x2b
#define BKGDCOLOR 0x2d
#define STRIPE 0x2c

#define READCOLOR 0x4a
#define READHCOLOR 0x47
#define VIEWCOLOR 0x7f
#define TEXTCOLOR 0x17
#define HIGHLIGHT 0x13

#define MenuFadeOut() VL_FadeOut(0, 255, 43, 0, 0, 10)
#define MenuFadeIn() VL_FadeIn(0, 255, gamepal, 10)

#define SENSITIVE 60
#define CENTER (SENSITIVE * 2)

#define MENU_X 76
#define MENU_Y 55
#define MENU_W 178
#define MENU_H (13 * 10 + 6)

#define SM_X 48
#define SM_W 250

#define SM_Y1 20
#define SM_H1 (4 * 13 - 7)
#define SM_Y2 (SM_Y1 + 5 * 13)
#define SM_H2 (4 * 13 - 7)
#define SM_Y3 (SM_Y2 + 5 * 13)
#define SM_H3 (3 * 13 - 7)

#define LSM_X 85
#define LSM_Y 55
#define LSM_W 175
#define LSM_H (10 * 13 + 10)

#define NM_X 50
#define NM_Y 100
#define NM_W 225
#define NM_H (13 * 4 + 15)

#define NE_X 10
#define NE_Y 23
#define NE_W (320 - NE_X * 2)
#define NE_H (200 - NE_Y * 2)

#define LSA_X 96
#define LSA_Y 80
#define LSA_W 130
#define LSA_H 42

/* FOREIGN.H */
#define CURGAME "You are currently in\n" \
                "a game. Continuing will\n" \
                "erase old game. Ok?"
#define GAMESVD "There's already a game\n" \
                "saved at this position.\n" \
                "      Overwrite?"
#define ENDGAMESTR "Are you sure you want\n" \
                   "to end the game you\n" \
                   "are playing? (Y or N):"
#define STR_NG "New Game"
#define STR_SD "Sound"
#define STR_CL "Control"
#define STR_LG "Load Game"
#define STR_SG "Save Game"
#define STR_VIDEO "Video" /* not in the original: Change View, fill and fullscreen */
#define STR_EG "End Game"
#define STR_BD "Back to Demo"
#define STR_QT "Quit"
#define STR_LOADING "Loading"
#define STR_SAVING "Saving"
#define STR_GAME "Game"
#define STR_DEMO "Demo"
#define STR_LGC "Load Game called\n\""
#define STR_EMPTY "empty"
#define STR_NONE "None"
#define STR_PC "PC Speaker"
#define STR_ALSB "AdLib/Sound Blaster"
#define STR_DISNEY "Disney Sound Source"
#define STR_SB "Sound Blaster"
#define STR_DADDY "Can I play, Daddy?"
#define STR_HURTME "Don't hurt me."
#define STR_BRINGEM "Bring 'em on!"
#define STR_DEATH "I am Death incarnate!"
#define STR_THINK "Thinking"
#define STR_SIZE1 "Use arrows to size"
#define STR_SIZE2 "ENTER to accept"
#define STR_SIZE3 "ESC to cancel"
#define STR_NOSPACE1 "There is not enough space"
#define STR_NOSPACE2 "on your disk to Save Game!"

static const char endStrings[9][80] = {
    "Dost thou wish to\nleave with such hasty\nabandon?",
    "Chickening out...\nalready?",
    "Press N for more carnage.\nPress Y to be a weenie.",
    "So, you think you can\nquit this easily, huh?",
    "Press N to save the world.\nPress Y to abandon it in\nits hour of need.",
    "Press N if you are brave.\nPress Y to cower in shame.",
    "Heroes, press N.\nWimps, press Y.",
    "You are at an intersection.\nA sign says, 'Press Y to quit.'\n>",
    "For guns and glory, press N.\nFor work and worry, press Y.",
};

static void CP_SoundPage(int temp1);
static void CP_Video(int temp1);

/* the table entries for the routines that take a quick flag */
static void MenuLoadGame(int temp1)
{
    (void)CP_LoadGame(temp1);
}

static void MenuSaveGame(int temp1)
{
    (void)CP_SaveGame(temp1);
}

CP_iteminfo MainItems = {MENU_X, MENU_Y, 10, readthis, 24};
static CP_iteminfo SndItems = {SM_X, SM_Y1, 12, 0, 52}, LSItems = {LSM_X, LSM_Y, 10, 0, 24},
                   NewEitems = {NE_X, NE_Y, 11, 0, 88}, NewItems = {NM_X, NM_Y, 4, 2, 24};

CP_itemtype MainMenu[] = {
    {1, STR_NG, CP_NewGame},
    {1, STR_SD, CP_SoundPage},
    {1, STR_CL, CP_Control},
    {1, STR_LG, MenuLoadGame},
    {0, STR_SG, MenuSaveGame},
    {1, STR_VIDEO, CP_Video},
    {2, "Read This!", CP_ReadThis},
    {1, STR_VS, CP_ViewScores},
    {1, STR_BD, NULL},
    {1, STR_QT, NULL},
};

static CP_itemtype SndMenu[] = {
    {1, STR_NONE, NULL}, {1, STR_PC, NULL},   {1, STR_ALSB, NULL},   {0, "", NULL},
    {0, "", NULL},       {1, STR_NONE, NULL}, {1, STR_DISNEY, NULL}, {1, STR_SB, NULL},
    {0, "", NULL},       {0, "", NULL},       {1, STR_NONE, NULL},   {1, STR_ALSB, NULL},
};

static CP_itemtype NewEmenu[] = {
    {1, "Episode 1\nEscape from Wolfenstein", NULL},
    {0, "", NULL},
    {3, "Episode 2\nOperation: Eisenfaust", NULL},
    {0, "", NULL},
    {3, "Episode 3\nDie, Fuhrer, Die!", NULL},
    {0, "", NULL},
    {3, "Episode 4\nA Dark Secret", NULL},
    {0, "", NULL},
    {3, "Episode 5\nTrail of the Madman", NULL},
    {0, "", NULL},
    {3, "Episode 6\nConfrontation", NULL},
};

static CP_itemtype NewMenu[] = {
    {1, STR_DADDY, NULL},
    {1, STR_HURTME, NULL},
    {1, STR_BRINGEM, NULL},
    {1, STR_DEATH, NULL},
};

static CP_itemtype LSMenu[] = {
    {1, "", NULL}, {1, "", NULL}, {1, "", NULL}, {1, "", NULL}, {1, "", NULL},
    {1, "", NULL}, {1, "", NULL}, {1, "", NULL}, {1, "", NULL}, {1, "", NULL},
};

static const int color_hlite[] = {DEACTIVE, HIGHLIGHT, READHCOLOR, 0x67};
static const int color_norml[] = {DEACTIVE, TEXTCOLOR, READCOLOR, 0x6b};

static int EpisodeSelect[6] = {1};

static int SaveGamesAvail[10], StartGame, SoundStatus = 1, pickquick;
static char SaveGameNames[10][32];

static void EraseGun(CP_iteminfo *item_i, CP_itemtype *items, int x, int y, int which);
static void DrawHalfStep(int x, int y);
static void DrawGun(CP_iteminfo *item_i, CP_itemtype *items, int x, int *y, int which,
                    int basey, void (*routine)(int w));
static void SetTextColor(CP_itemtype *items, int hlight);
static void DrawNewEpisode(void);
static void DrawNewGame(void);
static void DrawNewGameDiff(int w);
static void DrawSoundMenu(void);
static void DrawLoadSaveScreen(int loadsave);
static void PrintLSEntry(int w, int color);
static void TrackWhichGame(int w);
static void DrawLSAction(int which);
static void DrawChangeView(int view);
static void HomeMouse(void);

/*
 * Quit (ID_US_1.C) for the menus: not in the original, the program does not
 * end here; everything backs out and the platform closes the window.
 */
static void Quit(void)
{
    orig_quit = true;
    abortgame = true;
    playstate = ex_abort;
}

/*
====================
=
= US_ControlPanel
=
= Wolfenstein Control Panel!  Ta Da!
=
====================
*/

void US_ControlPanel(byte scancode)
{
    int which;

    if (ingame) {
        if (CP_CheckQuick(scancode)) {
            return;
        }
    }

    StartCPMusic(MENUSONG);
    SetupControlPanel();

    /* F-KEYS FROM WITHIN GAME */
    switch (scancode) {
    case sc_F1:
        HelpScreens();
        CleanupControlPanel();
        return;

    case sc_F2:
        CP_SaveGame(0);
        CleanupControlPanel();
        return;

    case sc_F3:
        CP_LoadGame(0);
        CleanupControlPanel();
        return;

    case sc_F4:
        CP_SoundPage(0);
        CleanupControlPanel();
        return;

    case sc_F5:
        CP_ChangeView(0);
        CleanupControlPanel();
        return;

    case sc_F6:
        CP_Control(0);
        CleanupControlPanel();
        return;
    }

    DrawMainMenu();
    MenuFadeIn();
    StartGame = 0;

    /* MAIN MENU LOOP */
    do {
        which = HandleMenu(&MainItems, &MainMenu[0], NULL);

        switch (which) {
        case viewscores:
            if (MainMenu[viewscores].routine == NULL) {
                if (CP_EndGame()) {
                    StartGame = 1;
                }
            }

            DrawMainMenu();
            MenuFadeIn();
            break;

        case backtodemo:
            StartGame = 1;
            if (!ingame) {
                StartCPMusic(INTROSONG);
            }
            VL_FadeOut(0, 255, 0, 0, 0, 10);
            break;

        case -1:
        case quit:
            CP_Quit();
            break;

        default:
            if (!StartGame) {
                DrawMainMenu();
                MenuFadeIn();
            }
        }

        /* "EXIT OPTIONS" OR "NEW GAME" EXITS */
    } while (!StartGame && !orig_quit);

    CleanupControlPanel();

    /* CHANGE MAINMENU ITEM */
    if (startgame || loadedgame) {
        MainMenu[viewscores].routine = NULL;
        strcpy(MainMenu[viewscores].string, STR_EG);
    }
}

/* DRAW MAIN MENU SCREEN */
void DrawMainMenu(void)
{
    ClearMScreen();

    VWB_DrawPic(112, 184, C_MOUSELBACKPIC);
    DrawStripes(10);
    VWB_DrawPic(84, 0, C_OPTIONSPIC);

    DrawWindow(MENU_X - 8, MENU_Y - 3, MENU_W, MENU_H, BKGDCOLOR);

    /* CHANGE "GAME" AND "DEMO" */
    if (ingame) {
        strcpy(&MainMenu[backtodemo].string[8], STR_GAME);
        MainMenu[backtodemo].active = 2;
    } else {
        strcpy(&MainMenu[backtodemo].string[8], STR_DEMO);
        MainMenu[backtodemo].active = 1;
    }

    DrawMenu(&MainItems, &MainMenu[0]);
    VW_UpdateScreen();
}

/*
====================
=
= IN_GetScanName (WL_MENU.C)
=
= The name of a scan code, for the Customize screen
=
====================
*/

static const char *const ScanNames[] = { /* Scan code names with single chars */
    "?", "?", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "+", "?", "?",
    "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "[", "]", "|", "?", "A", "S",
    "D", "F", "G", "H", "J", "K", "L", ";", "\"", "?", "?", "?", "Z", "X", "C", "V",
    "B", "N", "M", ",", ".", "/", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?",
    "?", "?", "?", "?", "?", "?", "?", "?", "\x0f", "?", "-", "\x15", "5", "\x11", "+", "?",
    "\x13", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?",
    "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?",
    "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?"};

static const byte ExtScanCodes[] = { /* Scan codes with >1 char names */
    1,    0xe,  0xf,  0x1d, 0x2a, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x57, 0x59, 0x46, 0x1c, 0x36, 0x37, 0x38,
    0x47, 0x49, 0x4f, 0x51, 0x52, 0x53, 0x45, 0x48, 0x50, 0x4b, 0x4d, 0x00};

static const char *const ExtScanNames[] = { /* Names corresponding to ExtScanCodes */
    "Esc",   "BkSp", "Tab", "Ctrl", "LShft", "Space", "CapsLk", "F1",  "F2",
    "F3",    "F4",   "F5",  "F6",   "F7",    "F8",    "F9",     "F10", "F11",
    "F12",   "ScrlLk", "Enter", "RShft", "PrtSc", "Alt", "Home", "PgUp", "End",
    "PgDn",  "Ins",  "Del", "NumLk", "Up",   "Down",  "Left",   "Right", ""};

static const char *IN_GetScanName(byte scan)
{
    int i;

    for (i = 0; ExtScanCodes[i]; i++) {
        if (ExtScanCodes[i] == scan) {
            return ExtScanNames[i];
        }
    }

    return scan < sizeof(ScanNames) / sizeof(ScanNames[0]) ? ScanNames[scan] : "?";
}

/* READ THIS! */
void CP_ReadThis(int temp1)
{
    (void)temp1;
    StartCPMusic(CORNER_MUS);
    HelpScreens();
    StartCPMusic(MENUSONG);
}

/*
=============================================================================

                            DEFINE CONTROLS

=============================================================================
*/

#define CTL_X 24
#define CTL_Y 70
#define CTL_W 284
#define CTL_H (13 * 8 - 7) /* the original's 13*7-7, plus a line for the controller */

#define CST_X 20
#define CST_Y 48
#define CST_START 60
#define CST_SPC 60

#define STR_MOUSEEN "Mouse Enabled"
#define STR_JOYEN "Joystick Enabled"
#define STR_PORT2 "Use joystick port 2"
#define STR_GAMEPAD "Gravis GamePad Enabled"
#define STR_SENS "Mouse Sensitivity"
#define STR_CUSTOM "Customize controls"
#define STR_PADSPEED "Controller look speed" /* not in the original */
#define STR_PADADJ "Adjust Controller Look Speed"
#define STR_MOUSEADJ "Adjust Mouse Sensitivity"
#define STR_SLOW "Slow"
#define STR_FAST "Fast"
#define STR_CRUN "Run"
#define STR_COPEN "Open"
#define STR_CFIRE "Fire"
#define STR_CSTRAFE "Strafe"
#define STR_LEFT "Left"
#define STR_RIGHT "Right"
#define STR_FRWD "Frwd"
#define STR_BKWD "Bkwrd"

static void MouseSensitivity(int temp1);
static void CustomControls(int temp1);
static void ControllerSpeed(int temp1);

static CP_iteminfo CtlItems = {CTL_X, CTL_Y, 7, -1, 56}, CusItems = {8, CST_Y + 13 * 2, 9, -1, 0};

static CP_itemtype CtlMenu[] = {
    {0, STR_MOUSEEN, NULL},
    {0, STR_JOYEN, NULL},
    {0, STR_PORT2, NULL},
    {0, STR_GAMEPAD, NULL},
    {0, STR_SENS, MouseSensitivity},
    {1, STR_CUSTOM, CustomControls},
    {1, STR_PADSPEED, ControllerSpeed},
};

static CP_itemtype CusMenu[] = {
    {1, "", NULL}, {0, "", NULL}, {0, "", NULL}, {1, "", NULL}, {0, "", NULL},
    {0, "", NULL}, {1, "", NULL}, {0, "", NULL}, {1, "", NULL},
};

/* the machine: a mouse when the platform reads one; no DOS joystick (a pad is read apart) */
static boolean MousePresent(void)
{
    return orig_hooks.mouse_delta != NULL;
}

static const boolean JoysPresent[2] = {false, false};

static void DrawCtlScreen(void);

void CP_Control(int temp1)
{
    enum { MOUSEENABLE, JOYENABLE, USEPORT2, PADENABLE, MOUSESENS, CUSTOMIZE, PADSPEED };
    int which;

    (void)temp1;
    DrawCtlScreen();
    MenuFadeIn();
    WaitKeyUp();

    do {
        which = HandleMenu(&CtlItems, &CtlMenu[0], NULL);
        switch (which) {
        case MOUSEENABLE:
            mouseenabled ^= 1;
            HomeMouse();
            DrawCtlScreen();
            CusItems.curpos = -1;
            ShootSnd();
            break;

        case JOYENABLE:
            joystickenabled ^= 1; /* CalibrateJoystick: there is no DOS joystick to calibrate */
            DrawCtlScreen();
            CusItems.curpos = -1;
            ShootSnd();
            break;

        case USEPORT2:
            joystickport ^= 1;
            DrawCtlScreen();
            ShootSnd();
            break;

        case PADENABLE:
            joypadenabled ^= 1;
            DrawCtlScreen();
            ShootSnd();
            break;

        case MOUSESENS:
        case CUSTOMIZE:
        case PADSPEED:
            DrawCtlScreen();
            MenuFadeIn();
            WaitKeyUp();
            break;
        }
        /*
         * Not in the original: a mouse or pad button still down would choose
         * the switch again at once (the keyboard's keys were cleared).
         */
        WaitKeyUp();
    } while (which >= 0 && !orig_quit);

    MenuFadeOut();
}

/* the slider of the mouse sensitivity screen */
static void DrawSliderBar(int value)
{
    VWB_Bar(60, 97, 200, 10, TEXTCOLOR);
    DrawOutline(60, 97, 200, 10, 0, HIGHLIGHT);
    DrawOutline(60 + 20 * value, 97, 20, 10, 0, READCOLOR);
    VWB_Bar(61 + 20 * value, 98, 19, 9, READHCOLOR);
}

/* DRAW MOUSE SENSITIVITY SCREEN */
static void DrawSliderScreen(const char *title, int value)
{
    ClearMScreen();
    VWB_DrawPic(112, 184, C_MOUSELBACKPIC);
    DrawWindow(10, 80, 300, 30, BKGDCOLOR);

    WindowX = 0;
    WindowW = 320;
    PrintY = 82;
    SETFONTCOLOR(READCOLOR, BKGDCOLOR);
    US_CPrint(title);

    SETFONTCOLOR(TEXTCOLOR, BKGDCOLOR);
    PrintX = 14;
    PrintY = 95;
    US_Print(STR_SLOW);
    PrintX = 269;
    US_Print(STR_FAST);

    DrawSliderBar(value);

    VW_UpdateScreen();
    MenuFadeIn();
}

/* until the key is let go */
static void WaitKeyRelease(byte scan)
{
    while (orig_keys[scan] && !orig_quit) {
        IN_Pump();
    }
}

/* ADJUST MOUSE SENSITIVITY: value 0-9, apply on each step, put back on Esc */
static void SliderScreen(const char *title, int value, void (*apply)(int value))
{
    ControlInfo ci;
    int exit = 0;
    const int old = value;

    DrawSliderScreen(title, value);
    do {
        ReadAnyControl(&ci);
        switch (ci.dir) {
        case dir_North:
        case dir_West:
            if (value) {
                apply(--value);
                DrawSliderBar(value);
                VW_UpdateScreen();
                OrigSD_PlaySound(MOVEGUN1SND);
                WaitKeyRelease(sc_LeftArrow);
                WaitKeyUp();
            }
            break;

        case dir_South:
        case dir_East:
            if (value < 9) {
                apply(++value);
                DrawSliderBar(value);
                VW_UpdateScreen();
                OrigSD_PlaySound(MOVEGUN1SND);
                WaitKeyRelease(sc_RightArrow);
                WaitKeyUp();
            }
            break;

        default:
            break;
        }

        if (ci.button0 || orig_keys[sc_Space] || orig_keys[sc_Enter]) {
            exit = 1;
        } else if (ci.button1 || orig_keys[sc_Escape] || orig_quit) {
            exit = 2;
        }

    } while (!exit);

    if (exit == 2) {
        apply(old);
        OrigSD_PlaySound(ESCPRESSEDSND);
    } else {
        OrigSD_PlaySound(SHOOTSND);
    }

    WaitKeyUp();
    MenuFadeOut();
}

static void ApplyMouseSens(int value)
{
    mouseadjustment = (int16_t)value;
}

static void MouseSensitivity(int temp1)
{
    (void)temp1;
    SliderScreen(STR_MOUSEADJ, mouseadjustment, ApplyMouseSens);
}

/* the platform's settings (not in the original) */
static int GetOption(int which)
{
    return orig_hooks.get_option ? orig_hooks.get_option(which) : 0;
}

static void SetOption(int which, int value)
{
    if (orig_hooks.set_option) {
        orig_hooks.set_option(which, value);
    }
}

static void ApplyPadSpeed(int value)
{
    SetOption(OPT_PADSENS, value);
}

/* not in the original: how fast the pad's right stick turns, as Mouse Sensitivity */
static void ControllerSpeed(int temp1)
{
    (void)temp1;
    SliderScreen(STR_PADADJ, GetOption(OPT_PADSENS), ApplyPadSpeed);
}

/* DRAW CONTROL MENU SCREEN */
static void DrawCtlScreen(void)
{
    int i, x, y;

    ClearMScreen();
    DrawStripes(10);
    VWB_DrawPic(80, 0, C_CONTROLPIC);
    VWB_DrawPic(112, 184, C_MOUSELBACKPIC);
    DrawWindow(CTL_X - 8, CTL_Y - 5, CTL_W, CTL_H, BKGDCOLOR);
    WindowX = 0;
    WindowW = 320;
    SETFONTCOLOR(TEXTCOLOR, BKGDCOLOR);

    if (JoysPresent[0]) {
        CtlMenu[1].active = CtlMenu[2].active = CtlMenu[3].active = 1;
    }

    CtlMenu[2].active = CtlMenu[3].active = joystickenabled;

    if (MousePresent()) {
        CtlMenu[4].active = CtlMenu[0].active = 1;
    }

    CtlMenu[4].active = mouseenabled;

    DrawMenu(&CtlItems, &CtlMenu[0]);

    x = CTL_X + CtlItems.indent - 24;
    y = CTL_Y + 3;
    VWB_DrawPic(x, y, mouseenabled ? C_SELECTEDPIC : C_NOTSELECTEDPIC);

    y = CTL_Y + 16;
    VWB_DrawPic(x, y, joystickenabled ? C_SELECTEDPIC : C_NOTSELECTEDPIC);

    y = CTL_Y + 29;
    VWB_DrawPic(x, y, joystickport ? C_SELECTEDPIC : C_NOTSELECTEDPIC);

    y = CTL_Y + 42;
    VWB_DrawPic(x, y, joypadenabled ? C_SELECTEDPIC : C_NOTSELECTEDPIC);

    /* PICK FIRST AVAILABLE SPOT */
    if (CtlItems.curpos < 0 || !CtlMenu[CtlItems.curpos].active) {
        for (i = 0; i < CtlItems.amount; i++) {
            if (CtlMenu[i].active) {
                CtlItems.curpos = (int16_t)i;
                break;
            }
        }
    }

    DrawMenuGun(&CtlItems);
    VW_UpdateScreen();
}

/*
====================
=
= CUSTOMIZE CONTROLS
=
====================
*/

enum { FIRE, STRAFE, RUN, OPEN };
static const char mbarray[4][3] = {"b0", "b1", "b2", "b3"};
static const int order[4] = {RUN, OPEN, FIRE, STRAFE};

enum { MOUSE, JOYSTICK, KEYBOARDBTNS, KEYBOARDMOVE }; /* FOR INPUT TYPES */

typedef struct {
    int allowed[4];
} CustomCtrls;

static void DrawCustomScreen(void);
static void FixupCustom(int w);
static void DrawCustMouse(int hilight);
static void DrawCustJoy(int hilight);
static void DrawCustKeybd(int hilight);
static void DrawCustKeys(int hilight);
static void PrintCustMouse(int i);
static void PrintCustJoy(int i);
static void PrintCustKeybd(int i);
static void PrintCustKeys(int i);
static void EnterCtrlData(int index, const CustomCtrls *cust, void (*DrawRtn)(int),
                          void (*PrintRtn)(int), int type);

static void CustomControls(int temp1)
{
    static const CustomCtrls mouseallowed = {{0, 1, 1, 1}}, allowed = {{1, 1, 1, 1}};
    int which;

    (void)temp1;
    DrawCustomScreen();
    do {
        which = HandleMenu(&CusItems, &CusMenu[0], FixupCustom);
        switch (which) {
        case 0: /* DefineMouseBtns */
            EnterCtrlData(2, &mouseallowed, DrawCustMouse, PrintCustMouse, MOUSE);
            DrawCustMouse(1);
            break;
        case 3: /* DefineJoyBtns */
            EnterCtrlData(5, &allowed, DrawCustJoy, PrintCustJoy, JOYSTICK);
            DrawCustJoy(0);
            break;
        case 6: /* DefineKeyBtns */
            EnterCtrlData(8, &allowed, DrawCustKeybd, PrintCustKeybd, KEYBOARDBTNS);
            DrawCustKeybd(0);
            break;
        case 8: /* DefineKeyMove */
            EnterCtrlData(10, &allowed, DrawCustKeys, PrintCustKeys, KEYBOARDMOVE);
            DrawCustKeys(0);
            break;
        }
    } while (which >= 0 && !orig_quit);

    MenuFadeOut();
}

/* ENTER CONTROL DATA FOR ANY TYPE OF CONTROL */
enum { FWRD, RIGHT, BKWD, LEFT };
static const int moveorder[4] = {LEFT, RIGHT, FWRD, BKWD};

static void EnterCtrlData(int index, const CustomCtrls *cust, void (*DrawRtn)(int),
                          void (*PrintRtn)(int), int type)
{
    int j, exit, tick, redraw, which = 0, x = 0, picked;
    ControlInfo ci;

    ShootSnd();
    PrintY = (uint16_t)(CST_Y + 13 * index);
    IN_ClearKeysDown();
    exit = 0;
    redraw = 1;

    /* FIND FIRST SPOT IN ALLOWED ARRAY */
    for (j = 0; j < 4; j++) {
        if (cust->allowed[j]) {
            which = j;
            break;
        }
    }

    do {
        if (redraw) {
            x = CST_START + CST_SPC * which;
            DrawWindow(5, PrintY - 1, 310, 13, BKGDCOLOR);

            DrawRtn(1);
            DrawWindow(x - 2, PrintY, CST_SPC, 11, TEXTCOLOR);
            DrawOutline(x - 2, PrintY, CST_SPC, 11, 0, HIGHLIGHT);
            SETFONTCOLOR(0, TEXTCOLOR);
            PrintRtn(which);
            PrintX = (uint16_t)x;
            SETFONTCOLOR(TEXTCOLOR, BKGDCOLOR);
            VW_UpdateScreen();
            WaitKeyUp();
            redraw = 0;
        }

        ReadAnyControl(&ci);

        if (type == MOUSE || type == JOYSTICK) {
            if (orig_keys[sc_Enter] || orig_keys[sc_Control] || orig_keys[sc_Alt]) {
                IN_ClearKeysDown();
                ci.button0 = ci.button1 = false;
            }
        }

        /* CHANGE BUTTON VALUE? */
        if ((ci.button0 | ci.button1 | ci.button2 | ci.button3) ||
            ((type == KEYBOARDBTNS || type == KEYBOARDMOVE) && orig_lastscan == sc_Enter)) {
            tick = picked = 0;
            SetTimeCount(0);
            SETFONTCOLOR(0, TEXTCOLOR);

            do {
                int button, result = 0;

                if (type == KEYBOARDBTNS || type == KEYBOARDMOVE) {
                    IN_ClearKeysDown();
                }
                IN_Pump();

                /* FLASH CURSOR */
                if (GetTimeCount() > 10) {
                    switch (tick) {
                    case 0:
                        VWB_Bar(x, PrintY + 1, CST_SPC - 2, 10, TEXTCOLOR);
                        break;
                    case 1:
                        PrintX = (uint16_t)x;
                        US_Print("?");
                        OrigSD_PlaySound(HITWALLSND);
                    }
                    tick ^= 1;
                    SetTimeCount(0);
                    VW_UpdateScreen();
                }

                /*
                 * Not in the original: Esc gives up without becoming the key
                 * (the original read it as the new key before checking for Esc).
                 */
                if (orig_keys[sc_Escape] || orig_quit) {
                    picked = 1;
                    continue;
                }

                /* WHICH TYPE OF INPUT DO WE PROCESS? */
                switch (type) {
                case MOUSE:
                    button = orig_hooks.mouse_buttons ? orig_hooks.mouse_buttons() : 0;
                    switch (button) {
                    case 1:
                        result = 1;
                        break;
                    case 2:
                        result = 2;
                        break;
                    case 4:
                        result = 3;
                        break;
                    }

                    if (result) {
                        int z;

                        for (z = 0; z < 4; z++) {
                            if (order[which] == buttonmouse[z]) {
                                buttonmouse[z] = bt_nobutton;
                                break;
                            }
                        }

                        buttonmouse[result - 1] = (int16_t)order[which];
                        picked = 1;
                        OrigSD_PlaySound(SHOOTDOORSND);
                    }
                    break;

                case JOYSTICK:
                    if (ci.button0) {
                        result = 1;
                    } else if (ci.button1) {
                        result = 2;
                    } else if (ci.button2) {
                        result = 3;
                    } else if (ci.button3) {
                        result = 4;
                    }

                    if (result) {
                        int z;

                        for (z = 0; z < 4; z++) {
                            if (order[which] == buttonjoy[z]) {
                                buttonjoy[z] = bt_nobutton;
                                break;
                            }
                        }

                        buttonjoy[result - 1] = (int16_t)order[which];
                        picked = 1;
                        OrigSD_PlaySound(SHOOTDOORSND);
                    }
                    break;

                case KEYBOARDBTNS:
                    if (orig_lastscan) {
                        buttonscan[order[which]] = orig_lastscan;
                        picked = 1;
                        ShootSnd();
                        IN_ClearKeysDown();
                    }
                    break;

                case KEYBOARDMOVE:
                    if (orig_lastscan) {
                        dirscan[moveorder[which]] = orig_lastscan;
                        picked = 1;
                        ShootSnd();
                        IN_ClearKeysDown();
                    }
                    break;
                }

            } while (!picked);

            SETFONTCOLOR(TEXTCOLOR, BKGDCOLOR);
            redraw = 1;
            WaitKeyUp();
            continue;
        }

        if (ci.button1 || orig_keys[sc_Escape] || orig_quit) {
            exit = 1;
        }

        /* MOVE TO ANOTHER SPOT? */
        switch (ci.dir) {
        case dir_West:
            do {
                which--;
                if (which < 0) {
                    which = 3;
                }
            } while (!cust->allowed[which]);
            redraw = 1;
            OrigSD_PlaySound(MOVEGUN1SND);
            while (ReadAnyControl(&ci), ci.dir != dir_None && !orig_quit) {
            }
            IN_ClearKeysDown();
            break;

        case dir_East:
            do {
                which++;
                if (which > 3) {
                    which = 0;
                }
            } while (!cust->allowed[which]);
            redraw = 1;
            OrigSD_PlaySound(MOVEGUN1SND);
            while (ReadAnyControl(&ci), ci.dir != dir_None && !orig_quit) {
            }
            IN_ClearKeysDown();
            break;
        case dir_North:
        case dir_South:
            exit = 1;
            break;
        default:
            break;
        }
    } while (!exit);

    OrigSD_PlaySound(ESCPRESSEDSND);
    WaitKeyUp();
    DrawWindow(5, PrintY - 1, 310, 13, BKGDCOLOR);
}

/* the two lines either side of the gun, which it overdraws */
static void FixupLines(int w)
{
    const int y = CST_Y + 26 + w * 13;

    VWB_Hlin(7, 32, y - 1, DEACTIVE);
    VWB_Hlin(7, 32, y + 12, BORD2COLOR);
    VWB_Hlin(7, 32, y - 2, BORDCOLOR);
    VWB_Hlin(7, 32, y + 13, BORDCOLOR);
}

/* FIXUP GUN CURSOR OVERDRAW SHIT */
static void FixupCustom(int w)
{
    static int lastwhich = -1;

    FixupLines(w);

    switch (w) {
    case 0:
        DrawCustMouse(1);
        break;
    case 3:
        DrawCustJoy(1);
        break;
    case 6:
        DrawCustKeybd(1);
        break;
    case 8:
        DrawCustKeys(1);
    }

    if (lastwhich >= 0) {
        FixupLines(lastwhich);

        if (lastwhich != w) {
            switch (lastwhich) {
            case 0:
                DrawCustMouse(0);
                break;
            case 3:
                DrawCustJoy(0);
                break;
            case 6:
                DrawCustKeybd(0);
                break;
            case 8:
                DrawCustKeys(0);
            }
        }
    }

    lastwhich = w;
}

/* one row of the four action names */
static void PrintActionNames(void)
{
    SETFONTCOLOR(TEXTCOLOR, BKGDCOLOR);
    PrintX = CST_START;
    US_Print(STR_CRUN);
    PrintX = CST_START + CST_SPC * 1;
    US_Print(STR_COPEN);
    PrintX = CST_START + CST_SPC * 2;
    US_Print(STR_CFIRE);
    PrintX = CST_START + CST_SPC * 3;
    US_Print(STR_CSTRAFE "\n");
}

/* DRAW CUSTOMIZE SCREEN */
static void DrawCustomScreen(void)
{
    int i;

    ClearMScreen();
    WindowX = 0;
    WindowW = 320;
    VWB_DrawPic(112, 184, C_MOUSELBACKPIC);
    DrawStripes(10);
    VWB_DrawPic(80, 0, C_CUSTOMIZEPIC);

    /* MOUSE */
    SETFONTCOLOR(READCOLOR, BKGDCOLOR);
    WindowX = 0;
    WindowW = 320;
    PrintY = CST_Y;
    US_CPrint("Mouse\n");

    PrintActionNames();
    DrawWindow(5, PrintY - 1, 310, 13, BKGDCOLOR);
    DrawCustMouse(0);
    US_Print("\n");

    /* JOYSTICK/PAD */
    SETFONTCOLOR(READCOLOR, BKGDCOLOR);
    US_CPrint("Joystick/Gravis GamePad\n");

    PrintActionNames();
    DrawWindow(5, PrintY - 1, 310, 13, BKGDCOLOR);
    DrawCustJoy(0);
    US_Print("\n");

    /* KEYBOARD */
    SETFONTCOLOR(READCOLOR, BKGDCOLOR);
    US_CPrint("Keyboard\n");
    PrintActionNames();
    DrawWindow(5, PrintY - 1, 310, 13, BKGDCOLOR);
    DrawCustKeybd(0);
    US_Print("\n");

    /* KEYBOARD MOVE KEYS */
    SETFONTCOLOR(TEXTCOLOR, BKGDCOLOR);
    PrintX = CST_START;
    US_Print(STR_LEFT);
    PrintX = CST_START + CST_SPC * 1;
    US_Print(STR_RIGHT);
    PrintX = CST_START + CST_SPC * 2;
    US_Print(STR_FRWD);
    PrintX = CST_START + CST_SPC * 3;
    US_Print(STR_BKWD "\n");
    DrawWindow(5, PrintY - 1, 310, 13, BKGDCOLOR);
    DrawCustKeys(0);

    /* PICK STARTING POINT IN MENU */
    if (CusItems.curpos < 0) {
        for (i = 0; i < CusItems.amount; i++) {
            if (CusMenu[i].active) {
                CusItems.curpos = (int16_t)i;
                break;
            }
        }
    }

    VW_UpdateScreen();
    MenuFadeIn();
}

static void PrintCustMouse(int i)
{
    int j;

    for (j = 0; j < 4; j++) {
        if (order[i] == buttonmouse[j]) {
            PrintX = (uint16_t)(CST_START + CST_SPC * i);
            US_Print(mbarray[j]);
            break;
        }
    }
}

static void DrawCustMouse(int hilight)
{
    int i;

    SETFONTCOLOR(hilight ? HIGHLIGHT : TEXTCOLOR, BKGDCOLOR);

    if (!mouseenabled) {
        SETFONTCOLOR(DEACTIVE, BKGDCOLOR);
        CusMenu[0].active = 0;
    } else {
        CusMenu[0].active = 1;
    }

    PrintY = CST_Y + 13 * 2;
    for (i = 0; i < 4; i++) {
        PrintCustMouse(i);
    }
}

static void PrintCustJoy(int i)
{
    int j;

    for (j = 0; j < 4; j++) {
        if (order[i] == buttonjoy[j]) {
            PrintX = (uint16_t)(CST_START + CST_SPC * i);
            US_Print(mbarray[j]);
            break;
        }
    }
}

static void DrawCustJoy(int hilight)
{
    int i;

    SETFONTCOLOR(hilight ? HIGHLIGHT : TEXTCOLOR, BKGDCOLOR);

    if (!joystickenabled) {
        SETFONTCOLOR(DEACTIVE, BKGDCOLOR);
        CusMenu[3].active = 0;
    } else {
        CusMenu[3].active = 1;
    }

    PrintY = CST_Y + 13 * 5;
    for (i = 0; i < 4; i++) {
        PrintCustJoy(i);
    }
}

static void PrintCustKeybd(int i)
{
    PrintX = (uint16_t)(CST_START + CST_SPC * i);
    US_Print(IN_GetScanName((byte)buttonscan[order[i]]));
}

static void DrawCustKeybd(int hilight)
{
    int i;

    SETFONTCOLOR(hilight ? HIGHLIGHT : TEXTCOLOR, BKGDCOLOR);

    PrintY = CST_Y + 13 * 8;
    for (i = 0; i < 4; i++) {
        PrintCustKeybd(i);
    }
}

static void PrintCustKeys(int i)
{
    PrintX = (uint16_t)(CST_START + CST_SPC * i);
    US_Print(IN_GetScanName((byte)dirscan[moveorder[i]]));
}

static void DrawCustKeys(int hilight)
{
    int i;

    SETFONTCOLOR(hilight ? HIGHLIGHT : TEXTCOLOR, BKGDCOLOR);

    PrintY = CST_Y + 13 * 10;
    for (i = 0; i < 4; i++) {
        PrintCustKeys(i);
    }
}

/*
====================
=
= Settings pages
=
= Not in the original: the settings this port adds, on pages drawn like
= the Control menu. Sound has the original sound screen, then music and
= effects volume; Video has the original Change View, then fill the screen
= and fullscreen. A slider works as in Mouse Sensitivity: choose it, then
= left and right (or the mouse wheel), Enter keeps and Esc puts it back; a
= change is heard at once. The values live with the platform (get_option /
= set_option).
=
====================
*/

#define SP_X 24
#define SP_Y 70
#define SP_W 284
#define SP_BARX 188 /* the sliders: ten steps of 10 pixels */
#define SP_BARW 100

typedef struct {
    const char *title;
    CP_iteminfo *info;
    CP_itemtype *items;
    const int8_t *option;    /* OPT_* for each line, or -1 for the line that opens a screen */
    void (*open)(int temp1); /* that screen */
} settings_t;

static CP_iteminfo SoundItems = {SP_X, SP_Y, 3, 0, 56}, VideoItems = {SP_X, SP_Y, 3, 0, 56};

static CP_itemtype SoundPage[] = {
    {1, "Sound devices", NULL},
    {1, "Music", NULL},
    {1, "Effects", NULL},
};
static const int8_t soundoptions[] = {-1, OPT_MUSICVOL, OPT_EFFECTSVOL};

static CP_itemtype VideoPage[] = {
    {1, "View size", NULL},
    {1, "Fill the screen", NULL},
    {1, "Fullscreen", NULL},
};
static const int8_t videooptions[] = {-1, OPT_FILLSCREEN, OPT_FULLSCREEN};

static const settings_t soundsettings = {"Sound", &SoundItems, SoundPage, soundoptions, CP_Sound},
                        videosettings = {"Video", &VideoItems, VideoPage, videooptions,
                                         CP_ChangeView};

static boolean IsSlider(int option)
{
    return option == OPT_MUSICVOL || option == OPT_EFFECTSVOL;
}

/* a slider as Mouse Sensitivity draws it, scaled to the row */
static void DrawOptionBar(int line, int value)
{
    const int y = SP_Y + line * 13 + 1;

    VWB_Bar(SP_BARX, y, SP_BARW, 9, TEXTCOLOR);
    DrawOutline(SP_BARX, y, SP_BARW, 9, 0, HIGHLIGHT);
    DrawOutline(SP_BARX + 10 * value, y, 10, 9, 0, READCOLOR);
    VWB_Bar(SP_BARX + 1 + 10 * value, y + 1, 9, 8, READHCOLOR);
}

static void DrawSettings(const settings_t *page)
{
    int i;

    ClearMScreen();
    DrawStripes(10);
    VWB_DrawPic(112, 184, C_MOUSELBACKPIC);

    fontnumber = 1;
    SETFONTCOLOR(READHCOLOR, 0);
    WindowX = 0;
    WindowW = 320;
    PrintY = 14;
    US_CPrint(page->title);

    DrawWindow(SP_X - 8, SP_Y - 5, SP_W, 13 * page->info->amount + 6, BKGDCOLOR);
    DrawMenu(page->info, page->items);

    for (i = 0; i < page->info->amount; i++) {
        const int option = page->option[i];

        if (IsSlider(option)) {
            DrawOptionBar(i, GetOption(option));
        } else if (option >= 0) {
            VWB_DrawPic(SP_X + page->info->indent - 24, SP_Y + i * 13 + 3,
                        GetOption(option) ? C_SELECTEDPIC : C_NOTSELECTEDPIC);
        }
    }

    DrawMenuGun(page->info);
    VW_UpdateScreen();
}

/* right / up (or the wheel up) for more, left / down for less; Enter keeps it, Esc puts it back */
static void AdjustOption(int line, int option)
{
    ControlInfo ci;
    int exit = 0, value = GetOption(option), step;
    const int old = value;

    WaitKeyUp();
    do {
        ReadAnyControl(&ci);
        step = (ci.dir == dir_East || ci.dir == dir_North)  ? 1
               : (ci.dir == dir_West || ci.dir == dir_South) ? -1
                                                              : 0;
        if (step && value + step >= 0 && value + step <= 9) {
            value += step;
            SetOption(option, value);
            DrawOptionBar(line, value);
            VW_UpdateScreen();
            OrigSD_PlaySound(MOVEGUN1SND);
            TicDelay(20);
        }

        if (ci.button0 || orig_keys[sc_Space] || orig_keys[sc_Enter]) {
            exit = 1;
        } else if (ci.button1 || orig_keys[sc_Escape] || orig_quit) {
            exit = 2;
        }
    } while (!exit);

    if (exit == 2) {
        SetOption(option, old);
        DrawOptionBar(line, old);
        VW_UpdateScreen();
        OrigSD_PlaySound(ESCPRESSEDSND);
    } else {
        ShootSnd();
    }
    WaitKeyUp();
    IN_ClearKeysDown();
}

static void SettingsPage(const settings_t *page)
{
    int which;

    DrawSettings(page);
    MenuFadeIn();
    WaitKeyUp();

    do {
        which = HandleMenu(page->info, page->items, NULL);
        if (which < 0) {
            break;
        }
        if (page->option[which] < 0) { /* the original screen */
            ShootSnd();
            MenuFadeOut();
            page->open(0);
            DrawSettings(page);
            MenuFadeIn();
        } else if (IsSlider(page->option[which])) {
            AdjustOption(which, page->option[which]);
            DrawSettings(page);
        } else {
            SetOption(page->option[which], !GetOption(page->option[which]));
            ShootSnd();
            DrawSettings(page);
        }
        WaitKeyUp(); /* a button still down would choose again at once */
    } while (!orig_quit);

    MenuFadeOut();
}

/* SOUND: the devices (CP_Sound), music and effects volume */
static void CP_SoundPage(int temp1)
{
    (void)temp1;
    SettingsPage(&soundsettings);
}

/* VIDEO: the view size (CP_ChangeView), fill the screen, fullscreen */
static void CP_Video(int temp1)
{
    (void)temp1;
    SettingsPage(&videosettings);
}

/*
====================
=
= CP_CheckQuick
=
= CHECK QUICK-KEYS & QUIT (WHILE IN A GAME)
=
====================
*/

int CP_CheckQuick(unsigned scancode)
{
    switch (scancode) {
    /* END GAME */
    case sc_F7:
        CA_CacheGrChunk(STARTFONT + 1);

        WindowH = 160;
        if (Confirm(ENDGAMESTR)) {
            playstate = ex_died;
            pickquick = gamestate.lives = 0;
        }

        DrawPlayBorder(); /* DrawAllPlayBorder */
        WindowH = 200;
        fontnumber = 0;
        MainMenu[savegame].active = 0;
        return 1;

    /* QUICKSAVE */
    case sc_F8:
        if (SaveGamesAvail[LSItems.curpos] && pickquick) {
            CA_CacheGrChunk(STARTFONT + 1);
            fontnumber = 1;
            Message(STR_SAVING "...");
            CP_SaveGame(1);
            fontnumber = 0;
        } else {
            VW_FadeOut();

            StartCPMusic(MENUSONG);
            pickquick = CP_SaveGame(0);

            SETFONTCOLOR(0, 15);
            IN_ClearKeysDown();
            DrawPlayScreen();

            if (!startgame && !loadedgame) {
                VW_FadeIn();
                StartMusic();
            }

            if (loadedgame) {
                playstate = ex_abort;
            }
            lasttimecount = 0;
            SetTimeCount(0); /* lasttimecount = TimeCount */

            if (orig_hooks.mouse_delta) {
                int dx, dy;
                orig_hooks.mouse_delta(&dx, &dy); /* Clear accumulated mouse movement */
            }
        }
        return 1;

    /* QUICKLOAD */
    case sc_F9:
        if (SaveGamesAvail[LSItems.curpos] && pickquick) {
            char string[100] = STR_LGC;

            CA_CacheGrChunk(STARTFONT + 1);
            fontnumber = 1;

            strcat(string, SaveGameNames[LSItems.curpos]);
            strcat(string, "\"?");

            if (Confirm(string)) {
                CP_LoadGame(1);
            }

            DrawPlayBorder();
            fontnumber = 0;
        } else {
            VW_FadeOut();

            StartCPMusic(MENUSONG);
            pickquick = CP_LoadGame(0);

            SETFONTCOLOR(0, 15);
            IN_ClearKeysDown();
            DrawPlayScreen();

            if (!startgame && !loadedgame) {
                VW_FadeIn();
                StartMusic();
            }

            if (loadedgame) {
                playstate = ex_abort;
            }

            lasttimecount = 0;
            SetTimeCount(0);

            if (orig_hooks.mouse_delta) {
                int dx, dy;
                orig_hooks.mouse_delta(&dx, &dy);
            }
        }
        return 1;

    /* QUIT */
    case sc_F10:
        CA_CacheGrChunk(STARTFONT + 1);

        WindowX = WindowY = 0;
        WindowW = 320;
        WindowH = 160;
        if (Confirm(endStrings[US_RndT() & (0x7 + (US_RndT() & 1))])) {
            VW_UpdateScreen();
            if (orig_hooks.stop_music) {
                orig_hooks.stop_music();
            }
            MenuFadeOut();
            Quit();
        }

        DrawPlayBorder();
        WindowH = 200;
        fontnumber = 0;
        return 1;
    }

    return 0;
}

/* END THE CURRENT GAME */
int CP_EndGame(void)
{
    if (!Confirm(ENDGAMESTR)) {
        return 0;
    }

    pickquick = gamestate.lives = 0;
    playstate = ex_died;

    MainMenu[savegame].active = 0;
    MainMenu[viewscores].routine = CP_ViewScores;
    strcpy(MainMenu[viewscores].string, STR_VS);

    return 1;
}

/* VIEW THE HIGH SCORES */
void CP_ViewScores(int temp1)
{
    (void)temp1;
    fontnumber = 0;

    StartCPMusic(ROSTER_MUS);

    DrawHighScores();
    VW_UpdateScreen();
    MenuFadeIn();
    fontnumber = 1;

    IN_Ack();

    StartCPMusic(MENUSONG);
    MenuFadeOut();
}

/*
====================
=
= CP_NewGame
=
====================
*/

void CP_NewGame(int temp1)
{
    int which, episode = 0;

    (void)temp1;

firstpart:
    DrawNewEpisode();
    do {
        which = HandleMenu(&NewEitems, &NewEmenu[0], NULL);
        switch (which) {
        case -1:
            MenuFadeOut();
            return;

        default:
            if (!EpisodeSelect[which / 2]) {
                OrigSD_PlaySound(NOWAYSND);
                Message("Please select \"Read This!\"\n"
                        "from the Options menu to\n"
                        "find out how to order this\n"
                        "episode from Apogee.");
                IN_ClearKeysDown();
                IN_Ack();
                DrawNewEpisode();
                which = 0;
            } else {
                episode = which / 2;
                which = 1;
            }
            break;
        }

    } while (!which && !orig_quit);

    if (orig_quit) {
        return;
    }

    ShootSnd();

    /* ALREADY IN A GAME? */
    if (ingame) {
        if (!Confirm(CURGAME)) {
            MenuFadeOut();
            return;
        }
    }

    MenuFadeOut();

    DrawNewGame();
    which = HandleMenu(&NewItems, &NewMenu[0], DrawNewGameDiff);
    if (which < 0) {
        MenuFadeOut();
        if (orig_quit) {
            return;
        }
        goto firstpart;
    }

    ShootSnd();
    NewGame(which, episode);
    StartGame = 1;
    MenuFadeOut();

    /* CHANGE "READ THIS!" TO NORMAL COLOR */
    MainMenu[readthis].active = 1;

    pickquick = 0;
}

/* DRAW NEW EPISODE MENU */
static void DrawNewEpisode(void)
{
    int i;

    ClearMScreen();
    VWB_DrawPic(112, 184, C_MOUSELBACKPIC);

    DrawWindow(NE_X - 4, NE_Y - 4, NE_W + 8, NE_H + 8, BKGDCOLOR);
    SETFONTCOLOR(READHCOLOR, BKGDCOLOR);
    PrintY = 2;
    WindowX = 0;
    US_CPrint("Which episode to play?");

    SETFONTCOLOR(TEXTCOLOR, BKGDCOLOR);
    DrawMenu(&NewEitems, &NewEmenu[0]);

    for (i = 0; i < 6; i++) {
        VWB_DrawPic(NE_X + 32, NE_Y + i * 26, C_EPISODE1PIC + i);
    }

    VW_UpdateScreen();
    MenuFadeIn();
    WaitKeyUp();
}

/* DRAW NEW GAME MENU */
static void DrawNewGame(void)
{
    ClearMScreen();
    VWB_DrawPic(112, 184, C_MOUSELBACKPIC);

    SETFONTCOLOR(READHCOLOR, BKGDCOLOR);
    PrintX = NM_X + 20;
    PrintY = NM_Y - 32;
    US_Print("How tough are you?");

    DrawWindow(NM_X - 5, NM_Y - 10, NM_W, NM_H, BKGDCOLOR);

    DrawMenu(&NewItems, &NewMenu[0]);
    DrawNewGameDiff(NewItems.curpos);
    VW_UpdateScreen();
    MenuFadeIn();
    WaitKeyUp();
}

/* DRAW NEW GAME GRAPHIC */
static void DrawNewGameDiff(int w)
{
    VWB_DrawPic(NM_X + 185, NM_Y + 7, w + C_BABYMODEPIC);
}

/*
====================
=
= CP_Sound
=
= The sound effects (off, PC speaker, AdLib), digitized sound (off, Sound
= Source, Sound Blaster) and music (off, AdLib) through the sound mode
= hooks.
=
====================
*/

static int SoundModeOf(int which)
{
    return orig_hooks.get_sound_mode ? orig_hooks.get_sound_mode(which) : 0;
}

static void SetSoundMode(int which, int mode)
{
    if (orig_hooks.set_sound_mode) {
        orig_hooks.set_sound_mode(which, mode);
    }
}

void CP_Sound(int temp1)
{
    /* menu item -> which, mode */
    static const struct {
        int item, which, mode;
    } choices[] = {
        {0, 0, 0}, {1, 0, 1}, {2, 0, 2},  /* SOUND EFFECTS */
        {5, 1, 0}, {6, 1, 1}, {7, 1, 2},  /* DIGITIZED SOUND */
        {10, 2, 0}, {11, 2, 1},           /* MUSIC */
    };
    int which;
    size_t c;

    (void)temp1;
    DrawSoundMenu();
    MenuFadeIn();
    WaitKeyUp();

    do {
        which = HandleMenu(&SndItems, &SndMenu[0], NULL);

        /* HANDLE MENU CHOICES */
        for (c = 0; c < sizeof(choices) / sizeof(choices[0]); c++) {
            if (choices[c].item != which || SoundModeOf(choices[c].which) == choices[c].mode) {
                continue;
            }
            if (choices[c].which == 0) {
                OrigSD_WaitSoundDone(); /* SD_WaitSoundDone; SD_SetSoundMode */
            }
            SetSoundMode(choices[c].which, choices[c].mode);
            DrawSoundMenu();
            /* the shot, as the original: not when the effects or digitized sound go off */
            if (which != 0 && which != 5) {
                ShootSnd();
            }
            if (which == 11) {
                StartCPMusic(MENUSONG);
            }
        }
    } while (which >= 0 && !orig_quit);

    MenuFadeOut();
}

/* DRAW THE SOUND MENU */
static void DrawSoundMenu(void)
{
    static const int8_t itemwhich[12] = {0, 0, 0, -1, -1, 1, 1, 1, -1, -1, 2, 2};
    static const int8_t itemmode[12] = {0, 1, 2, 0, 0, 0, 1, 2, 0, 0, 0, 1};
    int i;

    ClearMScreen();
    VWB_DrawPic(112, 184, C_MOUSELBACKPIC);

    DrawWindow(SM_X - 8, SM_Y1 - 3, SM_W, SM_H1, BKGDCOLOR);
    DrawWindow(SM_X - 8, SM_Y2 - 3, SM_W, SM_H2, BKGDCOLOR);
    DrawWindow(SM_X - 8, SM_Y3 - 3, SM_W, SM_H3, BKGDCOLOR);

    /* NON-CHOOSENESS for what this machine cannot play */
    for (i = 0; i < SndItems.amount; i++) {
        if (itemwhich[i] >= 0 && itemmode[i] && orig_hooks.sound_available &&
            !orig_hooks.sound_available(itemwhich[i], itemmode[i])) {
            SndMenu[i].active = 0;
        }
    }
    if (!SndMenu[6].active && !SndMenu[7].active) {
        SndMenu[5].active = 0;
    }

    DrawMenu(&SndItems, &SndMenu[0]);
    VWB_DrawPic(100, SM_Y1 - 20, C_FXTITLEPIC);
    VWB_DrawPic(100, SM_Y2 - 20, C_DIGITITLEPIC);
    VWB_DrawPic(100, SM_Y3 - 20, C_MUSICTITLEPIC);

    for (i = 0; i < SndItems.amount; i++) {
        if (SndMenu[i].string[0]) {
            /* DRAW SELECTED/NOT SELECTED GRAPHIC BUTTONS */
            const int on = SoundModeOf(itemwhich[i]) == itemmode[i];

            VWB_DrawPic(SM_X + 24, SM_Y1 + i * 13 + 2, on ? C_SELECTEDPIC : C_NOTSELECTEDPIC);
        }
    }

    DrawMenuGun(&SndItems);
    VW_UpdateScreen();
}

/*
====================
=
= Load and save
=
= Ten slots, each with the name typed for it. The save files are the
= platform's (save_name, write_game, read_game hooks); the stream is
= SaveTheGame's.
=
====================
*/

static byte savebuf[64 * 1024];

/* DRAW LOAD/SAVE IN PROGRESS */
static void DrawLSAction(int which)
{
    DrawWindow(LSA_X, LSA_Y, LSA_W, LSA_H, TEXTCOLOR);
    DrawOutline(LSA_X, LSA_Y, LSA_W, LSA_H, 0, HIGHLIGHT);
    VWB_DrawPic(LSA_X + 8, LSA_Y + 5, C_DISKLOADING1PIC);

    fontnumber = 1;
    SETFONTCOLOR(0, TEXTCOLOR);
    PrintX = LSA_X + 46;
    PrintY = LSA_Y + 13;

    if (!which) {
        US_Print(STR_LOADING "...");
    } else {
        US_Print(STR_SAVING "...");
    }

    VW_UpdateScreen();
}

/*
 * LoadTheGame from a slot. Not in the original, which trusted its files: a
 * slot that cannot be read or does not check out gives a message and leaves
 * the game alone. If the stream was fine but its level would not load, the
 * game is gone and GameLoop is left (abortgame).
 */
static boolean LoadSlot(int which)
{
    const size_t len = orig_hooks.read_game ? orig_hooks.read_game(which, savebuf, sizeof(savebuf)) : 0;

    loadedgame = false;
    if (len && LoadTheGame(savebuf, len)) {
        return true;
    }
    if (loadedgame) {
        loadedgame = false;
        if (ingame) {
            abortgame = true;
            playstate = ex_abort;
        }
    }
    Message("That saved game could\nnot be loaded.");
    IN_ClearKeysDown();
    IN_Ack();
    return false;
}

/* SaveTheGame into a slot; the original's message when the disk is full */
static boolean SaveSlot(int which, const char *name)
{
    const size_t len = SaveTheGame(savebuf, sizeof(savebuf));

    if (len && len <= sizeof(savebuf) && orig_hooks.write_game &&
        orig_hooks.write_game(which, name, savebuf, len)) {
        return true;
    }
    Message(STR_NOSPACE1 "\n" STR_NOSPACE2);
    IN_ClearKeysDown();
    IN_Ack();
    return false;
}

/* LOAD SAVED GAMES */
int CP_LoadGame(int quick)
{
    int which, exit = 0;

    /* QUICKLOAD? */
    if (quick) {
        which = LSItems.curpos;

        if (SaveGamesAvail[which]) {
            if (!LoadSlot(which)) {
                return 0;
            }
            loadedgame = false;

            DrawFace();
            DrawHealth();
            DrawLives();
            DrawLevel();
            DrawAmmo();
            DrawKeys();
            DrawWeapon();
            DrawScore();
            return 1;
        }
    }

    DrawLoadSaveScreen(0);

    do {
        which = HandleMenu(&LSItems, &LSMenu[0], TrackWhichGame);
        if (which >= 0 && SaveGamesAvail[which]) {
            ShootSnd();
            DrawLSAction(0);

            if (!LoadSlot(which)) {
                if (abortgame) {
                    StartGame = 1; /* the game is gone: out of the menus */
                    break;
                }
                DrawLoadSaveScreen(0);
                continue;
            }

            StartGame = 1;
            ShootSnd();

            /* CHANGE "READ THIS!" TO NORMAL COLOR */
            MainMenu[readthis].active = 1;

            exit = 1;
            break;
        }

    } while (which >= 0 && !orig_quit);

    MenuFadeOut();

    return exit;
}

/* HIGHLIGHT CURRENT SELECTED ENTRY */
static void TrackWhichGame(int w)
{
    static int lastgameon = 0;

    PrintLSEntry(lastgameon, TEXTCOLOR);
    PrintLSEntry(w, HIGHLIGHT);

    lastgameon = w;
}

/* DRAW THE LOAD/SAVE SCREEN */
static void DrawLoadSaveScreen(int loadsave)
{
    int i;

    ClearMScreen();
    fontnumber = 1;
    VWB_DrawPic(112, 184, C_MOUSELBACKPIC);
    DrawWindow(LSM_X - 10, LSM_Y - 5, LSM_W, LSM_H, BKGDCOLOR);
    DrawStripes(10);

    if (!loadsave) {
        VWB_DrawPic(60, 0, C_LOADGAMEPIC);
    } else {
        VWB_DrawPic(60, 0, C_SAVEGAMEPIC);
    }

    for (i = 0; i < 10; i++) {
        PrintLSEntry(i, TEXTCOLOR);
    }

    DrawMenu(&LSItems, &LSMenu[0]);
    VW_UpdateScreen();
    MenuFadeIn();
    WaitKeyUp();
}

/* PRINT LOAD/SAVE GAME ENTRY W/BOX OUTLINE */
static void PrintLSEntry(int w, int color)
{
    SETFONTCOLOR(color, BKGDCOLOR);
    DrawOutline(LSM_X + LSItems.indent, LSM_Y + w * 13, LSM_W - LSItems.indent - 15, 11, color,
                color);
    PrintX = (uint16_t)(LSM_X + LSItems.indent + 2);
    PrintY = (uint16_t)(LSM_Y + w * 13 + 1);
    fontnumber = 0;

    if (SaveGamesAvail[w]) {
        US_Print(SaveGameNames[w]);
    } else {
        US_Print("      - " STR_EMPTY " -");
    }

    fontnumber = 1;
}

/* SAVE CURRENT GAME */
int CP_SaveGame(int quick)
{
    int which, exit = 0;
    char input[32];

    /* QUICKSAVE? */
    if (quick) {
        which = LSItems.curpos;

        if (SaveGamesAvail[which]) {
            strcpy(input, SaveGameNames[which]);
            (void)SaveSlot(which, input);
            return 1;
        }
    }

    DrawLoadSaveScreen(1);

    do {
        which = HandleMenu(&LSItems, &LSMenu[0], TrackWhichGame);
        if (which >= 0) {
            /* OVERWRITE EXISTING SAVEGAME? */
            if (SaveGamesAvail[which]) {
                if (!Confirm(GAMESVD)) {
                    DrawLoadSaveScreen(1);
                    continue;
                } else {
                    DrawLoadSaveScreen(1);
                    PrintLSEntry(which, HIGHLIGHT);
                    VW_UpdateScreen();
                }
            }

            ShootSnd();

            strcpy(input, SaveGameNames[which]);

            fontnumber = 0;
            if (!SaveGamesAvail[which]) {
                VWB_Bar(LSM_X + LSItems.indent + 1, LSM_Y + which * 13 + 1,
                        LSM_W - LSItems.indent - 16, 10, BKGDCOLOR);
            }
            VW_UpdateScreen();

            if (US_LineInput(LSM_X + LSItems.indent + 2, LSM_Y + which * 13 + 1, input, input, true,
                             31, LSM_W - LSItems.indent - 30)) {
                DrawLSAction(1);
                if (SaveSlot(which, input)) {
                    SaveGamesAvail[which] = 1;
                    strcpy(SaveGameNames[which], input);
                    ShootSnd();
                    exit = 1;
                }
            } else {
                VWB_Bar(LSM_X + LSItems.indent + 1, LSM_Y + which * 13 + 1,
                        LSM_W - LSItems.indent - 16, 10, BKGDCOLOR);
                PrintLSEntry(which, HIGHLIGHT);
                VW_UpdateScreen();
                OrigSD_PlaySound(ESCPRESSEDSND);
                continue;
            }

            fontnumber = 1;
            break;
        }

    } while (which >= 0 && !orig_quit);

    MenuFadeOut();

    return exit;
}

/*
====================
=
= CP_ChangeView
=
= CHANGE SCREEN VIEWING SIZE
=
====================
*/

void CP_ChangeView(int temp1)
{
    int exit = 0, oldview, newview;
    ControlInfo ci;

    (void)temp1;
    WindowX = WindowY = 0;
    WindowW = 320;
    WindowH = 200;
    newview = oldview = viewwidth / 16;
    DrawChangeView(oldview);

    do {
        CheckPause();
        ReadAnyControl(&ci);
        switch (ci.dir) {
        case dir_South:
        case dir_West:
            newview--;
            if (newview < 4) {
                newview = 4;
            }
            ShowViewSize(newview);
            VW_UpdateScreen();
            OrigSD_PlaySound(HITWALLSND);
            TicDelay(10);
            break;

        case dir_North:
        case dir_East:
            newview++;
            if (newview > 19) {
                newview = 19;
            }
            ShowViewSize(newview);
            VW_UpdateScreen();
            OrigSD_PlaySound(HITWALLSND);
            TicDelay(10);
            break;

        default:
            break;
        }

        if (ci.button0 || orig_keys[sc_Enter]) {
            exit = 1;
        } else if (ci.button1 || orig_keys[sc_Escape] || orig_quit) {
            viewwidth = (int16_t)(oldview * 16);
            OrigSD_PlaySound(ESCPRESSEDSND);
            MenuFadeOut();
            return;
        }

    } while (!exit);

    if (oldview != newview) {
        OrigSD_PlaySound(SHOOTSND);
        Message(STR_THINK "...");
        NewViewSize(newview);
    }

    ShootSnd();
    MenuFadeOut();
}

/* DRAW THE CHANGEVIEW SCREEN */
static void DrawChangeView(int view)
{
    VWB_Bar(0, 160, 320, 40, VIEWCOLOR);
    ShowViewSize(view);

    PrintY = 161;
    WindowX = 0;
    WindowY = 320;
    SETFONTCOLOR(HIGHLIGHT, BKGDCOLOR);

    US_CPrint(STR_SIZE1 "\n");
    US_CPrint(STR_SIZE2 "\n");
    US_CPrint(STR_SIZE3);
    VW_UpdateScreen();

    MenuFadeIn();
}

/* QUIT THIS INFERNAL GAME! */
void CP_Quit(void)
{
    if (Confirm(endStrings[US_RndT() & (0x7 + (US_RndT() & 1))])) {
        VW_UpdateScreen();
        if (orig_hooks.stop_music) {
            orig_hooks.stop_music();
        }
        MenuFadeOut();
        Quit();
        return;
    }

    DrawMainMenu();
}

/*
=============================================================================

                            SUPPORT ROUTINES

=============================================================================
*/

/* Clear Menu screens to dark red */
void ClearMScreen(void)
{
    VWB_Bar(0, 0, 320, 200, BORDCOLOR);
}

void DrawWindow(int x, int y, int w, int h, int wcolor)
{
    VWB_Bar(x, y, w, h, wcolor);
    DrawOutline(x, y, w, h, BORD2COLOR, DEACTIVE);
}

void DrawOutline(int x, int y, int w, int h, int color1, int color2)
{
    VWB_Hlin(x, x + w, y, color2);
    VWB_Vlin(y, y + h, x, color2);
    VWB_Hlin(x, x + w, y + h, color1);
    VWB_Vlin(y, y + h, x + w, color1);
}

/* the mouse driver's position, homed at CENTER (Mouse(3) / Mouse(4)) */
static int mousex = CENTER, mousey = CENTER;

static int wheelsteps; /* not in the original: mouse wheel notches still to give, up positive */
static boolean wheelgiven;

static void HomeMouse(void)
{
    int dx, dy;

    mousex = mousey = CENTER;
    if (orig_hooks.mouse_delta) {
        orig_hooks.mouse_delta(&dx, &dy);
    }
    if (orig_hooks.mouse_wheel) {
        (void)orig_hooks.mouse_wheel(); /* turns made during play */
    }
    wheelsteps = 0;
}

/* the episodes this pack has (CheckForEpisodes, WL_MENU.C) */
static void CheckForEpisodes(void)
{
    int i;

    for (i = 1; i < 6; i++) {
        NewEmenu[i * 2].active = upload ? 3 : 1;
        EpisodeSelect[i] = !upload;
    }
}

/* Setup Control Panel stuff - graphics, etc. */
void SetupControlPanel(void)
{
    int which;

    CA_CacheGrChunk(STARTFONT + 1);
    CheckForEpisodes();

    SETFONTCOLOR(TEXTCOLOR, BKGDCOLOR);
    fontnumber = 1;
    WindowH = 200;

    if (ingame) {
        MainMenu[savegame].active = 1;
    }

    /* SEE WHICH SAVE GAME FILES ARE AVAILABLE & READ STRING IN */
    for (which = 0; which < 10; which++) {
        SaveGameNames[which][0] = '\0';
        SaveGamesAvail[which] = orig_hooks.save_name && orig_hooks.save_name(which, SaveGameNames[which]);
        SaveGameNames[which][31] = '\0';
    }

    /* CENTER MOUSE */
    HomeMouse();
}

/* Clean up all the Control Panel stuff */
void CleanupControlPanel(void)
{
    fontnumber = 0;
}

/*
====================
=
= HandleMenu
=
= Handle moving gun around a menu
=
====================
*/

int HandleMenu(CP_iteminfo *item_i, CP_itemtype *items, void (*routine)(int w))
{
    char key;
    static int redrawitem = 1, lastitem = -1;
    int i, x, y, basey, exit, which, shape, timer;
    ControlInfo ci;

    which = item_i->curpos;
    x = item_i->x & -8;
    basey = item_i->y - 2;
    y = basey + which * 13;

    VWB_DrawPic(x, y, C_CURSOR1PIC);
    SetTextColor(items + which, 1);
    if (redrawitem) {
        PrintX = (uint16_t)(item_i->x + item_i->indent);
        PrintY = (uint16_t)(item_i->y + which * 13);
        US_Print((items + which)->string);
    }

    /* CALL CUSTOM ROUTINE IF IT IS NEEDED */
    if (routine) {
        routine(which);
    }
    VW_UpdateScreen();

    shape = C_CURSOR1PIC;
    timer = 8;
    exit = 0;
    SetTimeCount(0);
    IN_ClearKeysDown();

    do {
        /* CHANGE GUN SHAPE */
        if (GetTimeCount() > timer) {
            SetTimeCount(0);
            if (shape == C_CURSOR1PIC) {
                shape = C_CURSOR2PIC;
                timer = 8;
            } else {
                shape = C_CURSOR1PIC;
                timer = 70;
            }
            VWB_DrawPic(x, y, shape);
            if (routine) {
                routine(which);
            }
            VW_UpdateScreen();
        }

        CheckPause();

        /* SEE IF ANY KEYS ARE PRESSED FOR INITIAL CHAR FINDING */
        key = (char)orig_lastascii;
        if (key) {
            int ok = 0;

            if (key >= 'a') {
                key -= 'a' - 'A';
            }

            for (i = which + 1; i < item_i->amount; i++) {
                if ((items + i)->active && (items + i)->string[0] == key) {
                    EraseGun(item_i, items, x, y, which);
                    which = i;
                    DrawGun(item_i, items, x, &y, which, basey, routine);
                    ok = 1;
                    IN_ClearKeysDown();
                    break;
                }
            }

            /* DIDN'T FIND A MATCH FIRST TIME THRU. CHECK AGAIN. */
            if (!ok) {
                for (i = 0; i < which; i++) {
                    if ((items + i)->active && (items + i)->string[0] == key) {
                        EraseGun(item_i, items, x, y, which);
                        which = i;
                        DrawGun(item_i, items, x, &y, which, basey, routine);
                        IN_ClearKeysDown();
                        break;
                    }
                }
            }
        }

        /* GET INPUT */
        ReadAnyControl(&ci);
        switch (ci.dir) {
        /* MOVE UP */
        case dir_North:
            EraseGun(item_i, items, x, y, which);

            /* ANIMATE HALF-STEP */
            if (which && (items + which - 1)->active) {
                y -= 6;
                DrawHalfStep(x, y);
            }

            /* MOVE TO NEXT AVAILABLE SPOT */
            do {
                if (!which) {
                    which = item_i->amount - 1;
                } else {
                    which--;
                }
            } while (!(items + which)->active);

            DrawGun(item_i, items, x, &y, which, basey, routine);

            /* WAIT FOR BUTTON-UP OR DELAY NEXT MOVE */
            TicDelay(20);
            break;

        /* MOVE DOWN */
        case dir_South:
            EraseGun(item_i, items, x, y, which);

            /* ANIMATE HALF-STEP */
            if (which != item_i->amount - 1 && (items + which + 1)->active) {
                y += 6;
                DrawHalfStep(x, y);
            }

            do {
                if (which == item_i->amount - 1) {
                    which = 0;
                } else {
                    which++;
                }
            } while (!(items + which)->active);

            DrawGun(item_i, items, x, &y, which, basey, routine);

            /* WAIT FOR BUTTON-UP OR DELAY NEXT MOVE */
            TicDelay(20);
            break;

        default:
            break;
        }

        if (ci.button0 || orig_keys[sc_Space] || orig_keys[sc_Enter]) {
            exit = 1;
        }

        if (ci.button1 || orig_keys[sc_Escape] || orig_quit) {
            exit = 2;
        }

    } while (!exit);

    IN_ClearKeysDown();

    /* ERASE EVERYTHING */
    if (lastitem != which) {
        VWB_Bar(x - 1, y, 25, 16, BKGDCOLOR);
        PrintX = (uint16_t)(item_i->x + item_i->indent);
        PrintY = (uint16_t)(item_i->y + which * 13);
        US_Print((items + which)->string);
        redrawitem = 1;
    } else {
        redrawitem = 0;
    }

    if (routine) {
        routine(which);
    }
    VW_UpdateScreen();

    item_i->curpos = (int16_t)which;

    lastitem = which;
    switch (exit) {
    case 1:
        /* CALL THE ROUTINE */
        if ((items + which)->routine != NULL) {
            ShootSnd();
            MenuFadeOut();
            (items + which)->routine(0);
        }
        return which;

    case 2:
        OrigSD_PlaySound(ESCPRESSEDSND);
        return -1;
    }

    return 0; /* JUST TO SHUT UP THE ERROR MESSAGES! */
}

/* ERASE GUN & DE-HIGHLIGHT STRING */
static void EraseGun(CP_iteminfo *item_i, CP_itemtype *items, int x, int y, int which)
{
    VWB_Bar(x - 1, y, 25, 16, BKGDCOLOR);
    SetTextColor(items + which, 0);

    PrintX = (uint16_t)(item_i->x + item_i->indent);
    PrintY = (uint16_t)(item_i->y + which * 13);
    US_Print((items + which)->string);
    VW_UpdateScreen();
}

/* DRAW HALF STEP OF GUN TO NEXT POSITION */
static void DrawHalfStep(int x, int y)
{
    VWB_DrawPic(x, y, C_CURSOR1PIC);
    VW_UpdateScreen();
    OrigSD_PlaySound(MOVEGUN1SND);
    SetTimeCount(0);
    while (GetTimeCount() < 8 && !orig_quit) {
        IN_Pump();
    }
}

/* DRAW GUN AT NEW POSITION */
static void DrawGun(CP_iteminfo *item_i, CP_itemtype *items, int x, int *y, int which,
                    int basey, void (*routine)(int w))
{
    VWB_Bar(x - 1, *y, 25, 16, BKGDCOLOR);
    *y = basey + which * 13;
    VWB_DrawPic(x, *y, C_CURSOR1PIC);
    SetTextColor(items + which, 1);

    PrintX = (uint16_t)(item_i->x + item_i->indent);
    PrintY = (uint16_t)(item_i->y + which * 13);
    US_Print((items + which)->string);

    /* CALL CUSTOM ROUTINE IF IT IS NEEDED */
    if (routine) {
        routine(which);
    }
    VW_UpdateScreen();
    OrigSD_PlaySound(MOVEGUN2SND);
}

/* DELAY FOR AN AMOUNT OF TICS OR UNTIL CONTROLS ARE INACTIVE */
void TicDelay(int count)
{
    ControlInfo ci;

    SetTimeCount(0);
    do {
        ReadAnyControl(&ci);
    } while (GetTimeCount() < count && ci.dir != dir_None && !orig_quit);
}

/* Draw a menu */
void DrawMenu(CP_iteminfo *item_i, CP_itemtype *items)
{
    int i, which = item_i->curpos;

    WindowX = PrintX = (uint16_t)(item_i->x + item_i->indent);
    WindowY = PrintY = (uint16_t)item_i->y;
    WindowW = 320;
    WindowH = 200;

    for (i = 0; i < item_i->amount; i++) {
        SetTextColor(items + i, which == i);

        PrintY = (uint16_t)(item_i->y + i * 13);
        if ((items + i)->active) {
            US_Print((items + i)->string);
        } else {
            SETFONTCOLOR(DEACTIVE, BKGDCOLOR);
            US_Print((items + i)->string);
            SETFONTCOLOR(TEXTCOLOR, BKGDCOLOR);
        }

        US_Print("\n");
    }
}

/* SET TEXT COLOR (HIGHLIGHT OR NO) */
static void SetTextColor(CP_itemtype *items, int hlight)
{
    if (hlight) {
        SETFONTCOLOR(color_hlite[items->active], BKGDCOLOR);
    } else {
        SETFONTCOLOR(color_norml[items->active], BKGDCOLOR);
    }
}

/* WAIT FOR CTRLKEY-UP OR BUTTON-UP */
void WaitKeyUp(void)
{
    ControlInfo ci;

    while (ReadAnyControl(&ci), (ci.button0 | ci.button1 | ci.button2 | ci.button3 |
                                 orig_keys[sc_Space] | orig_keys[sc_Enter] |
                                 orig_keys[sc_Escape]) && !orig_quit) {
    }
}

/* IN_ReadControl for the keyboard (KbdDefs: Ctrl and Alt, the arrows and Home / PgUp / End / PgDn) */
static void ReadKeyboard(ControlInfo *ci)
{
    static const Direction DirTable[9] = {dir_NorthWest, dir_North, dir_NorthEast,
                                          dir_West,      dir_None,  dir_East,
                                          dir_SouthWest, dir_South, dir_SouthEast};
    int mx = 0, my = 0, buttons = 0;

    if (orig_keys[sc_Home]) {
        mx = -1, my = -1;
    } else if (orig_keys[sc_PgUp]) {
        mx = 1, my = -1;
    } else if (orig_keys[sc_End]) {
        mx = -1, my = 1;
    } else if (orig_keys[sc_PgDn]) {
        mx = 1, my = 1;
    }

    if (orig_keys[sc_UpArrow]) {
        my = -1;
    } else if (orig_keys[sc_DownArrow]) {
        my = 1;
    }

    if (orig_keys[sc_LeftArrow]) {
        mx = -1;
    } else if (orig_keys[sc_RightArrow]) {
        mx = 1;
    }

    if (orig_keys[sc_Control]) {
        buttons += 1 << 0;
    }
    if (orig_keys[sc_Alt]) {
        buttons += 1 << 1;
    }

    ci->x = (int16_t)(mx * 127);
    ci->xaxis = (int8_t)mx;
    ci->y = (int16_t)(my * 127);
    ci->yaxis = (int8_t)my;
    ci->button0 = (buttons & (1 << 0)) != 0;
    ci->button1 = (buttons & (1 << 1)) != 0;
    ci->button2 = false;
    ci->button3 = false;
    ci->dir = DirTable[((my + 1) * 3) + (mx + 1)];
}

/* a stick and buttons as the joystick gives them (the joystick, or a game pad) */
static void StickControl(ControlInfo *ci, int jx, int jy, int jb, boolean fourbuttons)
{
    if (jy < -SENSITIVE) {
        ci->dir = dir_North;
    } else if (jy > SENSITIVE) {
        ci->dir = dir_South;
    }

    if (jx < -SENSITIVE) {
        ci->dir = dir_West;
    } else if (jx > SENSITIVE) {
        ci->dir = dir_East;
    }

    if (jb) {
        ci->button0 = (jb & 1) != 0;
        ci->button1 = (jb & 2) != 0;
        if (fourbuttons) {
            ci->button2 = (jb & 4) != 0;
            ci->button3 = (jb & 8) != 0;
        } else {
            ci->button2 = ci->button3 = false;
        }
    }
}

/* READ KEYBOARD, JOYSTICK AND MOUSE FOR INPUT */
void ReadAnyControl(ControlInfo *ci)
{
    int mouseactive = 0;

    IN_Pump();
    ReadKeyboard(ci);

    if (mouseenabled) {
        int dx = 0, dy = 0, b;

        /* READ MOUSE MOTION COUNTERS, RETURN DIRECTION, HOME MOUSE, CHECK MOUSE BUTTONS */
        if (orig_hooks.mouse_delta) {
            orig_hooks.mouse_delta(&dx, &dy);
        }
        mousex += dx;
        mousey += dy;

        if (mousey < CENTER - SENSITIVE) {
            ci->dir = dir_North;
            mousex = mousey = CENTER;
            mouseactive = 1;
        } else if (mousey > CENTER + SENSITIVE) {
            ci->dir = dir_South;
            mousex = mousey = CENTER;
            mouseactive = 1;
        }

        if (mousex < CENTER - SENSITIVE) {
            ci->dir = dir_West;
            mousex = mousey = CENTER;
            mouseactive = 1;
        } else if (mousex > CENTER + SENSITIVE) {
            ci->dir = dir_East;
            mousex = mousey = CENTER;
            mouseactive = 1;
        }

        b = orig_hooks.mouse_buttons ? orig_hooks.mouse_buttons() : 0;
        if (b) {
            ci->button0 = (b & 1) != 0;
            ci->button1 = (b & 2) != 0;
            ci->button2 = (b & 4) != 0;
            ci->button3 = false;
            mouseactive = 1;
        }
    }

    if (joystickenabled && !mouseactive) {
        int jx = 0, jy = 0;

        if (orig_hooks.joy_delta) {
            orig_hooks.joy_delta(&jx, &jy);
        }
        StickControl(ci, jx, jy, orig_hooks.joy_buttons ? orig_hooks.joy_buttons() : 0,
                     joypadenabled);
    }

    /*
     * Not in the original: the mouse wheel, one notch as one press of up or
     * down, with a read without it between (so TicDelay ends and the next
     * notch moves again).
     */
    if (orig_hooks.mouse_wheel) {
        wheelsteps += orig_hooks.mouse_wheel();
    }
    if (wheelsteps && !wheelgiven && ci->dir == dir_None) {
        ci->dir = wheelsteps > 0 ? dir_North : dir_South;
        wheelsteps += wheelsteps > 0 ? -1 : 1;
        wheelgiven = true;
    } else {
        wheelgiven = false;
    }

    /* not in the original: a game pad, whether or not the joystick is enabled for play */
    if (!mouseactive && orig_hooks.pad_menu) {
        int jx, jy, jb;

        if (orig_hooks.pad_menu(&jx, &jy, &jb)) {
            StickControl(ci, jx, jy, jb, false);
        }
    }
}

/* the game pad's buttons for Confirm, 0 without a pad */
static int PadButtons(void)
{
    int jx, jy, jb = 0;

    if (!orig_hooks.pad_menu || !orig_hooks.pad_menu(&jx, &jy, &jb)) {
        return 0;
    }
    return jb;
}

/*
====================
=
= Confirm
=
= DRAW DIALOG AND CONFIRM YES OR NO TO QUESTION. Not in the original: a
= game pad's A and B answer as Y and N; the window closing answers no.
=
====================
*/

int Confirm(const char *string)
{
    int xit = 0, x, y, tick = 0, padheld, pad = 0;
    static const int whichsnd[2] = {ESCPRESSEDSND, SHOOTSND};

    Message(string);
    IN_ClearKeysDown();
    padheld = PadButtons(); /* the button that chose this, still down */

    /* BLINK CURSOR */
    x = PrintX;
    y = PrintY;
    SetTimeCount(0);

    do {
        IN_Pump();
        if (GetTimeCount() >= 10) {
            switch (tick) {
            case 0:
                VWB_Bar(x, y, 8, 13, TEXTCOLOR);
                break;
            case 1:
                PrintX = (uint16_t)x;
                PrintY = (uint16_t)y;
                US_Print("_");
            }
            VW_UpdateScreen();
            tick ^= 1;
            SetTimeCount(0);
        }

        {
            const int jb = PadButtons();
            pad = jb & ~padheld & 3;
            padheld = jb;
        }
    } while (!orig_keys[sc_Y] && !orig_keys[sc_N] && !orig_keys[sc_Escape] && !pad && !orig_quit);

    if (orig_keys[sc_Y] || (pad & 1)) {
        xit = 1;
        ShootSnd();
    }

    while ((orig_keys[sc_Y] || orig_keys[sc_N] || orig_keys[sc_Escape] || (PadButtons() & 3)) &&
           !orig_quit) {
        IN_Pump();
    }

    IN_ClearKeysDown();
    OrigSD_PlaySound(whichsnd[xit]);
    return xit;
}

/* a framed message in the big font, centred across the screen and the current window's height */
void Message(const char *string)
{
    const byte *font;
    int h, w = 0, mw = 0, i;
    const int len = (int)strlen(string);

    CA_CacheGrChunk(STARTFONT + 1);
    fontnumber = 1;
    font = grsegs[STARTFONT + fontnumber];
    if (!font) {
        return;
    }
    h = font[0] | (font[1] << 8);
    for (i = 0; i < len; i++) {
        if (string[i] == '\n') {
            if (w > mw) {
                mw = w;
            }
            w = 0;
            h += font[0] | (font[1] << 8);
        } else {
            w += font[2 + 512 + (byte)string[i]];
        }
    }

    if (w + 10 > mw) {
        mw = w + 10;
    }

    PrintY = (uint16_t)((WindowH / 2) - h / 2);
    PrintX = WindowX = (uint16_t)(160 - mw / 2);

    DrawWindow(WindowX - 5, PrintY - 5, mw + 10, h + 10, TEXTCOLOR);
    DrawOutline(WindowX - 5, PrintY - 5, mw + 10, h + 10, 0, HIGHLIGHT);
    SETFONTCOLOR(0, TEXTCOLOR);
    US_Print(string);
    VW_UpdateScreen();
}

void StartCPMusic(int song)
{
    if (orig_hooks.stop_music) {
        orig_hooks.stop_music(); /* SD_MusicOff */
    }
    if (orig_hooks.play_music) {
        orig_hooks.play_music(song);
    }
}

/* CHECK FOR PAUSE KEY (FOR MUSIC ONLY) */
void CheckPause(void)
{
    if (Paused) {
        switch (SoundStatus) {
        case 0:
            if (orig_hooks.resume_music) {
                orig_hooks.resume_music();
            }
            break;
        case 1:
            if (orig_hooks.stop_music) {
                orig_hooks.stop_music();
            }
            break;
        }

        SoundStatus ^= 1;
        VL_WaitVBL(3);
        IN_ClearKeysDown();
        Paused = false;
    }
}

/* DRAW GUN CURSOR AT CORRECT POSITION IN MENU */
void DrawMenuGun(CP_iteminfo *iteminfo)
{
    const int x = iteminfo->x;
    const int y = iteminfo->y + iteminfo->curpos * 13 - 2;

    VWB_DrawPic(x, y, C_CURSOR1PIC);
}

/* DRAW SCREEN TITLE STRIPES */
void DrawStripes(int y)
{
    VWB_Bar(0, y, 320, 24, 0);
    VWB_Hlin(0, 319, y + 22, STRIPE);
}

void ShootSnd(void)
{
    OrigSD_PlaySound(SHOOTSND);
}
