#include "id_vl.h"

#include <string.h>

byte *framebuffer;
byte g_palette[768];

void VL_Startup(void)
{
    static byte fb[SCREENPIXELS];
    framebuffer = fb;
    memset(framebuffer, 0, SCREENPIXELS);
    memcpy(g_palette, gamepal, sizeof(g_palette));
}

void VL_Shutdown(void)
{
    framebuffer = NULL;
}

void FB_SetPalette(const byte *palette)
{
    if (palette) {
        memcpy(g_palette, palette, sizeof(g_palette));
    }
}

void FB_GetPalette(byte *palette)
{
    if (palette) {
        memcpy(palette, g_palette, sizeof(g_palette));
    }
}

void FB_Clear(int color)
{
    if (!framebuffer) {
        return;
    }
    memset(framebuffer, (byte)color, SCREENPIXELS);
}

void FB_Bar(int x, int y, int width, int height, int color)
{
    int yy, xx;
    if (!framebuffer || width <= 0 || height <= 0) {
        return;
    }
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
    if (width <= 0 || height <= 0) {
        return;
    }
    for (yy = 0; yy < height; ++yy) {
        byte *row = framebuffer + (y + yy) * SCREENWIDTH + x;
        for (xx = 0; xx < width; ++xx) {
            row[xx] = (byte)color;
        }
    }
}

void FB_Plot(int x, int y, int color)
{
    if (!framebuffer) {
        return;
    }
    if ((unsigned)x >= SCREENWIDTH || (unsigned)y >= SCREENHEIGHT) {
        return;
    }
    framebuffer[y * SCREENWIDTH + x] = (byte)color;
}

