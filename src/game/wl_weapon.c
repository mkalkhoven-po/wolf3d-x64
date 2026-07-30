#include "wl_weapon.h"

#include <stdio.h>
#include <string.h>

#include "id/id_pm.h"
#include "id/id_vl.h"
#include "id/wl_sprite.h"
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

/* Centered 64x64 scale into view (transparent = 0xFF from expand). */
static void blit_weapon_shape(int xcenter, int shapenum, unsigned height)
{
    byte tex[64 * 64];
    int sprite_h, sprite_w;
    int draw_start_x, draw_end_x, draw_start_y, draw_end_y;
    int stripe, y;

    if (!framebuffer) {
        return;
    }
    if (!Wolf_SpriteExpand(shapenum, tex)) {
        return;
    }
    if (height < 2) {
        return;
    }
    sprite_h = (int)height;
    sprite_w = sprite_h;
    draw_start_y = WEAPON_VIEW_H - sprite_h;
    if (draw_start_y < 0) {
        draw_start_y = 0;
    }
    draw_end_y = WEAPON_VIEW_H;
    draw_start_x = xcenter - sprite_w / 2;
    draw_end_x = xcenter + sprite_w / 2;

    for (stripe = draw_start_x; stripe < draw_end_x; ++stripe) {
        int tex_x;
        if ((unsigned)stripe >= (unsigned)SCREENWIDTH) {
            continue;
        }
        tex_x = (int)((stripe - draw_start_x) * 64 / sprite_w);
        if ((unsigned)tex_x >= 64u) {
            continue;
        }
        for (y = draw_start_y; y < draw_end_y; ++y) {
            int tex_y = (int)((y - draw_start_y) * 64 / sprite_h);
            byte c;
            if ((unsigned)tex_y >= 64u) {
                continue;
            }
            c = tex[tex_y * 64 + tex_x];
            if (c == 0xFF) {
                continue;
            }
            framebuffer[y * SCREENWIDTH + stripe] = c;
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
    blit_weapon_shape(SCREENWIDTH / 2, shape, (unsigned)(WEAPON_VIEW_H + 1));
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
    {
        player_t *p = &g_level.player;
        int x, y, nz = 0;
        boolean was_active = g_level.active;
        int old_w = p->weapon;
        int old_f = p->weapon_frame;
        int i;

        g_level.active = true;
        p->dead = false;
        p->weapon = 1;
        p->weapon_frame = 0;
        for (i = 0; i < SCREENPIXELS; ++i) {
            framebuffer[i] = 0xFE;
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
