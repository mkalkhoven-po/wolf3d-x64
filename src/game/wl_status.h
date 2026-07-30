#pragma once

#include "wl_types.h"
#include "wolf_present.h"

/* Draw full STATUSBARPIC + live digits/face/weapon/keys at y=160. */
void Wolf_DrawStatusBar(void);

/*
 * Get Psyched! load screen (GETPSYCHEDPIC) with red preload bar 0x37/0x32.
 * short_mode: smoke/tests — brief fake progress, no long wait.
 * Returns false if graphics missing (caller may ignore).
 */
boolean Wolf_ShowGetPsyched(wolf_present_t *pr, boolean short_mode);

/* Smoke: STATUSBAR + GETPSYCHED chunks cache and have positive size. */
boolean Wolf_StatusSmokeVerify(void);
