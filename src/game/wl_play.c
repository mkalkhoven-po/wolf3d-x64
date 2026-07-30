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
        if (tile == ELEVATORTILE) {
            return true;
        }
        return tile != 0;
    }

static boolean blocked_player(float x, float y)
{
    const float r = 0.18f;
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

    printf("Level %d ready: doors=%d enemies=%d pickups=%d player=(%.1f,%.1f)\n",
           mapnum, g_level.door_count, g_level.enemies_alive, g_level.pickup_count,
           g_level.player.x, g_level.player.y);
    return true;
}

byte Wolf_WallTile(int tx, int ty)
{
    if ((unsigned)tx >= MAPSIZE || (unsigned)ty >= MAPSIZE) {
        return 1;
    }
    return g_level.tilemap[tx][ty];
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
void Wolf_TryUse(void)
{
    player_t *p = &g_level.player;
    float c = cosf(p->angle);
    float s = sinf(p->angle);
    int did = -1;
    int i;
    int checkx, checky;
    int elev_x = -1, elev_y = -1;

    if (p->dead) {
        return;
    }

    for (i = 1; i <= 4; ++i) {
        float ux = p->x + c * (0.45f * (float)i);
        float uy = p->y - s * (0.45f * (float)i);
        checkx = (int)floorf(ux);
        checky = (int)floorf(uy);
        if ((unsigned)checkx >= MAPSIZE || (unsigned)checky >= MAPSIZE) {
            continue;
        }

        if (did < 0) {
            did = door_at(checkx, checky);
        }
        if (g_level.tilemap[checkx][checky] == ELEVATORTILE) {
            elev_x = checkx;
            elev_y = checky;
        }
        /* Elevator cab walls sometimes sit beside elev tile — probe adjacent. */
        if (elev_x < 0) {
            static const int ox[4] = {1, -1, 0, 0};
            static const int oy[4] = {0, 0, 1, -1};
            int k;
            for (k = 0; k < 4; ++k) {
                int ax = checkx + ox[k];
                int ay = checky + oy[k];
                if ((unsigned)ax < MAPSIZE && (unsigned)ay < MAPSIZE &&
                    g_level.tilemap[ax][ay] == ELEVATORTILE) {
                    elev_x = ax;
                    elev_y = ay;
                    break;
                }
            }
        }
    }

    if (did >= 0) {
        door_t *d = &g_level.doors[did];
        /* Matches original OperateDoor: only dr_lock1..dr_lock4 need keys. */
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

static void update_enemies(float dt)
{
    int i;
    player_t *pl = &g_level.player;

    for (i = 0; i < g_level.enemy_count; ++i) {
        enemy_t *e = &g_level.enemies[i];
        float dx, dy, dist, ang, c, s, nx, ny;
        if (!e->active || e->state == ST_DEAD || pl->dead) {
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

            if (dist < 1.1f && e->attack_cd <= 0.0f) {
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
