/*
 * Player (WL_AGENT.C): movement and collision, weapons, use, items, damage
 * and score. Status-bar drawing goes through orig_hooks.h.
 */
#include "orig.h"

#include <stdlib.h>

#include "id_ca.h"
#include "id_us_rnd.h"
#include "wl_fixed.h"

/*
=============================================================================

                         LOCAL CONSTANTS

=============================================================================
*/

#define MOVESCALE     150l
#define BACKMOVESCALE 100l
#define ANGLESCALE    20

/*
=============================================================================

                         GLOBAL VARIABLES

=============================================================================
*/

/* player state info */
boolean running;
int32_t thrustspeed;

uint16_t plux, pluy; /* player coordinates scaled to unsigned */

int16_t anglefrac;
int16_t gotgatgun; /* JR */

objtype *LastAttacker;

int16_t facecount;

statetype s_player = {false, 0, 0, T_Player, NULL, NULL};
statetype s_attack = {false, 0, 0, T_Attack, NULL, NULL};

int32_t playerxmove, playerymove;

/* attack is 1 for gun, 2 for knife; Borland char was signed */
static const struct atkinf {
    int8_t tics, attack, frame;
} attackinfo[4][14] = {
    {{6, 0, 1}, {6, 2, 2}, {6, 0, 3}, {6, -1, 4}},
    {{6, 0, 1}, {6, 1, 2}, {6, 0, 3}, {6, -1, 4}},
    {{6, 0, 1}, {6, 1, 2}, {6, 3, 3}, {6, -1, 4}},
    {{6, 0, 1}, {6, 1, 2}, {6, 4, 3}, {6, -1, 4}},
};

/*
=============================================================================

                        CONTROL STUFF

=============================================================================
*/

/*
======================
=
= CheckWeaponChange
=
= Keys 1-4 change weapons
=
======================
*/

void CheckWeaponChange(void)
{
    int i;

    if (!gamestate.ammo) { /* must use knife with no ammo */
        return;
    }

    for (i = wp_knife; i <= (int)gamestate.bestweapon; i++) {
        if (buttonstate[bt_readyknife + i - wp_knife]) {
            gamestate.weapon = gamestate.chosenweapon = (weapontype)i;
            DrawWeapon();
            return;
        }
    }
}

/*
=======================
=
= ControlMovement
=
= Takes controlx,controly, and buttonstate[bt_strafe]
=
= Changes the player's angle and position
=
= There is an angle hack because when going 70 fps, the roundoff becomes
= significant
=
=======================
*/

void ControlMovement(objtype *ob)
{
    int32_t oldx, oldy;
    int angle;
    int angleunits;

    thrustspeed = 0;

    oldx = player->x;
    oldy = player->y;

    /* side to side move */
    if (buttonstate[bt_strafe]) {
        /* strafing */
        if (controlx > 0) {
            angle = ob->angle - ANGLES / 4;
            if (angle < 0) {
                angle += ANGLES;
            }
            Thrust(angle, controlx * MOVESCALE); /* move to left */
        } else if (controlx < 0) {
            angle = ob->angle + ANGLES / 4;
            if (angle >= ANGLES) {
                angle -= ANGLES;
            }
            Thrust(angle, -controlx * MOVESCALE); /* move to right */
        }
    } else {
        /* not strafing */
        anglefrac = (int16_t)(anglefrac + controlx);
        angleunits = anglefrac / ANGLESCALE;
        anglefrac = (int16_t)(anglefrac - angleunits * ANGLESCALE);
        ob->angle = (int16_t)(ob->angle - angleunits);

        if (ob->angle >= ANGLES) {
            ob->angle -= ANGLES;
        }
        if (ob->angle < 0) {
            ob->angle += ANGLES;
        }
    }

    /* forward/backwards move */
    if (controly < 0) {
        Thrust(ob->angle, -controly * MOVESCALE); /* move forwards */
    } else if (controly > 0) {
        angle = ob->angle + ANGLES / 2;
        if (angle >= ANGLES) {
            angle -= ANGLES;
        }
        Thrust(angle, controly * BACKMOVESCALE); /* move backwards */
    }

    if (gamestate.victoryflag) { /* watching the BJ actor */
        return;
    }

    /* calculate total move */
    playerxmove = player->x - oldx;
    playerymove = player->y - oldy;
}

