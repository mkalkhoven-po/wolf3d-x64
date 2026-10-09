/*
 * Scalers (WL_SCALE.C) without compiled code.
 *
 * The original builds one machine-code scaler per even height and calls the
 * one picked by a table lookup. Here SetupScaling only computes the table's
 * parameters, ScalerHeight reproduces which scaler a table index reaches
 * (including the sharing of scalers above stepbytwo), and ScaleRange draws
 * exactly the rows a compiled scaler of that height writes. TexelWidth is the
 * scaler's width[] table, used to scale sprites horizontally.
 *
 * Sprites are VSWAP pages in the compiled-shape format: leftpix, rightpix,
 * dataofs[] (one command list per column), then posts of three words each:
 * end*2 (0 ends the list), the "corrected top" offset so that texel y is at
 * top + y, and start*2.
 */
#include "orig.h"

int16_t maxscale, maxscaleshl2;
int16_t stepbytwo;
static int16_t maxscaleheight;

scaledraw_t scaledraws[MAXVISABLE];
int numscaledraws;

/*
==========================
=
= SetupScaling
=
==========================
*/

void SetupScaling(int maxheight)
{
    maxheight /= 2; /* one scaler every two pixels */
    maxscaleheight = (int16_t)maxheight;
    maxscale = (int16_t)(maxheight - 1);
    maxscaleshl2 = (int16_t)(maxscale << 2);

    stepbytwo = (int16_t)(viewheight / 2); /* save space by double stepping */
}

/*
 * Height in pixels of the scaler at table index (scaledirectory[index]).
 * Scalers exist for heights 2*i, i = 1..stepbytwo, then only for every third
 * i, which the two following table entries share. Entry 0 is entry 1.
 */
int ScalerHeight(int index)
{
    if (index <= 0) {
        index = 1;
    }
    if (index >= stepbytwo) {
        index = stepbytwo + 3 * ((index - stepbytwo) / 3);
    }
    return index * 2;
}

/* width[src] of BuildCompScale: screen pixels texel src covers at height */
int TexelWidth(int height, int src)
{
    const int32_t step = ((int32_t)height << 16) / 64;
    const int startpix = (int)(((int32_t)src * step) >> 16);
    const int endpix = (int)(((int32_t)(src + 1) * step) >> 16);

    return endpix > startpix ? endpix - startpix : 0;
}

/*
 * Texels first..last-1 of a 64-texel column scaled to height pixels, centred
 * vertically in the view: what BuildCompScale's code does from
 * codeofs[first] up to the RETF patched at codeofs[last]. pixel(i) gives
 * texel i; it returns -1 for a texel that can't be read.
 */
static void ScaleRange(const byte *src, int first, int last, int height, byte *dest, int pitch)
{
    int32_t fix, step;
    int srcpix, startpix, endpix, toppix;

    step = ((int32_t)height << 16) / 64;
    toppix = (viewheight - height) / 2;

    for (srcpix = first; srcpix < last && srcpix < 64; srcpix++) {
        fix = (int32_t)srcpix * step;
        startpix = (int)(fix >> 16);
        endpix = (int)((fix + step) >> 16);

        startpix += toppix;
        endpix += toppix;
        if (startpix == endpix || endpix < 0 || startpix >= viewheight) {
            continue;
        }

        for (; startpix < endpix; startpix++) {
            if (startpix >= viewheight) {
                break; /* off the bottom of the view area */
            }
            if (startpix < 0) {
                continue; /* not into the view area */
            }
            dest[startpix * pitch] = src[srcpix];
        }
    }
}

void ScaleColumn(const byte *src, int height, byte *dest, int pitch)
{
    ScaleRange(src, 0, 64, height, dest, pitch);
}

/*
 * The view columns a scaled strip of width pixels from x writes. The VGA
 * code writes up to three 4-pixel bytes with the masks below, indexed by
 * (x & 3) * 8 + width - 1. The tables cover widths up to 8, the most a texel
 * column can span; a larger index reads on into the following table as in
 * the original (and 0 past the last one). Columns outside the view are
 * dropped.
 */
static const byte mapmasks[3][4][8] = {
    {{1, 3, 7, 15, 15, 15, 15, 15},
     {2, 6, 14, 14, 14, 14, 14, 14},
     {4, 12, 12, 12, 12, 12, 12, 12},
     {8, 8, 8, 8, 8, 8, 8, 8}},
    {{0, 0, 0, 0, 1, 3, 7, 15},
     {0, 0, 0, 1, 3, 7, 15, 15},
     {0, 0, 1, 3, 7, 15, 15, 15},
     {0, 1, 3, 7, 15, 15, 15, 15}},
    {{0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 1},
     {0, 0, 0, 0, 0, 0, 1, 3},
     {0, 0, 0, 0, 0, 1, 3, 7}}};

