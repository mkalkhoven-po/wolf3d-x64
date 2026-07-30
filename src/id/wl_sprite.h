#pragma once

#include "wl_types.h"

/* Pack-relative shapenum (0 .. PMSoundStart-PMSpriteStart-1). */
boolean Wolf_SpriteExpand(int shapenum, byte out[64 * 64]);

/* Common non-Spear WL6 shapenums (match sprite enum without Spear pages). */
enum {
    SPR_STAT_BASE = 2, /* SPR_STAT_n = SPR_STAT_BASE + n */
    SPR_GRD_S1 = 50,
    SPR_GRD_W1_1 = 58,
    SPR_GRD_W2_1 = 66,
    SPR_GRD_W3_1 = 74,
    SPR_GRD_W4_1 = 82,
    SPR_GRD_DEAD = 95,
    SPR_DOG_W1_1 = 99,
    SPR_DOG_W2_1 = 107,
    SPR_DOG_W3_1 = 115,
    SPR_DOG_W4_1 = 123,
    SPR_DOG_DEAD = 134,
    SPR_SS_S1 = 138,
    SPR_SS_W1_1 = 146,
    SPR_SS_W2_1 = 154,
    SPR_SS_W3_1 = 162,
    SPR_SS_W4_1 = 170,
    SPR_SS_DEAD = 183,
    SPR_OFC_S1 = 238,
    SPR_OFC_W1_1 = 246,
    SPR_OFC_W2_1 = 254,
    SPR_OFC_W3_1 = 262,
    SPR_OFC_W4_1 = 270,
    SPR_OFC_DEAD = 284,
    SPR_BOSS_W1 = 296,
    SPR_BOSS_DEAD = 303,
    /* Ceiling fixtures (hang from top of view). */
    SPR_STAT_CHANDELIER = SPR_STAT_BASE + 4,
    SPR_STAT_CEIL_LIGHT = SPR_STAT_BASE + 14
};

boolean Wolf_SpriteSmokeVerify(void);
