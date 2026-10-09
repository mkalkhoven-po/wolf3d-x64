#pragma once

/*
 * Globals and functions of the faithful engine (second half of WL_DEF.H).
 * Names match the original so ported code reads the same; the legacy
 * engine uses Wolf_* names, so the two can be linked into one binary.
 */

#include "orig_audio.h"
#include "orig_def.h"
#include "orig_hooks.h"
#include "orig_spr.h"
#include "orig_states.h"
#include "wl_tables.h"

/* WL_MAIN */
extern gametype gamestate;
extern boolean loadedgame;
void InitGame(void);
void NewGame(int difficulty, int episode);

/*
 * Set from the view size by the renderer setup (CalcProjection / SetViewSize,
 * S4). The player's hit test compares actors' viewx with them.
 */
extern int16_t centerx, shootdelta;

/* WL_GAME */
extern int16_t mapwidth, mapheight;
extern boolean fizzlein; /* renderer fizzles the next frame in */
extern int32_t spearx, speary;
extern uint16_t spearangle;
extern boolean spearflag;
void ScanInfoPlane(void);
boolean SetupGameLevel(void);
void SetupLoadedLevel(void);

/* WL_PLAY */
extern exit_t playstate;
extern boolean madenoise; /* true when shooting or screaming */
extern objtype objlist[MAXACTORS], *new, *obj, *player, *lastobj, *objfreelist, *killerobj;
extern int16_t objcount;
extern uint16_t farmapylookup[MAPSIZE];
extern byte tilemap[MAPSIZE][MAPSIZE]; /* wall values only */
extern byte spotvis[MAPSIZE][MAPSIZE];
extern objtype *actorat[MAPSIZE][MAPSIZE];
extern boolean singlestep, godmode, noclip;
extern boolean demorecord, demoplayback;

/* current user input (PollControls, S5) */
#define NUMBUTTONS 8
enum {
    bt_nobutton = -1,
    bt_attack = 0,
    bt_strafe,
    bt_run,
    bt_use,
    bt_readyknife,
    bt_readypistol,
    bt_readymachinegun,
    bt_readychaingun
};
extern int16_t controlx, controly; /* range from -100 to 100 per tic */
extern boolean buttonstate[NUMBUTTONS];
extern boolean buttonheld[NUMBUTTONS];
extern uint16_t tics;

/*
 * DigiMode != sds_Off in the original. Boss death states last longer when
 * digitized sound is on, so it affects timing. DigiMode itself is a legacy
 * id-layer symbol, hence the different name.
 */
extern boolean digisoundenabled;

void InitActorList(void);
void GetNewActor(void);
void RemoveObj(objtype *gone);
void DoActor(objtype *ob);

/* WL_ACT1 */
extern statobj_t statobjlist[MAXSTATS], *laststatobj;
extern doorobj_t doorobjlist[MAXDOORS], *lastdoorobj;
extern int16_t doornum;
extern uint16_t doorposition[MAXDOORS];
extern byte areaconnect[NUMAREAS][NUMAREAS];
extern boolean areabyplayer[NUMAREAS];

/*
 * areabyplayer[area] for an actor's area. An actor spawned on a non-floor
 * tile gets an area number past the table: WL6 map 17 has a boss on an
 * ambush tile (area 255). The original then read unrelated memory (by
 * WOLF3D.MAP, the screen update[] marks, normally clear); here it reads as
 * "not connected". To be confirmed by demo replay in S7.
 */
#define AREABYPLAYER(area) ((unsigned)(area) < NUMAREAS && areabyplayer[(area)])

extern uint16_t pwallstate;
extern uint16_t pwallpos; /* amount a pushable wall has been moved (0-63) */
extern uint16_t pwallx, pwally;
extern int16_t pwalldir;

void InitStaticList(void);
void SpawnStatic(int tilex, int tiley, int type);
void PlaceItemType(int itemtype, int tilex, int tiley);
void InitDoorList(void);
void SpawnDoor(int tilex, int tiley, boolean vertical, int lock);
void ConnectAreas(void);
void InitAreas(void);
void OpenDoor(int door);
void CloseDoor(int door);
void OperateDoor(int door);
void MoveDoors(void);
void PushWall(int checkx, int checky, int dir);
void MovePWalls(void);

