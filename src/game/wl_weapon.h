#pragma once

#include "wl_types.h"

/* Draw first-person weapon (VSWAP sprites) into play view above status bar. */
void Wolf_DrawPlayerWeapon(void);

/* Smoke: can expand pistol-ready sprite with nonzero texels. */
boolean Wolf_WeaponSmokeVerify(void);