int MaskedColumns(int x, int width, int cols[12])
{
    const byte *flat = &mapmasks[0][0][0];
    const int index = (x & 3) * 8 + width - 1;
    int b, plane, n = 0;

    if (width < 1) {
        return 0;
    }
    for (b = 0; b < 3; b++) {
        const int at = b * 32 + index;
        const byte mask = at < (int)sizeof(mapmasks) ? flat[at] : 0;
        if (!mask) {
            break; /* or al,al / jz nomore */
        }
        for (plane = 0; plane < 4; plane++) {
            const int col = ((x >> 2) + b) * 4 + plane;
            if ((mask & (1 << plane)) && col >= 0 && col < viewwidth) {
                cols[n++] = col;
            }
        }
    }
    return n;
}

/* ---------------------------------------------------------------- sprites */

static const byte *SpritePage(int shapenum)
{
    if (!orig_getpage || shapenum < 0) {
        return NULL;
    }
    return orig_getpage(orig_spritestart + shapenum);
}

static unsigned PageWord(const byte *page, unsigned ofs)
{
    if (ofs + 1 >= PMPAGESIZE) {
        return 0; /* not in the original: never read past the page */
    }
    return (unsigned)page[ofs] | ((unsigned)page[ofs + 1] << 8);
}

/*
 * ScaleLine: the posts of one sprite column (the command list at cmdofs in
 * the page), scaled to height, written to the columns of the strip
 * slinex, slinewidth.
 */
static void ScaleLine(const byte *shape, unsigned cmdofs, int height, int slinex,
                      int slinewidth)
{
    int cols[12], n, i;

    n = MaskedColumns(slinex, slinewidth, cols);
    if (!n) {
        return;
    }

    for (;;) {
        const unsigned end = PageWord(shape, cmdofs) / 2;
        const unsigned top = PageWord(shape, cmdofs + 2);
        const unsigned start = PageWord(shape, cmdofs + 4) / 2;
        byte texels[64];
        unsigned t;

        if (!end) {
            break; /* 0 signals end of segment list */
        }
        cmdofs += 6;

        /* texel y of this post is page byte (top + y) & 0xffff */
        for (t = start; t < end && t < 64; t++) {
            const unsigned at = (top + t) & 0xffffu;
            texels[t] = at < PMPAGESIZE ? shape[at] : 0;
        }
        for (i = 0; i < n; i++) {
            ScaleRange(texels, (int)start, (int)end, height, &screenbuf[screenofs + cols[i]],
                       SCREENWIDTH);
        }
    }
}

/*
=======================
=
= ScaleShape
=
= Draws a compiled shape at [scale] pixels high, behind closer walls
= (wallheight). The draw call is also recorded in scaledraws for tests.
=
= The loops count columns with an unsigned srcx in the original, which wraps
= for a shape with leftpix 0; no shape in the supported packs has one, and
= an int stops cleanly here.
=
=======================
*/