/*
=============================================================================

                    STATUS WINDOW STUFF

=============================================================================
*/

/*
===============
=
= UpdateFace
=
= Calls draw face if time to change
=
===============
*/

void UpdateFace(void)
{
    if (OrigSD_SoundPlaying() == GETGATLINGSND) {
        return;
    }

    facecount = (int16_t)(facecount + tics);
    if (facecount > US_RndT()) {
        gamestate.faceframe = (int16_t)(US_RndT() >> 6);
        if (gamestate.faceframe == 3) {
            gamestate.faceframe = 1;
        }

        facecount = 0;
        DrawFace();
    }
}

/*
===============
=
= TakeDamage
=
===============
*/

void TakeDamage(int points, objtype *attacker)
{
    LastAttacker = attacker;

    if (gamestate.victoryflag) {
        return;
    }
    if (gamestate.difficulty == gd_baby) {
        points >>= 2;
    }

    if (!godmode) {
        gamestate.health = (int16_t)(gamestate.health - points);
    }

    if (gamestate.health <= 0) {
        gamestate.health = 0;
        playstate = ex_died;
        killerobj = attacker;
    }

    StartDamageFlash(points);

    gotgatgun = 0;

    DrawHealth();
    DrawFace();
}

/*
===============
=
= HealSelf
=
===============
*/

void HealSelf(int points)
{
    gamestate.health = (int16_t)(gamestate.health + points);
    if (gamestate.health > 100) {
        gamestate.health = 100;
    }

    DrawHealth();
    gotgatgun = 0; /* JR */
    DrawFace();
}

/*
===============
=
= GiveExtraMan
=
===============
*/

void GiveExtraMan(void)
{
    if (gamestate.lives < 9) {
        gamestate.lives++;
    }
    DrawLives();
    OrigSD_PlaySound(BONUS1UPSND);
}

/*
===============
=
= GivePoints
=
===============
*/

void GivePoints(int32_t points)
{
    gamestate.score += points;
    while (gamestate.score >= gamestate.nextextra) {
        gamestate.nextextra += EXTRAPOINTS;
        GiveExtraMan();
    }
    DrawScore();
}

/*
==================
=
= GiveWeapon
=
==================
*/

void GiveWeapon(int weapon)
{
    GiveAmmo(6);

    if ((int)gamestate.bestweapon < weapon) {
        gamestate.bestweapon = gamestate.weapon = gamestate.chosenweapon = (weapontype)weapon;
    }

    DrawWeapon();
}

/*
===============
=
= GiveAmmo
=
===============
*/

void GiveAmmo(int ammo)
{
    if (!gamestate.ammo) { /* knife was out */
        if (!gamestate.attackframe) {
            gamestate.weapon = gamestate.chosenweapon;
            DrawWeapon();
        }
    }
    gamestate.ammo = (int16_t)(gamestate.ammo + ammo);
    if (gamestate.ammo > 99) {
        gamestate.ammo = 99;
    }
    DrawAmmo();
}

/*
==================
=
= GiveKey
=
==================
*/

void GiveKey(int key)
{
    gamestate.keys = (int16_t)(gamestate.keys | (1 << key));
    DrawKeys();
}

/*
=============================================================================

                            MOVEMENT

=============================================================================
*/

/*
===================
=
= GetBonus
=
===================
*/

