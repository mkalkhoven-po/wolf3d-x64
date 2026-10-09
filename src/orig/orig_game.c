/* Game setup (WL_MAIN.C NewGame/InitGame/view size, WL_GAME.C level setup). */
#include "orig.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "id_ca.h"
#include "id_us_rnd.h"

gametype gamestate;
boolean loadedgame;

int16_t mapwidth, mapheight;
boolean fizzlein;
int32_t spearx, speary;
uint16_t spearangle;
boolean spearflag;

/* view size (WL_MAIN.C / WL_PLAY.C) */
int16_t viewsize = 15; /* the original's default when there is no config */
int16_t viewwidth, viewheight;
int16_t centerx, shootdelta;

/* projection constants (WL_MAIN.C) */
fixed focallength;
fixed scale, maxslope;
int32_t heightnumerator;
int16_t minheightdiv;
int16_t pixelangle[MAXVIEWWIDTH];

const int16_t dirangle[9] = {0,             ANGLES / 8,     2 * ANGLES / 8,
                             3 * ANGLES / 8, 4 * ANGLES / 8, 5 * ANGLES / 8,
                             6 * ANGLES / 8, 7 * ANGLES / 8, ANGLES};

#define FOCALLENGTH (0x5700l) /* in global coordinates */
#define VIEWGLOBAL  0x10000   /* globals visable flush to wall */

/*
==========================
=
= InitGame
=
= Only the engine-core part of the original InitGame: map row lookups, the
= trig tables and the view size. Video, sound and memory setup stay with the
= platform layer.
=
==========================
*/

void InitGame(void)
{
    int i;

    for (i = 0; i < MAPSIZE; i++) {
        farmapylookup[i] = (uint16_t)(i * 64);
    }

    BuildTables(); /* trig tables */

    NewViewSize(viewsize);
}

/*
====================
=
= CalcProjection
=
= Uses focallength
=
= Floating point as in the original: scale is truncated from a double, the
= pixel angles come from atan in a float multiplied by radtoint. The x87
= forms that product exactly before truncating, so it is done in double
= here (a float product could round up to the next integer).
=
====================
*/

void CalcProjection(int32_t focal)
{
    int i;
    int32_t intang;
    float angle;
    double tang;
    int halfview;
    double facedist;
    const float radtoint = (float)FINEANGLES / 2 / PI;

    focallength = focal;
    facedist = focal + MINDIST;
    halfview = viewwidth / 2; /* half view in pixels */

    /*
     * calculate scale value for vertical height calculations
     * and sprite x calculations
     */
    scale = (fixed)(halfview * facedist / (VIEWGLOBAL / 2));

    /*
     * divide heightnumerator by a posts distance to get the posts height for
     * the heightbuffer.  The pixel height is height>>2
     */
    heightnumerator = (TILEGLOBAL * scale) >> 6;
    minheightdiv = (int16_t)(heightnumerator / 0x7fff + 1);

    /* calculate the angle offset from view angle of each pixel's ray */

    for (i = 0; i < halfview; i++) {
        /* start 1/2 pixel over, so viewangle bisects two middle pixels */
        tang = ((int32_t)i * VIEWGLOBAL / viewwidth) / facedist;
        angle = (float)atan(tang);
        intang = (int32_t)((double)angle * (double)radtoint);
        pixelangle[halfview - 1 - i] = (int16_t)intang;
        pixelangle[halfview + i] = (int16_t)-intang;
    }

    /*
     * if a point's abs(y/x) is greater than maxslope, the point is outside
     * the view area
     */
    maxslope = finetangent[pixelangle[0]];
    maxslope >>= 8;
}

/*
==========================
=
= SetViewSize
=
= The compiled scalers of the original are not needed: the scaler is
= ported as plain C (S4c).
=
==========================
*/

