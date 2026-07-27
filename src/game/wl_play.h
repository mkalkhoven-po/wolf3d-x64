#pragma once

#include "wl_types.h"

#define MAPSIZE 64
#define AREATILE 107
#define MAX_DOORS 64

typedef struct {
    float x; /* world: 1.0 = one map tile */
    float y;
    float angle; /* radians; 0=east, +CCW */
} player_t;

typedef struct {
    int tilex, tiley;
    int vertical;
    int lock;
    float open;   /* 0 closed .. 1 open */
    int opening;  /* -1/0/+1 */
} door_t;

typedef struct {
    byte tilemap[MAPSIZE][MAPSIZE];
    int door_id[MAPSIZE][MAPSIZE];
    door_t doors[MAX_DOORS];
    int door_count;
    player_t player;
    int mapnum;
    boolean active;
} level_t;

extern level_t g_level;

boolean Wolf_SetupLevel(int mapnum);
void Wolf_UpdateLevel(float dt, const boolean keys[8]);
void Wolf_TryUse(void);
boolean Wolf_SolidAt(float x, float y);
byte Wolf_WallTile(int tx, int ty);
