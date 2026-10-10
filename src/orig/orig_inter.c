/*
 * WL_INTER.C: the screens between levels. P5a: "Get Psyched!" before a
 * level; the intermission, victory and high scores follow in P5c.
 */

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