boolean SetViewSize(unsigned width, unsigned height)
{
    viewwidth = (int16_t)(width & ~15u); /* must be divisable by 16 */
    viewheight = (int16_t)(height & ~1u); /* must be even */
    centerx = (int16_t)(viewwidth / 2 - 1);
    shootdelta = (int16_t)(viewwidth / 10);

    /* calculate trace angles and projection constants */
    CalcProjection(FOCALLENGTH);

    return true;
}

/*
==========================
=
= NewViewSize
=
= width is the menu's view size 4-20; the window is width*16 by width*8.
=
==========================
*/

void NewViewSize(int width)
{
    viewsize = (int16_t)width;
    SetViewSize((unsigned)(width * 16), (unsigned)(width * 16 * HEIGHTRATIO));
}

/*
=====================
=
= NewGame
=
= Set up new game to start from the beginning
=
=====================
*/

void NewGame(int difficulty, int episode)
{
    memset(&gamestate, 0, sizeof(gamestate));
    gamestate.difficulty = (int16_t)difficulty;
    gamestate.weapon = gamestate.bestweapon = gamestate.chosenweapon = wp_pistol;
    gamestate.health = 100;
    gamestate.ammo = STARTAMMO;
    gamestate.lives = 3;
    gamestate.nextextra = EXTRAPOINTS;
    gamestate.episode = (int16_t)episode;
}

/*
==========================
=
= ScanInfoPlane
=
= Spawn all actors and mark down special places
=
==========================
*/

