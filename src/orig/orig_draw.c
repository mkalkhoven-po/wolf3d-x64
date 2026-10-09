/*
 * Renderer (WL_DRAW.C), part S4a: view variables, projection of actors and
 * tiles, wall heights, sprite rotation and the visibility pass of
 * DrawScaleds. The raycaster (S4b) and the pixel drawing (S4c) follow.
 *
 * Several results here are game logic, not just pictures: DrawScaleds
 * activates actors, sets FL_VISABLE and their viewx/transx (the player's hit
 * test), and picks up bonus items.
 */
#include "orig.h"

#include "wl_fixed.h"

#define ACTORSIZE 0x4000

fixed mindist = MINDIST;

uint16_t wallheight[MAXVIEWWIDTH];

/* refresh variables */
fixed viewx, viewy; /* the focal point */
int16_t viewangle;
fixed viewsin, viewcos;

int16_t focaltx, focalty, viewtx, viewty;
int16_t midangle;
uint16_t xpartialup, xpartialdown, ypartialup, ypartialdown;

int32_t xintercept, yintercept;

/*
 * Two 16-bit operations the original did in assembly or with 16-bit ints.
 */

/*
 * centerx + ny*scale/nx: the long multiply wraps in 32 bits, the result is
 * stored in a 16-bit int.
 */
static int16_t ProjectX(fixed ny, fixed nx)
{
    const int32_t product = (int32_t)((uint32_t)ny * (uint32_t)scale);
    return (int16_t)(centerx + product / nx);
}

/*
 * heightnumerator/(nx>>8): "idiv [WORD PTR nx+1]" divides the 32-bit
 * heightnumerator by the middle 16 bits of nx; the quotient is AX. nx is at
 * least mindist here, so the divisor is at least 0x58 and the quotient fits.
 */
static int16_t HeightDiv(fixed nx)
{
    const int16_t divisor = (int16_t)(nx >> 8);
    return (int16_t)(heightnumerator / divisor);
}

/*
====================
=
= CalcViewVariables
=
= The first half of WallRefresh: set up variables for this view.
=
====================
*/

void CalcViewVariables(void)
{
    viewangle = player->angle;
    midangle = (int16_t)(viewangle * (FINEANGLES / ANGLES));
    viewsin = sintable[viewangle];
    viewcos = costable[viewangle];
    viewx = player->x - FixedByFrac(focallength, viewcos);
    viewy = player->y + FixedByFrac(focallength, viewsin);

    focaltx = (int16_t)(viewx >> TILESHIFT);
    focalty = (int16_t)(viewy >> TILESHIFT);

    viewtx = (int16_t)(player->x >> TILESHIFT);
    viewty = (int16_t)(player->y >> TILESHIFT);

    xpartialdown = (uint16_t)(viewx & (TILEGLOBAL - 1));
    xpartialup = (uint16_t)(TILEGLOBAL - xpartialdown);
    ypartialdown = (uint16_t)(viewy & (TILEGLOBAL - 1));
    ypartialup = (uint16_t)(TILEGLOBAL - ypartialdown);
}

/*
========================
=
= TransformActor
=
= Takes paramaters:
=   gx,gy       : globalx/globaly of point
=
= globals:
=   viewx,viewy     : point of view
=   viewcos,viewsin : sin/cos of viewangle
=   scale       : conversion from global value to screen value
=
= sets:
=   screenx,transx,transy,screenheight: projected edge location and size
=
========================
*/

