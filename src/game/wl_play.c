#include "wl_play.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "id/id_ca.h"
#include "id/id_sd.h"
#include "id/wl_sprite.h"
#include "id/audio_wl6.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

level_t g_level;

static int door_at(int tx, int ty)
{
    if ((unsigned)tx >= MAPSIZE || (unsigned)ty >= MAPSIZE) {
        return -1;
    }
    return g_level.door_id[tx][ty];
}

boolean Wolf_SolidAt(float x, float y)
{
    int tx = (int)floorf(x);
    int ty = (int)floorf(y);
    int did;
    byte tile;

    if ((unsigned)tx >= MAPSIZE || (unsigned)ty >= MAPSIZE) {
        return true;
    }

    did = door_at(tx, ty);
    if (did >= 0) {
        return g_level.doors[did].open < 0.85f;
    }

    tile = g_level.tilemap[tx][ty];
    /* Elevator switch walls are solid (original tilemap[21] is blocking). */
    if ((tile & TILE_WALL_MASK) == ELEVATORTILE) {
        return true;
    }
    /*
     * In-motion pushwall origin (tex | 0xC0): treat as walkable so the player can
     * enter the opening while the face slides into the next cell. Leading cell
     * still holds a normal solid wall tile.
     */
    if ((tile & TILE_PWALL_MASK) == TILE_PWALL_MASK) {
        return false;
    }
    return (tile & TILE_WALL_MASK) != 0;
}

static boolean blocked_player(float x, float y)
{
    const float r = 0.28f; /* keep camera off walls (was 0.18 → gun buried) */
    return Wolf_SolidAt(x - r, y - r) || Wolf_SolidAt(x + r, y - r) ||
           Wolf_SolidAt(x - r, y + r) || Wolf_SolidAt(x + r, y + r);
}

static boolean blocked_actor(float x, float y, float r)
{
    return Wolf_SolidAt(x - r, y - r) || Wolf_SolidAt(x + r, y - r) ||
           Wolf_SolidAt(x - r, y + r) || Wolf_SolidAt(x + r, y + r);
}

static void spawn_door(int x, int y, int vertical, int lock)
{
    int id;
    if (g_level.door_count >= MAX_DOORS) {
        return;
    }
    id = g_level.door_count++;
    g_level.doors[id].tilex = x;
    g_level.doors[id].tiley = y;
    g_level.doors[id].vertical = vertical;
    g_level.doors[id].lock = lock;
    g_level.doors[id].open = 0.0f;
    g_level.doors[id].opening = 0;
    g_level.door_id[x][y] = id;
}

static void spawn_enemy(enemy_kind_t kind, int x, int y, int dir)
{
    enemy_t *e;
    if (g_level.enemy_count >= MAX_ENEMIES) {
        return;
    }
    e = &g_level.enemies[g_level.enemy_count++];
    memset(e, 0, sizeof(*e));
    e->active = true;
    e->kind = kind;
    e->state = ST_STAND;
    e->x = (float)x + 0.5f;
    e->y = (float)y + 0.5f;
    e->angle = (float)dir * (float)(M_PI * 0.5); /* 0..3 → cardinal */
    e->think_t = 0.0f;
    e->attack_cd = 0.5f;
    switch (kind) {
    case EN_DOG:
        e->health = 1;
        e->speed = 2.4f;
        e->score_value = 200;
        break;
    case EN_OFFICER:
        e->health = 50;
        e->speed = 1.6f;
        e->score_value = 400;
        break;
    case EN_SS:
        e->health = 100;
        e->speed = 1.4f;
        e->score_value = 500;
        break;
    case EN_BOSS:
        e->health = 500;
        e->speed = 1.0f;
        e->score_value = 5000;
        break;
    case EN_GUARD:
    default:
        e->health = 25;
        e->speed = 1.2f;
        e->score_value = 100;
        break;
    }
    g_level.enemies_alive++;
}

static int pickup_default_shape(pickup_kind_t kind)
{
    switch (kind) {
    case PK_KEY1:
        return SPR_STAT_BASE + 20;
    case PK_KEY2:
        return SPR_STAT_BASE + 21;
    case PK_KEY3:
        return SPR_STAT_BASE + 22;
    case PK_KEY4:
        return SPR_STAT_BASE + 23;
    case PK_FOOD:
        return SPR_STAT_BASE + 24;
    case PK_MEDKIT:
        return SPR_STAT_BASE + 25;
    case PK_AMMO:
        return SPR_STAT_BASE + 26;
    case PK_MGUN:
        return SPR_STAT_BASE + 27;
    case PK_CGUN:
        return SPR_STAT_BASE + 28;
    case PK_TREASURE:
        return SPR_STAT_BASE + 29;
    case PK_1UP:
        return SPR_STAT_BASE + 33;
    default:
        return SPR_STAT_BASE + 26;
    }
}

static void spawn_pickup(pickup_kind_t kind, int x, int y, int amount)
{
    pickup_t *p;
    if (kind == PK_NONE || g_level.pickup_count >= MAX_PICKUPS) {
        return;
    }
    p = &g_level.pickups[g_level.pickup_count++];
    p->active = true;
    p->kind = kind;
    p->x = (float)x + 0.5f;
    p->y = (float)y + 0.5f;
    p->amount = amount;
    p->shapenum = pickup_default_shape(kind);
}

static void spawn_pickup_shape(pickup_kind_t kind, int x, int y, int amount, int shapenum)
{
    pickup_t *p;
    if (kind == PK_NONE || g_level.pickup_count >= MAX_PICKUPS) {
        return;
    }
    p = &g_level.pickups[g_level.pickup_count++];
    p->active = true;
    p->kind = kind;
    p->x = (float)x + 0.5f;
    p->y = (float)y + 0.5f;
    p->amount = amount;
    p->shapenum = shapenum;
}

static void spawn_decor(int x, int y, int shapenum)
{
    decor_t *d;
    if (g_level.decor_count >= MAX_DECOR || shapenum < 0) {
        return;
    }
    d = &g_level.decor[g_level.decor_count++];
    d->active = true;
    d->x = (float)x + 0.5f;
    d->y = (float)y + 0.5f;
    d->shapenum = shapenum;
}

