#pragma once

#include "orig_def.h"

/*
 * Trig tables (WL_DRAW.C / BuildTables in WL_MAIN.C).
 *
 * sintable holds sign-magnitude fractions for FixedByFrac: the low word is
 * the magnitude, bit 31 the sign. costable overlays sintable with a quarter
 * phase shift. The original declared ANGLES+ANGLES/4 entries but BuildTables
 * writes one more; the extra slot is kept here so that write stays in bounds.
 */
#define SINTABLE_SIZE (ANGLES + ANGLES / 4 + 1)

extern fixed sintable[SINTABLE_SIZE];
extern fixed *const costable;
extern int32_t finetangent[FINEANGLES / 4];

/* Fill sintable/costable and finetangent exactly as BuildTables does. */
void BuildTables(void);
