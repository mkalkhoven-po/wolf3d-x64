/*
 * Scalers (WL_SCALE.C) without compiled code.
 *
 * The original builds one machine-code scaler per even height and calls the
 * one picked by a table lookup. Here SetupScaling only computes the table's
 * parameters, ScalerHeight reproduces which scaler a table index reaches
 * (including the sharing of scalers above stepbytwo), and ScaleColumn draws
 * exactly the rows a compiled scaler of that height writes.
 *
 * ScaleShape still records the draw calls DrawScaleds makes, in order; the
 * sprite pixels arrive in S4c.
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
 * Height in pixels of the scaler at table index (fullscalefarcall[index]).
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

/*
 * One 64-texel column scaled to height pixels, centred vertically in the
 * view: what BuildCompScale's code does. dest points at view row 0 of the
 * column; rows are pitch bytes apart.
 */
void ScaleColumn(const byte *src, int height, byte *dest, int pitch)
{
    int32_t fix, step;
    int srcpix, startpix, endpix, toppix;

    step = ((int32_t)height << 16) / 64;
    toppix = (viewheight - height) / 2;
    fix = 0;

    for (srcpix = 0; srcpix < 64; srcpix++) {
        startpix = (int)(fix >> 16);
        fix += step;
        endpix = (int)(fix >> 16);

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

void ScaleShape(int xcenter, int shapenum, unsigned height)
{
    if (numscaledraws < MAXVISABLE) {
        scaledraws[numscaledraws].viewx = (int16_t)xcenter;
        scaledraws[numscaledraws].shapenum = (int16_t)shapenum;
        scaledraws[numscaledraws].viewheight = (int16_t)height;
        numscaledraws++;
    }
}