void GetBonus(statobj_t *check)
{
    switch (check->itemnumber) {
    case bo_firstaid:
        if (gamestate.health == 100) {
            return;
        }

        OrigSD_PlaySound(HEALTH2SND);
        HealSelf(25);
        break;

    case bo_key1:
    case bo_key2:
    case bo_key3:
    case bo_key4:
        GiveKey(check->itemnumber - bo_key1);
        OrigSD_PlaySound(GETKEYSND);
        break;

    case bo_cross:
        OrigSD_PlaySound(BONUS1SND);
        GivePoints(100);
        gamestate.treasurecount++;
        break;
    case bo_chalice:
        OrigSD_PlaySound(BONUS2SND);
        GivePoints(500);
        gamestate.treasurecount++;
        break;
    case bo_bible:
        OrigSD_PlaySound(BONUS3SND);
        GivePoints(1000);
        gamestate.treasurecount++;
        break;
    case bo_crown:
        OrigSD_PlaySound(BONUS4SND);
        GivePoints(5000);
        gamestate.treasurecount++;
        break;

    case bo_clip:
        if (gamestate.ammo == 99) {
            return;
        }

        OrigSD_PlaySound(GETAMMOSND);
        GiveAmmo(8);
        break;
    case bo_clip2:
        if (gamestate.ammo == 99) {
            return;
        }

        OrigSD_PlaySound(GETAMMOSND);
        GiveAmmo(4);
        break;

    case bo_machinegun:
        OrigSD_PlaySound(GETMACHINESND);
        GiveWeapon(wp_machinegun);
        break;
    case bo_chaingun:
        OrigSD_PlaySound(GETGATLINGSND);
        GiveWeapon(wp_chaingun);

        DrawGotGatling();
        facecount = 0;
        gotgatgun = 1;
        break;

    case bo_fullheal:
        OrigSD_PlaySound(BONUS1UPSND);
        HealSelf(99);
        GiveAmmo(25);
        GiveExtraMan();
        gamestate.treasurecount++;
        break;

    case bo_food:
        if (gamestate.health == 100) {
            return;
        }

        OrigSD_PlaySound(HEALTH1SND);
        HealSelf(10);
        break;

    case bo_alpo:
        if (gamestate.health == 100) {
            return;
        }

        OrigSD_PlaySound(HEALTH1SND);
        HealSelf(4);
        break;

    case bo_gibs:
        if (gamestate.health > 10) {
            return;
        }

        OrigSD_PlaySound(SLURPIESND);
        HealSelf(1);
        break;

    case bo_spear:
        spearflag = true;
        spearx = player->x;
        speary = player->y;
        spearangle = (uint16_t)player->angle;
        playstate = ex_completed;
        break;

    default:
        break;
    }

    StartBonusFlash();
    check->shapenum = -1; /* remove from list */
}

/*
===================
=
= TryMove
=
= returns true if move ok
=
===================
*/

boolean TryMove(objtype *ob)
{
    int xl, yl, xh, yh, x, y;
    objtype *check;
    int32_t deltax, deltay;

    xl = (int)((ob->x - PLAYERSIZE) >> TILESHIFT);
    yl = (int)((ob->y - PLAYERSIZE) >> TILESHIFT);

    xh = (int)((ob->x + PLAYERSIZE) >> TILESHIFT);
    yh = (int)((ob->y + PLAYERSIZE) >> TILESHIFT);

    /* Not in the original: never index actorat outside the map. */
    if (xl < 0 || yl < 0 || xh >= MAPSIZE || yh >= MAPSIZE) {
        return false;
    }

    /* check for solid walls */
    for (y = yl; y <= yh; y++) {
        for (x = xl; x <= xh; x++) {
            check = actorat[x][y];
            if (check && !ISPOINTER(check)) { /* check < objlist: a tile */
                return false;
            }
        }
    }

    /* check for actors */
    if (yl > 0) {
        yl--;
    }
    if (yh < MAPSIZE - 1) {
        yh++;
    }
    if (xl > 0) {
        xl--;
    }
    if (xh < MAPSIZE - 1) {
        xh++;
    }

    for (y = yl; y <= yh; y++) {
        for (x = xl; x <= xh; x++) {
            check = actorat[x][y];
            if (ISPOINTER(check) && check != objlist && (check->flags & FL_SHOOTABLE)) {
                deltax = ob->x - check->x;
                if (deltax < -MINACTORDIST || deltax > MINACTORDIST) {
                    continue;
                }
                deltay = ob->y - check->y;
                if (deltay < -MINACTORDIST || deltay > MINACTORDIST) {
                    continue;
                }

                return false;
            }
        }
    }

    return true;
}

/*
===================
=
= ClipMove
=
===================
*/