void TransformActor(objtype *ob)
{
    fixed gx, gy, gxt, gyt, nx, ny;

    /* translate point to view centered coordinates */
    gx = ob->x - viewx;
    gy = ob->y - viewy;

    /* calculate newx */
    gxt = FixedByFrac(gx, viewcos);
    gyt = FixedByFrac(gy, viewsin);
    nx = gxt - gyt - ACTORSIZE; /* fudge the shape forward a bit, because   */
                                /* the midpoint could put parts of the shape */
                                /* into an adjacent wall                     */

    /* calculate newy */
    gxt = FixedByFrac(gx, viewsin);
    gyt = FixedByFrac(gy, viewcos);
    ny = gyt + gxt;

    /* calculate perspective ratio */
    ob->transx = nx;
    ob->transy = ny;

    if (nx < mindist) { /* too close, don't overflow the divide */
        ob->viewheight = 0;
        return;
    }

    ob->viewx = ProjectX(ny, nx);

    /* calculate height (heightnumerator/(nx>>8)) */
    ob->viewheight = (uint16_t)HeightDiv(nx);
}

/*
========================
=
= TransformTile
=
= Takes paramaters:
=   tx,ty       : tile the object is centered in
=
= globals:
=   viewx,viewy     : point of view
=   viewcos,viewsin : sin/cos of viewangle
=   scale       : conversion from global value to screen value
=
= sets:
=   screenx,transx,transy,screenheight: projected edge location and size
=
= Returns true if the tile is withing getting distance
=
========================
*/

boolean TransformTile(int tx, int ty, int16_t *dispx, int16_t *dispheight)
{
    fixed gx, gy, gxt, gyt, nx, ny;

    /* translate point to view centered coordinates */
    gx = ((fixed)tx << TILESHIFT) + 0x8000 - viewx;
    gy = ((fixed)ty << TILESHIFT) + 0x8000 - viewy;

    /* calculate newx */
    gxt = FixedByFrac(gx, viewcos);
    gyt = FixedByFrac(gy, viewsin);
    nx = gxt - gyt - 0x2000; /* 0x2000 is size of object */

    /* calculate newy */
    gxt = FixedByFrac(gx, viewsin);
    gyt = FixedByFrac(gy, viewcos);
    ny = gyt + gxt;

    /* calculate perspective ratio */
    if (nx < mindist) { /* too close, don't overflow the divide */
        *dispheight = 0;
        return false;
    }

    *dispx = ProjectX(ny, nx);

    /* calculate height (heightnumerator/(nx>>8)) */
    *dispheight = HeightDiv(nx);

    /* see if it should be grabbed */
    if (nx < TILEGLOBAL && ny > -TILEGLOBAL / 2 && ny < TILEGLOBAL / 2) {
        return true;
    }
    return false;
}

/*
====================
=
= CalcHeight
=
= Calculates the height of xintercept,yintercept from viewx,viewy
=
====================
*/

int16_t CalcHeight(void)
{
    fixed gxt, gyt, nx;
    int32_t gx, gy;

    gx = xintercept - viewx;
    gxt = FixedByFrac(gx, viewcos);

    gy = yintercept - viewy;
    gyt = FixedByFrac(gy, viewsin);

    nx = gxt - gyt;

    /* calculate perspective ratio (heightnumerator/(nx>>8)) */
    if (nx < mindist) {
        nx = mindist; /* don't let divide overflow */
    }

    return HeightDiv(nx);
}

/*
=====================
=
= CalcRotate
=
=====================
*/

int CalcRotate(objtype *ob)
{
    int angle, viewangle;

    /*
     * this isn't exactly correct, as it should vary by a trig value,
     * but it is close enough with only eight rotations
     */

    viewangle = player->angle + (centerx - ob->viewx) / 8;

    if (ob->obclass == rocketobj || ob->obclass == hrocketobj) {
        angle = (viewangle - 180) - ob->angle;
    } else {
        angle = (viewangle - 180) - dirangle[ob->dir];
    }

    angle += ANGLES / 16;
    while (angle >= ANGLES) {
        angle -= ANGLES;
    }
    while (angle < 0) {
        angle += ANGLES;
    }

    if (ob->state->rotate == 2) { /* 2 rotation pain frame */
        return 4 * (angle / (ANGLES / 2)); /* seperated by 3 (art layout...) */
    }

    return angle / (ANGLES / 8);
}

