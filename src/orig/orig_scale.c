/*
 * Scaled sprites (WL_SCALE.C). S4a records the draw calls DrawScaleds makes,
 * in order; the pixel scaler arrives in S4c and keeps the record for tests.
 */
#include "orig.h"

scaledraw_t scaledraws[MAXVISABLE];
int numscaledraws;

void ScaleShape(int xcenter, int shapenum, unsigned height)
{
    if (numscaledraws < MAXVISABLE) {
        scaledraws[numscaledraws].viewx = (int16_t)xcenter;
        scaledraws[numscaledraws].shapenum = (int16_t)shapenum;
        scaledraws[numscaledraws].viewheight = (int16_t)height;
        numscaledraws++;
    }
}
