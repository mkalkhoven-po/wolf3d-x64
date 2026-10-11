/*
 * WL_INTER.C: the screens between levels: "Get Psyched!" before a level,
 * the intermission after one, the victory, the high scores and the death
 * cam's "Let's see that again!".
 */

#include <stdio.h>
#include <string.h>

#include "id_ca.h"
#include "orig.h"

/* the 3D view's part of the screen for 2D windows (WL_INTER.C) */
void ClearSplitVWB(void)
{
    WindowX = 0;
    WindowY = 0;
    WindowW = 320;
    WindowH = 160;
}

/* the progress bar under "Get Psyched!" */
static void PreloadUpdate(unsigned current, unsigned total)
{
    unsigned w = WindowW - 10;

    VWB_Bar(WindowX + 5, WindowY + WindowH - 3, (int)w, 2, BLACK);
    w = (unsigned)(((long)w * current) / total);
    if (w) {
        VWB_Bar(WindowX + 5, WindowY + WindowH - 3, (int)w, 2, 0x37); /* SECONDCOLOR */
        VWB_Bar(WindowX + 5, WindowY + WindowH - 3, (int)w - 1, 1, 0x32);
    }
    VW_UpdateScreen();
}

void PreloadGraphics(void)
{
    DrawLevel();
    ClearSplitVWB(); /* set up for double buffering in split screen */

    VWB_Bar(0, 0, 320, 200 - STATUSLINES, 127);
    VWB_DrawPic((20 - 14) * 8, 80 - 3 * 8, GETPSYCHEDPIC); /* LatchDrawPic in 8-pixel columns */

    WindowX = 160 - 14 * 8;
    WindowY = 80 - 3 * 8;
    WindowW = 28 * 8;
    WindowH = 48;
    VW_UpdateScreen();
    VW_FadeIn();

    /* PM_Preload filled the page cache with this bar; every page is in memory here */
    PreloadUpdate(1, 1);
    IN_UserInput(70);
    VW_FadeOut();

    DrawPlayBorder();
    VW_UpdateScreen();
}

/*
=============================================================================

                        HIGH SCORES (ID_US_1.C, WL_INTER.C)

=============================================================================
*/

/* the platform replaces these with the saved table at startup */
HighScore Scores[MaxScores] = {
    {"id software-'92", 10000, 1, 0},
    {"Adrian Carmack", 10000, 1, 0},
    {"John Carmack", 10000, 1, 0},
    {"Kevin Cloud", 10000, 1, 0},
    {"Tom Hall", 10000, 1, 0},
    {"John Romero", 10000, 1, 0},
    {"Jay Wilbur", 10000, 1, 0},
};

/* digits as the fixed-width characters 129... of the small font */
static void FixedWidth(char *buffer)
{
    char *str;

    for (str = buffer; *str; str++) {
        *str = (char)(*str + (129 - '0'));
    }
}

void DrawHighScores(void)
{
    char buffer[16], buffer1[8];
    uint16_t w, h;
    int i;
    const HighScore *s;

    ClearMScreen();
    DrawStripes(10);

    VWB_DrawPic(48, 0, HIGHSCORESPIC);

    VWB_DrawPic(4 * 8, 68, C_NAMEPIC);
    VWB_DrawPic(20 * 8, 68, C_LEVELPIC);
    VWB_DrawPic(28 * 8, 68, C_SCOREPIC);
    fontnumber = 0;

    SETFONTCOLOR(15, 0x29);

    for (i = 0, s = Scores; i < MaxScores; i++, s++) {
        PrintY = (uint16_t)(76 + (16 * i));

        /* name */
        PrintX = 4 * 8;
        US_Print(s->name);

        /* level */
        snprintf(buffer, sizeof(buffer), "%u", (unsigned)s->completed);
        FixedWidth(buffer);
        VW_MeasurePropString(buffer, &w, &h);
        PrintX = (uint16_t)((22 * 8) - w);

        if (!upload) { /* the registered episodes show which one */
            PrintX -= 6;
            snprintf(buffer1, sizeof(buffer1), "%d", s->episode + 1);
            US_Print("E");
            US_Print(buffer1);
            US_Print("/L");
        }

        US_Print(buffer);

        /* score */
        snprintf(buffer, sizeof(buffer), "%lu", (unsigned long)(uint32_t)s->score);
        FixedWidth(buffer);
        VW_MeasurePropString(buffer, &w, &h);
        PrintX = (uint16_t)((34 * 8) - 8 - w);
        US_Print(buffer);
    }

    VW_UpdateScreen();
}

