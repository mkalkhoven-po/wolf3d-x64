/* Actor state helpers (WL_STATE.C). Movement, sight and damage come in S2. */
#include "orig.h"

#include "id_ca.h"
#include "id_us_rnd.h"

/*
===================
=
= SpawnNewObj
=
= Spaws a new actor at the given TILE coordinates, with the given state, and
= the given size in GLOBAL units.
=
= new                  = a pointer to an initialized new actor
=
===================
*/

void SpawnNewObj(unsigned tilex, unsigned tiley, statetype *state)
{
    GetNewActor();
    new->state = state;
    if (state->tictime) {
        new->ticcount = (int16_t)(US_RndT() % state->tictime);
    } else {
        new->ticcount = 0;
    }

    new->tilex = (uint16_t)tilex;
    new->tiley = (uint16_t)tiley;
    new->x = ((fixed)tilex << TILESHIFT) + TILEGLOBAL / 2;
    new->y = ((fixed)tiley << TILESHIFT) + TILEGLOBAL / 2;
    new->dir = nodir;

    actorat[tilex][tiley] = new;
    new->areanumber = (byte)(*(mapsegs[0] + farmapylookup[new->tiley] + new->tilex) - AREATILE);
}

/*
===================
=
= NewState
=
= Changes ob to a new state, setting ticcount to the max for that state
=
===================
*/

void NewState(objtype *ob, statetype *state)
{
    ob->state = state;
    ob->ticcount = state->tictime;
}
