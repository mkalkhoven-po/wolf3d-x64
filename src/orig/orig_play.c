/* Actor list and play-loop globals (WL_PLAY.C). */
#include "orig.h"

#include <string.h>

#include "id_us.h"

objtype objlist[MAXACTORS], *new, *obj, *player, *lastobj, *objfreelist, *killerobj;
int16_t objcount;

uint16_t farmapylookup[MAPSIZE];
byte tilemap[MAPSIZE][MAPSIZE];
byte spotvis[MAPSIZE][MAPSIZE];
objtype *actorat[MAPSIZE][MAPSIZE];

boolean demorecord, demoplayback;

/*
=========================
=
= InitActorList
=
= Call to clear out the actor object lists returning them all to the free
= list. Allocates a special spot for the player.
=
=========================
*/

void InitActorList(void)
{
    int i;

    /* init the actor lists */
    for (i = 0; i < MAXACTORS; i++) {
        objlist[i].prev = &objlist[i + 1];
        objlist[i].next = NULL;
    }

    objlist[MAXACTORS - 1].prev = NULL;

    objfreelist = &objlist[0];
    lastobj = NULL;

    objcount = 0;

    /* give the player the first free spots */
    GetNewActor();
    player = new;
}

/*
=========================
=
= GetNewActor
=
= Sets the global variable new to point to a free spot in objlist.
= The free spot is inserted at the end of the liked list
=
=========================
*/

void GetNewActor(void)
{
    if (!objfreelist) {
        Quit("GetNewActor: No free spots in objlist!");
    }

    new = objfreelist;
    objfreelist = new->prev;
    memset(new, 0, sizeof(*new));

    if (lastobj) {
        lastobj->next = new;
    }
    new->prev = lastobj; /* new->next is allready NULL from memset */

    new->active = ac_no;
    lastobj = new;

    objcount++;
}

/*
=========================
=
= RemoveObj
=
= Add the given object back into the free list, and unlink it from it's
= neighbors
=
=========================
*/

void RemoveObj(objtype *gone)
{
    if (gone == player) {
        Quit("RemoveObj: Tried to remove the player!");
    }

    gone->state = NULL;

    /* fix the next object's back link */
    if (gone == lastobj) {
        lastobj = gone->prev;
    } else {
        gone->next->prev = gone->prev;
    }

    /* fix the previous object's forward link */
    gone->prev->next = gone->next;

    /* add it back in to the free list */
    gone->prev = objfreelist;
    objfreelist = gone;

    objcount--;
}
