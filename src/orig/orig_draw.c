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

#include <string.h>

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

/*
=============================================================================

                    WALLS (S4b): raycaster, posts, floor/ceiling

=============================================================================
*/

/* the canonical 320x200 frame (ADR-002); the view window starts at screenofs */
byte screenbuf[SCREENHEIGHT * SCREENWIDTH];

/* wall textures: VSWAP pages, 64x64 column-major; set by the platform layer */
const byte *(*orig_getpage)(int page);
int16_t orig_spritestart; /* PMSpriteStart of the loaded VSWAP */

#define DOORWALL (orig_spritestart - 8)

int16_t horizwall[MAXWALLTILES], vertwall[MAXWALLTILES];

int16_t lastside; /* true for vertical */
int32_t lastintercept;
int16_t lasttilehit;

uint16_t tilehit;
uint16_t pixx;

int16_t xtile, ytile;
int16_t xtilestep, ytilestep;
int32_t xstep, ystep;

/* postsource was a far pointer: segment = page, offset = texture column */
static struct {
    const byte *page;
    uint16_t texture;
} postsource;
uint16_t postx;
uint16_t postwidth;

static const byte *GetPage(int page)
{
    static const byte blank[PMPAGESIZE];
    const byte *p = orig_getpage ? orig_getpage(page) : NULL;
    return p ? p : blank;
}

/*
==================
=
= SetupWalls
=
= Map tile values to scaled pics
=
==================
*/

void SetupWalls(void)
{
    int i;

    for (i = 1; i < MAXWALLTILES; i++) {
        horizwall[i] = (int16_t)((i - 1) * 2);
        vertwall[i] = (int16_t)((i - 1) * 2 + 1);
    }
}

/*
===================
=
= ScalePost
=
= The VGA version draws a post of postwidth columns from postx, all with the
= height of the first one, through the compiled scaler picked by
= wallheight[postx] (low 3 bits fraction). Which pixels of the 4-pixel VGA
= bytes it writes comes from the mapmasks tables, indexed by
= (postx & 3) * 8 + postwidth - 1. The tables cover widths up to 8, the most
= a texel column can span in practice; a larger index reads on into the
= following table as in the original (and 0 past the last one).
=
===================
*/

void ScalePost(void)
{
    int index, height, cols[12], n, i;

    index = (wallheight[postx] & 0xfff8) >> 3; /* heightscaler */
    if (index > maxscale) {
        index = maxscale;
    }
    height = ScalerHeight(index);

    n = MaskedColumns(postx, postwidth, cols);
    for (i = 0; i < n; i++) {
        ScaleColumn(postsource.page + postsource.texture, height,
                    &screenbuf[screenofs + cols[i]], SCREENWIDTH);
    }
}

/*
 * The shared tail of the Hit* functions: extend the post or start a new one.
 *
 * WallRefresh resets lastside but not lasttilehit, so a door or pushwall
 * hit in column 0 that matches the previous frame's last post lands here
 * with pixx 0; the original then read wallheight[-1]. Column 0 keeps its own
 * height instead (not in the original).
 */
static boolean SamePost(uint16_t texture)
{
    if (texture == postsource.texture) {
        /* wide scale */
        postwidth++;
        if (pixx > 0) {
            wallheight[pixx] = wallheight[pixx - 1];
        }
        return true;
    }
    ScalePost();
    postsource.texture = texture;
    postwidth = 1;
    postx = pixx;
    return false;
}

static void NewPost(int page, uint16_t texture)
{
    if (lastside != -1) { /* if not the first scaled post */
        ScalePost();
    }
    lasttilehit = (int16_t)tilehit;
    postx = pixx;
    postwidth = 1;
    postsource.page = GetPage(page);
    postsource.texture = texture;
}

/*
====================
=
= HitVertWall
=
= tilehit bit 7 is 0, because it's not a door tile
= if bit 6 is 1 and the adjacent tile is a door tile, use door side pic
=
====================
*/

