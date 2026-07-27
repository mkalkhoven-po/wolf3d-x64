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

void VL_SetPalette(const byte *palette)
{
    if (palette) {
        memcpy(g_palette, palette, sizeof(g_palette));
    }
}

void VL_GetPalette(byte *palette)
{
    if (palette) {
        memcpy(palette, g_palette, sizeof(g_palette));
    }
}

void VL_FadeIn(int start, int end, const byte *palette, int steps)
{
    (void)start;
    (void)end;
    (void)steps;
    VL_SetPalette(palette);
}

void VL_FadeOut(int start, int end, int red, int green, int blue, int steps)
{
    (void)start;
    (void)end;
    (void)red;
    (void)green;
    (void)blue;
    (void)steps;
}

void VL_WaitVBL(int vbls)
{
    (void)vbls;
}

void VL_ClearScreen(int color)
{
    if (!framebuffer) {
        return;
    }
    memset(framebuffer, (byte)color, SCREENPIXELS);
}

void VL_Bar(int x, int y, int width, int height, int color)
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

void VL_Plot(int x, int y, int color)
{
    if (!framebuffer) {
        return;
    }
    if ((unsigned)x >= SCREENWIDTH || (unsigned)y >= SCREENHEIGHT) {
        return;
    }
    framebuffer[y * SCREENWIDTH + x] = (byte)color;
}

/*
 * Pictures are stored post-munge as VGA planar (4 planes of width/4).
 * Convert on the fly into chunky pixels on the framebuffer.
 */
void VL_MemToScreen(const byte *source, int width, int height, int x, int y)
{
    int plane, yy, xx;
    int pwidth;
    if (!framebuffer || !source || width <= 0 || height <= 0) {
        return;
    }
    if (width & 3) {
        /* Not plane-aligned — treat as already-chunky. */
        for (yy = 0; yy < height; ++yy) {
            int dy = y + yy;
            if ((unsigned)dy >= SCREENHEIGHT) {
                continue;
            }
            for (xx = 0; xx < width; ++xx) {
                int dx = x + xx;
                if ((unsigned)dx >= SCREENWIDTH) {
                    continue;
                }
                framebuffer[dy * SCREENWIDTH + dx] = source[yy * width + xx];
            }
        }
        return;
    }

    pwidth = width / 4;
    for (plane = 0; plane < 4; ++plane) {
        const byte *psrc = source + plane * pwidth * height;
        for (yy = 0; yy < height; ++yy) {
            int dy = y + yy;
            if ((unsigned)dy >= SCREENHEIGHT) {
                psrc += pwidth;
                continue;
            }
            for (xx = 0; xx < pwidth; ++xx) {
                int dx = x + xx * 4 + plane;
                if ((unsigned)dx < SCREENWIDTH) {
                    framebuffer[dy * SCREENWIDTH + dx] = psrc[xx];
                }
            }
            psrc += pwidth;
        }
    }
}
