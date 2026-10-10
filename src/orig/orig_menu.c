/*
 * WL_MENU.C: the control panel. P5a: the window drawing and Message() that
 * the rest of the game uses too; the menus themselves follow in P5b.
 */

#include <string.h>

#include "id_ca.h"
#include "orig.h"

/* WL_MENU.H colours */
#define BORDCOLOR 0x29
#define BORD2COLOR 0x23
#define DEACTIVE 0x2b
#define BKGDCOLOR 0x2d
#define STRIPE 0x2c
#define TEXTCOLOR 0x17
#define HIGHLIGHT 0x13

void DrawOutline(int x, int y, int w, int h, int color1, int color2)
{
    VWB_Hlin(x, x + w, y, color2);
    VWB_Vlin(y, y + h, x, color2);
    VWB_Hlin(x, x + w, y + h, color1);
    VWB_Vlin(y, y + h, x + w, color1);
}

void DrawWindow(int x, int y, int w, int h, int wcolor)
{
    VWB_Bar(x, y, w, h, wcolor);
    DrawOutline(x, y, w, h, BORD2COLOR, DEACTIVE);
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
