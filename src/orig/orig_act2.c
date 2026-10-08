/* Enemy spawning (WL_ACT2.C). The full state tables and AI come in S2. */
#include "orig.h"

#include "id_ca.h"

const int16_t starthitpoints[4][NUMENEMIES] = {
    /* BABY MODE */
    {25,   /* guards */
     50,   /* officer */
     100,  /* SS */
     1,    /* dogs */
     850,  /* Hans */
     850,  /* Schabbs */
     200,  /* fake hitler */
     800,  /* mecha hitler */
     45,   /* mutants */
     25,   /* ghosts */
     25,   /* ghosts */
     25,   /* ghosts */
     25,   /* ghosts */

     850,  /* Gretel */
     850,  /* Gift */
     850,  /* Fat */
     5,    /* en_spectre, */
     1450, /* en_angel, */
     850,  /* en_trans, */
     1050, /* en_uber, */
     950,  /* en_will, */
     1250}, /* en_death */
    /* DON'T HURT ME MODE */
    {25, 50, 100, 1, 950, 950, 300, 950, 55, 25, 25, 25, 25,
     950, 950, 950, 10, 1550, 950, 1150, 1050, 1350},
    /* BRING 'EM ON MODE */
    {25, 50, 100, 1, 1050, 1550, 400, 1050, 55, 25, 25, 25, 25,
     1050, 1050, 1050, 15, 1650, 1050, 1250, 1150, 1450},
    /* DEATH INCARNATE MODE */
    {25, 50, 100, 1, 1200, 2400, 500, 1200, 65, 25, 25, 25, 25,
     1200, 1200, 1200, 25, 2000, 1200, 1400, 1300, 1600}};

/*
 * S1 subset: the states actors are spawned in, with the original rotate,
 * shape and tictime (tictime feeds US_RndT in SpawnNewObj). think/action and
 * links to states outside this subset are NULL until S2 adds the full tables.
 */
statetype s_grdstand = {true, SPR_GRD_S_1, 0, NULL, NULL, &s_grdstand};
statetype s_grdpath1 = {true, SPR_GRD_W1_1, 20, NULL, NULL, NULL};
statetype s_grddie4 = {false, SPR_GRD_DEAD, 0, NULL, NULL, &s_grddie4};

statetype s_ofcstand = {true, SPR_OFC_S_1, 0, NULL, NULL, &s_ofcstand};
statetype s_ofcpath1 = {true, SPR_OFC_W1_1, 20, NULL, NULL, NULL};

statetype s_ssstand = {true, SPR_SS_S_1, 0, NULL, NULL, &s_ssstand};
statetype s_sspath1 = {true, SPR_SS_W1_1, 20, NULL, NULL, NULL};

statetype s_mutstand = {true, SPR_MUT_S_1, 0, NULL, NULL, &s_mutstand};
statetype s_mutpath1 = {true, SPR_MUT_W1_1, 20, NULL, NULL, NULL};

statetype s_dogpath1 = {true, SPR_DOG_W1_1, 20, NULL, NULL, NULL};

statetype s_bossstand = {false, SPR_BOSS_W1, 0, NULL, NULL, &s_bossstand};
statetype s_gretelstand = {false, SPR_GRETEL_W1, 0, NULL, NULL, &s_gretelstand};
statetype s_schabbstand = {false, SPR_SCHABB_W1, 0, NULL, NULL, &s_schabbstand};
statetype s_giftstand = {false, SPR_GIFT_W1, 0, NULL, NULL, &s_giftstand};
statetype s_fatstand = {false, SPR_FAT_W1, 0, NULL, NULL, &s_fatstand};
statetype s_fakestand = {false, SPR_FAKE_W1, 0, NULL, NULL, &s_fakestand};
statetype s_mechastand = {false, SPR_MECHA_W1, 0, NULL, NULL, &s_mechastand};

statetype s_blinkychase1 = {false, SPR_BLINKY_W1, 10, NULL, NULL, NULL};
statetype s_inkychase1 = {false, SPR_INKY_W1, 10, NULL, NULL, NULL};
statetype s_pinkychase1 = {false, SPR_PINKY_W1, 10, NULL, NULL, NULL};
statetype s_clydechase1 = {false, SPR_CLYDE_W1, 10, NULL, NULL, NULL};

/*
 * The boss spawners also set the tictime of their death state (140 with
 * digitized sound, else 5). Those states arrive with S2.
 */

