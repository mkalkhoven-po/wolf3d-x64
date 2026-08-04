#pragma once

#include "wl_types.h"

#define MAPSIZE 64
#define AREATILE 107
#define ELEVATORTILE 21
#define PUSHABLETILE 98 /* plane1 secret marker (original) */
#define TILE_PWALL_MASK 0xC0 /* in-motion pushwall flag bits */
#define TILE_WALL_MASK 0x3F /* wall texture id while pushing */
#define MAX_DOORS 64
#define MAX_ENEMIES 128
#define MAX_PICKUPS 256
#define MAX_DECOR 384
#define SHAREWARE_MAPS 10

typedef enum {
    EN_NONE = 0,
    EN_GUARD,
    EN_OFFICER,
    EN_SS,
    EN_DOG,
    EN_BOSS
} enemy_kind_t;

typedef enum {
    ST_DEAD = 0,
    ST_STAND,
    ST_CHASE,
    ST_ATTACK
} enemy_state_t;

typedef enum {
    DI_NORTH = 0,
    DI_EAST,
    DI_SOUTH,
    DI_WEST
} dir4_t;

typedef enum {
    PK_NONE = 0,
    PK_KEY1,
    PK_KEY2,
    PK_KEY3,
    PK_KEY4,
    PK_AMMO,
    PK_FOOD,
    PK_MEDKIT,
    PK_1UP,
    PK_TREASURE,
    PK_MGUN,
    PK_CGUN
} pickup_kind_t;

typedef struct {
    float x;
    float y;
    float angle;
    int health;
    int lives;
    int ammo;
    int score;
    int keys; /* bit0 gold, bit1 silver, etc. */
    int weapon; /* 0 knife 1 pistol 2 mg 3 chain */
    int best_weapon;
    int weapon_frame; /* 0=ready, 1..4 = attack frames (F2) */
    float attack_cd;
    float hurt_flash;
    boolean dead;
} player_t;

typedef struct {
    int tilex, tiley;
    int vertical;
    int lock; /* 0 free, 1 gold, 2 silver, ... */
    float open;
    int opening;
} door_t;

typedef struct {
    boolean active;
    enemy_kind_t kind;
    enemy_state_t state;
    float x, y;
    float angle;
    int health;
    float think_t;
    float attack_cd;
    float speed;
    float anim_t; /* walk-cycle timer */
    float die_t; /* death anim 0..done when ST_DEAD */
    int score_value;
} enemy_t;

typedef struct {
    boolean active;
    pickup_kind_t kind;
    float x, y;
    int amount;
    int shapenum;
} pickup_t;

typedef struct {
    boolean active;
    float x, y;
    int shapenum;
} decor_t;

typedef struct {
    byte tilemap[MAPSIZE][MAPSIZE];
    int door_id[MAPSIZE][MAPSIZE];
    door_t doors[MAX_DOORS];
    int door_count;
    enemy_t enemies[MAX_ENEMIES];
    int enemy_count;
    int enemies_alive;
    pickup_t pickups[MAX_PICKUPS];
    int pickup_count;
    decor_t decor[MAX_DECOR];
    int decor_count;
    player_t player;
    int mapnum;
    int episode_map; /* 0..SHAREWARE_MAPS-1 progress */
    boolean active;
    boolean level_complete;
    boolean elevator_ready;
    int kill_count;
    int pickup_got;
    int secret_count; /* secrets found this level */
    int secret_total; /* PUSHABLETILE count at load */
    /* Active pushwall (one at a time, original). */
    int pwall_state; /* 0 idle; else tic accumulator */
    int pwall_pos; /* 0..63 sub-tile */
    int pwall_x, pwall_y;
    int pwall_dir; /* dir4_t */
    byte pwall_tile; /* wall tex id being moved */
    /* Object plane snapshot (push markers live here until used). */
    unsigned short objmap[MAPSIZE][MAPSIZE];
} level_t;

extern level_t g_level;

boolean Wolf_SetupLevel(int mapnum);
void Wolf_UpdateLevel(float dt, const boolean keys[12]);
void Wolf_TryUse(void);
void Wolf_TryShoot(void);
boolean Wolf_SolidAt(float x, float y);
byte Wolf_WallTile(int tx, int ty);
void Wolf_ResetPlayerStats(void);
boolean Wolf_CombatSmokeVerify(void);
boolean Wolf_PushwallSmokeVerify(void);

/* keys[]: 0 fwd 1 back 2 turnL 3 turnR 4 strafeL 5 strafeR 6 shoot held */
