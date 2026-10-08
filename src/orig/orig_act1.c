/* Statics and doors (WL_ACT1.C): level setup part. Door movement comes in S5. */
#include "orig.h"

#include <string.h>

#include "id_ca.h"
#include "id_us.h"

/*
=============================================================================

                            STATICS

=============================================================================
*/

statobj_t statobjlist[MAXSTATS], *laststatobj;

static const struct {
    int16_t picnum;
    stat_t type;
} statinfo[] = {
    {SPR_STAT_0, dressing},         /* puddle          spr1v */
    {SPR_STAT_1, block},            /* Green Barrel    " */
    {SPR_STAT_2, block},            /* Table/chairs    " */
    {SPR_STAT_3, block},            /* Floor lamp      " */
    {SPR_STAT_4, dressing},         /* Chandelier      " */
    {SPR_STAT_5, block},            /* Hanged man      " */
    {SPR_STAT_6, bo_alpo},          /* Bad food        " */
    {SPR_STAT_7, block},            /* Red pillar      " */
    /* NEW PAGE */
    {SPR_STAT_8, block},            /* Tree            spr2v */
    {SPR_STAT_9, dressing},         /* Skeleton flat   " */
    {SPR_STAT_10, block},           /* Sink            " */
    {SPR_STAT_11, block},           /* Potted plant    " */
    {SPR_STAT_12, block},           /* Urn             " */
    {SPR_STAT_13, block},           /* Bare table      " */
    {SPR_STAT_14, dressing},        /* Ceiling light   " */
    {SPR_STAT_15, dressing},        /* Kitchen stuff   " */
    /* NEW PAGE */
    {SPR_STAT_16, block},           /* suit of armor   spr3v */
    {SPR_STAT_17, block},           /* Hanging cage    " */
    {SPR_STAT_18, block},           /* SkeletoninCage  " */
    {SPR_STAT_19, dressing},        /* Skeleton relax  " */
    {SPR_STAT_20, bo_key1},         /* Key 1           " */
    {SPR_STAT_21, bo_key2},         /* Key 2           " */
    {SPR_STAT_22, block},           /* stuff */
    {SPR_STAT_23, dressing},        /* stuff */
    /* NEW PAGE */
    {SPR_STAT_24, bo_food},         /* Good food       spr4v */
    {SPR_STAT_25, bo_firstaid},     /* First aid       " */
    {SPR_STAT_26, bo_clip},         /* Clip            " */
    {SPR_STAT_27, bo_machinegun},   /* Machine gun     " */
    {SPR_STAT_28, bo_chaingun},     /* Gatling gun     " */
    {SPR_STAT_29, bo_cross},        /* Cross           " */
    {SPR_STAT_30, bo_chalice},      /* Chalice         " */
    {SPR_STAT_31, bo_bible},        /* Bible           " */
    /* NEW PAGE */
    {SPR_STAT_32, bo_crown},        /* crown           spr5v */
    {SPR_STAT_33, bo_fullheal},     /* one up          " */
    {SPR_STAT_34, bo_gibs},         /* gibs            " */
    {SPR_STAT_35, block},           /* barrel          " */
    {SPR_STAT_36, block},           /* well            " */
    {SPR_STAT_37, block},           /* Empty well      " */
    {SPR_STAT_38, bo_gibs},         /* Gibs 2          " */
    {SPR_STAT_39, block},           /* flag            " */
    /* NEW PAGE */
    {SPR_STAT_40, block},           /* Call Apogee     spr7v */
    /* NEW PAGE */
    {SPR_STAT_41, dressing},        /* junk            " */
    {SPR_STAT_42, dressing},        /* junk            " */
    {SPR_STAT_43, dressing},        /* junk            " */
    {SPR_STAT_44, dressing},        /* pots            " */
    {SPR_STAT_45, block},           /* stove           " */
    {SPR_STAT_46, block},           /* spears          " */
    {SPR_STAT_47, dressing},        /* vines           " */
    /* NEW PAGE */
    {SPR_STAT_26, bo_clip2},        /* Clip            " */
    {-1, dressing}                  /* terminator */
};

#define NUMSTATINFO ((int)(sizeof(statinfo) / sizeof(statinfo[0])))

/*
===============
=
= InitStaticList
=
===============
*/

void InitStaticList(void)
{
    laststatobj = &statobjlist[0];
}

/*
===============
=
= SpawnStatic
=
= Info-plane tiles 72-74 give types past the end of the non-SPEAR table;
= the original read whatever followed it. Those are clamped to the
= terminator here, which spawns an already-removed object (shapenum -1).
=
= bo_clip2 has no case in the switch, so flags and itemnumber keep whatever
= the slot held before, exactly as in the original.
=
===============
*/

void SpawnStatic(int tilex, int tiley, int type)
{
    if (type < 0 || type >= NUMSTATINFO) {
        type = NUMSTATINFO - 1;
    }

    laststatobj->shapenum = statinfo[type].picnum;
    laststatobj->tilex = (byte)tilex;
    laststatobj->tiley = (byte)tiley;
    laststatobj->visspot = &spotvis[tilex][tiley];

    switch (statinfo[type].type) {
    case block:
        actorat[tilex][tiley] = ACTORAT_TILE(1); /* consider it a blocking tile */
        /* fall through */
    case dressing:
        laststatobj->flags = 0;
        break;

    case bo_cross:
    case bo_chalice:
    case bo_bible:
    case bo_crown:
    case bo_fullheal:
        if (!loadedgame) {
            gamestate.treasuretotal++;
        }
        /* fall through */

    case bo_firstaid:
    case bo_key1:
    case bo_key2:
    case bo_key3:
    case bo_key4:
    case bo_clip:
    case bo_25clip:
    case bo_machinegun:
    case bo_chaingun:
    case bo_food:
    case bo_alpo:
    case bo_gibs:
    case bo_spear:
        laststatobj->flags = FL_BONUS;
        laststatobj->itemnumber = (byte)statinfo[type].type;
        break;

    default:
        break;
    }

    laststatobj++;

    if (laststatobj == &statobjlist[MAXSTATS]) {
        Quit("Too many static objects!\n");
    }
}

