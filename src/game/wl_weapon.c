#include "wl_weapon.h"

#include <stdio.h>
#include <string.h>

#include "id/id_pm.h"
#include "id/id_vl.h"
#include "wl_play.h"

/*
 * Full sprite enum: SPR_KNIFEREADY=514 .. CHAINATK4=533.
 * WL6 VSWAP drops Spear-only sprites; weapons are last 20 pages:
 *   pack_shape = formal_shape - 98  => knife ready = 416
 */
#define SPR_FORMAL_KNIFEREADY 514
#define SPR_PACK_DELTA 98
#define SPR_WEAPON_BASE_PACK (SPR_FORMAL_KNIFEREADY - SPR_PACK_DELTA)

#define WEAPON_VIEW_H 160
#define SPRITE_SCALE_FACTOR 2

static void simple_scale_shape(int xcenter, int shapenum, unsigned height)
{
    const byte *shape;
    unsigned scale, pixheight;
    int actx, upperedge;
    int i;
    int pixcnt, rpix, lpix;
    int max_shape;
    int page;
    word leftpix, rightpix;

    if (!framebuffer || !PMPages || PMSoundStart <= PMSpriteStart) {
        return;
    }
    max_shape = (int)PMSoundStart - (int)PMSpriteStart - 1;
    if (shapenum < 0 || shapenum > max_shape) {
        return;
    }
    page = (int)PMSpriteStart + shapenum;
    shape = (const byte *)PM_GetPage(page);
    if (!shape) {
        return;
    }

    leftpix = (word)(shape[0] | (shape[1] << 8));
    rightpix = (word)(shape[2] | (shape[3] << 8));
    if (rightpix < leftpix || rightpix >= 64) {
        return;
    }

    scale = height >> 1;
    if (scale == 0) {
        return;
    }
    pixheight = scale * SPRITE_SCALE_FACTOR;
    actx = xcenter - (int)scale;
    upperedge = WEAPON_VIEW_H / 2 - (int)scale;

    for (i = (int)leftpix, pixcnt = i * (int)pixheight, rpix = (pixcnt >> 6) + actx;
         i <= (int)rightpix; ++i) {
        const byte *line;
        word col_ofs;

        lpix = rpix;
        if (lpix >= SCREENWIDTH) {
            break;
        }
        pixcnt += (int)pixheight;
        rpix = (pixcnt >> 6) + actx;
        if (lpix == rpix || rpix <= 0) {
            continue;
        }
        if (lpix < 0) {
            lpix = 0;
        }
        if (rpix > SCREENWIDTH) {
            rpix = SCREENWIDTH;
            i = (int)rightpix + 1;
        }

        col_ofs = (word)(shape[4 + (i - (int)leftpix) * 2] |
                         (shape[5 + (i - (int)leftpix) * 2] << 8));
        line = shape + col_ofs;

        while (lpix < rpix) {
            const byte *cmd = line;
            for (;;) {
                word endy, newstart, starty;
                unsigned j;
                int ycnt;
                int screndy;

                endy = (word)(cmd[0] | (cmd[1] << 8));
                cmd += 2;
                if (endy == 0) {
                    break;
                }
                endy >>= 1;
                newstart = (word)(cmd[0] | (cmd[1] << 8));
                cmd += 2;
                starty = (word)(cmd[0] | (cmd[1] << 8));
                cmd += 2;
                starty >>= 1;

                j = starty;
                ycnt = (int)j * (int)pixheight;
                screndy = (ycnt >> 6) + upperedge;

                for (; j < endy; ++j) {
                    int scrstarty = screndy;
                    byte col;

                    ycnt += (int)pixheight;
                    screndy = (ycnt >> 6) + upperedge;
                    if (scrstarty == screndy || screndy <= 0) {
                        continue;
                    }
                    col = shape[newstart + j];
                    if (scrstarty < 0) {
                        scrstarty = 0;
                    }
                    if (screndy > WEAPON_VIEW_H) {
                        screndy = WEAPON_VIEW_H;
                        j = endy;
                    }
                    while (scrstarty < screndy) {
                        if ((unsigned)lpix < SCREENWIDTH &&
                            (unsigned)scrstarty < (unsigned)WEAPON_VIEW_H) {
                            framebuffer[scrstarty * SCREENWIDTH + lpix] = col;
                        }
                        scrstarty++;
                    }
                }
            }
            lpix++;
        }
    }
}

static int weapon_shape(int weapon, int frame)
{
    int base = SPR_WEAPON_BASE_PACK + weapon * 5;
    if (frame < 0) {
        frame = 0;
    }
    if (frame > 4) {
        frame = 4;
    }
    return base + frame;
}

void Wolf_DrawPlayerWeapon(void)
{
    player_t *p = &g_level.player;
    int w;
    int shape;

    if (!g_level.active || p->dead) {
        return;
    }
    w = p->weapon;
    if (w < 0) {
        w = 0;
    }
    if (w > 3) {
        w = 3;
    }
    shape = weapon_shape(w, p->weapon_frame);
    simple_scale_shape(SCREENWIDTH / 2, shape, (unsigned)(WEAPON_VIEW_H + 1));
}

boolean Wolf_WeaponSmokeVerify(void)
{
    int shape = weapon_shape(1, 0);
    int page;
    const byte *data;
    word left, right;
    int max_shape;

    if (!PMPages || PMSoundStart <= PMSpriteStart) {
        fprintf(stderr, "Weapon smoke: VSWAP not open\n");
        return false;
    }
    max_shape = (int)PMSoundStart - (int)PMSpriteStart - 1;
    if (shape < 0 || shape > max_shape) {
        fprintf(stderr, "Weapon smoke: shape %d out of range max=%d\n", shape, max_shape);
        return false;
    }
    page = (int)PMSpriteStart + shape;
    data = (const byte *)PM_GetPage(page);
    if (!data) {
        fprintf(stderr, "Weapon smoke: page missing\n");
        return false;
    }
    left = (word)(data[0] | (data[1] << 8));
    right = (word)(data[2] | (data[3] << 8));
    printf("Weapon smoke: pistol_ready shape=%d page=%d left=%u right=%u view_h=%d\n",
           shape, page, left, right, WEAPON_VIEW_H);
    if (right < left || right >= 64) {
        return false;
    }
    /* Paint into framebuffer and require some coverage in the view. */
    {
        player_t *p = &g_level.player;
        int x, y, nz = 0;
        boolean was_active = g_level.active;
        int old_w = p->weapon;
        int old_f = p->weapon_frame;

        g_level.active = true;
        p->dead = false;
        p->weapon = 1;
        p->weapon_frame = 0;
        /* Sentinel so pure-0 palette texels still count as painted. */
        {
            int i;
            for (i = 0; i < SCREENPIXELS; ++i) {
                framebuffer[i] = 0xFE;
            }
        }
        Wolf_DrawPlayerWeapon();
        for (y = 0; y < WEAPON_VIEW_H; ++y) {
            for (x = 0; x < SCREENWIDTH; ++x) {
                if (framebuffer[y * SCREENWIDTH + x] != 0xFE) {
                    nz++;
                }
            }
        }
        p->weapon = old_w;
        p->weapon_frame = old_f;
        g_level.active = was_active;
        printf("Weapon smoke: painted_texels=%d\n", nz);
        if (nz < 200) {
            fprintf(stderr, "Weapon smoke: FP paint thin (%d)\n", nz);
            return false;
        }
    }
    return true;
}