/*
=============================================================================

                    THE INTERMISSION AND THE VICTORY (WL_INTER.C)

=============================================================================
*/

#define BORDCOLOR 0x29 /* WL_MENU.H */
#define READHCOLOR 0x47

/* FOREIGN.H */
#define STR_YOUWIN "you win!"
#define STR_TOTALTIME "total time"
#define STR_RATKILL "kill    %"
#define STR_RATSECRET "secret    %"
#define STR_RATTREASURE "treasure    %"
#define STR_BONUS "bonus"
#define STR_TIME "time"
#define STR_PAR " par"
#define STR_RAT2KILL "kill ratio    %"
#define STR_RAT2SECRET "secret ratio    %"
#define STR_RAT2TREASURE "treasure ratio    %"
#define STR_SEEAGAIN "Let's see that again!"

LRstruct LevelRatios[8];

/* the intermission letters: a 16x16 picture each (8 for ! ' :), at 8-pixel columns and rows */
void Write(int x, int y, const char *string)
{
    const size_t len = strlen(string);
    int ox, nx, ny;
    size_t i;
    int ch, pic;

    ox = nx = x * 8;
    ny = y * 8;
    for (i = 0; i < len; i++) {
        if (string[i] == '\n') {
            nx = ox;
            ny += 16;
            continue;
        }

        ch = (byte)string[i];
        if (ch >= 'a') {
            ch -= 'a' - 'A';
        }
        ch -= '0';

        switch (string[i]) {
        case '!':
            VWB_DrawPic(nx, ny, L_EXPOINTPIC);
            nx += 8;
            continue;

        case '\'':
            VWB_DrawPic(nx, ny, L_APOSTROPHEPIC);
            nx += 8;
            continue;

        case ' ':
            break;
        case ':':
            VWB_DrawPic(nx, ny, L_COLONPIC);
            nx += 8;
            continue;

        case '%':
            VWB_DrawPic(nx, ny, L_PERCENTPIC);
            break;

        default:
            /* alpha[]: 0-9, then ':', six without a picture, then A-Z */
            pic = 0;
            if (ch >= 0 && ch <= 9) {
                pic = L_NUM0PIC + ch;
            } else if (ch == 10) {
                pic = L_COLONPIC;
            } else if (ch >= 17 && ch <= 42) {
                pic = L_APIC + (ch - 17);
            }
            if (pic) {
                VWB_DrawPic(nx, ny, pic);
            }
        }
        nx += 16;
    }
}

/* Breathe Mr. BJ!!! (and, not in the original, read the input while waiting) */
static void BJ_Breathe(void)
{
    static int which = 0, max = 10;

    IN_Pump();
    if (GetTimeCount() > max) {
        which ^= 1;
        VWB_DrawPic(0, 16, which ? L_GUY2PIC : L_GUYPIC); /* pics[which] */
        VW_UpdateScreen();
        SetTimeCount(0);
        max = 35;
    }
}

/* while a tally sound plays, BJ breathes */
static void WaitSound(void)
{
    while (OrigSD_SoundPlaying() && !orig_quit) {
        BJ_Breathe();
    }
}

/* a number right-aligned so its last digit ends at column right */
static void WriteNumber(int right, int y, int32_t n)
{
    char tempstr[16];

    snprintf(tempstr, sizeof(tempstr), "%ld", (long)n);
    Write(right - (int)strlen(tempstr) * 2, y, tempstr);
}

/*
==================
=
= LevelCompleted
=
= Entered with the screen faded out
= Still in split screen mode with the status bar
=
= Exit with the screen faded out
=
==================
*/

