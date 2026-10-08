#include "wl_fixed.h"

/*
 * Port of the inline ASM in WL_DRAW.C. The original multiplies the high and
 * low word of |a| by the fraction separately and drops the low word of
 * low*frac; that is the same as (|a| * frac) >> 16, which always fits in
 * 32 bits.
 */
fixed FixedByFrac(fixed a, fixed b)
{
    uint32_t mag = (uint32_t)a;
    const uint32_t frac = (uint32_t)b & 0xffffu;
    int negative = ((uint32_t)b & 0x80000000u) != 0;
    uint32_t result;

    if (a < 0) {
        mag = 0u - mag; /* neg cx / neg ax / sbb cx,0 */
        negative = !negative;
    }

    result = (uint32_t)(((uint64_t)mag * frac) >> 16);
    if (negative) {
        result = 0u - result;
    }
    return (fixed)result;
}