/* WL_STATE */
extern const dirtype opposite[9];
extern const dirtype diagonal[9][9];

void SpawnNewObj(unsigned tilex, unsigned tiley, statetype *state);
void NewState(objtype *ob, statetype *state);
boolean TryWalk(objtype *ob);
void SelectDodgeDir(objtype *ob);
void SelectChaseDir(objtype *ob);
void SelectRunDir(objtype *ob);
void MoveObj(objtype *ob, int32_t move);
void DropItem(stat_t itemtype, int tilex, int tiley);
void KillActor(objtype *ob);
void DamageActor(objtype *ob, unsigned damage);
boolean CheckLine(objtype *ob);
boolean CheckSight(objtype *ob);
void FirstSighting(objtype *ob);
boolean SightPlayer(objtype *ob);

/* WL_ACT2 */
extern const int16_t starthitpoints[4][NUMENEMIES];

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
boolean ProjectileTryMove(objtype *ob);
void SelectPathDir(objtype *ob);
void SpawnBJVictory(void);
boolean CheckPosition(objtype *ob);

/* WL_MAIN: view size and projection constants (set by NewViewSize) */
extern int16_t viewsize; /* menu setting 4-20 */
extern int16_t viewwidth, viewheight;
extern fixed focallength;
extern fixed scale, maxslope;
extern int32_t heightnumerator;
extern int16_t minheightdiv;
extern int16_t pixelangle[MAXVIEWWIDTH];
extern const int16_t dirangle[9];

void CalcProjection(int32_t focal);
boolean SetViewSize(unsigned width, unsigned height);
void NewViewSize(int width);

/* WL_DRAW */
#define MAXVISABLE 50

typedef struct {
    int16_t viewx, viewheight, shapenum;
} visobj_t;

extern fixed mindist;
extern uint16_t wallheight[MAXVIEWWIDTH];
extern fixed viewx, viewy; /* the focal point */
extern int16_t viewangle;
extern fixed viewsin, viewcos;
extern int16_t focaltx, focalty, viewtx, viewty;
extern int16_t midangle;
extern uint16_t xpartialup, xpartialdown, ypartialup, ypartialdown;
extern int32_t xintercept, yintercept;
extern visobj_t vislist[MAXVISABLE], *visptr, *visstep, *farthest;

void CalcViewVariables(void);
void TransformActor(objtype *ob);
boolean TransformTile(int tx, int ty, int16_t *dispx, int16_t *dispheight);
int16_t CalcHeight(void);
int CalcRotate(objtype *ob);
void DrawScaleds(void);

/* WL_SCALE: the draw calls made, in order (cleared by the caller) */
typedef visobj_t scaledraw_t;
extern scaledraw_t scaledraws[MAXVISABLE];
extern int numscaledraws;

void ScaleShape(int xcenter, int shapenum, unsigned height);

/* WL_AGENT */
extern statetype s_player, s_attack;
extern boolean running;
extern int32_t thrustspeed;
extern uint16_t plux, pluy; /* player coordinates scaled to unsigned */
extern int16_t anglefrac;
extern int16_t gotgatgun;
extern objtype *LastAttacker;
extern int16_t facecount;
extern int32_t playerxmove, playerymove;

void T_Player(objtype *ob);
void T_Attack(objtype *ob);
void CheckWeaponChange(void);
void ControlMovement(objtype *ob);
void UpdateFace(void);
void TakeDamage(int points, objtype *attacker);
void HealSelf(int points);
void GiveExtraMan(void);
void GivePoints(int32_t points);
void GiveWeapon(int weapon);
void GiveAmmo(int ammo);
void GiveKey(int key);
void GetBonus(statobj_t *check);
boolean TryMove(objtype *ob);
void ClipMove(objtype *ob, int32_t xmove, int32_t ymove);
void VictoryTile(void);
void Thrust(int angle, int32_t speed);
void Cmd_Fire(void);
void Cmd_Use(void);
void SpawnPlayer(int tilex, int tiley, int dir);
void KnifeAttack(objtype *ob);
void GunAttack(objtype *ob);
void VictorySpin(void);
