#pragma once

#include "wl_types.h"

/* Pack-relative shapenum (0 .. PMSoundStart-PMSpriteStart-1). */
boolean Wolf_SpriteExpand(int shapenum, byte out[64 * 64]);

/* Common non-Spear WL6 shapenums (match sprite enum without Spear pages). */
enum {
    SPR_STAT_BASE = 2, /* SPR_STAT_n = SPR_STAT_BASE + n */
    SPR_GRD_S1 = 50,
    SPR_GRD_W1_1 = 58,
    SPR_GRD_DEAD = 95,
    SPR_DOG_W1_1 = 99,
    SPR_DOG_DEAD = 134,
    SPR_SS_S1 = 138,
    SPR_SS_DEAD = 183,
    SPR_OFC_S1 = 238,
    SPR_OFC_DEAD = 284,
    SPR_BOSS_W1 = 296,
    SPR_BOSS_DEAD = 303
};

boolean Wolf_SpriteSmokeVerify(void);
