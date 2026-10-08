/* Enemy spawning and AI (WL_ACT2.C). State tables: orig_states.c (generated). */
#include "orig.h"

#include <stdio.h>
#include <stdlib.h>

#include "id_ca.h"
#include "id_us.h"
#include "id_us_rnd.h"
#include "wl_fixed.h"

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
    if (digisoundenabled) {
        s_schabbdie2.tictime = 140;
    } else {
        s_schabbdie2.tictime = 5;
    }

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
    if (digisoundenabled) {
        s_giftdie2.tictime = 140;
    } else {
        s_giftdie2.tictime = 5;
    }

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
    if (digisoundenabled) {
        s_fatdie2.tictime = 140;
    } else {
        s_fatdie2.tictime = 5;
    }

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
    if (digisoundenabled) {
        s_hitlerdie2.tictime = 140;
    } else {
        s_hitlerdie2.tictime = 5;
    }

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
    if (digisoundenabled) {
        s_hitlerdie2.tictime = 140;
    } else {
        s_hitlerdie2.tictime = 5;
    }

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

/*
=============================================================================

                        PROJECTILES

=============================================================================
*/

#define PROJECTILESIZE 0xc000l

/*
=================
=
= A_Smoke
=
=================
*/

void A_Smoke(objtype *ob)
{
    GetNewActor();
    new->state = &s_smoke1;
    new->ticcount = 6;

    new->tilex = ob->tilex;
    new->tiley = ob->tiley;
    new->x = ob->x;
    new->y = ob->y;
    new->obclass = inertobj;
    new->active = ac_yes;

    new->flags = FL_NEVERMARK;
}

/*
===================
=
= ProjectileTryMove
=
= returns true if move ok
===================
*/

#define PROJSIZE 0x2000

boolean ProjectileTryMove(objtype *ob)
{
    int xl, yl, xh, yh, x, y;
    objtype *check;

    xl = (int)((ob->x - PROJSIZE) >> TILESHIFT);
    yl = (int)((ob->y - PROJSIZE) >> TILESHIFT);

    xh = (int)((ob->x + PROJSIZE) >> TILESHIFT);
    yh = (int)((ob->y + PROJSIZE) >> TILESHIFT);

    /* check for solid walls */
    for (y = yl; y <= yh; y++) {
        for (x = xl; x <= xh; x++) {
            if (x < 0 || y < 0 || x >= MAPSIZE || y >= MAPSIZE) {
                return false; /* not in the original: it read outside actorat */
            }
            check = actorat[x][y];
            if (check && !ISPOINTER(check)) { /* check < objlist: a tile, not an actor */
                return false;
            }
        }
    }

    return true;
}

/*
=================
=
= T_Projectile
=
=================
*/

void T_Projectile(objtype *ob)
{
    int32_t deltax, deltay;
    int damage = 0;
    int32_t speed;

    speed = (int32_t)ob->speed * tics;

    deltax = FixedByFrac(speed, costable[ob->angle]);
    deltay = -FixedByFrac(speed, sintable[ob->angle]);

    if (deltax > 0x10000l) {
        deltax = 0x10000l;
    }
    if (deltay > 0x10000l) {
        deltay = 0x10000l;
    }

    ob->x += deltax;
    ob->y += deltay;

    deltax = labs(ob->x - player->x);
    deltay = labs(ob->y - player->y);

    if (!ProjectileTryMove(ob)) {
        if (ob->obclass == rocketobj) {
            PlaySoundLocActor(MISSILEHITSND, ob);
            ob->state = &s_boom1;
        } else {
            ob->state = NULL; /* mark for removal */
        }

        return;
    }

    if (deltax < PROJECTILESIZE && deltay < PROJECTILESIZE) { /* hit the player */
        switch (ob->obclass) {
        case needleobj:
            damage = (US_RndT() >> 3) + 20;
            break;
        case rocketobj:
        case hrocketobj:
        case sparkobj:
            damage = (US_RndT() >> 3) + 30;
            break;
        case fireobj:
            damage = (US_RndT() >> 3);
            break;
        default:
            break; /* damage was uninitialized in the original */
        }

        TakeDamage(damage, ob);
        ob->state = NULL; /* mark for removal */
        return;
    }

    ob->tilex = (uint16_t)(ob->x >> TILESHIFT);
    ob->tiley = (uint16_t)(ob->y >> TILESHIFT);
}