void LevelCompleted(void)
{
#define VBLWAIT 30
#define PAR_AMOUNT 500
#define PERCENT100AMT 10000
#define RATIOXX 37
    typedef struct {
        float time;
        char timestr[6];
    } times;

    static const times parTimes[] = {
        /* Episode One Par Times */
        {1.5, "01:30"}, {2, "02:00"}, {2, "02:00"}, {3.5, "03:30"}, {3, "03:00"},
        {3, "03:00"}, {2.5, "02:30"}, {2.5, "02:30"},
        {0, "??:??"}, /* Boss level */
        {0, "??:??"}, /* Secret level */

        /* Episode Two Par Times */
        {1.5, "01:30"}, {3.5, "03:30"}, {3, "03:00"}, {2, "02:00"}, {4, "04:00"},
        {6, "06:00"}, {1, "01:00"}, {3, "03:00"}, {0, "??:??"}, {0, "??:??"},

        /* Episode Three Par Times */
        {1.5, "01:30"}, {1.5, "01:30"}, {2.5, "02:30"}, {2.5, "02:30"}, {3.5, "03:30"},
        {2.5, "02:30"}, {2, "02:00"}, {6, "06:00"}, {0, "??:??"}, {0, "??:??"},

        /* Episode Four Par Times */
        {2, "02:00"}, {2, "02:00"}, {1.5, "01:30"}, {1, "01:00"}, {4.5, "04:30"},
        {3.5, "03:30"}, {2, "02:00"}, {4.5, "04:30"}, {0, "??:??"}, {0, "??:??"},

        /* Episode Five Par Times */
        {2.5, "02:30"}, {1.5, "01:30"}, {2.5, "02:30"}, {2.5, "02:30"}, {4, "04:00"},
        {3, "03:00"}, {4.5, "04:30"}, {3.5, "03:30"}, {0, "??:??"}, {0, "??:??"},

        /* Episode Six Par Times */
        {6.5, "06:30"}, {4, "04:00"}, {4.5, "04:30"}, {6, "06:00"}, {5, "05:00"},
        {5.5, "05:30"}, {5.5, "05:30"}, {8.5, "08:30"}, {0, "??:??"}, {0, "??:??"},
    };

    const int mapon = gamestate.mapon; /* the level in the episode, as the original's mapon */
    const times *par = &parTimes[(gamestate.episode * 10 + mapon) % 60];
    int i, min = 0, sec = 0, ratio, kr = 0, sr = 0, tr = 0;
    int32_t bonus, timeleft = 0;
    char tempstr[10];

    ClearSplitVWB(); /* set up for double buffering in split screen */
    VWB_Bar(0, 0, 320, 200 - STATUSLINES, 127);
    StartCPMusic(ENDLEVEL_MUS);

    /* do the intermission */
    IN_ClearKeysDown();
    IN_StartAck();

    VWB_DrawPic(0, 16, L_GUYPIC);

    if (mapon < 8) {
        Write(14, 2, "floor\ncompleted");

        Write(14, 7, STR_BONUS "     0");
        Write(16, 10, STR_TIME);
        Write(16, 12, STR_PAR);

        Write(9, 14, STR_RAT2KILL);
        Write(5, 16, STR_RAT2SECRET);
        Write(1, 18, STR_RAT2TREASURE);

        snprintf(tempstr, sizeof(tempstr), "%d", gamestate.mapon + 1);
        Write(26, 2, tempstr);

        Write(26, 12, par->timestr);

        /* PRINT TIME */
        sec = (int)(gamestate.TimeCount / 70);

        if (sec > 99 * 60) { /* 99 minutes max */
            sec = 99 * 60;
        }

        if (gamestate.TimeCount < par->time * 4200) {
            timeleft = (int32_t)((par->time * 4200) / 70 - sec);
        }

        min = sec / 60;
        sec %= 60;

        i = 26 * 8;
        VWB_DrawPic(i, 10 * 8, L_NUM0PIC + (min / 10));
        i += 2 * 8;
        VWB_DrawPic(i, 10 * 8, L_NUM0PIC + (min % 10));
        i += 2 * 8;
        Write(i / 8, 10, ":");
        i += 1 * 8;
        VWB_DrawPic(i, 10 * 8, L_NUM0PIC + (sec / 10));
        i += 2 * 8;
        VWB_DrawPic(i, 10 * 8, L_NUM0PIC + (sec % 10));

        VW_UpdateScreen();
        VW_FadeIn();

        /* FIGURE RATIOS OUT BEFOREHAND */
        if (gamestate.killtotal) {
            kr = (gamestate.killcount * 100) / gamestate.killtotal;
        }
        if (gamestate.secrettotal) {
            sr = (gamestate.secretcount * 100) / gamestate.secrettotal;
        }
        if (gamestate.treasuretotal) {
            tr = (gamestate.treasurecount * 100) / gamestate.treasuretotal;
        }

        /* PRINT TIME BONUS */
        bonus = timeleft * PAR_AMOUNT;
        if (bonus) {
            for (i = 0; i <= timeleft; i++) {
                WriteNumber(36, 7, (int32_t)i * PAR_AMOUNT);
                if (!(i % (PAR_AMOUNT / 10))) {
                    OrigSD_PlaySound(ENDBONUS1SND);
                }
                VW_UpdateScreen();
                WaitSound();
                if (IN_CheckAck()) {
                    goto done;
                }
            }

            VW_UpdateScreen();
            OrigSD_PlaySound(ENDBONUS2SND);
            WaitSound();
        }

        /* KILL, SECRET AND TREASURE RATIOS, each counted up the same way */
        {
            const int ratios[3] = {kr, sr, tr};
            int r;

            for (r = 0; r < 3; r++) {
                ratio = ratios[r];
                for (i = 0; i <= ratio; i++) {
                    WriteNumber(RATIOXX, 14 + 2 * r, i);
                    if (!(i % 10)) {
                        OrigSD_PlaySound(ENDBONUS1SND);
                    }
                    VW_UpdateScreen();
                    WaitSound();
                    if (r == 1) {
                        BJ_Breathe(); /* the secret ratio breathes once more each step */
                    }

                    if (IN_CheckAck()) {
                        goto done;
                    }
                }
                if (ratio == 100) {
                    VL_WaitVBL(VBLWAIT);
                    OrigSD_StopSound();
                    bonus += PERCENT100AMT;
                    WriteNumber(RATIOXX - 1, 7, bonus);
                    VW_UpdateScreen();
                    OrigSD_PlaySound(PERCENT100SND);
                } else if (!ratio) {
                    VL_WaitVBL(VBLWAIT);
                    OrigSD_StopSound();
                    OrigSD_PlaySound(NOBONUSSND);
                } else {
                    OrigSD_PlaySound(ENDBONUS2SND);
                }

                VW_UpdateScreen();
                WaitSound();
            }
        }

        /* JUMP STRAIGHT HERE IF KEY PRESSED */
    done:
        WriteNumber(RATIOXX, 14, kr);
        WriteNumber(RATIOXX, 16, sr);
        WriteNumber(RATIOXX, 18, tr);

        bonus = timeleft * PAR_AMOUNT + (PERCENT100AMT * (kr == 100)) +
                (PERCENT100AMT * (sr == 100)) + (PERCENT100AMT * (tr == 100));

        GivePoints(bonus);
        WriteNumber(36, 7, bonus);

        /* SAVE RATIO INFORMATION FOR ENDGAME */
        LevelRatios[mapon].kill = (int16_t)kr;
        LevelRatios[mapon].secret = (int16_t)sr;
        LevelRatios[mapon].treasure = (int16_t)tr;
        LevelRatios[mapon].time = min * 60 + sec;
    } else {
        Write(14, 4, "secret floor\n completed!");

        Write(10, 16, "15000 bonus!");

        VW_UpdateScreen();
        VW_FadeIn();

        GivePoints(15000);
    }

    DrawScore();
    VW_UpdateScreen();

    SetTimeCount(0);
    IN_StartAck();
    while (!IN_CheckAck() && !orig_quit) {
        BJ_Breathe();
    }

    VW_FadeOut();
    DrawPlayBorder();
}