void HitVertWall(void)
{
    int wallpic;
    uint16_t texture;

    texture = (uint16_t)((yintercept >> 4) & 0xfc0);
    if (xtilestep == -1) {
        texture = (uint16_t)(0xfc0 - texture);
        xintercept += TILEGLOBAL;
    }
    wallheight[pixx] = (uint16_t)CalcHeight();

    if (lastside == 1 && lastintercept == xtile && lasttilehit == (int16_t)tilehit) {
        /* in the same wall type as last time, so check for optimized draw */
        SamePost(texture);
        return;
    }

    /* new wall */
    if (tilehit & 0x40) { /* check for adjacent doors */
        ytile = (int16_t)(yintercept >> TILESHIFT);
        if (tilemap[xtile - xtilestep][ytile] & 0x80) {
            wallpic = DOORWALL + 3;
        } else {
            wallpic = vertwall[tilehit & ~0x40];
        }
    } else {
        wallpic = vertwall[tilehit];
    }

    NewPost(wallpic, texture);
    lastside = true;
    lastintercept = xtile;
}

/*
====================
=
= HitHorizWall
=
= tilehit bit 7 is 0, because it's not a door tile
= if bit 6 is 1 and the adjacent tile is a door tile, use door side pic
=
====================
*/

void HitHorizWall(void)
{
    int wallpic;
    uint16_t texture;

    texture = (uint16_t)((xintercept >> 4) & 0xfc0);
    if (ytilestep == -1) {
        yintercept += TILEGLOBAL;
    } else {
        texture = (uint16_t)(0xfc0 - texture);
    }
    wallheight[pixx] = (uint16_t)CalcHeight();

    if (lastside == 0 && lastintercept == ytile && lasttilehit == (int16_t)tilehit) {
        /* in the same wall type as last time, so check for optimized draw */
        SamePost(texture);
        return;
    }

    /* new wall */
    if (tilehit & 0x40) { /* check for adjacent doors */
        xtile = (int16_t)(xintercept >> TILESHIFT);
        if (tilemap[xtile][ytile - ytilestep] & 0x80) {
            wallpic = DOORWALL + 2;
        } else {
            wallpic = horizwall[tilehit & ~0x40];
        }
    } else {
        wallpic = horizwall[tilehit];
    }

    NewPost(wallpic, texture);
    lastside = 0;
    lastintercept = ytile;
}

static int DoorPage(unsigned doornum)
{
    switch (doorobjlist[doornum].lock) {
    case dr_lock1:
    case dr_lock2:
    case dr_lock3:
    case dr_lock4:
        return DOORWALL + 6;
    case dr_elevator:
        return DOORWALL + 4;
    case dr_normal:
    default:
        return DOORWALL;
    }
}

/*
====================
=
= HitHorizDoor
=
====================
*/

void HitHorizDoor(void)
{
    uint16_t texture, doornum;

    doornum = tilehit & 0x7f;
    texture = (uint16_t)(((xintercept - doorposition[doornum]) >> 4) & 0xfc0);

    wallheight[pixx] = (uint16_t)CalcHeight();

    if (lasttilehit == (int16_t)tilehit) {
        /* in the same door as last time, so check for optimized draw */
        SamePost(texture);
        return;
    }

    /* first pixel in this door */
    NewPost(DoorPage(doornum), texture);
    lastside = 2;
}

/*
====================
=
= HitVertDoor
=
====================
*/

void HitVertDoor(void)
{
    uint16_t texture, doornum;

    doornum = tilehit & 0x7f;
    texture = (uint16_t)(((yintercept - doorposition[doornum]) >> 4) & 0xfc0);

    wallheight[pixx] = (uint16_t)CalcHeight();

    if (lasttilehit == (int16_t)tilehit) {
        /* in the same door as last time, so check for optimized draw */
        SamePost(texture);
        return;
    }

    /* first pixel in this door */
    NewPost(DoorPage(doornum) + 1, texture);
    lastside = 2;
}

/*
====================
=
= HitHorizPWall
=
= A pushable wall in action has been hit
=
====================
*/

void HitHorizPWall(void)
{
    uint16_t texture, offset;

    texture = (uint16_t)((xintercept >> 4) & 0xfc0);
    offset = (uint16_t)(pwallpos << 10);
    if (ytilestep == -1) {
        yintercept += TILEGLOBAL - offset;
    } else {
        texture = (uint16_t)(0xfc0 - texture);
        yintercept += offset;
    }

    wallheight[pixx] = (uint16_t)CalcHeight();

    if (lasttilehit == (int16_t)tilehit) {
        /* in the same wall type as last time, so check for optimized draw */
        SamePost(texture);
        return;
    }

    /* new wall; lastside is left as it was, as in the original */
    NewPost(horizwall[tilehit & 63], texture);
}

/*
====================
=
= HitVertPWall
=
= A pushable wall in action has been hit
=
====================
*/