/*
==================
=
= A_DeathScream
=
= mapon here is the map within the episode (the original subtracted
= episode*10 after loading), i.e. gamestate.mapon.
=
==================
*/

void A_DeathScream(objtype *ob)
{
    if (gamestate.mapon == 9 && !US_RndT()) {
        switch (ob->obclass) {
        case mutantobj:
        case guardobj:
        case officerobj:
        case ssobj:
        case dogobj:
            PlaySoundLocActor(DEATHSCREAM6SND, ob);
            return;
        default:
            break;
        }
    }

    switch (ob->obclass) {
    case mutantobj:
        PlaySoundLocActor(AHHHGSND, ob);
        break;

    case guardobj: {
        /* nine slots, eight sounds: the ninth is 0 and never picked */
        static const int sounds[9] = {DEATHSCREAM1SND, DEATHSCREAM2SND, DEATHSCREAM3SND,
                                      DEATHSCREAM4SND, DEATHSCREAM5SND, DEATHSCREAM7SND,
                                      DEATHSCREAM8SND, DEATHSCREAM9SND};

        PlaySoundLocActor(sounds[US_RndT() % 8], ob);
    } break;
    case officerobj:
        PlaySoundLocActor(NEINSOVASSND, ob);
        break;
    case ssobj:
        PlaySoundLocActor(LEBENSND, ob); /* JAB */
        break;
    case dogobj:
        PlaySoundLocActor(DOGDEATHSND, ob); /* JAB */
        break;
    case bossobj:
        OrigSD_PlaySound(MUTTISND); /* JAB */
        break;
    case schabbobj:
        OrigSD_PlaySound(MEINGOTTSND);
        break;
    case fakeobj:
        OrigSD_PlaySound(HITLERHASND);
        break;
    case mechahitlerobj:
        OrigSD_PlaySound(SCHEISTSND);
        break;
    case realhitlerobj:
        OrigSD_PlaySound(EVASND);
        break;
    case gretelobj:
        OrigSD_PlaySound(MEINSND);
        break;
    case giftobj:
        OrigSD_PlaySound(DONNERSND);
        break;
    case fatobj:
        OrigSD_PlaySound(ROSESND);
        break;
    default:
        break;
    }
}

/*
============================================================================

                            STAND

============================================================================
*/

/*
===============
=
= T_Stand
=
===============
*/

void T_Stand(objtype *ob)
{
    SightPlayer(ob);
}

/*
============================================================================

                            CHASE

============================================================================
*/

/*
=================
=
= T_Chase
=
=================
*/

void T_Chase(objtype *ob)
{
    int32_t move;
    int dx, dy, dist, chance;
    boolean dodge;

    if (gamestate.victoryflag) {
        return;
    }

    dodge = false;
    if (CheckLine(ob)) { /* got a shot at player? */
        dx = abs(ob->tilex - player->tilex);
        dy = abs(ob->tiley - player->tiley);
        dist = dx > dy ? dx : dy;
        if (!dist || (dist == 1 && ob->distance < 0x4000)) {
            chance = 300;
        } else {
            chance = (tics << 4) / dist;
        }

        if (US_RndT() < chance) {
            /* go into attack frame */
            switch (ob->obclass) {
            case guardobj:
                NewState(ob, &s_grdshoot1);
                break;
            case officerobj:
                NewState(ob, &s_ofcshoot1);
                break;
            case mutantobj:
                NewState(ob, &s_mutshoot1);
                break;
            case ssobj:
                NewState(ob, &s_ssshoot1);
                break;
            case bossobj:
                NewState(ob, &s_bossshoot1);
                break;
            case gretelobj:
                NewState(ob, &s_gretelshoot1);
                break;
            case mechahitlerobj:
                NewState(ob, &s_mechashoot1);
                break;
            case realhitlerobj:
                NewState(ob, &s_hitlershoot1);
                break;
            default:
                break;
            }
            return;
        }
        dodge = true;
    }

    if (ob->dir == nodir) {
        if (dodge) {
            SelectDodgeDir(ob);
        } else {
            SelectChaseDir(ob);
        }
        if (ob->dir == nodir) {
            return; /* object is blocked in */
        }
    }

    move = ob->speed * tics;

    while (move) {
        if (ob->distance < 0) {
            /* waiting for a door to open */
            OpenDoor(-ob->distance - 1);
            if (doorobjlist[-ob->distance - 1].action != dr_open) {
                return;
            }
            ob->distance = TILEGLOBAL; /* go ahead, the door is now opoen */
        }

        if (move < ob->distance) {
            MoveObj(ob, move);
            break;
        }

        /* reached goal tile, so select another one */

        /* fix position to account for round off during moving */
        ob->x = ((fixed)ob->tilex << TILESHIFT) + TILEGLOBAL / 2;
        ob->y = ((fixed)ob->tiley << TILESHIFT) + TILEGLOBAL / 2;

        move -= ob->distance;

        if (dodge) {
            SelectDodgeDir(ob);
        } else {
            SelectChaseDir(ob);
        }

        if (ob->dir == nodir) {
            return; /* object is blocked in */
        }
    }
}

