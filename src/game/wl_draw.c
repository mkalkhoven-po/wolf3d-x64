#include "wl_draw.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "id/id_pm.h"
#include "id/wl_sprite.h"
#include "id/id_vh.h"
#include "id/id_vl.h"
#include "wl_play.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define TEX_SIZE 64
/* Playview is 160px with 40px status strip (classic STATUSLINES). */
#define VIEW_HEIGHT 160
/*
 * Original seales with SetupScaling(viewwidth*1.5). A pure 1/dist
 * line height looks too squat; multiply project height toward that.
 */
#define WALL_HEIGHT_MUL 1.45f
/* Match wall ray FOV with billboard camera plane (tan(fov/2)). */
#define FOV_RAD ((float)(M_PI * 50.0 / 180.0))
#define CAMERA_PLANE ((float)(0.46630765815)) /* tan(25°) */

static float zbuffer[SCREENWIDTH];

/* Door wall pages live just before sprite pages (original DOORWALL). */
static int door_wall_base(void)
{
    if (PMSpriteStart >= 8) {
        return (int)PMSpriteStart - 8;
    }
    return 0;
}

/*
 * Sample a VSWAP wall/door page. page is absolute VSWAP page index.
 * page_is_door: skip wallish (wall_id-1)*2 mapping.
 */
static byte sample_page(int page, int tex_x, int tex_y)
{
    const byte *tex;
    int tx = tex_x & 63;
    int ty = tex_y & 63;

    if (page < 0) {
        page = 0;
    }
    /* Door pages are PMSpriteStart-8 .. SpriteStart-1; walls [0, SpriteStart). */
    if (PMSoundStart && (word)page >= PMSoundStart) {
        page = (int)PMSoundStart - 1;
        if (page < 0) {
            page = 0;
        }
    }
    tex = (const byte *)PM_GetPage(page);
    if (!tex) {
        return 0;
    }
    return tex[ty + tx * TEX_SIZE];
}

static byte sample_wall(int wall_id, int tex_x, int tex_y, int side)
{
    int page;

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
    return sample_page(page, tex_x, tex_y);
}

/* lock: 0 normal, 1-4 keys, 5 elevator (dr_*). Matches Hit*Door switch. */
static byte sample_door(int lock, int tex_x, int tex_y)
{
    int base = door_wall_base();
    int page = base; /* dr_normal */

    if (lock >= 1 && lock <= 4) {
        page = base + 6; /* locked doors */
    } else if (lock >= 5) {
        page = base + 4; /* elevator door */
    }
    return sample_page(page, tex_x, tex_y);
}

#define SPR_PLACE_FLOOR 0
#define SPR_PLACE_CEIL 1

/*
 * Textured billboard. Height tracks wall projection so scale matches walls.
 * Floor: feet on wall-bottom. Ceiling fixtures: hang from wall-top.
 */