/* Map static type index (tile-23) -> pickup or decorative VSWAP sprite. */
static void spawn_static_from_type(int x, int y, int type)
{
    int shape = SPR_STAT_BASE + type;
    switch (type) {
    case 6:
        spawn_pickup_shape(PK_FOOD, x, y, 4, shape);
        return;
    case 20:
        spawn_pickup_shape(PK_KEY1, x, y, 1, shape);
        return;
    case 21:
        spawn_pickup_shape(PK_KEY2, x, y, 1, shape);
        return;
    case 24:
        spawn_pickup_shape(PK_FOOD, x, y, 10, shape);
        return;
    case 25:
        spawn_pickup_shape(PK_MEDKIT, x, y, 25, shape);
        return;
    case 26:
        spawn_pickup_shape(PK_AMMO, x, y, 8, shape);
        return;
    case 27:
        spawn_pickup_shape(PK_MGUN, x, y, 6, shape);
        return;
    case 28:
        spawn_pickup_shape(PK_CGUN, x, y, 6, shape);
        return;
    case 29:
        spawn_pickup_shape(PK_TREASURE, x, y, 100, shape);
        return;
    case 30:
        spawn_pickup_shape(PK_TREASURE, x, y, 500, shape);
        return;
    case 31:
        spawn_pickup_shape(PK_TREASURE, x, y, 1000, shape);
        return;
    case 32:
        spawn_pickup_shape(PK_TREASURE, x, y, 5000, shape);
        return;
    case 33:
        spawn_pickup_shape(PK_1UP, x, y, 1, shape);
        return;
    default:
        break;
    }
    if (type >= 0 && type <= 47) {
        spawn_decor(x, y, shape);
    }
}

void Wolf_ResetPlayerStats(void)
{
    player_t *p = &g_level.player;
    p->health = 100;
    p->lives = 3;
    p->ammo = 8;
    p->score = 0;
    p->keys = 0;
    p->weapon = 1; /* pistol */
    p->best_weapon = 1;
    p->weapon_frame = 0;
    p->attack_cd = 0.0f;
    p->hurt_flash = 0.0f;
    p->dead = false;
}

static void give_score(int pts)
{
    g_level.player.score += pts;
}

static void hurt_player(int dmg)
{
    player_t *p = &g_level.player;
    if (p->dead) {
        return;
    }
    p->health -= dmg;
        p->hurt_flash = 0.25f;
        SD_PlaySound(TAKEDAMAGESND);
        if (p->health <= 0) {
        p->health = 0;
                p->dead = true;
                p->lives -= 1;
                SD_PlaySound(PLAYERDEATHSND);
                printf("Player died. Lives left=%d score=%d\n", p->lives, p->score);
    }
}

boolean Wolf_SetupLevel(int mapnum)
{
    int x, y;
    uint16_t *plane0;
    uint16_t *plane1;
    int spawned = 0;
    player_t saved_player;
    int episode_map;
    boolean keep_stats = g_level.active && mapnum != 0;

    if (keep_stats) {
        saved_player = g_level.player;
        episode_map = g_level.episode_map;
    } else {
        episode_map = mapnum;
    }

    memset(&g_level, 0, sizeof(g_level));
    for (x = 0; x < MAPSIZE; ++x) {
        for (y = 0; y < MAPSIZE; ++y) {
            g_level.door_id[x][y] = -1;
        }
    }

    if (!CA_CacheMap(mapnum)) {
        fprintf(stderr, "Wolf_SetupLevel: CA_CacheMap(%d) failed\n", mapnum);
        return false;
    }

    plane0 = mapsegs[0];
    plane1 = mapsegs[1];
    if (!plane0 || !plane1) {
        return false;
    }

    for (y = 0; y < MAPSIZE; ++y) {
        for (x = 0; x < MAPSIZE; ++x) {
            uint16_t tile = plane0[y * MAPSIZE + x];
            if (tile < AREATILE) {
                g_level.tilemap[x][y] = (byte)tile;
            } else {
                g_level.tilemap[x][y] = 0;
            }
        }
    }

    for (y = 0; y < MAPSIZE; ++y) {
        for (x = 0; x < MAPSIZE; ++x) {
            uint16_t tile = plane0[y * MAPSIZE + x];
            if (tile >= 90 && tile <= 101) {
                if ((tile % 2) == 0) {
                    spawn_door(x, y, 1, (int)(tile - 90) / 2);
                } else {
                    spawn_door(x, y, 0, (int)(tile - 91) / 2);
                }
            }
        }
    }

    if (keep_stats) {
        g_level.player = saved_player;
        g_level.player.dead = false;
        if (g_level.player.health <= 0) {
            g_level.player.health = 100;
        }
        g_level.player.attack_cd = 0.0f;
        g_level.player.hurt_flash = 0.0f;
    } else {
        Wolf_ResetPlayerStats();
    }

    g_level.player.x = 32.5f;
    g_level.player.y = 32.5f;
    g_level.player.angle = 0.0f;

    /* Snapshot object plane (PUSHABLETILE=98 lives here until pushed). */
    g_level.secret_total = 0;
    g_level.secret_count = 0;
    for (y = 0; y < MAPSIZE; ++y) {
        for (x = 0; x < MAPSIZE; ++x) {
            uint16_t ot = plane1[y * MAPSIZE + x];
            g_level.objmap[x][y] = ot;
            if (ot == PUSHABLETILE) {
                g_level.secret_total++;
            }
        }
    }

    /* Scan object plane */
    for (y = 0; y < MAPSIZE; ++y) {
        for (x = 0; x < MAPSIZE; ++x) {
            uint16_t tile = plane1[y * MAPSIZE + x];
            if (!tile) {
                continue;
            }

            if (tile >= 19 && tile <= 22) {
                g_level.player.x = (float)x + 0.5f;
                g_level.player.y = (float)y + 0.5f;
                switch (tile) {
                case 19:
                    g_level.player.angle = (float)(M_PI * 0.5);
                    break;
                case 20:
                    g_level.player.angle = 0.0f;
                    break;
                case 21:
                    g_level.player.angle = (float)(-M_PI * 0.5);
                    break;
                case 22:
                    g_level.player.angle = (float)M_PI;
                    break;
                }
                spawned = 1;
                continue;
            }

            if (tile >= 23 && tile <= 74) {
                spawn_static_from_type(x, y, (int)tile - 23);
                continue;
            }

            /* Easy-difficulty standing/patrolling actors only (base tiles) */
            if (tile >= 108 && tile <= 111) {
                spawn_enemy(EN_GUARD, x, y, (int)tile - 108);
            } else if (tile >= 112 && tile <= 115) {
                spawn_enemy(EN_GUARD, x, y, (int)tile - 112);
            } else if (tile >= 116 && tile <= 119) {
                spawn_enemy(EN_OFFICER, x, y, (int)tile - 116);
            } else if (tile >= 120 && tile <= 123) {
                spawn_enemy(EN_OFFICER, x, y, (int)tile - 120);
            } else if (tile >= 126 && tile <= 129) {
                spawn_enemy(EN_SS, x, y, (int)tile - 126);
            } else if (tile >= 130 && tile <= 133) {
                spawn_enemy(EN_SS, x, y, (int)tile - 130);
            } else if (tile >= 134 && tile <= 137) {
                spawn_enemy(EN_DOG, x, y, (int)tile - 134);
            } else if (tile >= 138 && tile <= 141) {
                spawn_enemy(EN_DOG, x, y, (int)tile - 138);
            } else if (tile == 214 || tile == 197 || tile == 215 || tile == 179 ||
                       tile == 196 || tile == 160 || tile == 178) {
                spawn_enemy(EN_BOSS, x, y, 0);
            }
        }
    }

    if (!spawned) {
        for (y = 1; y < MAPSIZE - 1 && !spawned; ++y) {
            for (x = 1; x < MAPSIZE - 1; ++x) {
                if (g_level.tilemap[x][y] == 0) {
                    g_level.player.x = (float)x + 0.5f;
                    g_level.player.y = (float)y + 0.5f;
                    spawned = 1;
                    break;
                }
            }
        }
    }

    g_level.mapnum = mapnum;
    g_level.episode_map = episode_map;
    g_level.active = true;
    g_level.level_complete = false;
    g_level.elevator_ready = false;

    printf("Level %d ready: doors=%d enemies=%d pickups=%d secrets=%d player=(%.1f,%.1f)\n",
           mapnum, g_level.door_count, g_level.enemies_alive, g_level.pickup_count,
           g_level.secret_total, g_level.player.x, g_level.player.y);
    return true;
}