/*
=================
=
= T_Ghosts
=
=================
*/

void T_Ghosts(objtype *ob)
{
    int32_t move;

    if (ob->dir == nodir) {
        SelectChaseDir(ob);
        if (ob->dir == nodir) {
            return; /* object is blocked in */
        }
    }

    move = ob->speed * tics;

    while (move) {
        if (move < ob->distance) {
            MoveObj(ob, move);
            break;
        }

        /* reached goal tile, so select another one */

        /* fix position to account for round off during moving */
        ob->x = ((fixed)ob->tilex << TILESHIFT) + TILEGLOBAL / 2;
        ob->y = ((fixed)ob->tiley << TILESHIFT) + TILEGLOBAL / 2;

        move -= ob->distance;

        SelectChaseDir(ob);

        if (ob->dir == nodir) {
            return; /* object is blocked in */
        }
    }
}

/*
=================
=
= T_DogChase
=
=================
*/

void T_DogChase(objtype *ob)
{
    int32_t move;
    int32_t dx, dy;

    if (ob->dir == nodir) {
        SelectDodgeDir(ob);
        if (ob->dir == nodir) {
            return; /* object is blocked in */
        }
    }

    move = ob->speed * tics;

    while (move) {
        /* check for byte range */
        dx = player->x - ob->x;
        if (dx < 0) {
            dx = -dx;
        }
        dx -= move;
        if (dx <= MINACTORDIST) {
            dy = player->y - ob->y;
            if (dy < 0) {
                dy = -dy;
            }
            dy -= move;
            if (dy <= MINACTORDIST) {
                NewState(ob, &s_dogjump1);
                return;
            }
        }

        if (move < ob->distance) {
            MoveObj(ob, move);
            break;
        }

        /* reached goal tile, so select another one */

        /* fix position to account for round off during moving */
        ob->x = ((fixed)ob->tilex << TILESHIFT) + TILEGLOBAL / 2;
        ob->y = ((fixed)ob->tiley << TILESHIFT) + TILEGLOBAL / 2;

        move -= ob->distance;

        SelectDodgeDir(ob);

        if (ob->dir == nodir) {
            return; /* object is blocked in */
        }
    }
}

/*
============================================================================

                            PATH

============================================================================
*/

/*
===============
=
= SelectPathDir
=
===============
*/

void SelectPathDir(objtype *ob)
{
    unsigned spot;

    spot = (uint16_t)(*(mapsegs[1] + farmapylookup[ob->tiley] + ob->tilex) - ICONARROWS);

    if (spot < 8) {
        /* new direction */
        ob->dir = (dirtype)spot;
    }

    ob->distance = TILEGLOBAL;

    if (!TryWalk(ob)) {
        ob->dir = nodir;
    }
}

/*
===============
=
= T_Path
=
===============
*/

