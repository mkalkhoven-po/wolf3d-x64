/*
 * The 2D toolkit of the original (ID_VH.C, ID_VL.C, ID_US_1.C): pictures,
 * the proportional fonts, bars and lines, the print window, palette fades.
 * Everything draws into screenbuf, the same 320x200 frame as the 3D view and
 * the status bar; the platform shows it (update_screen / present_frame
 * hooks) with the palette set through set_palette. The VGA planes, latches
 * and page flipping of the original have no counterpart: there is one frame.
 *
 * VGAGRAPH chunks come from the shared id layer (id_ca.c), which has the
 * pictures un-munged (chunky) already. Without data loaded (tests on
 * synthetic maps) the picture and font calls draw nothing.
 */

#include <stdio.h>
#include <string.h>

#include "id_ca.h"
#include "orig.h"

byte fontcolor, backcolor;
int16_t fontnumber;
int16_t px, py;
uint16_t PrintX, PrintY, WindowX, WindowY, WindowW, WindowH;

static byte curpal[768]; /* the palette on screen (VL_GetPalette) */

/* ------------------------------------------------------------ primitives */

void VWB_Bar(int x, int y, int width, int height, int color)
{
    int row;

    if (x < 0) {
        width += x;
        x = 0;
    }
    if (y < 0) {
        height += y;
        y = 0;
    }
    if (x + width > SCREENWIDTH) {
        width = SCREENWIDTH - x;
    }
    if (y + height > SCREENHEIGHT) {
        height = SCREENHEIGHT - y;
    }
    for (row = 0; row < height && width > 0; row++) {
        memset(&screenbuf[(y + row) * SCREENWIDTH + x], color, (size_t)width);
    }
}

void VWB_Plot(int x, int y, int color)
{
    if ((unsigned)x < SCREENWIDTH && (unsigned)y < SCREENHEIGHT) {
        screenbuf[y * SCREENWIDTH + x] = (byte)color;
    }
}

void VWB_Hlin(int x1, int x2, int y, int color)
{
    VWB_Bar(x1, y, x2 - x1 + 1, 1, color);
}

void VWB_Vlin(int y1, int y2, int x, int color)
{
    VWB_Bar(x, y1, 1, y2 - y1 + 1, color);
}

/* The picture's size, false when it is not there (no data loaded). */
static boolean PicSize(int chunknum, int *width, int *height)
{
    const int picnum = chunknum - STARTPICS;

    if (!pictable || picnum < 0 || picnum >= NUMPICS) {
        return false;
    }
    *width = pictable[picnum].width;
    *height = pictable[picnum].height;
    return *width > 0 && *height > 0;
}

void VWB_DrawPic(int x, int y, int chunknum)
{
    int width, height, row, x0, cols;
    const byte *src;

    x &= ~7; /* pictures sit on 8-pixel columns */
    if (!PicSize(chunknum, &width, &height)) {
        return;
    }
    CA_CacheGrChunk(chunknum);
    src = grsegs[chunknum];
    if (!src) {
        return;
    }
    x0 = x < 0 ? -x : 0;
    cols = (x + width > SCREENWIDTH ? SCREENWIDTH - x : width) - x0;
    for (row = 0; row < height && cols > 0; row++) {
        if ((unsigned)(y + row) < SCREENHEIGHT) {
            memcpy(&screenbuf[(y + row) * SCREENWIDTH + x + x0], src + row * width + x0,
                   (size_t)cols);
        }
    }
}

/* ----------------------------------------------------------------- fonts */

/*
 * fontstruct (ID_VH.H): int height; int location[256]; char width[256];
 * then the glyphs, each width columns by height rows, row by row, one byte
 * per pixel: non-zero draws in fontcolor.
 */
static const byte *Font(void)
{
    if (STARTFONT + fontnumber >= NUMCHUNKS_MAX) {
        return NULL;
    }
    CA_CacheGrChunk(STARTFONT + fontnumber);
    return grsegs[STARTFONT + fontnumber];
}

#define FONTHEIGHT(f) ((f)[0] | ((f)[1] << 8))
#define FONTLOCATION(f, c) ((f)[2 + 2 * (c)] | ((f)[3 + 2 * (c)] << 8))
#define FONTWIDTH(f, c) ((f)[2 + 512 + (c)])

void VW_MeasurePropString(const char *string, uint16_t *width, uint16_t *height)
{
    const byte *font = Font();

    *width = *height = 0;
    if (!font) {
        return;
    }
    *height = (uint16_t)FONTHEIGHT(font);
    for (; *string; string++) {
        *width = (uint16_t)(*width + FONTWIDTH(font, (byte)*string));
    }
}

void VW_DrawPropString(const char *string)
{
    const byte *font = Font();
    int height;

    if (!font) {
        return;
    }
    height = FONTHEIGHT(font);
    for (; *string; string++) {
        const int ch = (byte)*string;
        const int width = FONTWIDTH(font, ch);
        const byte *glyph = font + FONTLOCATION(font, ch);
        int col, row;

        for (col = 0; col < width; col++, px++) {
            for (row = 0; row < height; row++) {
                if (glyph[row * width + col]) {
                    VWB_Plot(px, py + row, fontcolor);
                }
            }
        }
    }
}

void VWB_DrawPropString(const char *string)
{
    VW_DrawPropString(string);
}