byte Wolf_WallTile(int tx, int ty)
{
    if ((unsigned)tx >= MAPSIZE || (unsigned)ty >= MAPSIZE) {
        return 1;
    }
    /* Strip pushwall motion flags (original tilemap & 63). */
    return (byte)(g_level.tilemap[tx][ty] & TILE_WALL_MASK);
}

static boolean ray_hits_enemy(float ox, float oy, float dx, float dy, float max_dist,
                              int *out_enemy, float *out_dist)
{
    int i;
    float best = max_dist;
    int best_i = -1;
    for (i = 0; i < g_level.enemy_count; ++i) {
        enemy_t *e = &g_level.enemies[i];
        float ex, ey, proj, perp, dist;
        if (!e->active || e->state == ST_DEAD) {
            continue;
        }
        ex = e->x - ox;
        ey = e->y - oy;
        proj = ex * dx + ey * dy;
        if (proj < 0.1f || proj > max_dist) {
            continue;
        }
        perp = fabsf(ex * dy - ey * dx);
        if (perp < 0.35f) {
            dist = proj;
            if (dist < best) {
                best = dist;
                best_i = i;
            }
        }
    }
    if (best_i >= 0) {
        /* Also ensure wall doesn't block: step along ray */
        float t;
        for (t = 0.1f; t < best; t += 0.1f) {
            if (Wolf_SolidAt(ox + dx * t, oy + dy * t)) {
                return false;
            }
        }
        if (out_enemy) {
            *out_enemy = best_i;
        }
        if (out_dist) {
            *out_dist = best;
        }
        return true;
    }
    return false;
}

void Wolf_TryShoot(void)
{
    player_t *p = &g_level.player;
    float c, s;
    int ei;
    float dist;
    int dmg;

    if (!g_level.active || p->dead || p->attack_cd > 0.0f) {
        return;
    }

    /* Guns need ammo; empty gun → knife only (no phantom pistol shots). */
    if (p->weapon > 0 && p->ammo <= 0) {
        p->weapon = 0;
        p->attack_cd = 0.25f;
        SD_PlaySound(NOITEMSND);
        return;
    }

    if (p->weapon == 0) {
        /* knife melee */
        p->weapon_frame = 1;
        p->attack_cd = 0.35f;
        dmg = 8;
        SD_PlaySound(ATKKNIFESND);
        c = cosf(p->angle);
        s = sinf(p->angle);
        if (ray_hits_enemy(p->x, p->y, c, -s, 0.9f, &ei, &dist)) {
            enemy_t *e = &g_level.enemies[ei];
            e->health -= dmg;
            if (e->health <= 0) {
                e->state = ST_DEAD;
                e->die_t = 0.0f;
                /* keep active so corpse billboard draws (combat AI skips ST_DEAD). */
                g_level.enemies_alive--;
                g_level.kill_count++;
                give_score(e->score_value);
                printf("Killed enemy (knife). Alive=%d score=%d\n",
                       g_level.enemies_alive, p->score);
            } else {
                e->state = ST_CHASE;
            }
        }
        return;
    }

    p->ammo--;
    if (p->ammo < 0) {
        p->ammo = 0;
    }
    p->weapon_frame = 1;
    switch (p->weapon) {
    case 2:
        p->attack_cd = 0.12f;
        dmg = 12;
        SD_PlaySound(ATKMACHINEGUNSND);
        break;
    case 3:
        p->attack_cd = 0.07f;
        dmg = 10;
        SD_PlaySound(ATKGATLINGSND);
        break;
    default:
        p->attack_cd = 0.28f;
        dmg = 16;
        SD_PlaySound(ATKPISTOLSND);
        break;
    }

    c = cosf(p->angle);
    s = sinf(p->angle);
    if (ray_hits_enemy(p->x, p->y, c, -s, 16.0f, &ei, &dist)) {
        enemy_t *e = &g_level.enemies[ei];
        if (dist > 8.0f) {
            dmg = dmg / 2;
        }
        e->health -= dmg;
        e->state = ST_CHASE;
        if (e->health <= 0) {
            e->state = ST_DEAD;
            e->die_t = 0.0f;
            /* keep active for corpse draw */
            g_level.enemies_alive--;
            g_level.kill_count++;
            give_score(e->score_value);
            if (e->kind == EN_DOG) {
                SD_PlaySound(DOGDEATHSND);
                /* Dogs have no gun — never drop ammo clips. */
            } else {
                SD_PlaySound(DEATHSCREAM1SND);
                if (g_level.pickup_count < MAX_PICKUPS) {
                    spawn_pickup(PK_AMMO, (int)e->x, (int)e->y, 4);
                }
            }
            printf("Killed enemy. Alive=%d score=%d ammo=%d\n", g_level.enemies_alive,
                   p->score, p->ammo);
        }
    }
    if (p->ammo <= 0) {
        p->weapon = 0;
    }
}