void T_Path(objtype *ob)
{
    int32_t move;

    if (SightPlayer(ob)) {
        return;
    }

    if (ob->dir == nodir) {
        SelectPathDir(ob);
        if (ob->dir == nodir) {
            return; /* all movement is blocked */
        }
    }

    move = ob->speed * tics;

    while (move) {
        if (ob->distance < 0) {
            /* waiting for a door to open */
            OpenDoor(-ob->distance - 1);
            if (doorobjlist[-ob->distance - 1].action != dr_open) {
                return;
            }
            ob->distance = TILEGLOBAL; /* go ahead, the door is now opoen */
        }

        if (move < ob->distance) {
            MoveObj(ob, move);
            break;
        }

        if (ob->tilex > MAPSIZE || ob->tiley > MAPSIZE) {
            char str[80];
            snprintf(str, sizeof(str), "T_Path hit a wall at %u,%u, dir %u", ob->tilex,
                     ob->tiley, (unsigned)ob->dir);
            Quit(str);
        }

        ob->x = ((fixed)ob->tilex << TILESHIFT) + TILEGLOBAL / 2;
        ob->y = ((fixed)ob->tiley << TILESHIFT) + TILEGLOBAL / 2;
        move -= ob->distance;

        SelectPathDir(ob);

        if (ob->dir == nodir) {
            return; /* all movement is blocked */
        }
    }
}

/*
=============================================================================

                            FIGHT

=============================================================================
*/

/*
===============
=
= T_Shoot
=
= Try to damage the player, based on skill level and player's speed
=
===============
*/

void T_Shoot(objtype *ob)
{
    int dx, dy, dist;
    int hitchance, damage;

    hitchance = 128;

    if (!AREABYPLAYER(ob->areanumber)) {
        return;
    }

    if (!CheckLine(ob)) { /* player is behind a wall */
        return;
    }

    dx = abs(ob->tilex - player->tilex);
    dy = abs(ob->tiley - player->tiley);
    dist = dx > dy ? dx : dy;

    if (ob->obclass == ssobj || ob->obclass == bossobj) {
        dist = dist * 2 / 3; /* ss are better shots */
    }

    if (thrustspeed >= RUNSPEED) {
        if (ob->flags & FL_VISABLE) {
            hitchance = 160 - dist * 16; /* player can see to dodge */
        } else {
            hitchance = 160 - dist * 8;
        }
    } else {
        if (ob->flags & FL_VISABLE) {
            hitchance = 256 - dist * 16; /* player can see to dodge */
        } else {
            hitchance = 256 - dist * 8;
        }
    }

    /* see if the shot was a hit */

    if (US_RndT() < hitchance) {
        if (dist < 2) {
            damage = US_RndT() >> 2;
        } else if (dist < 4) {
            damage = US_RndT() >> 3;
        } else {
            damage = US_RndT() >> 4;
        }

        TakeDamage(damage, ob);
    }

    switch (ob->obclass) {
    case ssobj:
        PlaySoundLocActor(SSFIRESND, ob);
        break;
    case giftobj:
    case fatobj:
        PlaySoundLocActor(MISSILEFIRESND, ob);
        break;
    case mechahitlerobj:
    case realhitlerobj:
    case bossobj:
        PlaySoundLocActor(BOSSFIRESND, ob);
        break;
    case schabbobj:
        PlaySoundLocActor(SCHABBSTHROWSND, ob);
        break;
    case fakeobj:
        PlaySoundLocActor(FLAMETHROWERSND, ob);
        break;
    default:
        PlaySoundLocActor(NAZIFIRESND, ob);
    }
}

/*
===============
=
= T_Bite
=
===============
*/

void T_Bite(objtype *ob)
{
    int32_t dx, dy;

    PlaySoundLocActor(DOGATTACKSND, ob); /* JAB */

    dx = player->x - ob->x;
    if (dx < 0) {
        dx = -dx;
    }
    dx -= TILEGLOBAL;
    if (dx <= MINACTORDIST) {
        dy = player->y - ob->y;
        if (dy < 0) {
            dy = -dy;
        }
        dy -= TILEGLOBAL;
        if (dy <= MINACTORDIST) {
            if (US_RndT() < 180) {
                TakeDamage(US_RndT() >> 4, ob);
                return;
            }
        }
    }
}
