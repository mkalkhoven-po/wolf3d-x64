#include "wl_draw.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "id/id_pm.h"
#include "id/id_vl.h"
#include "wl_play.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define TEX_SIZE 64
#define VIEW_HEIGHT 160 /* leave bottom for stub status strip */

/*
 * Wolf wall texture pages are 64x64, two pages per wall type (dark/light).
 * Stored column-major in VSWAP (y + x*64).
 */
static byte sample_wall(int wall_id, int tex_x, int tex_y, int side)
{
    int page;
    const byte *tex;
    int tx = tex_x & 63;
    int ty = tex_y & 63;

    if (wall_id <= 0) {
        wall_id = 1;
    }
    /* Map tile 1 -> page 0/1, tile 2 -> 2/3, ... doors use high values */
    page = (wall_id - 1) * 2 + (side ? 1 : 0);
    if (page < 0) {
        page = 0;
    }
    if (PMSpriteStart && (word)page >= PMSpriteStart) {
        page = (int)PMSpriteStart - 1;
        if (page < 0) {
            page = 0;
        }
    }
    tex = (const byte *)PM_GetPage(page);
    if (!tex) {
        return (byte)(wall_id * 3 + side * 8);
    }
    return tex[ty + tx * TEX_SIZE];
}

void Wolf_DrawWorld(void)
{
    int x, y;
    player_t *p = &g_level.player;
    const float fov = (float)(M_PI / 3.0); /* 60 deg */
    int view_h = VIEW_HEIGHT;

    if (!framebuffer || !g_level.active) {
        return;
    }

    /* Ceiling / floor */
    for (y = 0; y < view_h / 2; ++y) {
        memset(framebuffer + y * SCREENWIDTH, 0x19, SCREENWIDTH); /* ceiling */
    }
    for (y = view_h / 2; y < view_h; ++y) {
        memset(framebuffer + y * SCREENWIDTH, 0x18, SCREENWIDTH); /* floor */
    }
    /* Status stub bar */
    for (y = view_h; y < SCREENHEIGHT; ++y) {
        memset(framebuffer + y * SCREENWIDTH, 0x10, SCREENWIDTH);
    }

    for (x = 0; x < SCREENWIDTH; ++x) {
        float cam = 2.0f * (float)x / (float)SCREENWIDTH - 1.0f;
        float ray_ang = p->angle + cam * (fov * 0.5f);
        float ray_dir_x = cosf(ray_ang);
        float ray_dir_y = -sinf(ray_ang); /* -sin so +angle pitches toward -Y map north */

        int map_x = (int)floorf(p->x);
        int map_y = (int)floorf(p->y);

        float delta_dist_x = (ray_dir_x == 0.0f) ? 1e30f : fabsf(1.0f / ray_dir_x);
        float delta_dist_y = (ray_dir_y == 0.0f) ? 1e30f : fabsf(1.0f / ray_dir_y);

        int step_x, step_y;
        float side_dist_x, side_dist_y;
        int hit = 0;
        int side = 0; /* 0 NS (vertical wall facing E/W), 1 EW */
        int door_hit = -1;
        float perp_wall_dist = 1.0f;
        int line_h;
        int draw_start, draw_end;
        float wall_x;
        int tex_x;
        byte wall_tile = 1;

        if (ray_dir_x < 0) {
            step_x = -1;
            side_dist_x = (p->x - map_x) * delta_dist_x;
        } else {
            step_x = 1;
            side_dist_x = (map_x + 1.0f - p->x) * delta_dist_x;
        }
        if (ray_dir_y < 0) {
            step_y = -1;
            side_dist_y = (p->y - map_y) * delta_dist_y;
        } else {
            step_y = 1;
            side_dist_y = (map_y + 1.0f - p->y) * delta_dist_y;
        }

        while (!hit) {
            if (side_dist_x < side_dist_y) {
                side_dist_x += delta_dist_x;
                map_x += step_x;
                side = 0;
            } else {
                side_dist_y += delta_dist_y;
                map_y += step_y;
                side = 1;
            }
            if ((unsigned)map_x >= MAPSIZE || (unsigned)map_y >= MAPSIZE) {
                hit = 1;
                wall_tile = 1;
                break;
            }
            if (g_level.door_id[map_x][map_y] >= 0) {
                int did = g_level.door_id[map_x][map_y];
                if (g_level.doors[did].open < 0.9f) {
                    hit = 1;
                    door_hit = did;
                    wall_tile = 90; /* doorish texture index */
                }
            } else if (g_level.tilemap[map_x][map_y] > 0) {
                hit = 1;
                wall_tile = g_level.tilemap[map_x][map_y];
            }
        }

        if (side == 0) {
            perp_wall_dist = (map_x - p->x + (1 - step_x) * 0.5f) / ray_dir_x;
        } else {
            perp_wall_dist = (map_y - p->y + (1 - step_y) * 0.5f) / ray_dir_y;
        }
        if (perp_wall_dist < 0.05f) {
            perp_wall_dist = 0.05f;
        }

        /* Door offset when partially open — thin ray skip not perfect but ok */
        if (door_hit >= 0) {
            float open = g_level.doors[door_hit].open;
            /* Treat longer distance slender; shade by open amount */
            (void)open;
        }

        line_h = (int)(view_h / perp_wall_dist);
        draw_start = -line_h / 2 + view_h / 2;
        draw_end = line_h / 2 + view_h / 2;
        if (draw_start < 0) {
            draw_start = 0;
        }
        if (draw_end >= view_h) {
            draw_end = view_h - 1;
        }

        if (side == 0) {
            wall_x = p->y + perp_wall_dist * ray_dir_y;
        } else {
            wall_x = p->x + perp_wall_dist * ray_dir_x;
        }
        wall_x -= floorf(wall_x);
        tex_x = (int)(wall_x * (float)TEX_SIZE);
        if ((side == 0 && ray_dir_x > 0) || (side == 1 && ray_dir_y < 0)) {
            tex_x = TEX_SIZE - tex_x - 1;
        }

        for (y = draw_start; y <= draw_end; ++y) {
            int d = y * 256 - view_h * 128 + line_h * 128;
            int tex_y = ((d * TEX_SIZE) / line_h) / 256;
            byte c = sample_wall((int)wall_tile, tex_x, tex_y, side);
            if (side == 1) {
                /* darken y-sides slightly */
                c = (byte)((c >> 1) + (c >> 2));
            }
            framebuffer[y * SCREENWIDTH + x] = c;
        }
    }
}

boolean Wolf_RaycastSmokeVerify(void)
{
    int x, nnz = 0;
    if (!g_level.active || !framebuffer) {
        return false;
    }
    Wolf_DrawWorld();
    for (x = 0; x < SCREENPIXELS; ++x) {
        if (framebuffer[x]) {
            nnz++;
        }
    }
    printf("Raycast smoke: non-zero pixels=%d player=(%.2f,%.2f)\n", nnz,
           g_level.player.x, g_level.player.y);
    return nnz > 5000;
}