void HitVertPWall(void)
{
    uint16_t texture, offset;

    texture = (uint16_t)((yintercept >> 4) & 0xfc0);
    offset = (uint16_t)(pwallpos << 10);
    if (xtilestep == -1) {
        xintercept += TILEGLOBAL - offset;
        texture = (uint16_t)(0xfc0 - texture);
    } else {
        xintercept += offset;
    }

    wallheight[pixx] = (uint16_t)CalcHeight();

    if (lasttilehit == (int16_t)tilehit) {
        /* in the same wall type as last time, so check for optimized draw */
        SamePost(texture);
        return;
    }

    /* new wall */
    NewPost(vertwall[tilehit & 63], texture);
}

/*
============================
=
= AsmRefresh (WL_DR_A.ASM)
=
= Casts one ray per view column and calls the Hit* routine for the first
= wall, door or pushwall it meets, marking every tile it passes in spotvis.
=
= Register-level port: the intercepts are 32-bit, but the tile numbers the
= loop compares and indexes with are their 16-bit high words, and the
= tilemap / spotvis index is (tile << 6) + tile in 16 bits, as in the
= assembly. The two jumps the original patched into its code per quadrant
= (jle / jge) are the horizjge / vertjge flags.
=
============================
*/

#define DEG90  900
#define DEG180 1800
#define DEG270 2700
#define DEG360 3600

/* long [step] * word partial, sign-magnitude (xpartialbyystep / ypartialbyxstep) */
static int32_t PartialByStep(int32_t step, uint16_t partial)
{
    uint32_t mag = (uint32_t)step;
    uint32_t result;
    const int negative = step < 0;

    if (negative) {
        mag = 0u - mag;
    }
    result = (uint32_t)(((uint64_t)mag * partial) >> 16);
    if (negative) {
        result = 0u - result;
    }
    return (int32_t)result;
}

/* step * pwallpos / 64: unsigned 16-bit multiplies, low 32 bits, six sar */
static int32_t PWallStepPart(int32_t step)
{
    const uint32_t product = (uint32_t)step * (uint32_t)pwallpos;
    return (int32_t)product >> 6;
}

/*
 * (tile << 6) + tile in 16 bits. The first intercept of a steep ray can have
 * any high word, negative included; the assembly shifts it anyway (the
 * loop's comparisons keep it from being used). A multiply keeps that
 * defined in C.
 */
static int16_t Spot(int16_t hi, int16_t lo)
{
    return (int16_t)((int32_t)hi * 64 + lo);
}

static int16_t HighWord(int32_t v)
{
    return (int16_t)((uint32_t)v >> 16);
}

/*
 * tilemap / spotvis as the flat 64*64 arrays the assembly indexes. Valid maps
 * have solid borders, so the index stays inside; anything outside reads as
 * a plain wall and is not marked (not in the original).
 */
static byte TileAtSpot(int16_t spot)
{
    if (spot < 0 || spot >= MAPSIZE * MAPSIZE) {
        return 1;
    }
    return tilemap[spot >> 6][spot & 63];
}

static void MarkSpot(int16_t spot)
{
    if (spot >= 0 && spot < MAPSIZE * MAPSIZE) {
        spotvis[spot >> 6][spot & 63] = 1;
    }
}

