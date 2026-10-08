/* Player (WL_AGENT.C): spawn only. Movement, weapons and items come in S3. */
#include "orig.h"

#include "id_ca.h"

statetype s_player = {false, 0, 0, NULL, NULL, NULL};

int32_t thrustspeed;

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