void ScanInfoPlane(void)
{
    unsigned x, y;
    int tile;
    uint16_t *start;

    start = mapsegs[1];
    for (y = 0; y < (unsigned)mapheight; y++) {
        for (x = 0; x < (unsigned)mapwidth; x++) {
            tile = *start++;
            if (!tile) {
                continue;
            }

            switch (tile) {
            case 19:
            case 20:
            case 21:
            case 22:
                SpawnPlayer(x, y, NORTH + tile - 19);
                break;

            case 23: case 24: case 25: case 26: case 27: case 28: case 29: case 30:
            case 31: case 32: case 33: case 34: case 35: case 36: case 37: case 38:
            case 39: case 40: case 41: case 42: case 43: case 44: case 45: case 46:
            case 47: case 48: case 49: case 50: case 51: case 52: case 53: case 54:
            case 55: case 56: case 57: case 58: case 59: case 60: case 61: case 62:
            case 63: case 64: case 65: case 66: case 67: case 68: case 69: case 70:
            case 71:
            case 72: /* TRUCK AND SPEAR! */
            case 73:
            case 74:
                SpawnStatic(x, y, tile - 23);
                break;

            /* P wall */
            case 98:
                if (!loadedgame) {
                    gamestate.secrettotal++;
                }
                break;

            /* guard */
            case 180: case 181: case 182: case 183:
                if (gamestate.difficulty < gd_hard) {
                    break;
                }
                tile -= 36;
                /* fall through */
            case 144: case 145: case 146: case 147:
                if (gamestate.difficulty < gd_medium) {
                    break;
                }
                tile -= 36;
                /* fall through */
            case 108: case 109: case 110: case 111:
                SpawnStand(en_guard, x, y, tile - 108);
                break;

            case 184: case 185: case 186: case 187:
                if (gamestate.difficulty < gd_hard) {
                    break;
                }
                tile -= 36;
                /* fall through */
            case 148: case 149: case 150: case 151:
                if (gamestate.difficulty < gd_medium) {
                    break;
                }
                tile -= 36;
                /* fall through */
            case 112: case 113: case 114: case 115:
                SpawnPatrol(en_guard, x, y, tile - 112);
                break;

            case 124:
                SpawnDeadGuard(x, y);
                break;

            /* officer */
            case 188: case 189: case 190: case 191:
                if (gamestate.difficulty < gd_hard) {
                    break;
                }
                tile -= 36;
                /* fall through */
            case 152: case 153: case 154: case 155:
                if (gamestate.difficulty < gd_medium) {
                    break;
                }
                tile -= 36;
                /* fall through */
            case 116: case 117: case 118: case 119:
                SpawnStand(en_officer, x, y, tile - 116);
                break;

            case 192: case 193: case 194: case 195:
                if (gamestate.difficulty < gd_hard) {
                    break;
                }
                tile -= 36;
                /* fall through */
            case 156: case 157: case 158: case 159:
                if (gamestate.difficulty < gd_medium) {
                    break;
                }
                tile -= 36;
                /* fall through */
            case 120: case 121: case 122: case 123:
                SpawnPatrol(en_officer, x, y, tile - 120);
                break;

            /* ss */
            case 198: case 199: case 200: case 201:
                if (gamestate.difficulty < gd_hard) {
                    break;
                }
                tile -= 36;
                /* fall through */
            case 162: case 163: case 164: case 165:
                if (gamestate.difficulty < gd_medium) {
                    break;
                }
                tile -= 36;
                /* fall through */
            case 126: case 127: case 128: case 129:
                SpawnStand(en_ss, x, y, tile - 126);
                break;

            case 202: case 203: case 204: case 205:
                if (gamestate.difficulty < gd_hard) {
                    break;
                }
                tile -= 36;
                /* fall through */
            case 166: case 167: case 168: case 169:
                if (gamestate.difficulty < gd_medium) {
                    break;
                }
                tile -= 36;
                /* fall through */
            case 130: case 131: case 132: case 133:
                SpawnPatrol(en_ss, x, y, tile - 130);
                break;

            /* dogs */
            case 206: case 207: case 208: case 209:
                if (gamestate.difficulty < gd_hard) {
                    break;
                }
                tile -= 36;
                /* fall through */
            case 170: case 171: case 172: case 173:
                if (gamestate.difficulty < gd_medium) {
                    break;
                }
                tile -= 36;
                /* fall through */
            case 134: case 135: case 136: case 137:
                SpawnStand(en_dog, x, y, tile - 134);
                break;

            case 210: case 211: case 212: case 213:
                if (gamestate.difficulty < gd_hard) {
                    break;
                }
                tile -= 36;
                /* fall through */
            case 174: case 175: case 176: case 177:
                if (gamestate.difficulty < gd_medium) {
                    break;
                }
                tile -= 36;
                /* fall through */
            case 138: case 139: case 140: case 141:
                SpawnPatrol(en_dog, x, y, tile - 138);
                break;

            /* boss */
            case 214:
                SpawnBoss(x, y);
                break;
            case 197:
                SpawnGretel(x, y);
                break;
            case 215:
                SpawnGift(x, y);
                break;
            case 179:
                SpawnFat(x, y);
                break;
            case 196:
                SpawnSchabbs(x, y);
                break;
            case 160:
                SpawnFakeHitler(x, y);
                break;
            case 178:
                SpawnHitler(x, y);
                break;

            /* mutants */
            case 252: case 253: case 254: case 255:
                if (gamestate.difficulty < gd_hard) {
                    break;
                }
                tile -= 18;
                /* fall through */
            case 234: case 235: case 236: case 237:
                if (gamestate.difficulty < gd_medium) {
                    break;
                }
                tile -= 18;
                /* fall through */
            case 216: case 217: case 218: case 219:
                SpawnStand(en_mutant, x, y, tile - 216);
                break;

            case 256: case 257: case 258: case 259:
                if (gamestate.difficulty < gd_hard) {
                    break;
                }
                tile -= 18;
                /* fall through */
            case 238: case 239: case 240: case 241:
                if (gamestate.difficulty < gd_medium) {
                    break;
                }
                tile -= 18;
                /* fall through */
            case 220: case 221: case 222: case 223:
                SpawnPatrol(en_mutant, x, y, tile - 220);
                break;

            /* ghosts */
            case 224:
                SpawnGhosts(en_blinky, x, y);
                break;
            case 225:
                SpawnGhosts(en_clyde, x, y);
                break;
            case 226:
                SpawnGhosts(en_pinky, x, y);
                break;
            case 227:
                SpawnGhosts(en_inky, x, y);
                break;
            }
        }
    }
}

