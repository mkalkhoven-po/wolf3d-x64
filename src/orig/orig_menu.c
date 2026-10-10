/*
 * WL_MENU.C: the control panel (P5b). The main menu, New Game, Sound,
 * Load / Save Game, Change View, View Scores, Quit and the quick keys
 * F7-F10, drawn into screenbuf as the original drew them. The Control menu
 * and Read This! (the help screens, WL_TEXT.C) are P5b2; until then their
 * items show a note.
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
#define STR_CV "Change View"
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
    {1, STR_SD, CP_Sound},
    {1, STR_CL, CP_Control},
    {1, STR_LG, MenuLoadGame},
    {0, STR_SG, MenuSaveGame},
    {1, STR_CV, CP_ChangeView},
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
        CP_ReadThis(0); /* HelpScreens */
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
        CP_Sound(0);
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
 * The screens P5b2 ports: the help screens (HelpScreens, WL_TEXT.C) and the
 * Control menu. Until then a note in a window, as Message draws it.
 */
static void NotPortedYet(const char *what)
{
    char text[128];

    snprintf(text, sizeof(text), "%s\nis not ported yet.\n\nPress a key.", what);
    ClearMScreen();
    VWB_DrawPic(112, 184, C_MOUSELBACKPIC);
    WindowX = WindowY = 0;
    WindowW = 320;
    WindowH = 200;
    Message(text);
    MenuFadeIn();
    IN_ClearKeysDown();
    IN_Ack();
    WaitKeyUp();
    MenuFadeOut();
    fontnumber = 1;
}

/* READ THIS! */
void CP_ReadThis(int temp1)
{
    (void)temp1;
    StartCPMusic(CORNER_MUS);
    NotPortedYet("Read This!"); /* HelpScreens (P5b2) */
    StartCPMusic(MENUSONG);
}

/* DEFINE CONTROLS (P5b2) */
void CP_Control(int temp1)
{
    (void)temp1;
    NotPortedYet("The Control menu");
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

static void HomeMouse(void)
{
    int dx, dy;

    mousex = mousey = CENTER;
    if (orig_hooks.mouse_delta) {
        orig_hooks.mouse_delta(&dx, &dy);
    }
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
