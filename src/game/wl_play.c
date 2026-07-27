#include "wl_play.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "id/id_ca.h"

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

    if ((unsigned)tx >= MAPSIZE || (unsigned)ty >= MAPSIZE) {
        return true;
    }

    did = door_at(tx, ty);
    if (did >= 0) {
        return g_level.doors[did].open < 0.85f;
    }

    return g_level.tilemap[tx][ty] != 0;
}

static boolean blocked_player(float x, float y)
{
    const float r = 0.18f;
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

boolean Wolf_SetupLevel(int mapnum)
{
    int x, y;
    uint16_t *plane0;
    uint16_t *plane1;
    int spawned = 0;

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

    g_level.player.x = 32.5f;
    g_level.player.y = 32.5f;
    g_level.player.angle = 0.0f;

    for (y = 0; y < MAPSIZE; ++y) {
        for (x = 0; x < MAPSIZE; ++x) {
            uint16_t tile = plane1[y * MAPSIZE + x];
            if (tile >= 19 && tile <= 22) {
                g_level.player.x = (float)x + 0.5f;
                g_level.player.y = (float)y + 0.5f;
                /* plane codes: 19 N, 20 E, 21 S, 22 W */
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
    g_level.active = true;
    printf("Level %d ready: doors=%d player=(%.1f,%.1f) ang=%.2f\n", mapnum,
           g_level.door_count, g_level.player.x, g_level.player.y,
           g_level.player.angle);
    return true;
}

byte Wolf_WallTile(int tx, int ty)
{
    if ((unsigned)tx >= MAPSIZE || (unsigned)ty >= MAPSIZE) {
        return 1;
    }
    return g_level.tilemap[tx][ty];
}

void Wolf_TryUse(void)
{
    float c = cosf(g_level.player.angle);
    float s = sinf(g_level.player.angle);
    int did = -1;
    int i;

    for (i = 1; i <= 3 && did < 0; ++i) {
        float ux = g_level.player.x + c * (0.4f * (float)i);
        float uy = g_level.player.y - s * (0.4f * (float)i);
        int tx = (int)floorf(ux);
        int ty = (int)floorf(uy);
        did = door_at(tx, ty);
    }

    if (did >= 0) {
        door_t *d = &g_level.doors[did];
        if (d->open < 0.5f) {
            d->opening = 1;
            printf("Door %d opening at %d,%d\n", did, d->tilex, d->tiley);
        } else {
            d->opening = -1;
            printf("Door %d closing at %d,%d\n", did, d->tilex, d->tiley);
        }
    }
}

/*
 * keys layout from game loop (ascii-ish flags):
 * 0 forward, 1 back, 2 turn left, 3 turn right, 4 strafe left, 5 strafe right
 */
void Wolf_UpdateLevel(float dt, const boolean keys[8])
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

    /* Axis-separated collision */
    if (!blocked_player(nx, p->y)) {
        p->x = nx;
    }
    if (!blocked_player(p->x, ny)) {
        p->y = ny;
    }
}
