#include "wl_tables.h"

#include <math.h>

fixed sintable[SINTABLE_SIZE];
fixed *const costable = sintable + (ANGLES / 4);
int32_t finetangent[FINEANGLES / 4];

/*
 * Keep the original float/double mix: radtoint and the accumulated angle are
 * single precision, sin/tan run in double, and the results are truncated to
 * integers. Changing any of these changes table entries.
 */
static const float radtoint = (float)FINEANGLES / 2 / PI;

void BuildTables(void)
{
    int i;
    float angle, anglestep;
    double tang;
    fixed value;

    /* fine tangents */
    for (i = 0; i < FINEANGLES / 8; i++) {
        tang = tan((i + 0.5) / radtoint);
        finetangent[i] = (int32_t)(tang * TILEGLOBAL);
        finetangent[FINEANGLES / 4 - 1 - i] = (int32_t)(1 / tang * TILEGLOBAL);
    }

    /*
     * costable overlays sintable with a quarter phase shift. The low word of
     * the value is the fraction, the high bit is the sign bit, bits 16-30
     * should be 0.
     */
    angle = 0;
    anglestep = (float)(PI / 2 / ANGLEQUAD);
    for (i = 0; i <= ANGLEQUAD; i++) {
        value = (fixed)(GLOBAL1 * sin(angle));
        sintable[i] = sintable[i + ANGLES] = sintable[ANGLES / 2 - i] = value;
        sintable[ANGLES - i] = sintable[ANGLES / 2 + i] =
            (fixed)((uint32_t)value | 0x80000000u);
        angle += anglestep;
    }
}

fixed tileglobal = TILEGLOBAL;