/*
===============
=
= SpawnStand
=
= en_dog has no case, so a standing dog (info tiles 134-137) spawns nothing
= and the fields below are written to the previously spawned actor. This is
= the original behaviour and is kept on purpose.
=
===============
*/

void SpawnStand(enemy_t which, int tilex, int tiley, int dir)
{
    uint16_t *map, tile;

    switch (which) {
    case en_guard:
        SpawnNewObj(tilex, tiley, &s_grdstand);
        new->speed = SPDPATROL;
        if (!loadedgame) {
            gamestate.killtotal++;
        }
        break;

    case en_officer:
        SpawnNewObj(tilex, tiley, &s_ofcstand);
        new->speed = SPDPATROL;
        if (!loadedgame) {
            gamestate.killtotal++;
        }
        break;

    case en_mutant:
        SpawnNewObj(tilex, tiley, &s_mutstand);
        new->speed = SPDPATROL;
        if (!loadedgame) {
            gamestate.killtotal++;
        }
        break;

    case en_ss:
        SpawnNewObj(tilex, tiley, &s_ssstand);
        new->speed = SPDPATROL;
        if (!loadedgame) {
            gamestate.killtotal++;
        }
        break;

    default:
        break;
    }

    map = mapsegs[0] + farmapylookup[tiley] + tilex;
    if (*map == AMBUSHTILE) {
        tilemap[tilex][tiley] = 0;

        tile = 0; /* left uninitialized in the original */
        if (tilex < MAPSIZE - 1 && *(map + 1) >= AREATILE) {
            tile = *(map + 1);
        }
        if (tiley > 0 && *(map - mapwidth) >= AREATILE) {
            tile = *(map - mapwidth);
        }
        if (tiley < MAPSIZE - 1 && *(map + mapwidth) >= AREATILE) {
            tile = *(map + mapwidth);
        }
        if (tilex > 0 && *(map - 1) >= AREATILE) {
            tile = *(map - 1);
        }

        *map = tile;
        new->areanumber = (byte)(tile - AREATILE);

        new->flags |= FL_AMBUSH;
    }

    new->obclass = (classtype)(guardobj + which);
    new->hitpoints = starthitpoints[gamestate.difficulty][which];
    new->dir = (dirtype)(dir * 2);
    new->flags |= FL_SHOOTABLE;
}

/*
===============
=
= SpawnDeadGuard
=
===============
*/

void SpawnDeadGuard(int tilex, int tiley)
{
    SpawnNewObj(tilex, tiley, &s_grddie4);
    new->obclass = inertobj;
}

/*
===============
=
= SpawnBoss
=
===============
*/

void SpawnBoss(int tilex, int tiley)
{
    SpawnNewObj(tilex, tiley, &s_bossstand);
    new->speed = SPDPATROL;

    new->obclass = bossobj;
    new->hitpoints = starthitpoints[gamestate.difficulty][en_boss];
    new->dir = south;
    new->flags |= FL_SHOOTABLE | FL_AMBUSH;
    if (!loadedgame) {
        gamestate.killtotal++;
    }
}

/*
===============
=
= SpawnGretel
=
===============
*/

void SpawnGretel(int tilex, int tiley)
{
    SpawnNewObj(tilex, tiley, &s_gretelstand);
    new->speed = SPDPATROL;

    new->obclass = gretelobj;
    new->hitpoints = starthitpoints[gamestate.difficulty][en_gretel];
    new->dir = north;
    new->flags |= FL_SHOOTABLE | FL_AMBUSH;
    if (!loadedgame) {
        gamestate.killtotal++;
    }
}

/*
===============
=
= SpawnPatrol
=
===============
*/