void ClipMove(objtype *ob, int32_t xmove, int32_t ymove)
{
    int32_t basex, basey;

    basex = ob->x;
    basey = ob->y;

    ob->x = basex + xmove;
    ob->y = basey + ymove;
    if (TryMove(ob)) {
        return;
    }

    if (noclip && ob->x > 2 * TILEGLOBAL && ob->y > 2 * TILEGLOBAL &&
        ob->x < (((int32_t)(mapwidth - 1)) << TILESHIFT) &&
        ob->y < (((int32_t)(mapheight - 1)) << TILESHIFT)) {
        return; /* walk through walls */
    }

    if (!OrigSD_SoundPlaying()) {
        OrigSD_PlaySound(HITWALLSND);
    }

    ob->x = basex + xmove;
    ob->y = basey;
    if (TryMove(ob)) {
        return;
    }

    ob->x = basex;
    ob->y = basey + ymove;
    if (TryMove(ob)) {
        return;
    }

    ob->x = basex;
    ob->y = basey;
}

/*
===================
=
= VictoryTile
=
===================
*/

void VictoryTile(void)
{
    SpawnBJVictory();

    gamestate.victoryflag = true;
}

/*
===================
=
= Thrust
=
===================
*/

void Thrust(int angle, int32_t speed)
{
    int32_t xmove, ymove;
    unsigned offset;

    thrustspeed += speed;

    /* moving bounds speed */
    if (speed >= MINDIST * 2) {
        speed = MINDIST * 2 - 1;
    }

    xmove = FixedByFrac(speed, costable[angle]);
    ymove = -FixedByFrac(speed, sintable[angle]);

    ClipMove(player, xmove, ymove);

    player->tilex = (uint16_t)(player->x >> TILESHIFT); /* scale to tile values */
    player->tiley = (uint16_t)(player->y >> TILESHIFT);

    offset = farmapylookup[player->tiley] + player->tilex;
    player->areanumber = (byte)(*(mapsegs[0] + offset) - AREATILE);

    if (*(mapsegs[1] + offset) == EXITTILE) {
        VictoryTile();
    }
}

/*
=============================================================================

                                ACTIONS

=============================================================================
*/

/*
===============
=
= Cmd_Fire
=
===============
*/

void Cmd_Fire(void)
{
    buttonheld[bt_attack] = true;

    gamestate.weaponframe = 0;

    player->state = &s_attack;

    gamestate.attackframe = 0;
    gamestate.attackcount = attackinfo[gamestate.weapon][gamestate.attackframe].tics;
    gamestate.weaponframe = attackinfo[gamestate.weapon][gamestate.attackframe].frame;
}

/* Cmd_Use's local doornum (the tile value) shadows the global door count. */
static int DoorCount(void)
{
    return doornum;
}

/*
===============
=
= Cmd_Use
=
===============
*/

void Cmd_Use(void)
{
    int checkx, checky, doornum, dir;
    boolean elevatorok;

    /* find which cardinal direction the player is facing */
    if (player->angle < ANGLES / 8 || player->angle > 7 * ANGLES / 8) {
        checkx = player->tilex + 1;
        checky = player->tiley;
        dir = di_east;
        elevatorok = true;
    } else if (player->angle < 3 * ANGLES / 8) {
        checkx = player->tilex;
        checky = player->tiley - 1;
        dir = di_north;
        elevatorok = false;
    } else if (player->angle < 5 * ANGLES / 8) {
        checkx = player->tilex - 1;
        checky = player->tiley;
        dir = di_west;
        elevatorok = true;
    } else {
        checkx = player->tilex;
        checky = player->tiley + 1;
        dir = di_south;
        elevatorok = false;
    }

    doornum = tilemap[checkx][checky];
    if (*(mapsegs[1] + farmapylookup[checky] + checkx) == PUSHABLETILE) {
        /* pushable wall */
        PushWall(checkx, checky, dir);
        return;
    }
    if (!buttonheld[bt_use] && doornum == ELEVATORTILE && elevatorok) {
        /* use elevator */
        buttonheld[bt_use] = true;

        tilemap[checkx][checky]++; /* flip switch */
        if (*(mapsegs[0] + farmapylookup[player->tiley] + player->tilex) == ALTELEVATORTILE) {
            playstate = ex_secretlevel;
        } else {
            playstate = ex_completed;
        }
        OrigSD_PlaySound(LEVELDONESND);
        OrigSD_WaitSoundDone();
    } else if (!buttonheld[bt_use] && (doornum & 0x80)) {
        buttonheld[bt_use] = true;
        /*
         * A moving pushwall is marked tile | 0xc0, so using one that slides
         * away in front of the player lands here with a "door" number of
         * 64 or more. The original then wrote past doorobjlist; not here.
         */
        if ((doornum & ~0x80) < DoorCount()) {
            OperateDoor(doornum & ~0x80);
        }
    } else {
        OrigSD_PlaySound(DONOTHINGSND);
    }
}