void AsmRefresh(void)
{
    for (pixx = 0; pixx < (uint16_t)viewwidth; pixx++) {
        int angle;
        uint16_t xpartial, ypartial;
        boolean horizjge, vertjge;
        int16_t bx, bp, cx, dx; /* xtile, ytile, xintercept / yintercept high words */
        int16_t xspot, yspot;   /* si, di */

        /* Setup to trace a ray through pixx view pixel */
        angle = midangle + pixelangle[pixx];
        if (angle < 0) {
            angle += FINEANGLES; /* -90 is the same as 270 */
        } else if (angle >= DEG360) {
            angle -= FINEANGLES; /* -449 is the same as 89 */
        }

        if (angle < DEG90) {
            /* 0-89 degree arc */
            xtilestep = 1;
            ytilestep = -1;
            xstep = finetangent[DEG90 - 1 - angle];
            ystep = -finetangent[angle];
            xpartial = xpartialup;
            ypartial = ypartialdown;
        } else if (angle < DEG180) {
            /* 90-179 degree arc */
            xtilestep = -1;
            ytilestep = -1;
            xstep = -finetangent[angle - DEG90];
            ystep = -finetangent[DEG180 - 1 - angle];
            xpartial = xpartialdown;
            ypartial = ypartialdown;
        } else if (angle < DEG270) {
            /* 180-269 degree arc */
            xtilestep = -1;
            ytilestep = 1;
            xstep = -finetangent[DEG270 - 1 - angle];
            ystep = finetangent[angle - DEG180];
            xpartial = xpartialdown;
            ypartial = ypartialup;
        } else {
            /* 270-359 degree arc */
            xtilestep = 1;
            ytilestep = 1;
            xstep = finetangent[angle - DEG270];
            ystep = finetangent[DEG360 - 1 - angle];
            xpartial = xpartialup;
            ypartial = ypartialup;
        }
        horizjge = xtilestep == 1;
        vertjge = ytilestep == 1;

        /* initialise variables for intersection testing */
        yintercept = PartialByStep(ystep, xpartial) + viewy;
        bx = (int16_t)(focaltx + xtilestep);
        xtile = bx;
        dx = HighWord(yintercept);
        xspot = Spot(bx, dx); /* xspot = (xtile<<6) + yinttile */

        xintercept = PartialByStep(xstep, ypartial) + viewx;
        cx = HighWord(xintercept);
        bp = (int16_t)(focalty + ytilestep); /* ytile = focalty+ytilestep */
        yspot = Spot(cx, bp); /* yspot = (xinttile<<6) + ytile */

        /* trace along this angle until we hit a wall */
        for (;;) {
            int32_t t;

            /* check intersections with vertical walls */
            if (vertjge ? dx >= bp : dx <= bp) {
                goto horizentry;
            }
        vertentry:
            if (TileAtSpot(xspot)) {
                tilehit = TileAtSpot(xspot);
                if (!(tilehit & 0x80)) {
                    xintercept = (int32_t)((uint32_t)(uint16_t)bx << 16);
                    xtile = bx;
                    ytile = dx;
                    HitVertWall();
                    break;
                }

                /* vertdoor */
                xtile = bx;
                if (tilehit & 0x40) {
                    /* hit a sliding vertical wall */
                    t = yintercept + PWallStepPart(ystep);
                    if (HighWord(yintercept) != HighWord(t)) {
                        goto continuevert; /* no, it hit the side */
                    }
                    yintercept = t;
                    xintercept = (int32_t)((uint32_t)(uint16_t)xtile << 16);
                    HitVertPWall();
                    break;
                }
                /* half a step gets to the door position */
                t = yintercept + (ystep >> 1);
                if (HighWord(yintercept) == HighWord(t) &&
                    (uint16_t)t >= doorposition[tilehit & 0x7f]) {
                    /* the trace hit the door plane at t, and the door is closed that much */
                    yintercept = t;
                    xintercept = (int32_t)(((uint32_t)(uint16_t)xtile << 16) | 0x8000);
                    HitVertDoor();
                    break;
                }
            continuevert:
                bx = xtile;
                dx = HighWord(yintercept);
            }
            /* passvert */
            MarkSpot(xspot);
            bx = (int16_t)(bx + xtilestep);
            yintercept += ystep;
            dx = HighWord(yintercept);
            xspot = Spot(bx, dx);
            continue;

        horizcheck:
            /* check intersections with horizontal walls */
            if (horizjge ? cx >= bx : cx <= bx) {
                goto vertentry;
            }
        horizentry:
            if (TileAtSpot(yspot)) {
                tilehit = TileAtSpot(yspot);
                if (!(tilehit & 0x80)) {
                    xtile = cx;
                    yintercept = (int32_t)((uint32_t)(uint16_t)bp << 16);
                    ytile = bp;
                    HitHorizWall();
                    break;
                }

                /* horizdoor */
                xtile = bx;
                if (tilehit & 0x40) {
                    /* hit a sliding horizontal wall */
                    t = xintercept + PWallStepPart(xstep);
                    if (cx != HighWord(t)) {
                        goto continuehoriz; /* no, it hit the side */
                    }
                    xintercept = t;
                    yintercept = (int32_t)((uint32_t)(uint16_t)bp << 16);
                    HitHorizPWall();
                    break;
                }
                /* half a step gets to the door position */
                t = xintercept + (xstep >> 1);
                if (cx == HighWord(t) && (uint16_t)t >= doorposition[tilehit & 0x7f]) {
                    /* the trace hit the door plane at t, and the door is closed that much */
                    xintercept = t;
                    yintercept = (int32_t)(((uint32_t)(uint16_t)bp << 16) | 0x8000);
                    HitHorizDoor();
                    break;
                }
            continuehoriz:
                bx = xtile;
                dx = HighWord(yintercept);
            }
            /* passhoriz */
            MarkSpot(yspot);
            bp = (int16_t)(bp + ytilestep);
            xintercept += xstep;
            cx = HighWord(xintercept);
            yspot = Spot(cx, bp);
            goto horizcheck;
        }
    }
}