/* ----------------------------------------------------- the print window */

/* US_Print: newlines go back to the window's left edge */
void US_Print(const char *s)
{
    char line[256];

    while (*s) {
        size_t n = strcspn(s, "\n");
        uint16_t w, h;

        if (n >= sizeof(line)) {
            n = sizeof(line) - 1;
        }
        memcpy(line, s, n);
        line[n] = '\0';
        VW_MeasurePropString(line, &w, &h);
        px = (int16_t)PrintX;
        py = (int16_t)PrintY;
        VW_DrawPropString(line);
        s += n;
        if (*s == '\n') {
            s++;
            PrintX = WindowX;
            PrintY = (uint16_t)(PrintY + h);
        } else {
            PrintX = (uint16_t)(PrintX + w);
        }
    }
}

void US_PrintUnsigned(uint32_t n)
{
    char buffer[16];

    snprintf(buffer, sizeof(buffer), "%lu", (unsigned long)n);
    US_Print(buffer);
}

void US_PrintSigned(int32_t n)
{
    char buffer[16];

    snprintf(buffer, sizeof(buffer), "%ld", (long)n);
    US_Print(buffer);
}

/* USL_PrintInCenter / US_PrintCentered: the middle of the window */
void US_PrintCentered(const char *s)
{
    uint16_t w, h;

    VW_MeasurePropString(s, &w, &h);
    px = (int16_t)(WindowX + (WindowW - w) / 2);
    py = (int16_t)(WindowY + (WindowH - h) / 2);
    VW_DrawPropString(s);
}

/* centred on the current line, then down a line; no newlines */
void US_CPrintLine(const char *s)
{
    uint16_t w, h;

    VW_MeasurePropString(s, &w, &h);
    px = (int16_t)(WindowX + ((int)WindowW - (int)w) / 2); /* the original quits if too wide */
    py = (int16_t)PrintY;
    VW_DrawPropString(s);
    PrintY = (uint16_t)(PrintY + h);
}

void US_CPrint(const char *s)
{
    char line[256];

    while (*s) {
        size_t n = strcspn(s, "\n");

        if (n >= sizeof(line)) {
            n = sizeof(line) - 1;
        }
        memcpy(line, s, n);
        line[n] = '\0';
        US_CPrintLine(line);
        s += n;
        if (*s == '\n') {
            s++;
        }
    }
}

void US_ClearWindow(void)
{
    VWB_Bar(WindowX, WindowY, WindowW, WindowH, WHITE);
    PrintX = WindowX;
    PrintY = WindowY;
}

/* -------------------------------------------------------------- palette */

void VL_SetPalette(const byte *palette)
{
    memcpy(curpal, palette, sizeof(curpal));
    if (orig_hooks.set_palette) {
        orig_hooks.set_palette(curpal);
    }
}

void VL_GetPalette(byte *palette)
{
    memcpy(palette, curpal, sizeof(curpal));
}

void VL_FillPalette(int red, int green, int blue)
{
    byte pal[768];
    int i;

    for (i = 0; i < 256; i++) {
        pal[i * 3 + 0] = (byte)red;
        pal[i * 3 + 1] = (byte)green;
        pal[i * 3 + 2] = (byte)blue;
    }
    VL_SetPalette(pal);
}

void VL_WaitVBL(int vbls)
{
    if (orig_hooks.wait_vbl) {
        orig_hooks.wait_vbl(vbls);
    }
}

/* fade colours start..end to red/green/blue in steps retraces */
void VL_FadeOut(int start, int end, int red, int green, int blue, int steps)
{
    byte from[768], to[768];
    int i, j;

    VL_WaitVBL(1);
    VL_GetPalette(from);
    memcpy(to, from, sizeof(to));
    for (i = 0; i < steps; i++) {
        for (j = start; j <= end; j++) {
            to[j * 3 + 0] = (byte)(from[j * 3 + 0] + (red - from[j * 3 + 0]) * i / steps);
            to[j * 3 + 1] = (byte)(from[j * 3 + 1] + (green - from[j * 3 + 1]) * i / steps);
            to[j * 3 + 2] = (byte)(from[j * 3 + 2] + (blue - from[j * 3 + 2]) * i / steps);
        }
        VL_WaitVBL(1);
        VL_SetPalette(to);
    }
    VL_FillPalette(red, green, blue); /* the final colour */
    screenfaded = true;
}

/* fade colours start..end up to palette in steps retraces */
void VL_FadeIn(int start, int end, const byte *palette, int steps)
{
    byte from[768], to[768];
    int i, j;

    VL_WaitVBL(1);
    VL_GetPalette(from);
    memcpy(to, from, sizeof(to));
    for (i = 0; i < steps; i++) {
        for (j = start * 3; j <= end * 3 + 2; j++) {
            to[j] = (byte)(from[j] + (palette[j] - from[j]) * i / steps);
        }
        VL_WaitVBL(1);
        VL_SetPalette(to);
    }
    VL_SetPalette(palette); /* the final colours */
    screenfaded = false;
}

void VW_FadeOut(void)
{
    VL_FadeOut(0, 255, 0, 0, 0, 30);
}

void VW_FadeIn(void)
{
    VL_FadeIn(0, 255, gamepal, 30);
}

void VW_UpdateScreen(void)
{
    if (orig_hooks.update_screen) {
        orig_hooks.update_screen();
    }
}