/*
==================
=
= Victory
=
==================
*/

void Victory(void)
{
#define RATIOX 6
#define RATIOY 14
#define TIMEX 14
#define TIMEY 8
    int32_t sec;
    int i, min, kr, sr, tr;
    char tempstr[8];

    StartCPMusic(URAHERO_MUS);
    ClearSplitVWB();

    VWB_Bar(0, 0, 320, 200 - STATUSLINES, 127);
    Write(18, 2, STR_YOUWIN);

    Write(TIMEX, TIMEY - 2, STR_TOTALTIME);

    Write(12, RATIOY - 2, "averages");

    Write(RATIOX + 8, RATIOY, STR_RATKILL);
    Write(RATIOX + 4, RATIOY + 2, STR_RATSECRET);
    Write(RATIOX, RATIOY + 4, STR_RATTREASURE);

    VWB_DrawPic(8, 4, L_BJWINSPIC);

    for (kr = sr = tr = 0, sec = i = 0; i < 8; i++) {
        sec += LevelRatios[i].time;
        kr += LevelRatios[i].kill;
        sr += LevelRatios[i].secret;
        tr += LevelRatios[i].treasure;
    }

    kr /= 8;
    sr /= 8;
    tr /= 8;

    min = (int)(sec / 60);
    sec %= 60;

    if (min > 99) {
        min = sec = 99;
    }

    i = TIMEX * 8 + 1;
    VWB_DrawPic(i, TIMEY * 8, L_NUM0PIC + (min / 10));
    i += 2 * 8;
    VWB_DrawPic(i, TIMEY * 8, L_NUM0PIC + (min % 10));
    i += 2 * 8;
    Write(i / 8, TIMEY, ":");
    i += 1 * 8;
    VWB_DrawPic(i, TIMEY * 8, L_NUM0PIC + (int)(sec / 10));
    i += 2 * 8;
    VWB_DrawPic(i, TIMEY * 8, L_NUM0PIC + (int)(sec % 10));
    VW_UpdateScreen();

    WriteNumber(RATIOX + 24, RATIOY, kr);
    WriteNumber(RATIOX + 24, RATIOY + 2, sr);
    WriteNumber(RATIOX + 24, RATIOY + 4, tr);

    /* TOTAL TIME VERIFICATION CODE (the registered game) */
    if (!upload && gamestate.difficulty >= gd_medium) {
        VWB_DrawPic(30 * 8, TIMEY * 8, C_TIMECODEPIC);
        fontnumber = 0;
        fontcolor = READHCOLOR;
        PrintX = 30 * 8 - 3;
        PrintY = TIMEY * 8 + 8;
        PrintX += 4;
        tempstr[0] = (char)((((min / 10) ^ (min % 10)) ^ 0xa) + 'A');
        tempstr[1] = (char)((((sec / 10) ^ (sec % 10)) ^ 0xa) + 'A');
        tempstr[2] = (char)((tempstr[0] ^ tempstr[1]) + 'A');
        tempstr[3] = 0;
        US_Print(tempstr);
    }

    fontnumber = 1;

    VW_UpdateScreen();
    VW_FadeIn();

    IN_Ack();

    VW_FadeOut();

    EndText();
}