/*
=====================
=
= DrawScaleds
=
= Draws all objects that are visable
=
=====================
*/

visobj_t vislist[MAXVISABLE], *visptr, *visstep, *farthest;

/*
 * spotvis / tilemap at index spot of the flat 64*64 array. DrawScaleds reads
 * the eight neighbours as offsets from the actor's spot, so an actor on the
 * map's top or bottom row reads the next or previous column; that is kept.
 * Only reads outside the whole array (map corners, never in valid maps)
 * return 0 instead of unrelated memory.
 */
static byte FlatSpot(byte (*array)[MAPSIZE], int spot)
{
    if (spot < 0 || spot >= MAPSIZE * MAPSIZE) {
        return 0;
    }
    return array[spot / MAPSIZE][spot % MAPSIZE];
}

static boolean SeenNear(int spot, int offset)
{
    return FlatSpot(spotvis, spot + offset) && !FlatSpot(tilemap, spot + offset);
}

void DrawScaleds(void)
{
    int i, least, numvisable, height;
    unsigned spotloc;

    statobj_t *statptr;
    objtype *obj;

    visptr = &vislist[0];

    /* place static objects */
    for (statptr = &statobjlist[0]; statptr != laststatobj; statptr++) {
        if ((visptr->shapenum = statptr->shapenum) == -1) {
            continue; /* object has been deleted */
        }

        if (!*statptr->visspot) {
            continue; /* not visable */
        }

        if (TransformTile(statptr->tilex, statptr->tiley, &visptr->viewx, &visptr->viewheight) &&
            (statptr->flags & FL_BONUS)) {
            GetBonus(statptr);
            continue;
        }

        if (!visptr->viewheight) {
            continue; /* to close to the object */
        }

        if (visptr < &vislist[MAXVISABLE - 1]) { /* don't let it overflow */
            visptr++;
        }
    }

    /* place active objects */
    for (obj = player->next; obj; obj = obj->next) {
        int spot;

        if (!(visptr->shapenum = obj->state->shapenum)) {
            continue; /* no shape */
        }

        spotloc = ((unsigned)obj->tilex << 6) + obj->tiley; /* optimize: keep in struct? */
        spot = (int)spotloc;

        /* could be in any of the nine surrounding tiles */
        if (FlatSpot(spotvis, spot) || SeenNear(spot, -1) || SeenNear(spot, 1) ||
            SeenNear(spot, -65) || SeenNear(spot, -64) || SeenNear(spot, -63) ||
            SeenNear(spot, 65) || SeenNear(spot, 64) || SeenNear(spot, 63)) {
            obj->active = ac_yes;
            TransformActor(obj);
            if (!obj->viewheight) {
                continue; /* too close or far away */
            }

            visptr->viewx = obj->viewx;
            visptr->viewheight = (int16_t)obj->viewheight;
            if (visptr->shapenum == -1) {
                visptr->shapenum = obj->temp1; /* special shape */
            }

            if (obj->state->rotate) {
                visptr->shapenum = (int16_t)(visptr->shapenum + CalcRotate(obj));
            }

            if (visptr < &vislist[MAXVISABLE - 1]) { /* don't let it overflow */
                visptr++;
            }
            obj->flags |= FL_VISABLE;
        } else {
            obj->flags &= ~FL_VISABLE;
        }
    }

    /* draw from back to front */
    numvisable = (int)(visptr - &vislist[0]);

    if (!numvisable) {
        return; /* no visable objects */
    }

    for (i = 0; i < numvisable; i++) {
        least = 32000;
        for (visstep = &vislist[0]; visstep < visptr; visstep++) {
            height = visstep->viewheight;
            if (height < least) {
                least = height;
                farthest = visstep;
            }
        }

        /* draw farthest */
        ScaleShape(farthest->viewx, farthest->shapenum, farthest->viewheight);

        farthest->viewheight = 32000;
    }
}
