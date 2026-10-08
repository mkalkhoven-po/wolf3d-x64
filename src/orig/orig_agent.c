/*
 * Player (WL_AGENT.C): spawn, damage and score. Movement, weapons and items
 * come in S3.
 */
#include "orig.h"

#include "id_ca.h"

/* T_Player is ported in S3; until then the player state does nothing. */
statetype s_player = {false, 0, 0, NULL, NULL, NULL};

int32_t thrustspeed;
uint16_t plux, pluy; /* player coordinates scaled to unsigned */
int16_t gotgatgun;   /* JR */
objtype *LastAttacker;

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
 * The part of Thrust(0,0) that SpawnPlayer relies on. With a zero move,
 * ClipMove leaves the position unchanged (at most it plays HITWALLSND), so
 * only the tile and area recomputation remain. Replaced by the real Thrust
 * in S3.
 */
static void SpawnThrust(void)
{
    unsigned offset;

    player->tilex = (uint16_t)(player->x >> TILESHIFT); /* scale to tile values */
    player->tiley = (uint16_t)(player->y >> TILESHIFT);

    offset = farmapylookup[player->tiley] + player->tilex;
    player->areanumber = (byte)(*(mapsegs[0] + offset) - AREATILE);
}

/*
===================
=
= SpawnPlayer
=
===================
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
    SpawnThrust(); /* Thrust (0,0): set some variables */

    InitAreas();
}