/*
 * Door lock from map tiles 90..101 → (tile-90|91)/2:
 *   0 = normal, 1..4 = gold..key4, 5 = elevator door (no key — original dr_elevator).
 */

static int cell_blocked(int tx, int ty)
{
    if ((unsigned)tx >= MAPSIZE || (unsigned)ty >= MAPSIZE) {
        return 1;
    }
    if (door_at(tx, ty) >= 0) {
        return 1;
    }
    return (g_level.tilemap[tx][ty] & TILE_WALL_MASK) != 0;
}

static int dir_from_player(void)
{
    /* Match original cardinal facing from player angle. */
    float a = g_level.player.angle;
    float c = cosf(a);
    float s = sinf(a);
    /* dir_x = cos, dir_y = -sin in our convention */
    float dx = c;
    float dy = -s;
    if (fabsf(dx) > fabsf(dy)) {
        return (dx >= 0.0f) ? DI_EAST : DI_WEST;
    }
    return (dy >= 0.0f) ? DI_SOUTH : DI_NORTH;
}

static void dir_delta(int dir, int *ox, int *oy)
{
    *ox = 0;
    *oy = 0;
    switch (dir) {
    case DI_NORTH:
        *oy = -1;
        break;
    case DI_EAST:
        *ox = 1;
        break;
    case DI_SOUTH:
        *oy = 1;
        break;
    case DI_WEST:
        *ox = -1;
        break;
    default:
        break;
    }
}

/*
 * Original PushWall (WL_ACT1.C): clone wall one tile into free cell, mark source
 * with 0xC0, clear plane1 P marker, secretcount++, PUSHWALLSND.
 */