void ScaleShape(int xcenter, int shapenum, unsigned height)
{
    const byte *shape;
    int scale, sheight, srcx, stopx, leftpix, rightpix;
    unsigned cmdptr; /* index into dataofs[] */
    int slinex, slinewidth;
    boolean leftvis, rightvis;

    if (numscaledraws < MAXVISABLE) {
        scaledraws[numscaledraws].viewx = (int16_t)xcenter;
        scaledraws[numscaledraws].shapenum = (int16_t)shapenum;
        scaledraws[numscaledraws].viewheight = (int16_t)height;
        numscaledraws++;
    }

    shape = SpritePage(shapenum);
    if (!shape) {
        return;
    }

    scale = (int)(height >> 3); /* low three bits are fractional */
    if (!scale || scale > maxscale) {
        return; /* too close or far away */
    }
    sheight = ScalerHeight(scale);

    leftpix = (int)PageWord(shape, 0);
    rightpix = (int)PageWord(shape, 2);

    /* scale to the left (from pixel 31 to shape->leftpix) */
    srcx = 32;
    slinex = xcenter;
    stopx = leftpix;
    cmdptr = (unsigned)(31 - stopx);
    while (--srcx >= stopx && slinex > 0) {
        const unsigned linecmds = PageWord(shape, 4 + 2 * cmdptr--);

        if (!(slinewidth = TexelWidth(sheight, srcx))) {
            continue;
        }

        if (slinewidth == 1) {
            slinex--;
            if (slinex < viewwidth) {
                if (wallheight[slinex] >= height) {
                    continue; /* obscured by closer wall */
                }
                ScaleLine(shape, linecmds, sheight, slinex, 1);
            }
            continue;
        }

        /* handle multi pixel lines */
        if (slinex > viewwidth) {
            slinex -= slinewidth;
            slinewidth = viewwidth - slinex;
            if (slinewidth < 1) {
                continue; /* still off the right side */
            }
        } else {
            if (slinewidth > slinex) {
                slinewidth = slinex;
            }
            slinex -= slinewidth;
        }

        leftvis = wallheight[slinex] < height;
        rightvis = wallheight[slinex + slinewidth - 1] < height;

        if (leftvis) {
            if (!rightvis) {
                while (wallheight[slinex + slinewidth - 1] >= height) {
                    slinewidth--;
                }
            }
            ScaleLine(shape, linecmds, sheight, slinex, slinewidth);
        } else {
            if (!rightvis) {
                continue; /* totally obscured */
            }
            while (wallheight[slinex] >= height) {
                slinex++;
                slinewidth--;
            }
            ScaleLine(shape, linecmds, sheight, slinex, slinewidth);
            break; /* the rest of the shape is gone */
        }
    }

    /* scale to the right */
    slinex = xcenter;
    stopx = rightpix;
    if (leftpix < 31) {
        srcx = 31;
        cmdptr = (unsigned)(32 - leftpix);
    } else {
        srcx = leftpix - 1;
        cmdptr = 0;
    }
    slinewidth = 0;

    while (++srcx <= stopx && (slinex += slinewidth) < viewwidth) {
        const unsigned linecmds = PageWord(shape, 4 + 2 * cmdptr++);

        if (!(slinewidth = TexelWidth(sheight, srcx))) {
            continue;
        }

        if (slinewidth == 1) {
            if (slinex >= 0 && wallheight[slinex] < height) {
                ScaleLine(shape, linecmds, sheight, slinex, 1);
            }
            continue;
        }

        /* handle multi pixel lines */
        if (slinex < 0) {
            if (slinewidth <= -slinex) {
                continue; /* still off the left edge */
            }
            slinewidth += slinex;
            slinex = 0;
        } else {
            if (slinex + slinewidth > viewwidth) {
                slinewidth = viewwidth - slinex;
            }
        }

        leftvis = wallheight[slinex] < height;
        rightvis = wallheight[slinex + slinewidth - 1] < height;

        if (leftvis) {
            if (rightvis) {
                ScaleLine(shape, linecmds, sheight, slinex, slinewidth);
            } else {
                while (wallheight[slinex + slinewidth - 1] >= height) {
                    slinewidth--;
                }
                ScaleLine(shape, linecmds, sheight, slinex, slinewidth);
                break; /* the rest of the shape is gone */
            }
        } else {
            if (rightvis) {
                while (wallheight[slinex] >= height) {
                    slinex++;
                    slinewidth--;
                }
                ScaleLine(shape, linecmds, sheight, slinex, slinewidth);
            } else {
                continue; /* totally obscured */
            }
        }
    }
}

/*
=======================
=
= SimpleScaleShape
=
= NO CLIPPING, height in pixels
=
= Draws a compiled shape at [scale] pixels high (the player's weapon).
=
=======================
*/

void SimpleScaleShape(int xcenter, int shapenum, unsigned height)
{
    const byte *shape;
    int sheight, srcx, stopx, leftpix, rightpix;
    unsigned cmdptr;
    int slinex, slinewidth;

    shape = SpritePage(shapenum);
    if (!shape) {
        return;
    }

    sheight = ScalerHeight((int)(height >> 1));

    leftpix = (int)PageWord(shape, 0);
    rightpix = (int)PageWord(shape, 2);

    /* scale to the left (from pixel 31 to shape->leftpix) */
    srcx = 32;
    slinex = xcenter;
    stopx = leftpix;
    cmdptr = (unsigned)(31 - stopx);
    while (--srcx >= stopx) {
        const unsigned linecmds = PageWord(shape, 4 + 2 * cmdptr--);

        if (!(slinewidth = TexelWidth(sheight, srcx))) {
            continue;
        }
        slinex -= slinewidth;
        ScaleLine(shape, linecmds, sheight, slinex, slinewidth);
    }

    /* scale to the right */
    slinex = xcenter;
    stopx = rightpix;
    if (leftpix < 31) {
        srcx = 31;
        cmdptr = (unsigned)(32 - leftpix);
    } else {
        srcx = leftpix - 1;
        cmdptr = 0;
    }
    slinewidth = 0;
    while (++srcx <= stopx) {
        const unsigned linecmds = PageWord(shape, 4 + 2 * cmdptr++);

        if (!(slinewidth = TexelWidth(sheight, srcx))) {
            continue;
        }
        ScaleLine(shape, linecmds, sheight, slinex, slinewidth);
        slinex += slinewidth;
    }
}