/*
====================
=
= WallRefresh
=
====================
*/

void WallRefresh(void)
{
    CalcViewVariables(); /* set up variables for this view */

    lastside = -1; /* the first pixel is on a new wall */
    AsmRefresh();
    ScalePost(); /* no more optimization on last post */
}

/*
=====================
=
= VGAClearScreen
=
= Ceiling colour per map (vgaCeiling, non-SPEAR), floor 0x19, over the view
= window: the top half and the bottom half of viewheight.
=
=====================
*/

static const byte vgaCeiling[60] = {
    0x1d, 0x1d, 0x1d, 0x1d, 0x1d, 0x1d, 0x1d, 0x1d, 0x1d, 0xbf,
    0x4e, 0x4e, 0x4e, 0x1d, 0x8d, 0x4e, 0x1d, 0x2d, 0x1d, 0x8d,
    0x1d, 0x1d, 0x1d, 0x1d, 0x1d, 0x2d, 0xdd, 0x1d, 0x1d, 0x98,

    0x1d, 0x9d, 0x2d, 0xdd, 0xdd, 0x9d, 0x2d, 0x4d, 0x1d, 0xdd,
    0x7d, 0x1d, 0x2d, 0x2d, 0xdd, 0xd7, 0x1d, 0x1d, 0x1d, 0x2d,
    0x1d, 0x1d, 0x1d, 0x1d, 0xdd, 0xdd, 0x7d, 0xdd, 0xdd, 0xdd};

void VGAClearScreen(void)
{
    int y, mapindex;
    byte ceiling;

    mapindex = gamestate.episode * 10 + gamestate.mapon;
    ceiling = (mapindex >= 0 && mapindex < 60) ? vgaCeiling[mapindex] : 0x1d;

    for (y = 0; y < viewheight / 2; y++) {
        memset(&screenbuf[screenofs + y * SCREENWIDTH], ceiling, (size_t)viewwidth);
    }
    for (; y < (viewheight / 2) * 2; y++) {
        memset(&screenbuf[screenofs + y * SCREENWIDTH], 0x19, (size_t)viewwidth);
    }
}

/*
=============================================================================

                    FRAME (S4c): player weapon, ThreeDRefresh

=============================================================================
*/

int32_t lasttimecount;
int32_t frameon;

/*
==============
=
= DrawPlayerWeapon
=
= Draw the player's hands
=
==============
*/

static const int16_t weaponscale[NUMWEAPONS] = {SPR_KNIFEREADY, SPR_PISTOLREADY,
                                                SPR_MACHINEGUNREADY, SPR_CHAINREADY};

void DrawPlayerWeapon(void)
{
    int shapenum;

    if (gamestate.victoryflag) {
        if (player->state == &s_deathcam && (TimeCount & 32)) {
            SimpleScaleShape(viewwidth / 2, SPR_DEATHCAM, (unsigned)viewheight + 1);
        }
        return;
    }

    if ((int)gamestate.weapon != -1) {
        shapenum = weaponscale[gamestate.weapon] + gamestate.weaponframe;
        SimpleScaleShape(viewwidth / 2, shapenum, (unsigned)viewheight + 1);
    }

    if (demorecord || demoplayback) {
        SimpleScaleShape(viewwidth / 2, SPR_DEMO, (unsigned)viewheight + 1);
    }
}

/*
========================
=
= ThreeDRefresh
=
= Renders the view into the view window of screenbuf. Page flipping and the
= VGA register work of the original are the platform layer's job; the
= fizzle-in goes through a hook.
=
========================
*/

void ThreeDRefresh(void)
{
    /* clear out the traced array */
    memset(spotvis, 0, sizeof(spotvis));

    /* follow the walls from there to the right, drawwing as we go */
    VGAClearScreen();

    WallRefresh();

    /* draw all the scaled images */
    DrawScaleds();       /* draw scaled stuff */
    DrawPlayerWeapon(); /* draw player's hands */

    /* show screen and time last cycle */
    if (fizzlein) {
        FizzleIn();
        fizzlein = false;

        lasttimecount = TimeCount = 0; /* don't make a big tic count */
    }

    frameon++;
}