/*
==================
=
= CheckHighScore
=
= A place in the table: the name goes in where the score is. other is the
= floor reached.
=
==================
*/

void CheckHighScore(int32_t score, uint16_t other)
{
    int i, j, n;
    HighScore myscore;

    memset(&myscore, 0, sizeof(myscore));
    myscore.score = score;
    myscore.episode = (uint16_t)gamestate.episode;
    myscore.completed = other;

    for (i = 0, n = -1; i < MaxScores; i++) {
        if ((myscore.score > Scores[i].score) ||
            ((myscore.score == Scores[i].score) && (myscore.completed > Scores[i].completed))) {
            for (j = MaxScores; --j > i;) {
                Scores[j] = Scores[j - 1];
            }
            Scores[i] = myscore;
            n = i;
            break;
        }
    }

    StartCPMusic(ROSTER_MUS);
    DrawHighScores();

    VW_FadeIn();

    if (n != -1) {
        /* got a high score */
        PrintY = (uint16_t)(76 + (16 * n));
        PrintX = 4 * 8;
        backcolor = BORDCOLOR;
        fontcolor = 15;
        US_LineInput(PrintX, PrintY, Scores[n].name, NULL, true, MaxHighName, 100);
    } else {
        IN_ClearKeysDown();
        IN_UserInput(500);
    }
}

/*
==================
=
= The death cam's screen work (A_StartDeathCam, WL_ACT2.C)
=
==================
*/

/* FinishPaletteShifts, then a moment (also when the cam is already over) */
void DeathCamPause(void)
{
    FinishPaletteShifts();
    VL_WaitVBL(100);
}

/* the view fizzles to the border colour, "Let's see that again!" */
void DeathCamIntro(void)
{
    VWB_Bar(0, 0, 320, 200 - STATUSLINES, 127);
    FizzleFade(0, 0, 320, 200 - STATUSLINES, 70);

    Write(0, 7, STR_SEEAGAIN);

    VW_UpdateScreen();

    IN_UserInput(300);
}

/* go back to the game */
void DeathCamOutro(void)
{
    DrawPlayBorder();
}