/*
==================
=
= SetupGameLevel
=
= Returns false when the map cannot be loaded (the original quit instead).
=
==================
*/

boolean SetupGameLevel(void)
{
    int mapnum;

    if (!loadedgame) {
        gamestate.TimeCount = gamestate.secrettotal = gamestate.killtotal =
            gamestate.treasuretotal = gamestate.secretcount = gamestate.killcount =
                gamestate.treasurecount = 0;
    }

    if (demoplayback || demorecord) {
        US_InitRndT(false);
    } else {
        US_InitRndT(true);
    }

    /* load the level */
    mapnum = gamestate.mapon + 10 * gamestate.episode;
    if (!CA_CacheMap(mapnum)) {
        return false;
    }

    mapwidth = (int16_t)mapheaderseg[mapnum]->width;
    mapheight = (int16_t)mapheaderseg[mapnum]->height;

    if (mapwidth != 64 || mapheight != 64) {
        fprintf(stderr, "SetupGameLevel: Map not 64*64!\n");
        return false;
    }

    SetupLoadedLevel();
    return true;
}

/*
==================
=
= SetupLoadedLevel
=
= The rest of SetupGameLevel, once mapsegs hold the level and mapwidth /
= mapheight are set. Split out so tests can run it on synthetic planes.
= Map edges are checked before reading a neighbour of an ambush tile; the
= original read outside the plane there.
=
==================
*/

void SetupLoadedLevel(void)
{
    int x, y;
    uint16_t *map, tile;

    /* copy the wall data to a data segment array */
    memset(tilemap, 0, sizeof(tilemap));
    memset(actorat, 0, sizeof(actorat));
    map = mapsegs[0];
    for (y = 0; y < mapheight; y++) {
        for (x = 0; x < mapwidth; x++) {
            tile = *map++;
            if (tile < AREATILE) {
                /* solid wall */
                tilemap[x][y] = (byte)tile;
                actorat[x][y] = ACTORAT_TILE(tile);
            } else {
                /* area floor */
                tilemap[x][y] = 0;
                actorat[x][y] = NULL;
            }
        }
    }

    /* spawn doors */
    InitActorList(); /* start spawning things with a clean slate */
    InitDoorList();
    InitStaticList();

    map = mapsegs[0];
    for (y = 0; y < mapheight; y++) {
        for (x = 0; x < mapwidth; x++) {
            tile = *map++;
            if (tile >= 90 && tile <= 101) {
                /* door */
                switch (tile) {
                case 90:
                case 92:
                case 94:
                case 96:
                case 98:
                case 100:
                    SpawnDoor(x, y, true, (tile - 90) / 2);
                    break;
                case 91:
                case 93:
                case 95:
                case 97:
                case 99:
                case 101:
                    SpawnDoor(x, y, false, (tile - 91) / 2);
                    break;
                }
            }
        }
    }

    /* spawn actors */
    ScanInfoPlane();

    /* take out the ambush markers */
    map = mapsegs[0];
    for (y = 0; y < mapheight; y++) {
        for (x = 0; x < mapwidth; x++) {
            tile = *map++;
            if (tile == AMBUSHTILE) {
                tilemap[x][y] = 0;
                if (ACTORAT_VALUE(actorat[x][y]) == AMBUSHTILE) {
                    actorat[x][y] = NULL;
                }

                if (x < mapwidth - 1 && *map >= AREATILE) {
                    tile = *map;
                }
                if (y > 0 && *(map - 1 - mapwidth) >= AREATILE) {
                    tile = *(map - 1 - mapwidth);
                }
                if (y < mapheight - 1 && *(map - 1 + mapwidth) >= AREATILE) {
                    tile = *(map - 1 + mapwidth);
                }
                if (x > 0 && *(map - 2) >= AREATILE) {
                    tile = *(map - 2);
                }

                *(map - 1) = tile;
            }
        }
    }
}
