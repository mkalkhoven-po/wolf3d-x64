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
 * L1.2 / #108 shared projection family (original CalcProjection intent):
 * - Walls: line_h = VIEW_HEIGHT * WALL_HEIGHT_MUL / perp_dist
 * - World sprites: same WALL_HEIGHT_MUL then ScaleShape-style center (#108)
 * - SetupScaling(viewwidth*1.5) → mul ~1.45 vs naive 1/dist (not per-sprite muls)
 * Tune here only — do not fork a second height path for props.
 */
#define WALL_HEIGHT_MUL 1.45f
/* Match wall ray FOV with billboard camera plane (tan(fov/2)). */
#define FOV_RAD ((float)(M_PI * 50.0 / 180.0))
#define CAMERA_PLANE ((float)(0.46630765815)) /* tan(25°) */

/*
 * Original vgaCeiling[] (WL_DRAW.C) — episode*10+mapon, low byte = color.
 * Floor fill uses classic VGA pair 0x19 (see VGAClearScreen floor mov).
 * Shareware E1 maps 0..9.
 */
static const byte vga_ceiling_e1[10] = {
    0x1d, 0x1d, 0x1d, 0x1d, 0x1d, 0x1d, 0x1d, 0x1d, 0x1d, 0xbf
};
#define VGA_FLOOR_COLOR 0x19

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

    /* L1.4: strip pushwall 0xC0 flags; wall pages from low 6 bits only. */
    wall_id &= 63;
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

/*
 * Authentic world-sprite projection (original Transform* + ScaleShape intent):
 * - One depth->height for all world sprites (same WALL_HEIGHT_MUL family as walls)
 * - Vertically centered in the play view (art posts encode floor vs ceiling)
 * - mindist: too close -> do not draw (original MINDIST ~0.34 tile)
 * - No per-type scale_mul / floor-seat hacks
 *
 * Refs: WL_DRAW.C TransformActor/Tile, WL_SCALE.C ScaleShape, WL_MAIN CalcProjection
 */
#define MINDIST_TILES 0.34375f /* original MINDIST 0x5800 / TILEGLOBAL */

static void draw_shape_billboard(float ex, float ey, int shapenum)
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
    int half;

    if (shapenum < 0) {
        return;
    }
    if (!Wolf_SpriteExpand(shapenum, tex)) {
        return;
    }

    plane_x = cosf(p->angle - (float)(M_PI * 0.5)) * CAMERA_PLANE;
    plane_y = -sinf(p->angle - (float)(M_PI * 0.5)) * CAMERA_PLANE;

    {
        float dir_x = cosf(p->angle);
        float dir_y = -sinf(p->angle);
        inv_det = 1.0f / (plane_x * dir_y - dir_x * plane_y);
        transform_x = inv_det * (dir_y * dx - dir_x * dy);
        transform_y = inv_det * (-plane_y * dx + plane_x * dy);
    }

    /*
     * mindist hide (original). No depth floor — flooring made lamps "follow"
     * overhead at constant size while walking under them.
     */
    if (transform_y < MINDIST_TILES) {
        return;
    }

    sprite_screen_x = (int)((SCREENWIDTH / 2) * (1.0f + transform_x / transform_y));
    {
        int top_op = 0, bot_op = 63, yy, xx, found;
        int wall_half;
        int box_y0, box_y1, box_x0;
        int is_ceil = (shapenum == SPR_STAT_CHANDELIER || shapenum == SPR_STAT_CEIL_LIGHT);
        int op_h;
        int wall_mid = VIEW_HEIGHT / 2;

        wall_half = (int)fabsf((float)VIEW_HEIGHT * WALL_HEIGHT_MUL / transform_y) / 2;
        if (wall_half < 1) {
            wall_half = 1;
        }

        found = 0;
        for (yy = 0; yy < 64 && !found; ++yy) {
            for (xx = 0; xx < 64; ++xx) {
                if (tex[yy * 64 + xx] != 0xFF) {
                    top_op = yy;
                    found = 1;
                    break;
                }
            }
        }
        found = 0;
        for (yy = 63; yy >= 0 && !found; --yy) {
            for (xx = 0; xx < 64; ++xx) {
                if (tex[yy * 64 + xx] != 0xFF) {
                    bot_op = yy;
                    found = 1;
                    break;
                }
            }
        }
        if (bot_op < top_op) {
            top_op = 0;
            bot_op = 63;
        }
        op_h = bot_op - top_op + 1;
        if (op_h < 1) {
            op_h = 1;
        }

        /* Opaque span scales with wall height (same family as walls). */
        sprite_h = (int)((float)(wall_half * 2) * ((float)op_h / 64.0f));
        if (sprite_h < 1) {
            return;
        }
        sprite_w = (int)((float)sprite_h * (64.0f / (float)op_h));
        if (sprite_w < 1) {
            sprite_w = 1;
        }
        half = sprite_w / 2;

        if (is_ceil) {
            box_y0 = wall_mid - wall_half;
            box_y1 = box_y0 + sprite_h;
        } else {
            box_y1 = wall_mid + wall_half;
            box_y0 = box_y1 - sprite_h;
        }
        box_x0 = sprite_screen_x - half;

        draw_start_y = box_y0 < 0 ? 0 : box_y0;
        draw_end_y = box_y1 > VIEW_HEIGHT ? VIEW_HEIGHT : box_y1;
        if (draw_end_y <= draw_start_y) {
            return;
        }

        for (stripe = box_x0; stripe < box_x0 + sprite_w; ++stripe) {
            int tex_x;
            if ((unsigned)stripe >= (unsigned)SCREENWIDTH) {
                continue;
            }
            if (transform_y >= zbuffer[stripe]) {
                continue;
            }
            tex_x = ((stripe - box_x0) * 64) / sprite_w;
            if ((unsigned)tex_x >= 64u) {
                continue;
            }
            for (y = draw_start_y; y < draw_end_y; ++y) {
                int tex_y = top_op + ((y - box_y0) * op_h) / (sprite_h > 0 ? sprite_h : 1);
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
}

static int enemy_live_shape(const enemy_t *e)
{
    int phase = ((int)e->anim_t) & 3;

    switch (e->kind) {
    case EN_DOG:
        if (e->state == ST_STAND) {
            return SPR_DOG_W1_1;
        }
        if (e->state == ST_ATTACK) {
            /* JUMP1..3 sit just after DEAD in pack. */
            return SPR_DOG_DEAD + 1 + (phase % 3);
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
    int frame;

    /* die_t 0..0.45 → DIE_1..3, then hold DEAD (pack frames present now). */
    if (e->die_t < 0.45f) {
        frame = (int)(e->die_t / 0.15f);
        if (frame > 2) {
            frame = 2;
        }
        switch (e->kind) {
        case EN_DOG:
            return SPR_DOG_DIE_1 + frame;
        case EN_SS:
            return SPR_SS_DIE_1 + frame;
        case EN_OFFICER:
            return SPR_OFC_DIE_1 + frame;
        case EN_BOSS:
            return SPR_BOSS_DEAD; /* boss die strip differs; hold dead for now */
        case EN_GUARD:
        default:
            return SPR_GRD_DIE_1 + frame;
        }
    }
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

/* Local dir deltas — same as play DI_* (0N 1E 2S 3W). */
static void dir_delta_draw(int dir, int *ox, int *oy)
{
    *ox = 0;
    *oy = 0;
    switch (dir) {
    case 0: *oy = -1; break;
    case 1: *ox = 1; break;
    case 2: *oy = 1; break;
    case 3: *ox = -1; break;
    default: break;
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

    {
        /* L1.3: original solid ceiling from vgaCeiling[map]; floor 0x19. */
        byte ceil_c = 0x1d;
        byte floor_c = (byte)VGA_FLOOR_COLOR;
        int mi = g_level.mapnum;
        if (mi < 0) {
            mi = 0;
        }
        if (mi > 9) {
            mi = 9; /* E2+ tables later; clamp shareware-style */
        }
        ceil_c = vga_ceiling_e1[mi];
        for (y = 0; y < view_h / 2; ++y) {
            memset(framebuffer + y * SCREENWIDTH, ceil_c, SCREENWIDTH);
        }
        for (y = view_h / 2; y < view_h; ++y) {
            memset(framebuffer + y * SCREENWIDTH, floor_c, SCREENWIDTH);
        }
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
                door_t *dd = &g_level.doors[did];
                float openf = dd->open;
                float door_dist;
                float door_x;
                int door_tex;
                int remain;
                int in_gap;

                if (openf >= 0.99f) {
                    /* Fully open — walk-through, keep casting. */
                } else {
                    /* Distance to door plane at center of this cell. */
                    if (side == 0) {
                        door_dist = (map_x - p->x + (1 - step_x) * 0.5f) / ray_dir_x;
                    } else {
                        door_dist = (map_y - p->y + (1 - step_y) * 0.5f) / ray_dir_y;
                    }
                    if (door_dist < 0.05f) {
                        door_dist = 0.05f;
                    }
                    if (side == 0) {
                        door_x = p->y + door_dist * ray_dir_y;
                    } else {
                        door_x = p->x + door_dist * ray_dir_x;
                    }
                    door_x -= floorf(door_x);
                    door_tex = (int)(door_x * (float)TEX_SIZE);
                    if ((side == 0 && ray_dir_x > 0) || (side == 1 && ray_dir_y < 0)) {
                        door_tex = TEX_SIZE - door_tex - 1;
                    }
                    if (door_tex < 0) {
                        door_tex = 0;
                    }
                    if (door_tex >= TEX_SIZE) {
                        door_tex = TEX_SIZE - 1;
                    }
                    if (openf < 0.0f) {
                        openf = 0.0f;
                    }
                    remain = (int)((1.0f - openf) * (float)TEX_SIZE + 0.5f);
                    /* Gap = low U; remaining slab = high U (matches draw path). */
                    in_gap = (remain <= 0) || (door_tex < (TEX_SIZE - remain));
                    if (!in_gap) {
                        hit = 1;
                        door_hit = did;
                        wall_tile = 90;
                    }
                    /* else: open gap — continue DDA to walls beyond (no grey void) */
                }
            } else {
                byte tm = g_level.tilemap[map_x][map_y];
                byte tid = (byte)(tm & TILE_WALL_MASK);
                int is_pwall_cell = 0;

                if (g_level.pwall_state > 0) {
                    int lox = 0, loy = 0;
                    dir_delta_draw(g_level.pwall_dir, &lox, &loy);
                    /* Hit either origin or leading cell — both form the moving segment. */
                    if ((map_x == g_level.pwall_x && map_y == g_level.pwall_y) ||
                        (map_x == g_level.pwall_x + lox &&
                         map_y == g_level.pwall_y + loy)) {
                        is_pwall_cell = 1;
                    }
                }

                if (is_pwall_cell) {
                    hit = 1;
                    wall_tile = g_level.pwall_tile ? g_level.pwall_tile : tid;
                    if (!wall_tile) {
                        wall_tile = 1;
                    }
                    door_hit = -2; /* sliding pushwall face */
                } else if (tid == ELEVATORTILE) {
                    hit = 1;
                    wall_tile = ELEVATORTILE;
                } else if (tid > 0) {
                    hit = 1;
                    wall_tile = tid;
                }
            }
        }

        if (side == 0) {
            perp_wall_dist = (map_x - p->x + (1 - step_x) * 0.5f) / ray_dir_x;
        } else {
            perp_wall_dist = (map_y - p->y + (1 - step_y) * 0.5f) / ray_dir_y;
        }

        /*
         * Sliding face: world position = origin + (block_index + pos/64) along dir.
         * One continuous plane so the wood panel moves sideways instead of vanishing
         * into a wall behind the secret cell.
         */
        if (door_hit == -2 && g_level.pwall_state > 0) {
            float moved = (float)(g_level.pwall_state / 2) / 64.0f; /* tiles moved 0..~2 */
            float fx = (float)g_level.pwall_x + 0.5f;
            float fy = (float)g_level.pwall_y + 0.5f;
            int pdir = g_level.pwall_dir;
            float plane;
            float dist;
            float wx, wy;

            if (pdir == 1) { /* E */
                fx += moved;
            } else if (pdir == 3) { /* W */
                fx -= moved;
            } else if (pdir == 2) { /* S */
                fy += moved;
            } else { /* N */
                fy -= moved;
            }

            /* Plane through the moving face, perpendicular to push dir. */
            if (pdir == 1 || pdir == 3) {
                plane = fx;
                if (fabsf(ray_dir_x) > 1e-6f) {
                    dist = (plane - p->x) / ray_dir_x;
                    if (dist > 0.05f) {
                        perp_wall_dist = dist;
                        side = 0;
                    }
                }
            } else {
                plane = fy;
                if (fabsf(ray_dir_y) > 1e-6f) {
                    dist = (plane - p->y) / ray_dir_y;
                    if (dist > 0.05f) {
                        perp_wall_dist = dist;
                        side = 1;
                    }
                }
            }
            wall_tile = g_level.pwall_tile ? g_level.pwall_tile : wall_tile;
            door_hit = -1;
        }

        if (perp_wall_dist < 0.05f) {
            perp_wall_dist = 0.05f;
        }
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
        if (tex_x < 0) {
            tex_x = 0;
        }
        if (tex_x >= TEX_SIZE) {
            tex_x = TEX_SIZE - 1;
        }

        if (door_hit >= 0) {
            /* Ray only hits remaining slab (gap continues DDA past door). */
            door_t *dd = &g_level.doors[door_hit];
            float openf = dd->open;
            int remain;
            int u;
            int sample_u;
            if (openf < 0.0f) {
                openf = 0.0f;
            }
            if (openf > 1.0f) {
                openf = 1.0f;
            }
            remain = (int)((1.0f - openf) * (float)TEX_SIZE + 0.5f);
            if (remain < 1) {
                remain = 1;
            }
            u = tex_x - (TEX_SIZE - remain);
            if (u < 0) {
                u = 0;
            }
            sample_u = u * TEX_SIZE / remain;
            if (sample_u >= TEX_SIZE) {
                sample_u = TEX_SIZE - 1;
            }
            for (y = draw_start; y <= draw_end; ++y) {
                int d = y * 256 - view_h * 128 + line_h * 128;
                int tex_y = ((d * TEX_SIZE) / line_h) / 256;
                byte c = sample_door(dd->lock, sample_u, tex_y);
                framebuffer[y * SCREENWIDTH + x] = c;
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

    /* Decorative statics — same projection as actors (authentic). */
    for (i = 0; i < g_level.decor_count; ++i) {
        if (g_level.decor[i].active) {
            draw_shape_billboard(g_level.decor[i].x, g_level.decor[i].y,
                                 g_level.decor[i].shapenum);
        }
    }

    /* Pickups */
    for (i = 0; i < g_level.pickup_count; ++i) {
        if (g_level.pickups[i].active) {
            draw_shape_billboard(g_level.pickups[i].x, g_level.pickups[i].y,
                                 g_level.pickups[i].shapenum);
        }
    }

    /* Living enemies + corpses */
    for (i = 0; i < g_level.enemy_count; ++i) {
        enemy_t *e = &g_level.enemies[i];
        int shape;
        if (!e->active) {
            continue;
        }
        if (e->state == ST_DEAD) {
            shape = enemy_dead_shape(e);
        } else {
            shape = enemy_live_shape(e);
        }
        draw_shape_billboard(e->x, e->y, shape);
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