/*
=============================================================================

                           PLAYER CONTROL

=============================================================================
*/

/*
===============
=
= SpawnPlayer
=
===============
*/

void SpawnPlayer(int tilex, int tiley, int dir)
{
    player->obclass = playerobj;
    player->active = ac_yes;
    player->tilex = (uint16_t)tilex;
    player->tiley = (uint16_t)tiley;
    player->areanumber = (byte)*(mapsegs[0] + farmapylookup[player->tiley] + player->tilex);
    player->x = ((fixed)tilex << TILESHIFT) + TILEGLOBAL / 2;
    player->y = ((fixed)tiley << TILESHIFT) + TILEGLOBAL / 2;
    player->state = &s_player;
    player->angle = (int16_t)((1 - dir) * 90);
    if (player->angle < 0) {
        player->angle += ANGLES;
    }
    player->flags = FL_NEVERMARK;
    Thrust(0, 0); /* set some variables */

    InitAreas();
}

/*
===============
=
= T_KnifeAttack
=
= Update player hands, and try to do damage when the proper frame is reached
=
= The target choice uses viewx, transx and FL_VISABLE, which the renderer
= sets while drawing (S4).
=
===============
*/

void KnifeAttack(objtype *ob)
{
    objtype *check, *closest;
    int32_t dist;

    OrigSD_PlaySound(ATKKNIFESND);
    /* actually fire */
    dist = 0x7fffffff;
    closest = NULL;
    for (check = ob->next; check; check = check->next) {
        if ((check->flags & FL_SHOOTABLE) && (check->flags & FL_VISABLE) &&
            abs(check->viewx - centerx) < shootdelta) {
            if (check->transx < dist) {
                dist = check->transx;
                closest = check;
            }
        }
    }

    if (!closest || dist > 0x18000l) {
        /* missed */
        return;
    }

    /* hit something */
    DamageActor(closest, US_RndT() >> 4);
}

void GunAttack(objtype *ob)
{
    objtype *check, *closest, *oldclosest;
    int damage;
    int dx, dy, dist;
    int32_t viewdist;

    switch (gamestate.weapon) {
    case wp_pistol:
        OrigSD_PlaySound(ATKPISTOLSND);
        break;
    case wp_machinegun:
        OrigSD_PlaySound(ATKMACHINEGUNSND);
        break;
    case wp_chaingun:
        OrigSD_PlaySound(ATKGATLINGSND);
        break;
    default:
        break;
    }

    madenoise = true;

    /* find potential targets */
    viewdist = 0x7fffffffl;
    closest = NULL;

    while (1) {
        oldclosest = closest;

        for (check = ob->next; check; check = check->next) {
            if ((check->flags & FL_SHOOTABLE) && (check->flags & FL_VISABLE) &&
                abs(check->viewx - centerx) < shootdelta) {
                if (check->transx < viewdist) {
                    viewdist = check->transx;
                    closest = check;
                }
            }
        }

        if (closest == oldclosest) {
            return; /* no more targets, all missed */
        }

        /* trace a line from player to enemey */
        if (CheckLine(closest)) {
            break;
        }
    }

    /* hit something */
    dx = abs(closest->tilex - player->tilex);
    dy = abs(closest->tiley - player->tiley);
    dist = dx > dy ? dx : dy;

    if (dist < 2) {
        damage = US_RndT() / 4;
    } else if (dist < 4) {
        damage = US_RndT() / 6;
    } else {
        if ((US_RndT() / 12) < dist) { /* missed */
            return;
        }
        damage = US_RndT() / 6;
    }

    DamageActor(closest, (unsigned)damage);
}

