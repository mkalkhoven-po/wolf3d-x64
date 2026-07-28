#include "wl_draw.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "id/id_pm.h"
#include "id/id_vh.h"
#include "id/id_vl.h"
#include "wl_play.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define TEX_SIZE 64
#define VIEW_HEIGHT 160

static float zbuffer[SCREENWIDTH];

static byte sample_wall(int wall_id, int tex_x, int tex_y, int side)
{
    int page;
    const byte *tex;
    int tx = tex_x & 63;
    int ty = tex_y & 63;

    if (wall_id <= 0) {
        wall_id = 1;
    }
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

static void draw_billboard(float ex, float ey, int color, float scale_mul)
{
    player_t *p = &g_level.player;
    float dx = ex - p->x;
    float dy = ey - p->y;
    float dist = dx * cosf(p->angle) + (-dy) * sinf(p->angle);
    float inv_det, transform_x, transform_y;
    int sprite_screen_x, sprite_h, sprite_w;
    int draw_start_y, draw_end_y, draw_start_x, draw_end_x;
    int stripe, y;
    float plane_x, plane_y;

    if (dist <= 0.15f) {
        return;
    }

    /* Camera plane perpendicular to dir */
    plane_x = cosf(p->angle - (float)(M_PI * 0.5)) * 0.66f;
    plane_y = -sinf(p->angle - (float)(M_PI * 0.5)) * 0.66f;

    {
        float dir_x = cosf(p->angle);
        float dir_y = -sinf(p->angle);
        inv_det = 1.0f / (plane_x * dir_y - dir_x * plane_y);
        transform_x = inv_det * (dir_y * dx - dir_x * dy);
        transform_y = inv_det * (-plane_y * dx + plane_x * dy);
    }
    if (transform_y <= 0.05f) {
        return;
    }

    sprite_screen_x = (int)((SCREENWIDTH / 2) * (1.0f + transform_x / transform_y));
    sprite_h = (int)fabsf((float)VIEW_HEIGHT / transform_y * scale_mul);
    sprite_w = sprite_h;
    draw_start_y = -sprite_h / 2 + VIEW_HEIGHT / 2;
    draw_end_y = sprite_h / 2 + VIEW_HEIGHT / 2;
    if (draw_start_y < 0) {
        draw_start_y = 0;
    }
    if (draw_end_y >= VIEW_HEIGHT) {
        draw_end_y = VIEW_HEIGHT - 1;
    }
    draw_start_x = -sprite_w / 2 + sprite_screen_x;
    draw_end_x = sprite_w / 2 + sprite_screen_x;
    if (draw_start_x < 0) {
        draw_start_x = 0;
    }
    if (draw_end_x >= SCREENWIDTH) {
        draw_end_x = SCREENWIDTH - 1;
    }

    for (stripe = draw_start_x; stripe < draw_end_x; ++stripe) {
        if (transform_y >= zbuffer[stripe]) {
            continue;
        }
        for (y = draw_start_y; y < draw_end_y; ++y) {
            /* Simple filled body with outline */
            float v = (float)(y - draw_start_y) / (float)(draw_end_y - draw_start_y + 1);
            byte c = (byte)color;
            if (v < 0.15f) {
                c = (byte)(color + 4);
            }
            framebuffer[y * SCREENWIDTH + stripe] = c;
        }
    }
}

void Wolf_DrawWorld(void)
{
    int x, y, i;
    player_t *p = &g_level.player;
    const float fov = (float)(M_PI / 3.0);
    int view_h = VIEW_HEIGHT;

    if (!framebuffer || !g_level.active) {
        return;
    }

    for (y = 0; y < view_h / 2; ++y) {
        memset(framebuffer + y * SCREENWIDTH, 0x19, SCREENWIDTH);
    }
    for (y = view_h / 2; y < view_h; ++y) {
        memset(framebuffer + y * SCREENWIDTH, 0x18, SCREENWIDTH);
    }
    for (y = view_h; y < SCREENHEIGHT; ++y) {
        byte bar = p->hurt_flash > 0 ? (byte)0x28 : (byte)0x10;
        memset(framebuffer + y * SCREENWIDTH, bar, SCREENWIDTH);
    }

    for (x = 0; x < SCREENWIDTH; ++x) {
        float cam = 2.0f * (float)x / (float)SCREENWIDTH - 1.0f;
        float ray_ang = p->angle + cam * (fov * 0.5f);
        float ray_dir_x = cosf(ray_ang);
        float ray_dir_y = -sinf(ray_ang);

        int map_x = (int)floorf(p->x);
        int map_y = (int)floorf(p->y);

        float delta_dist_x = (ray_dir_x == 0.0f) ? 1e30f : fabsf(1.0f / ray_dir_x);
        float delta_dist_y = (ray_dir_y == 0.0f) ? 1e30f : fabsf(1.0f / ray_dir_y);

        int step_x, step_y;
        float side_dist_x, side_dist_y;
        int hit = 0;
        int side = 0;
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
                    wall_tile = 90;
                }
            } else if (g_level.tilemap[map_x][map_y] > 0 &&
                       g_level.tilemap[map_x][map_y] != ELEVATORTILE) {
                hit = 1;
                wall_tile = g_level.tilemap[map_x][map_y];
            } else if (g_level.tilemap[map_x][map_y] == ELEVATORTILE) {
                hit = 1;
                wall_tile = ELEVATORTILE;
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
        (void)door_hit;
        zbuffer[x] = perp_wall_dist;

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
                c = (byte)((c >> 1) + (c >> 2));
            }
            framebuffer[y * SCREENWIDTH + x] = c;
        }
    }

    /* Pickups as small blips */
    for (i = 0; i < g_level.pickup_count; ++i) {
        if (g_level.pickups[i].active) {
            int col = 14;
            if (g_level.pickups[i].kind == PK_KEY1 ||
                g_level.pickups[i].kind == PK_KEY2) {
                col = 14;
            } else if (g_level.pickups[i].kind == PK_AMMO) {
                col = 7;
            } else if (g_level.pickups[i].kind == PK_MEDKIT ||
                       g_level.pickups[i].kind == PK_FOOD) {
                col = 12;
            }
            draw_billboard(g_level.pickups[i].x, g_level.pickups[i].y, col, 0.35f);
        }
    }

    /* Enemies as colored sprites */
    for (i = 0; i < g_level.enemy_count; ++i) {
        enemy_t *e = &g_level.enemies[i];
        int col = 40;
        if (!e->active || e->state == ST_DEAD) {
            continue;
        }
        switch (e->kind) {
        case EN_DOG:
            col = 6;
            break;
        case EN_SS:
            col = 1;
            break;
        case EN_OFFICER:
            col = 4;
            break;
        case EN_BOSS:
            col = 5;
            break;
        default:
            col = 12;
            break;
        }
        draw_billboard(e->x, e->y, col, 0.85f);
    }

    /* Kill flash */
    if (p->hurt_flash > 0.1f) {
        for (y = 0; y < view_h; y += 2) {
            for (x = 0; x < SCREENWIDTH; x += 2) {
                framebuffer[y * SCREENWIDTH + x] = 0x28;
            }
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