static void draw_shape_billboard(float ex, float ey, int shapenum, float scale_mul,
                                 int placement)
{
    player_t *p = &g_level.player;
    float dx = ex - p->x;
    float dy = ey - p->y;
    float inv_det, transform_x, transform_y;
    int sprite_screen_x, sprite_h, sprite_w;
    int draw_start_y, draw_end_y, draw_start_x, draw_end_x;
    int stripe, y;
    float plane_x, plane_y;
    byte tex[64 * 64];
    int v_mid;
    int wall_h;
    int top_op, bot_op, op_h;

    if (shapenum < 0) {
        return;
    }
    if (!Wolf_SpriteExpand(shapenum, tex)) {
        return;
    }
    top_op = 0;
    bot_op = 63;
    {
        int yy, xx, found;
        found = 0;
        for (yy = 0; yy < 64; ++yy) {
            for (xx = 0; xx < 64; ++xx) {
                if (tex[yy * 64 + xx] != 0xFF) {
                    top_op = yy;
                    found = 1;
                    break;
                }
            }
            if (found) {
                break;
            }
        }
        found = 0;
        for (yy = 63; yy >= 0; --yy) {
            for (xx = 0; xx < 64; ++xx) {
                if (tex[yy * 64 + xx] != 0xFF) {
                    bot_op = yy;
                    found = 1;
                    break;
                }
            }
            if (found) {
                break;
            }
        }
        if (bot_op < top_op) {
            top_op = 0;
            bot_op = 63;
        }
    }
    op_h = bot_op - top_op + 1;

    plane_x = cosf(p->angle - (float)(M_PI * 0.5)) * CAMERA_PLANE;
    plane_y = -sinf(p->angle - (float)(M_PI * 0.5)) * CAMERA_PLANE;

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
    wall_h = (int)fabsf((float)VIEW_HEIGHT * WALL_HEIGHT_MUL / transform_y);
    if (wall_h < 1) {
        wall_h = 1;
    }
    sprite_h = (int)((float)wall_h * scale_mul);
    if (sprite_h < 1) {
        return;
    }
    sprite_w = sprite_h;
    /* Project only the opaque row range so empty padding cannot float the art. */
    {
        int full_h = sprite_h;
        int op_pix = (op_h * full_h + 32) / 64;
        if (op_pix < 1) {
            op_pix = 1;
        }
        sprite_h = op_pix;
    }

    v_mid = VIEW_HEIGHT / 2;
    if (placement == SPR_PLACE_CEIL) {
        draw_start_y = v_mid - wall_h / 2;
        draw_end_y = draw_start_y + sprite_h;
    } else {
        draw_end_y = v_mid + wall_h / 2;
        draw_start_y = draw_end_y - sprite_h;
    }
    if (draw_start_y < 0) {
        draw_start_y = 0;
    }
    if (draw_end_y >= VIEW_HEIGHT) {
        draw_end_y = VIEW_HEIGHT - 1;
    }
    if (draw_end_y <= draw_start_y) {
        return;
    }
    draw_start_x = -sprite_w / 2 + sprite_screen_x;
    draw_end_x = sprite_w / 2 + sprite_screen_x;

    for (stripe = draw_start_x; stripe < draw_end_x; ++stripe) {
        int tex_x;
        int span_h = draw_end_y - draw_start_y + 1;
        if ((unsigned)stripe >= (unsigned)SCREENWIDTH) {
            continue;
        }
        if (transform_y >= zbuffer[stripe]) {
            continue;
        }
        tex_x = (int)((stripe - (-sprite_w / 2 + sprite_screen_x)) * 64 / sprite_w);
        if ((unsigned)tex_x >= 64u) {
            continue;
        }
        for (y = draw_start_y; y <= draw_end_y; ++y) {
            int tex_y = top_op + ((y - draw_start_y) * op_h) / span_h;
            byte c;
            if (tex_y < top_op) {
                tex_y = top_op;
            }
            if (tex_y > bot_op) {
                tex_y = bot_op;
            }
            c = tex[tex_y * 64 + tex_x];
            if (c == 0xFF) {
                continue;
            }
            framebuffer[y * SCREENWIDTH + stripe] = c;
        }
    }
}

static int is_ceiling_stat(int shapenum)
{
    return shapenum == (SPR_STAT_BASE + 4) || shapenum == (SPR_STAT_BASE + 14);
}

static int enemy_live_shape(const enemy_t *e)
{
    int phase = ((int)e->anim_t) & 3;

    switch (e->kind) {
    case EN_DOG:
        if (e->state == ST_STAND) {
            return SPR_DOG_W1_1;
        }
        return SPR_DOG_W1_1 + phase * 8;
    case EN_SS:
        if (e->state == ST_STAND) {
            return SPR_SS_S1;
        }
        return SPR_SS_W1_1 + phase * 8;
    case EN_OFFICER:
        if (e->state == ST_STAND) {
            return SPR_OFC_S1;
        }
        return SPR_OFC_W1_1 + phase * 8;
    case EN_BOSS:
        return SPR_BOSS_W1 + (phase & 3);
    case EN_GUARD:
    default:
        if (e->state == ST_STAND) {
            return SPR_GRD_S1;
        }
        return SPR_GRD_W1_1 + phase * 8;
    }
}