/*
===============
=
= PlaceItemType
=
= Called during game play to drop actors' items.  It finds the proper
= item number based on the item type (bo_???).  If there are no free item
= spots, nothing is done.
=
===============
*/

void PlaceItemType(int itemtype, int tilex, int tiley)
{
    int type;
    statobj_t *spot;

    /* find the item number */
    for (type = 0;; type++) {
        if (statinfo[type].picnum == -1) { /* end of list */
            Quit("PlaceItemType: couldn't find type!");
        }
        if ((int)statinfo[type].type == itemtype) {
            break;
        }
    }

    /* find a spot in statobjlist to put it in */
    for (spot = &statobjlist[0];; spot++) {
        if (spot == laststatobj) {
            if (spot == &statobjlist[MAXSTATS]) {
                return; /* no free spots */
            }
            laststatobj++; /* space at end */
            break;
        }

        if (spot->shapenum == -1) { /* -1 is a free spot */
            break;
        }
    }

    /* place it */
    spot->shapenum = statinfo[type].picnum;
    spot->tilex = (byte)tilex;
    spot->tiley = (byte)tiley;
    spot->visspot = &spotvis[tilex][tiley];
    spot->flags = FL_BONUS;
    spot->itemnumber = (byte)statinfo[type].type;
}

/*
=============================================================================

                            DOORS

doorobjlist[] holds most of the information for the doors

doorposition[] holds the amount the door is open, ranging from 0 to 0xffff

The number of doors is limited to 64 because a spot in tilemap holds the
door number in the low 6 bits, with the high bit meaning a door center
and bit 6 meaning a door side tile

Open doors conect two areas, so sounds will travel between them and sight
will be checked when the player is in a connected area.

Areaconnect is incremented/decremented by each door. If >0 they connect

Every time a door opens or closes the areabyplayer matrix gets recalculated.
An area is true if it connects with the player's current spor.

=============================================================================
*/

doorobj_t doorobjlist[MAXDOORS], *lastdoorobj;
int16_t doornum;

uint16_t doorposition[MAXDOORS]; /* leading edge of door 0=closed, 0xffff = fully open */

byte areaconnect[NUMAREAS][NUMAREAS];

boolean areabyplayer[NUMAREAS];

/*
==============
=
= ConnectAreas
=
= Scans outward from playerarea, marking all connected areas
=
==============
*/

static void RecursiveConnect(int areanumber)
{
    int i;

    for (i = 0; i < NUMAREAS; i++) {
        if (areaconnect[areanumber][i] && !areabyplayer[i]) {
            areabyplayer[i] = true;
            RecursiveConnect(i);
        }
    }
}

void ConnectAreas(void)
{
    memset(areabyplayer, 0, sizeof(areabyplayer));
    areabyplayer[player->areanumber] = true;
    RecursiveConnect(player->areanumber);
}

void InitAreas(void)
{
    memset(areabyplayer, 0, sizeof(areabyplayer));
    areabyplayer[player->areanumber] = true;
}

/*
===============
=
= InitDoorList
=
===============
*/

void InitDoorList(void)
{
    memset(areabyplayer, 0, sizeof(areabyplayer));
    memset(areaconnect, 0, sizeof(areaconnect));

    lastdoorobj = &doorobjlist[0];
    doornum = 0;
}

/*
===============
=
= SpawnDoor
=
===============
*/

void SpawnDoor(int tilex, int tiley, boolean vertical, int lock)
{
    uint16_t *map;

    if (doornum == MAXDOORS) {
        Quit("64+ doors on level!");
    }
    /* Not in the original: a door on the map edge would write outside tilemap. */
    if (tilex < 1 || tiley < 1 || tilex > MAPSIZE - 2 || tiley > MAPSIZE - 2) {
        Quit("SpawnDoor: door on the map edge!");
    }

    doorposition[doornum] = 0; /* doors start out fully closed */
    lastdoorobj->tilex = (byte)tilex;
    lastdoorobj->tiley = (byte)tiley;
    lastdoorobj->vertical = vertical;
    lastdoorobj->lock = (byte)lock;
    lastdoorobj->action = dr_closed;

    actorat[tilex][tiley] = ACTORAT_TILE(doornum | 0x80); /* consider it a solid wall */

    /* make the door tile a special tile, and mark the adjacent tiles for door sides */
    tilemap[tilex][tiley] = (byte)(doornum | 0x80);
    map = mapsegs[0] + farmapylookup[tiley] + tilex;
    if (vertical) {
        *map = *(map - 1); /* set area number */
        tilemap[tilex][tiley - 1] |= 0x40;
        tilemap[tilex][tiley + 1] |= 0x40;
    } else {
        *map = *(map - mapwidth); /* set area number */
        tilemap[tilex - 1][tiley] |= 0x40;
        tilemap[tilex + 1][tiley] |= 0x40;
    }

    doornum++;
    lastdoorobj++;
}
