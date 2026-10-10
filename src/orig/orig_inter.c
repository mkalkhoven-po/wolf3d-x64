/*
 * WL_INTER.C: the screens between levels. P5a: "Get Psyched!" before a
 * level; the intermission, victory and high scores follow in P5c.
 */

#include <stdio.h>

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