/*
===============
=
= VictorySpin
=
===============
*/

void VictorySpin(void)
{
    int32_t desty;

    if (player->angle > 270) {
        player->angle = (int16_t)(player->angle - tics * 3);
        if (player->angle < 270) {
            player->angle = 270;
        }
    } else if (player->angle < 270) {
        player->angle = (int16_t)(player->angle + tics * 3);
        if (player->angle > 270) {
            player->angle = 270;
        }
    }

    desty = (((int32_t)player->tiley - 5) << TILESHIFT) - 0x3000;

    if (player->y > desty) {
        player->y -= (int32_t)tics * 4096;
        if (player->y < desty) {
            player->y = desty;
        }
    }
}

/*
===============
=
= T_Attack
=
===============
*/

void T_Attack(objtype *ob)
{
    const struct atkinf *cur;

    UpdateFace();

    if (gamestate.victoryflag) { /* watching the BJ actor */
        VictorySpin();
        return;
    }

    if (buttonstate[bt_use] && !buttonheld[bt_use]) {
        buttonstate[bt_use] = false;
    }

    if (buttonstate[bt_attack] && !buttonheld[bt_attack]) {
        buttonstate[bt_attack] = false;
    }

    ControlMovement(ob);
    if (gamestate.victoryflag) { /* watching the BJ actor */
        return;
    }

    plux = (uint16_t)(player->x >> UNSIGNEDSHIFT); /* scale to fit in unsigned */
    pluy = (uint16_t)(player->y >> UNSIGNEDSHIFT);
    player->tilex = (uint16_t)(player->x >> TILESHIFT); /* scale to tile values */
    player->tiley = (uint16_t)(player->y >> TILESHIFT);

    /* change frame and fire */
    gamestate.attackcount = (int16_t)(gamestate.attackcount - tics);
    while (gamestate.attackcount <= 0) {
        cur = &attackinfo[gamestate.weapon][gamestate.attackframe];
        switch (cur->attack) {
        case -1:
            ob->state = &s_player;
            if (!gamestate.ammo) {
                gamestate.weapon = wp_knife;
                DrawWeapon();
            } else {
                if (gamestate.weapon != gamestate.chosenweapon) {
                    gamestate.weapon = gamestate.chosenweapon;
                    DrawWeapon();
                }
            }
            gamestate.attackframe = gamestate.weaponframe = 0;
            return;

        case 4:
            if (!gamestate.ammo) {
                break;
            }
            if (buttonstate[bt_attack]) {
                gamestate.attackframe -= 2;
            }
            /* fall through */
        case 1:
            if (!gamestate.ammo) { /* can only happen with chain gun */
                gamestate.attackframe++;
                break;
            }
            GunAttack(ob);
            gamestate.ammo--;
            DrawAmmo();
            break;

        case 2:
            KnifeAttack(ob);
            break;

        case 3:
            if (gamestate.ammo && buttonstate[bt_attack]) {
                gamestate.attackframe -= 2;
            }
            break;

        default:
            break;
        }

        gamestate.attackcount = (int16_t)(gamestate.attackcount + cur->tics);
        gamestate.attackframe++;
        gamestate.weaponframe = attackinfo[gamestate.weapon][gamestate.attackframe].frame;
    }
}

/*
===============
=
= T_Player
=
===============
*/

void T_Player(objtype *ob)
{
    if (gamestate.victoryflag) { /* watching the BJ actor */
        VictorySpin();
        return;
    }

    UpdateFace();
    CheckWeaponChange();

    if (buttonstate[bt_use]) {
        Cmd_Use();
    }

    if (buttonstate[bt_attack] && !buttonheld[bt_attack]) {
        Cmd_Fire();
    }

    ControlMovement(ob);
    if (gamestate.victoryflag) { /* watching the BJ actor */
        return;
    }

    plux = (uint16_t)(player->x >> UNSIGNEDSHIFT); /* scale to fit in unsigned */
    pluy = (uint16_t)(player->y >> UNSIGNEDSHIFT);
    player->tilex = (uint16_t)(player->x >> TILESHIFT); /* scale to tile values */
    player->tiley = (uint16_t)(player->y >> TILESHIFT);
}
