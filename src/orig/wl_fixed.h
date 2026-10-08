#pragma once

#include "orig_def.h"

/*
 * FixedByFrac (WL_DRAW.C): multiply a 16.16 two's complement fixed-point
 * number by a 16-bit fraction passed as a sign-magnitude 32-bit number.
 * Only bit 31 (sign) and the low word (magnitude) of b are used; bits 16-30
 * are ignored, so 1.0 cannot be represented (the tables top out at 0xffff).
 * The magnitude is truncated, so negative results round towards zero.
 */
fixed FixedByFrac(fixed a, fixed b);