static int enemy_dead_shape(const enemy_t *e)
{
    switch (e->kind) {
    case EN_DOG:
        return SPR_DOG_DEAD;
    case EN_SS:
        return SPR_SS_DEAD;
    case EN_OFFICER:
        return SPR_OFC_DEAD;
    case EN_BOSS:
        return SPR_BOSS_DEAD;
    case EN_GUARD:
    default:
        return SPR_GRD_DEAD;
    }
}

void Wolf_DrawWorld(void)
{
    int x, y, i;
    player_t *p = &g_level.player;
    int view_h = VIEW_HEIGHT;
    (void)FOV_RAD;

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

    {
        float dir_x = cosf(p->angle);
        float dir_y = -sinf(p->angle);
        float plane_x = cosf(p->angle - (float)(M_PI * 0.5)) * CAMERA_PLANE;
        float plane_y = -sinf(p->angle - (float)(M_PI * 0.5)) * CAMERA_PLANE;
    for (x = 0; x < SCREENWIDTH; ++x) {
        float cam = 2.0f * (float)x / (float)SCREENWIDTH - 1.0f;
        float ray_dir_x = dir_x + plane_x * cam;
        float ray_dir_y = dir_y + plane_y * cam;

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

        line_h = (int)((view_h * WALL_HEIGHT_MUL) / perp_wall_dist);
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

        if (door_hit >= 0) {
            /* Sliding open: only lower portion of tex u remains as open rises. */
            door_t *dd = &g_level.doors[door_hit];
            float openf = dd->open;
            int remain;
            if (openf < 0.0f) {
                openf = 0.0f;
            }
            if (openf > 1.0f) {
                openf = 1.0f;
            }
            remain = (int)((1.0f - openf) * (float)TEX_SIZE + 0.5f);
            if (remain > 0 && tex_x < remain) {
                for (y = draw_start; y <= draw_end; ++y) {
                    int d = y * 256 - view_h * 128 + line_h * 128;
                    int tex_y = ((d * TEX_SIZE) / line_h) / 256;
                    byte c = sample_door(dd->lock, tex_x, tex_y);
                    framebuffer[y * SCREENWIDTH + x] = c;
                }
            }
            /* When mostly open, do not occlude sprites beyond the doorway. */
            if (openf > 0.55f) {
                zbuffer[x] = 1e30f;
            }
        } else {
            for (y = draw_start; y <= draw_end; ++y) {
                int d = y * 256 - view_h * 128 + line_h * 128;
                int tex_y = ((d * TEX_SIZE) / line_h) / 256;
                byte c = sample_wall((int)wall_tile, tex_x, tex_y, side);
                (void)side;
                framebuffer[y * SCREENWIDTH + x] = c;
            }
        }
    }
    } /* dir/plane scope */

    /* Decorative statics (lamps, plants, furniture, ...) */
    for (i = 0; i < g_level.decor_count; ++i) {
        if (g_level.decor[i].active) {
            int place = is_ceiling_stat(g_level.decor[i].shapenum) ? SPR_PLACE_CEIL
                                                                    : SPR_PLACE_FLOOR;
            float sc = is_ceiling_stat(g_level.decor[i].shapenum) ? 0.35f : 0.90f;
            draw_shape_billboard(g_level.decor[i].x, g_level.decor[i].y,
                                 g_level.decor[i].shapenum, sc, place);
        }
    }

    /* Pickups on the floor */
    for (i = 0; i < g_level.pickup_count; ++i) {
        if (g_level.pickups[i].active) {
            draw_shape_billboard(g_level.pickups[i].x, g_level.pickups[i].y,
                                 g_level.pickups[i].shapenum, 0.35f, SPR_PLACE_FLOOR);
        }
    }

    /* Living enemies + corpses (grounded) */
    for (i = 0; i < g_level.enemy_count; ++i) {
        enemy_t *e = &g_level.enemies[i];
        int shape;
        float sc;
        if (!e->active) {
            continue;
        }
        if (e->state == ST_DEAD) {
            shape = enemy_dead_shape(e);
            sc = 0.55f;
        } else {
            shape = enemy_live_shape(e);
            sc = (e->kind == EN_DOG) ? 0.55f : 0.95f;
        }
        draw_shape_billboard(e->x, e->y, shape, sc, SPR_PLACE_FLOOR);
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
