#pragma once

/*
 * Constants and types for the faithful engine (P4, ADR-002), taken from
 * WL_DEF.H. The legacy float engine in src/game does not use this header.
 *
 * Borland `int` and `unsigned` were 16 bits and `long` 32 bits; struct fields
 * keep those widths (int16_t / uint16_t / int32_t) so stored values wrap the
 * same way as in the original.
 */

#include "wl_types.h"

#define PI          3.141592657

#define MAXACTORS   150     /* max number of nazis, etc / map */
#define MAXSTATS    400     /* max number of lamps, bonus, etc */
#define MAXDOORS    64      /* max number of sliding doors */

#define ICONARROWS      90
#define PUSHABLETILE    98
#define EXITTILE        99  /* at end of castle */
#define AREATILE        107 /* first of NUMAREAS floor tiles */
#define NUMAREAS        37
#define ELEVATORTILE    21
#define AMBUSHTILE      106
#define ALTELEVATORTILE 107

#define EXTRAPOINTS     40000

#define PLAYERSPEED     3000
#define RUNSPEED        6000

#define GLOBAL1     (1l << 16)
#define TILEGLOBAL  GLOBAL1
#define PIXGLOBAL   (GLOBAL1 / 64)
#define TILESHIFT   16l
#define UNSIGNEDSHIFT 8

#define ANGLES      360     /* must be divisable by 4 */
#define ANGLEQUAD   (ANGLES / 4)
#define FINEANGLES  3600
#define ANG90       (FINEANGLES / 4)
#define ANG180      (ANG90 * 2)
#define ANG270      (ANG90 * 3)
#define ANG360      (ANG90 * 4)
#define VANG90      (ANGLES / 4)
#define VANG180     (VANG90 * 2)
#define VANG270     (VANG90 * 3)
#define VANG360     (VANG90 * 4)

#define MINDIST     (0x5800l)

#define PLAYERSIZE      MINDIST     /* player radius */
#define MINACTORDIST    0x10000l    /* minimum dist from player center to any actor center */

#define MAXTICS     10
#define DEMOTICS    4

#define MAPSIZE     64      /* maps are 64*64 max */
#define NORTH       0
#define EAST        1
#define SOUTH       2
#define WEST        3

#define STARTAMMO   8

#define MAXVIEWWIDTH    320
#define HEIGHTRATIO     0.50

/* the canonical frame (ADR-002): 320x200, 8-bit, status bar at the bottom */
#define SCREENWIDTH     320
#define SCREENHEIGHT    200
#ifndef STATUSLINES
#define STATUSLINES     40
#endif

#define MAXWALLTILES    64 /* max number of wall tiles */
#define PMPAGESIZE      4096 /* VSWAP page: a 64x64 wall texture, column-major */

#define SPDPATROL   512
#define SPDDOG      1500

/* Rate of TimeCount (ID_SD.C runs it from the PIT at ~70 Hz). */
#define TICRATE     70

/* object flag values */
#define FL_SHOOTABLE    1
#define FL_BONUS        2
#define FL_NEVERMARK    4
#define FL_VISABLE      8
#define FL_ATTACKMODE   16
#define FL_FIRSTATTACK  32
#define FL_AMBUSH       64
#define FL_NONMARK      128

typedef enum {
    di_north,
    di_east,
    di_south,
    di_west
} controldir_t;

typedef enum {
    dr_normal,
    dr_lock1,
    dr_lock2,
    dr_lock3,
    dr_lock4,
    dr_elevator
} door_t;

typedef enum {
    ac_badobject = -1,
    ac_no,
    ac_yes,
    ac_allways
} activetype;

typedef enum {
    nothing,
    playerobj,
    inertobj,
    guardobj,
    officerobj,
    ssobj,
    dogobj,
    bossobj,
    schabbobj,
    fakeobj,
    mechahitlerobj,
    mutantobj,
    needleobj,
    fireobj,
    bjobj,
    ghostobj,
    realhitlerobj,
    gretelobj,
    giftobj,
    fatobj,
    rocketobj,

    spectreobj,
    angelobj,
    transobj,
    uberobj,
    willobj,
    deathobj,
    hrocketobj,
    sparkobj
} classtype;