void SpawnPatrol(enemy_t which, int tilex, int tiley, int dir)
{
    switch (which) {
    case en_guard:
        SpawnNewObj(tilex, tiley, &s_grdpath1);
        new->speed = SPDPATROL;
        if (!loadedgame) {
            gamestate.killtotal++;
        }
        break;

    case en_officer:
        SpawnNewObj(tilex, tiley, &s_ofcpath1);
        new->speed = SPDPATROL;
        if (!loadedgame) {
            gamestate.killtotal++;
        }
        break;

    case en_ss:
        SpawnNewObj(tilex, tiley, &s_sspath1);
        new->speed = SPDPATROL;
        if (!loadedgame) {
            gamestate.killtotal++;
        }
        break;

    case en_mutant:
        SpawnNewObj(tilex, tiley, &s_mutpath1);
        new->speed = SPDPATROL;
        if (!loadedgame) {
            gamestate.killtotal++;
        }
        break;

    case en_dog:
        SpawnNewObj(tilex, tiley, &s_dogpath1);
        new->speed = SPDDOG;
        if (!loadedgame) {
            gamestate.killtotal++;
        }
        break;

    default:
        break;
    }

    new->obclass = (classtype)(guardobj + which);
    new->dir = (dirtype)(dir * 2);
    new->hitpoints = starthitpoints[gamestate.difficulty][which];
    new->distance = tileglobal;
    new->flags |= FL_SHOOTABLE;
    new->active = ac_yes;

    actorat[new->tilex][new->tiley] = NULL; /* don't use original spot */

    switch (dir) {
    case 0:
        new->tilex++;
        break;
    case 1:
        new->tiley--;
        break;
    case 2:
        new->tilex--;
        break;
    case 3:
        new->tiley++;
        break;
    }

    actorat[new->tilex][new->tiley] = new;
}

/*
===============
=
= SpawnGhosts
=
===============
*/

void SpawnGhosts(int which, int tilex, int tiley)
{
    switch (which) {
    case en_blinky:
        SpawnNewObj(tilex, tiley, &s_blinkychase1);
        break;
    case en_clyde:
        SpawnNewObj(tilex, tiley, &s_clydechase1);
        break;
    case en_pinky:
        SpawnNewObj(tilex, tiley, &s_pinkychase1);
        break;
    case en_inky:
        SpawnNewObj(tilex, tiley, &s_inkychase1);
        break;
    }

    new->obclass = ghostobj;
    new->speed = SPDDOG;

    new->dir = east;
    new->flags |= FL_AMBUSH;
    if (!loadedgame) {
        gamestate.killtotal++;
    }
}

/*
===============
=
= SpawnSchabbs
=
===============
*/

void SpawnSchabbs(int tilex, int tiley)
{
    SpawnNewObj(tilex, tiley, &s_schabbstand);
    new->speed = SPDPATROL;

    new->obclass = schabbobj;
    new->hitpoints = starthitpoints[gamestate.difficulty][en_schabbs];
    new->dir = south;
    new->flags |= FL_SHOOTABLE | FL_AMBUSH;
    if (!loadedgame) {
        gamestate.killtotal++;
    }
}

/*
===============
=
= SpawnGift
=
===============
*/

void SpawnGift(int tilex, int tiley)
{
    SpawnNewObj(tilex, tiley, &s_giftstand);
    new->speed = SPDPATROL;

    new->obclass = giftobj;
    new->hitpoints = starthitpoints[gamestate.difficulty][en_gift];
    new->dir = north;
    new->flags |= FL_SHOOTABLE | FL_AMBUSH;
    if (!loadedgame) {
        gamestate.killtotal++;
    }
}

/*
===============
=
= SpawnFat
=
===============
*/

void SpawnFat(int tilex, int tiley)
{
    SpawnNewObj(tilex, tiley, &s_fatstand);
    new->speed = SPDPATROL;

    new->obclass = fatobj;
    new->hitpoints = starthitpoints[gamestate.difficulty][en_fat];
    new->dir = south;
    new->flags |= FL_SHOOTABLE | FL_AMBUSH;
    if (!loadedgame) {
        gamestate.killtotal++;
    }
}

/*
===============
=
= SpawnFakeHitler
=
===============
*/

void SpawnFakeHitler(int tilex, int tiley)
{
    SpawnNewObj(tilex, tiley, &s_fakestand);
    new->speed = SPDPATROL;

    new->obclass = fakeobj;
    new->hitpoints = starthitpoints[gamestate.difficulty][en_fake];
    new->dir = north;
    new->flags |= FL_SHOOTABLE | FL_AMBUSH;
    if (!loadedgame) {
        gamestate.killtotal++;
    }
}

/*
===============
=
= SpawnHitler
=
===============
*/

void SpawnHitler(int tilex, int tiley)
{
    SpawnNewObj(tilex, tiley, &s_mechastand);
    new->speed = SPDPATROL;

    new->obclass = mechahitlerobj;
    new->hitpoints = starthitpoints[gamestate.difficulty][en_hitler];
    new->dir = south;
    new->flags |= FL_SHOOTABLE | FL_AMBUSH;
    if (!loadedgame) {
        gamestate.killtotal++;
    }
}