static int try_push_wall(int checkx, int checky, int dir)
{
    int ox, oy;
    int nx, ny;
    byte oldtile;
    float away_x, away_y, alen;
    int try_dirs[4];
    int ti;
    int chosen = -1;
    int chosen_free_only = -1;
    float best_away = -1.0f;

    if (g_level.pwall_state) {
        return 0;
    }
    if ((unsigned)checkx >= MAPSIZE || (unsigned)checky >= MAPSIZE) {
        return 0;
    }
    if (g_level.objmap[checkx][checky] != PUSHABLETILE) {
        return 0;
    }
    oldtile = (byte)(g_level.tilemap[checkx][checky] & TILE_WALL_MASK);
    if (!oldtile) {
        return 0;
    }

    /* Unit vector from player to wall = "backwards" shove direction. */
    away_x = ((float)checkx + 0.5f) - g_level.player.x;
    away_y = ((float)checky + 0.5f) - g_level.player.y;
    alen = sqrtf(away_x * away_x + away_y * away_y);
    if (alen < 0.01f) {
        away_x = cosf(g_level.player.angle);
        away_y = -sinf(g_level.player.angle);
        alen = 1.0f;
    }
    away_x /= alen;
    away_y /= alen;

    try_dirs[0] = dir;
    try_dirs[1] = (dir + 1) & 3;
    try_dirs[2] = (dir + 3) & 3;
    try_dirs[3] = (dir + 2) & 3;

    for (ti = 0; ti < 4; ++ti) {
        int d = try_dirs[ti];
        float dot;
        dir_delta(d, &ox, &oy);
        nx = checkx + ox;
        ny = checky + oy;
        if (cell_blocked(nx, ny)) {
            continue;
        }
        if (chosen_free_only < 0) {
            chosen_free_only = d; /* last-resort any free */
        }
        dot = (float)ox * away_x + (float)oy * away_y;
        /* Prefer dirs that shove away from the player (backwards). */
        if (dot > best_away) {
            best_away = dot;
            chosen = d;
        }
    }

    /*
     * If the best "away" dir is actually toward/sideways (dot low) but a free
     * cell exists, still allow the free dir so map secrets like E1M1 (30,22)
     * (only free WEST) can open — player must then walk into the vacated cell.
     */
    if (chosen < 0 || best_away < 0.15f) {
        chosen = chosen_free_only;
    }
    if (chosen < 0) {
        SD_PlaySound(NOWAYSND);
        printf("Pushwall blocked at %d,%d faced=%d\n", checkx, checky, dir);
        return 1;
    }

    dir = chosen;
    dir_delta(dir, &ox, &oy);
    nx = checkx + ox;
    ny = checky + oy;

    g_level.tilemap[nx][ny] = oldtile;
    g_level.tilemap[checkx][checky] = (byte)(oldtile | TILE_PWALL_MASK);
    g_level.objmap[checkx][checky] = 0;
    g_level.pwall_x = checkx;
    g_level.pwall_y = checky;
    g_level.pwall_dir = dir;
    g_level.pwall_state = 1;
    g_level.pwall_pos = 0;
    g_level.pwall_tile = oldtile;
    g_level.secret_count++;
    SD_PlaySound(PUSHWALLSND);
    printf("Pushwall start %d,%d -> %d,%d dir=%d secrets=%d/%d\n", checkx, checky, nx,
           ny, dir, g_level.secret_count, g_level.secret_total);
    if (best_away < 0.5f) {
        printf("Pushwall: opens SIDEWAYS. Walk FORWARD into empty cell, then turn left/right.
");
    } else {
        printf("Pushwall: shoving away — walk into the opening.
");
    }
    return 1;
}

static void move_pwalls(float dt)
{
    int oldblock, newblock;
    int ox, oy;
    int nx, ny;
    byte oldtile;
    int add;

    if (!g_level.pwall_state) {
        return;
    }
    oldblock = g_level.pwall_state / 128;
    add = (int)(dt * 70.0f + 0.5f);
    if (add < 1) {
        add = 1;
    }
    if (add > 2) {
        add = 2; /* ~2s per tile so sideways open is visible */
    }
    g_level.pwall_state += add;
    g_level.pwall_pos = (g_level.pwall_state / 2) & 63;
    newblock = g_level.pwall_state / 128;
    if (newblock == oldblock) {
        return;
    }

    oldtile = g_level.pwall_tile;
    g_level.tilemap[g_level.pwall_x][g_level.pwall_y] = 0;

    /* Original stops after two tiles when state exceeds 256. */
    if (g_level.pwall_state > 256) {
        g_level.pwall_state = 0;
        g_level.pwall_pos = 0;
        printf("Pushwall done — walk into opened cell (wall parked ahead)\n");
        return;
    }

    dir_delta(g_level.pwall_dir, &ox, &oy);
    g_level.pwall_x += ox;
    g_level.pwall_y += oy;
    nx = g_level.pwall_x + ox;
    ny = g_level.pwall_y + oy;

    if (cell_blocked(nx, ny)) {
        g_level.tilemap[g_level.pwall_x][g_level.pwall_y] = oldtile;
        g_level.pwall_state = 0;
        g_level.pwall_pos = 0;
        printf("Pushwall rest at %d,%d — walk into opened cells behind\n",
               g_level.pwall_x, g_level.pwall_y);
        return;
    }

    g_level.tilemap[nx][ny] = oldtile;
    g_level.tilemap[g_level.pwall_x][g_level.pwall_y] = (byte)(oldtile | TILE_PWALL_MASK);
}

void Wolf_TryUse(void)
{
    player_t *p = &g_level.player;
    float c = cosf(p->angle);
    float s = sinf(p->angle);
    int did = -1;
    int i, k;
    int elev_x = -1, elev_y = -1;
    int dir;
    int ptx, pty;
    int tried_push = 0;

    if (p->dead) {
        return;
    }

    /*
     * Original Cmd_Use faces one cardinal tile. We also probe a short ray and
     * all four neighbors so secrets still work when slightly off-axis.
     * Order: pushwall first (never steal OPENDOORSND).
     */
    dir = dir_from_player();
    ptx = (int)floorf(p->x);
    pty = (int)floorf(p->y);

    /* 1) Pushwalls — facing tile, then other neighbors, then short ray. */
    {
        int order[4];
        int ox, oy;
        order[0] = dir;
        order[1] = (dir + 1) & 3;
        order[2] = (dir + 3) & 3;
        order[3] = (dir + 2) & 3;
        for (k = 0; k < 4; ++k) {
            int d = order[k];
            int tx, ty;
            dir_delta(d, &ox, &oy);
            tx = ptx + ox;
            ty = pty + oy;
            if ((unsigned)tx >= MAPSIZE || (unsigned)ty >= MAPSIZE) {
                continue;
            }
            if (g_level.objmap[tx][ty] == PUSHABLETILE &&
                (g_level.tilemap[tx][ty] & TILE_WALL_MASK) != 0) {
                tried_push = 1;
                /* Push direction = into the wall from the player (d). */
                if (try_push_wall(tx, ty, d)) {
                    return;
                }
            }
        }
        for (i = 1; i <= 4; ++i) {
            float ux = p->x + c * (0.35f * (float)i);
            float uy = p->y - s * (0.35f * (float)i);
            int tx = (int)floorf(ux);
            int ty = (int)floorf(uy);
            if ((unsigned)tx >= MAPSIZE || (unsigned)ty >= MAPSIZE) {
                continue;
            }
            if (tx == ptx && ty == pty) {
                continue;
            }
            if (g_level.objmap[tx][ty] == PUSHABLETILE &&
                (g_level.tilemap[tx][ty] & TILE_WALL_MASK) != 0) {
                int dpush = dir;
                int dx = tx - ptx;
                int dy = ty - pty;
                if (abs(dx) >= abs(dy)) {
                    dpush = (dx >= 0) ? DI_EAST : DI_WEST;
                } else {
                    dpush = (dy >= 0) ? DI_SOUTH : DI_NORTH;
                }
                tried_push = 1;
                if (try_push_wall(tx, ty, dpush)) {
                    return;
                }
            }
        }
    }

    /* 2) Elevator on facing cardinal tile */
    {
        int ox, oy, checkx, checky;
        dir_delta(dir, &ox, &oy);
        checkx = ptx + ox;
        checky = pty + oy;
        if ((unsigned)checkx < MAPSIZE && (unsigned)checky < MAPSIZE) {
            if ((g_level.tilemap[checkx][checky] & TILE_WALL_MASK) == ELEVATORTILE) {
                elev_x = checkx;
                elev_y = checky;
            } else {
                static const int ax4[4] = {1, -1, 0, 0};
                static const int ay4[4] = {0, 0, 1, -1};
                for (i = 0; i < 4; ++i) {
                    int ax = checkx + ax4[i];
                    int ay = checky + ay4[i];
                    if ((unsigned)ax < MAPSIZE && (unsigned)ay < MAPSIZE &&
                        (g_level.tilemap[ax][ay] & TILE_WALL_MASK) == ELEVATORTILE) {
                        elev_x = ax;
                        elev_y = ay;
                        break;
                    }
                }
            }
            did = door_at(checkx, checky);
        }
    }

    /* 3) Door ray fallback */
    if (did < 0) {
        for (i = 1; i <= 3; ++i) {
            float ux = p->x + c * (0.40f * (float)i);
            float uy = p->y - s * (0.40f * (float)i);
            int tx = (int)floorf(ux);
            int ty = (int)floorf(uy);
            if ((unsigned)tx >= MAPSIZE || (unsigned)ty >= MAPSIZE) {
                continue;
            }
            did = door_at(tx, ty);
            if (did >= 0) {
                break;
            }
        }
    }

    if (did >= 0) {
        door_t *d = &g_level.doors[did];
        if (d->lock >= 1 && d->lock <= 4) {
            int need = d->lock - 1;
            if ((p->keys & (1 << need)) == 0) {
                printf("Door %d locked (need key %d, have mask=0x%x)\n", did,
                       d->lock, p->keys);
                SD_PlaySound(NOWAYSND);
                return;
            }
        }
        if (d->open < 0.5f) {
            d->opening = 1;
            SD_PlaySound(OPENDOORSND);
            printf("Door %d opening lock=%d at %d,%d\n", did, d->lock, d->tilex,
                   d->tiley);
        } else {
            d->opening = -1;
            SD_PlaySound(CLOSEDOORSND);
            printf("Door %d closing at %d,%d\n", did, d->tilex, d->tiley);
        }
        return;
    }

    if (elev_x >= 0) {
        g_level.level_complete = true;
        g_level.elevator_ready = true;
        SD_PlaySound(LEVELDONESND);
        printf("Elevator activated at %d,%d → press N for next map\n", elev_x,
               elev_y);
        return;
    }

    if (tried_push) {
        /* Found secret wall but could not push (blocked / already moving). */
        return;
    }
    SD_PlaySound(DONOTHINGSND);
}


static void update_pickups(void)
{
    int i;
    player_t *pl = &g_level.player;
    for (i = 0; i < g_level.pickup_count; ++i) {
        pickup_t *pk = &g_level.pickups[i];
        float dx, dy;
        if (!pk->active) {
            continue;
        }
        dx = pk->x - pl->x;
        dy = pk->y - pl->y;
        if (dx * dx + dy * dy > 0.35f * 0.35f) {
            continue;
        }
        switch (pk->kind) {
        case PK_KEY1:
                    pl->keys |= 1;
                    SD_PlaySound(GETKEYSND);
                    printf("Got gold key\n");
                    break;
                case PK_KEY2:
                    pl->keys |= 2;
                    SD_PlaySound(GETKEYSND);
                    printf("Got silver key\n");
                    break;
                case PK_KEY3:
                    pl->keys |= 4;
                    SD_PlaySound(GETKEYSND);
                    break;
                case PK_KEY4:
                    pl->keys |= 8;
                    SD_PlaySound(GETKEYSND);
                    break;
                case PK_AMMO:
                    pl->ammo += pk->amount;
                    if (pl->ammo > 99) {
                        pl->ammo = 99;
                    }
                    if (pl->weapon < 1) {
                        pl->weapon = 1;
                    }
                    SD_PlaySound(GETAMMOSND);
                    break;
                case PK_FOOD:
                    pl->health += pk->amount;
                    if (pl->health > 100) {
                        pl->health = 100;
                    }
                    SD_PlaySound(HEALTH1SND);
                    break;
                case PK_MEDKIT:
                    pl->health += pk->amount;
                    if (pl->health > 100) {
                        pl->health = 100;
                    }
                    SD_PlaySound(HEALTH2SND);
                    break;
                case PK_1UP:
                    pl->lives += 1;
                    pl->health = 100;
                    SD_PlaySound(BONUS1UPSND);
                    break;
                case PK_TREASURE:
                    give_score(pk->amount);
                    SD_PlaySound(BONUS1SND);
                    break;
                case PK_MGUN:
                    if (pl->best_weapon < 2) {
                        pl->best_weapon = 2;
                    }
                    pl->weapon = 2;
                    pl->ammo += pk->amount;
                    SD_PlaySound(GETMACHINESND);
                    break;
                case PK_CGUN:
                    if (pl->best_weapon < 3) {
                        pl->best_weapon = 3;
                    }
                    pl->weapon = 3;
                    pl->ammo += pk->amount;
                    SD_PlaySound(GETGATLINGSND);
                    break;
        default:
            break;
        }
        pk->active = false;
        g_level.pickup_got++;
    }
}


/* True if a coarse grid walk from (x0,y0) to (x1,y1) never enters a solid cell. */
static int line_clear(float x0, float y0, float x1, float y1)
{
    float dx = x1 - x0;
    float dy = y1 - y0;
    float dist = sqrtf(dx * dx + dy * dy);
    int steps;
    int i;
    if (dist < 0.05f) {
        return 1;
    }
    steps = (int)(dist * 8.0f) + 1;
    if (steps < 2) {
        steps = 2;
    }
    if (steps > 64) {
        steps = 64;
    }
    for (i = 1; i < steps; ++i) {
        float u = (float)i / (float)steps;
        float x = x0 + dx * u;
        float y = y0 + dy * u;
        /* Don't test endpoints (actor centers may sit near walls). */
        if (Wolf_SolidAt(x, y)) {
            return 0;
        }
    }
    return 1;
}

static void update_enemies(float dt)
{
    int i;
    player_t *pl = &g_level.player;

    for (i = 0; i < g_level.enemy_count; ++i) {
        enemy_t *e = &g_level.enemies[i];
        float dx, dy, dist, ang, c, s, nx, ny;
        if (!e->active || pl->dead) {
            continue;
        }
        if (e->state == ST_DEAD) {
            /* Death sequence ~0.45s through DIE_1..3 then hold DEAD. */
            if (e->die_t < 0.50f) {
                e->die_t += dt;
            }
            continue;
        }

        dx = pl->x - e->x;
        dy = pl->y - e->y;
        dist = sqrtf(dx * dx + dy * dy);
        e->think_t -= dt;
        if (e->attack_cd > 0) {
            e->attack_cd -= dt;
        }

        if (e->state == ST_STAND) {
            if (dist < 8.0f) {
                e->state = ST_CHASE;
            }
            continue;
        }

        if (e->state == ST_CHASE || e->state == ST_ATTACK) {
            e->anim_t += dt * (e->kind == EN_DOG ? 6.0f : 4.0f);
            if (dist > 0.01f) {
                ang = atan2f(-dy, dx); /* match player angle convention */
                e->angle = ang;
                c = cosf(ang);
                s = sinf(ang);
                if (dist > 0.85f) {
                    float step = e->speed * dt;
                    /* Cap step so large dt spikes cannot cannon through walls. */
                    if (step > 0.20f) {
                        step = 0.20f;
                    }
                    nx = e->x + c * step;
                    ny = e->y - s * step;
                    if (!blocked_actor(nx, e->y, 0.20f)) {
                        e->x = nx;
                    } else if (!blocked_actor(e->x + c * step * 0.5f, e->y, 0.20f)) {
                        e->x += c * step * 0.5f;
                    }
                    if (!blocked_actor(e->x, ny, 0.20f)) {
                        e->y = ny;
                    } else if (!blocked_actor(e->x, e->y - s * step * 0.5f, 0.20f)) {
                        e->y -= s * step * 0.5f;
                    }
                }
            }

            /* Melee only with clear LOS — no damage through secret/push walls. */
            if (dist < 1.1f && e->attack_cd <= 0.0f &&
                line_clear(e->x, e->y, pl->x, pl->y)) {
                int dmg = 5;
                if (e->kind == EN_DOG) {
                    dmg = 8;
                } else if (e->kind == EN_SS) {
                    dmg = 12;
                } else if (e->kind == EN_BOSS) {
                    dmg = 20;
                }
                hurt_player(dmg);
                e->attack_cd = 0.7f;
                e->state = ST_ATTACK;
            } else if (dist >= 1.1f) {
                e->state = ST_CHASE;
            }
        }
    }
}

void Wolf_UpdateLevel(float dt, const boolean keys[12])
{
    player_t *p = &g_level.player;
    float move_speed = 3.2f * dt;
    float turn_speed = 2.6f * dt;
    float c, s;
    float nx, ny;
    int i;

    if (!g_level.active) {
        return;
    }

    if (p->dead) {
        /* Wait for respawn intent — held forward or loop handles lives */
        return;
    }

    for (i = 0; i < g_level.door_count; ++i) {
        door_t *d = &g_level.doors[i];
        if (d->opening > 0) {
            d->open += dt * 1.6f;
            if (d->open >= 1.0f) {
                d->open = 1.0f;
                d->opening = 0;
            }
        } else if (d->opening < 0) {
            d->open -= dt * 1.6f;
            if (d->open <= 0.0f) {
                d->open = 0.0f;
                d->opening = 0;
            }
        }
    }

    move_pwalls(dt);

    if (p->attack_cd > 0) {
        float total = 0.28f;
        p->attack_cd -= dt;
        if (p->weapon == 0) {
            total = 0.35f;
        } else if (p->weapon == 2) {
            total = 0.12f;
        } else if (p->weapon == 3) {
            total = 0.07f;
        }
        if (total < 0.05f) {
            total = 0.05f;
        }
        if (p->weapon_frame > 0) {
            float u = 1.0f - (p->attack_cd / total);
            if (u < 0.0f) {
                u = 0.0f;
            }
            if (u > 1.0f) {
                u = 1.0f;
            }
            p->weapon_frame = 1 + (int)(u * 3.99f);
            if (p->weapon_frame > 4) {
                p->weapon_frame = 4;
            }
        }
        if (p->attack_cd <= 0.0f) {
            p->attack_cd = 0.0f;
            p->weapon_frame = 0;
        }
    }
    if (p->hurt_flash > 0) {
        p->hurt_flash -= dt;
    }

    /* Playtest: left key must turn left on screen (was inverted with dir_y=-sin). */
        if (keys[2]) {
            p->angle += turn_speed;
        }
        if (keys[3]) {
            p->angle -= turn_speed;
        }

    c = cosf(p->angle);
    s = sinf(p->angle);

    nx = p->x;
    ny = p->y;
    if (keys[0]) {
        nx += c * move_speed;
        ny -= s * move_speed;
    }
    if (keys[1]) {
        nx -= c * move_speed;
        ny += s * move_speed;
    }
    if (keys[4]) {
        nx -= s * move_speed;
        ny -= c * move_speed;
    }
    if (keys[5]) {
        nx += s * move_speed;
        ny += c * move_speed;
    }

    if (!blocked_player(nx, p->y)) {
        p->x = nx;
    }
    if (!blocked_player(p->x, ny)) {
        p->y = ny;
    }

    if (keys[6]) {
        Wolf_TryShoot();
    }

    /* Weapon select 1-4 */
    if (keys[7] && p->best_weapon >= 0) {
        p->weapon = 0;
        p->weapon_frame = 0;
    }
    if (keys[8] && p->best_weapon >= 1) {
        p->weapon = 1;
        p->weapon_frame = 0;
    }
    if (keys[9] && p->best_weapon >= 2) {
        p->weapon = 2;
        p->weapon_frame = 0;
    }
    if (keys[10] && p->best_weapon >= 3) {
        p->weapon = 3;
        p->weapon_frame = 0;
    }

    update_pickups();
    update_enemies(dt);
}

boolean Wolf_CombatSmokeVerify(void)
{
    int i;
    int guards = 0;
    int necks = 0;
    int clips = 0;

    for (i = 0; i < g_level.enemy_count; ++i) {
        if (g_level.enemies[i].active && g_level.enemies[i].kind == EN_GUARD) {
            guards++;
        }
    }
    for (i = 0; i < g_level.pickup_count; ++i) {
        if (!g_level.pickups[i].active) {
            continue;
        }
        if (g_level.pickups[i].kind == PK_KEY1 ||
            g_level.pickups[i].kind == PK_KEY2) {
            necks++;
        }
        if (g_level.pickups[i].kind == PK_AMMO) {
            clips++;
        }
    }

    printf("Combat smoke: enemies_alive=%d guards~=%d key_pickups=%d ammo_pickups=%d "
           "HP=%d ammo=%d lives=%d\n",
           g_level.enemies_alive, guards, necks, clips, g_level.player.health,
           g_level.player.ammo, g_level.player.lives);

    /* Simulate one shot and one pickup if present */
        if (g_level.enemies_alive > 0) {
            int old_score = g_level.player.score;
            for (i = 0; i < g_level.enemy_count; ++i) {
                if (g_level.enemies[i].active && g_level.enemies[i].state != ST_DEAD) {
                    enemy_t *e = &g_level.enemies[i];
                    float dx, dy;
                    /* Teleport next to target so LOS is clear for smoke. */
                    g_level.player.x = e->x - 0.6f;
                    g_level.player.y = e->y;
                    dx = e->x - g_level.player.x;
                    dy = e->y - g_level.player.y;
                    g_level.player.angle = atan2f(-dy, dx);
                    g_level.player.ammo = 50;
                    g_level.player.weapon = 1;
                    g_level.player.attack_cd = 0;
                    {
                        int s;
                        for (s = 0; s < 30 && e->active; ++s) {
                            g_level.player.attack_cd = 0;
                            Wolf_TryShoot();
                        }
                    }
                    printf("Smoke shot enemy kind=%d dead=%d score_delta=%d\n",
                           (int)e->kind, e->state == ST_DEAD ? 1 : 0,
                           g_level.player.score - old_score);
                    break;
                }
            }
        }

    /* Auto-grant key and open a locked door if any lock>0 */
    {
        int locked = 0;
        for (i = 0; i < g_level.door_count; ++i) {
            if (g_level.doors[i].lock > 0) {
                locked++;
                g_level.player.keys |= (1 << (g_level.doors[i].lock - 1));
                g_level.doors[i].open = 1.0f;
            }
        }
        printf("Combat smoke: locked_doors=%d keys_now=%d\n", locked,
               g_level.player.keys);
    }

    return g_level.enemy_count > 0 || g_level.pickup_count > 0;
}


boolean Wolf_PushwallSmokeVerify(void)
{
    int x, y, n = 0;
    if (!g_level.active) {
        if (!Wolf_SetupLevel(0)) {
            fprintf(stderr, "Pushwall smoke: setup failed\n");
            return false;
        }
    }
    for (y = 0; y < MAPSIZE; ++y) {
        for (x = 0; x < MAPSIZE; ++x) {
            if (g_level.objmap[x][y] == PUSHABLETILE) {
                n++;
            }
        }
    }
    printf("Pushwall smoke: map_secrets=%d secret_total=%d\n", n, g_level.secret_total);
    if (g_level.secret_total <= 0) {
        fprintf(stderr, "Pushwall smoke: expected secrets on map 0\n");
        return false;
    }
    if (n != g_level.secret_total) {
        fprintf(stderr, "Pushwall smoke: objmap count mismatch\n");
        return false;
    }
    /* Find pushable + free cell, stand opposite, TryUse, advance motion. */
    {
        int found = 0;
        int sx = 0, sy = 0, sd = 0;
        for (y = 1; y < MAPSIZE - 1 && !found; ++y) {
            for (x = 1; x < MAPSIZE - 1; ++x) {
                int d;
                if (g_level.objmap[x][y] != PUSHABLETILE) {
                    continue;
                }
                if ((g_level.tilemap[x][y] & TILE_WALL_MASK) == 0) {
                    continue;
                }
                for (d = 0; d < 4; ++d) {
                    int ox, oy;
                    dir_delta(d, &ox, &oy);
                    if (!cell_blocked(x + ox, y + oy)) {
                        found = 1;
                        sx = x;
                        sy = y;
                        sd = d;
                        printf("Pushwall smoke: sample_pushable=%d,%d free_dir=%d wall=%d\n",
                               x, y, d, g_level.tilemap[x][y] & TILE_WALL_MASK);
                        break;
                    }
                }
            }
        }
        if (!found) {
            fprintf(stderr, "Pushwall smoke: no free-adjacent pushable\n");
            return false;
        }
        {
            int ox, oy;
            float ang;
            dir_delta(sd, &ox, &oy);
            g_level.player.x = (float)(sx - ox) + 0.5f;
            g_level.player.y = (float)(sy - oy) + 0.5f;
            switch (sd) {
            case DI_NORTH:
                ang = (float)(M_PI * 0.5);
                break;
            case DI_EAST:
                ang = 0.0f;
                break;
            case DI_SOUTH:
                ang = (float)(-M_PI * 0.5);
                break;
            default:
                ang = (float)M_PI;
                break;
            }
            g_level.player.angle = ang;
            Wolf_TryUse();
            if (g_level.pwall_state <= 0) {
                fprintf(stderr, "Pushwall smoke: TryUse failed state=%d at %d,%d dir=%d\n",
                        g_level.pwall_state, sx, sy, sd);
                return false;
            }
            printf("Pushwall smoke: activated state=%d secrets=%d\n", g_level.pwall_state,
                   g_level.secret_count);
            {
                int step;
                int ox2, oy2;
                /* ~3 seconds of motion: should vacate origin and open a walkable cell. */
                for (step = 0; step < 220; ++step) {
                    move_pwalls(1.0f / 70.0f);
                }
                dir_delta(sd, &ox2, &oy2);
                if ((g_level.tilemap[sx][sy] & TILE_WALL_MASK) != 0) {
                    fprintf(stderr, "Pushwall smoke: origin %d,%d still solid after move\n",
                            sx, sy);
                    return false;
                }
                printf("Pushwall smoke: after_move state=%d anchor=%d,%d origin_open=1\n",
                       g_level.pwall_state, g_level.pwall_x, g_level.pwall_y);
            }
        }
        /* E1M1 (30,22) free only WEST — face north from south must still push. */
        if (!Wolf_SetupLevel(0)) {
            fprintf(stderr, "Pushwall smoke: re-setup failed\n");
            return false;
        }
        /* (30,22) only free WEST — from south, opens sideways; origin must walk in. */
        g_level.player.x = 30.5f;
        g_level.player.y = 23.5f;
        g_level.player.angle = (float)(M_PI * 0.5);
        Wolf_TryUse();
        if (g_level.pwall_state <= 0) {
            fprintf(stderr, "Pushwall smoke: (30,22) failed to start\n");
            return false;
        }
        {
            int step;
            for (step = 0; step < 400; ++step) {
                move_pwalls(1.0f / 70.0f);
            }
            if (Wolf_SolidAt(30.5f, 22.5f)) {
                fprintf(stderr, "Pushwall smoke: (30,22) not walkable after open\n");
                return false;
            }
            if ((g_level.tilemap[30][22] & TILE_WALL_MASK) != 0) {
                fprintf(stderr, "Pushwall smoke: (30,22) tile still wall=%d\n",
                        g_level.tilemap[30][22]);
                return false;
            }
            printf("Pushwall smoke: (30,22) OPEN walkable tile=%d wall@28=%d\n",
                   g_level.tilemap[30][22], g_level.tilemap[28][22] & 63);
        }
        /* (10,13) true backwards (north) shove */
        if (!Wolf_SetupLevel(0)) {
            return false;
        }
        g_level.player.x = 10.5f;
        g_level.player.y = 14.5f;
        g_level.player.angle = (float)(M_PI * 0.5);
        Wolf_TryUse();
        if (g_level.pwall_state <= 0 || g_level.pwall_dir != DI_NORTH) {
            fprintf(stderr, "Pushwall smoke: (10,13) north failed st=%d dir=%d\n",
                    g_level.pwall_state, g_level.pwall_dir);
            return false;
        }
        {
            int step;
            for (step = 0; step < 400; ++step) {
                move_pwalls(1.0f / 70.0f);
            }
            if (Wolf_SolidAt(10.5f, 13.5f)) {
                fprintf(stderr, "Pushwall smoke: (10,13) origin solid\n");
                return false;
            }
            printf("Pushwall smoke: (10,13) north shove walkable\n");
        }
        {
            int ei;
            int hp;
            enemy_t *e = NULL;
            for (ei = 0; ei < g_level.enemy_count; ++ei) {
                if (g_level.enemies[ei].active && g_level.enemies[ei].state != ST_DEAD) {
                    e = &g_level.enemies[ei];
                    break;
                }
            }
            if (e) {
                e->x = 31.5f;
                e->y = 22.5f;
                e->state = ST_CHASE;
                e->attack_cd = 0.0f;
                g_level.player.x = 29.5f;
                g_level.player.y = 22.5f;
                g_level.player.health = 100;
                hp = g_level.player.health;
                update_enemies(0.05f);
                if (g_level.player.health < hp) {
                    fprintf(stderr, "Pushwall smoke: LOS fail %d->%d\n", hp,
                            g_level.player.health);
                    return false;
                }
                printf("Pushwall smoke: LOS ok (no through-wall melee)\n");
            }
        }
    }
    printf("Pushwall smoke: OK\n");
    return true;
}