typedef enum {
    dressing,
    block,
    bo_gibs,
    bo_alpo,
    bo_firstaid,
    bo_key1,
    bo_key2,
    bo_key3,
    bo_key4,
    bo_cross,
    bo_chalice,
    bo_bible,
    bo_crown,
    bo_clip,
    bo_clip2,
    bo_machinegun,
    bo_chaingun,
    bo_food,
    bo_fullheal,
    bo_25clip,
    bo_spear
} stat_t;

typedef enum {
    east,
    northeast,
    north,
    northwest,
    west,
    southwest,
    south,
    southeast,
    nodir
} dirtype;

#define NUMENEMIES 22
typedef enum {
    en_guard,
    en_officer,
    en_ss,
    en_dog,
    en_boss,
    en_schabbs,
    en_fake,
    en_hitler,
    en_mutant,
    en_blinky,
    en_clyde,
    en_pinky,
    en_inky,
    en_gretel,
    en_gift,
    en_fat,
    en_spectre,
    en_angel,
    en_trans,
    en_uber,
    en_will,
    en_death
} enemy_t;

typedef struct objstruct objtype;

typedef struct statestruct {
    int16_t rotate; /* Borland boolean was an int: pain states use 2 */
    int16_t shapenum; /* a shapenum of -1 means get from ob->temp1 */
    int16_t tictime;
    void (*think)(objtype *ob), (*action)(objtype *ob);
    struct statestruct *next;
} statetype;

/* trivial actor structure */
typedef struct statstruct {
    byte tilex, tiley;
    byte *visspot;
    int16_t shapenum; /* if shapenum == -1 the obj has been removed */
    byte flags;
    byte itemnumber;
} statobj_t;

/* door actor structure */
typedef enum { dr_open, dr_closed, dr_opening, dr_closing } dooraction_t;

typedef struct doorstruct {
    byte tilex, tiley;
    boolean vertical;
    byte lock;
    dooraction_t action;
    int16_t ticcount;
} doorobj_t;

/* thinking actor structure */
struct objstruct {
    activetype active;
    int16_t ticcount;
    classtype obclass;
    statetype *state;

    byte flags; /* FL_SHOOTABLE, etc */

    int32_t distance; /* if negative, wait for that door to open */
    dirtype dir;

    fixed x, y;
    uint16_t tilex, tiley;
    byte areanumber;

    int16_t viewx;
    uint16_t viewheight;
    fixed transx, transy; /* in global coord */

    int16_t angle;
    int16_t hitpoints;
    int32_t speed;

    int16_t temp1, temp2, temp3;
    struct objstruct *next, *prev;
};

#define NUMWEAPONS 5
typedef enum {
    wp_knife,
    wp_pistol,
    wp_machinegun,
    wp_chaingun
} weapontype;

enum {
    gd_baby,
    gd_easy,
    gd_medium,
    gd_hard
};

/* gamestate structure */
typedef struct {
    int16_t difficulty;
    int16_t mapon;
    int32_t oldscore, score, nextextra;
    int16_t lives;
    int16_t health;
    int16_t ammo;
    int16_t keys;
    weapontype bestweapon, weapon, chosenweapon;

    int16_t faceframe;
    int16_t attackframe, attackcount, weaponframe;

    int16_t episode, secretcount, treasurecount, killcount,
        secrettotal, treasuretotal, killtotal;
    int32_t TimeCount;
    int32_t killx, killy;
    boolean victoryflag; /* set during victory animations */
} gametype;

typedef enum {
    ex_stillplaying,
    ex_completed,
    ex_died,
    ex_warped,
    ex_resetgame,
    ex_loadedgame,
    ex_victorious,
    ex_abort,
    ex_demodone,
    ex_secretlevel
} exit_t;

/*
 * actorat holds either an objtype pointer or a small tile number cast to a
 * pointer (wall tile, door number | 0x80, or 1 for a blocking static), as in
 * the original. These helpers make the two cases explicit.
 */
#define ACTORAT_TILE(t)   ((objtype *)(uintptr_t)(t))
#define ACTORAT_VALUE(p)  ((uintptr_t)(p))
#define ISPOINTER(p)      ((uintptr_t)(p) > 0xffffu)
