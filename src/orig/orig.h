#pragma once

/*
 * Globals and functions of the faithful engine (second half of WL_DEF.H).
 * Names match the original so ported code reads the same; the legacy
 * engine uses Wolf_* names, so the two can be linked into one binary.
 */

#include "orig_def.h"
#include "orig_spr.h"
#include "wl_tables.h"

/* WL_MAIN */
extern gametype gamestate;
extern boolean loadedgame;
void InitGame(void);
void NewGame(int difficulty, int episode);

/* WL_GAME */
extern int16_t mapwidth, mapheight;
void ScanInfoPlane(void);
boolean SetupGameLevel(void);
void SetupLoadedLevel(void);

/* WL_PLAY */
extern objtype objlist[MAXACTORS], *new, *obj, *player, *lastobj, *objfreelist, *killerobj;
extern int16_t objcount;
extern uint16_t farmapylookup[MAPSIZE];
extern byte tilemap[MAPSIZE][MAPSIZE]; /* wall values only */
extern byte spotvis[MAPSIZE][MAPSIZE];
extern objtype *actorat[MAPSIZE][MAPSIZE];
extern boolean demorecord, demoplayback;

void InitActorList(void);
void GetNewActor(void);
void RemoveObj(objtype *gone);

/* WL_ACT1 */
extern statobj_t statobjlist[MAXSTATS], *laststatobj;
extern doorobj_t doorobjlist[MAXDOORS], *lastdoorobj;
extern int16_t doornum;
extern uint16_t doorposition[MAXDOORS];
extern byte areaconnect[NUMAREAS][NUMAREAS];
extern boolean areabyplayer[NUMAREAS];

void InitStaticList(void);
void SpawnStatic(int tilex, int tiley, int type);
void PlaceItemType(int itemtype, int tilex, int tiley);
void InitDoorList(void);
void SpawnDoor(int tilex, int tiley, boolean vertical, int lock);
void ConnectAreas(void);
void InitAreas(void);

/* WL_STATE */
void SpawnNewObj(unsigned tilex, unsigned tiley, statetype *state);
void NewState(objtype *ob, statetype *state);

/* WL_ACT2 */
extern const int16_t starthitpoints[4][NUMENEMIES];
extern statetype s_grdstand, s_grdpath1, s_grddie4;
extern statetype s_ofcstand, s_ofcpath1;
extern statetype s_ssstand, s_sspath1;
extern statetype s_mutstand, s_mutpath1;
extern statetype s_dogpath1;
extern statetype s_bossstand, s_gretelstand, s_schabbstand, s_giftstand, s_fatstand;
extern statetype s_fakestand, s_mechastand;
extern statetype s_blinkychase1, s_inkychase1, s_pinkychase1, s_clydechase1;

void SpawnStand(enemy_t which, int tilex, int tiley, int dir);
void SpawnDeadGuard(int tilex, int tiley);
void SpawnBoss(int tilex, int tiley);
void SpawnGretel(int tilex, int tiley);
void SpawnPatrol(enemy_t which, int tilex, int tiley, int dir);
void SpawnGhosts(int which, int tilex, int tiley);
void SpawnSchabbs(int tilex, int tiley);
void SpawnGift(int tilex, int tiley);
void SpawnFat(int tilex, int tiley);
void SpawnFakeHitler(int tilex, int tiley);
void SpawnHitler(int tilex, int tiley);

/* WL_AGENT */
extern statetype s_player;
extern int32_t thrustspeed;
void SpawnPlayer(int tilex, int tiley, int dir);
