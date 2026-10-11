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
extern boolean upload; /* shareware build (UPLOAD): set for WL1 packs */
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
/*
 * Not in the original: a sideways move next to turning, for a twin-stick
 * game pad (left stick strafes while the right stick turns). The original
 * has one sideways value, controlx, that turns or (with bt_strafe) strafes.
 * Same units as controlx (right is positive); always 0 while a demo plays or
 * records, since the demo format cannot hold it.
 */
extern int16_t controlstrafe;
extern boolean buttonstate[NUMBUTTONS];
extern boolean buttonheld[NUMBUTTONS];
extern uint16_t tics;
extern int32_t TimeCount; /* the 70 Hz clock, driven by the platform layer (S5) */

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
/* the angle (0..ANGLES) of a point, via atan2 kept in a float as in the original */
int AtanAngle(int32_t deltax, int32_t deltay);
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

/* WL_DRAW walls (S4b) */
extern byte screenbuf[SCREENHEIGHT * SCREENWIDTH]; /* the canonical frame */
extern uint16_t screenofs; /* view window's top left in screenbuf */
extern const byte *(*orig_getpage)(int page); /* VSWAP page data, set by the platform */
extern int16_t orig_spritestart; /* PMSpriteStart of the loaded VSWAP */
extern int16_t horizwall[MAXWALLTILES], vertwall[MAXWALLTILES];
extern int16_t lastside;
extern int32_t lastintercept;
extern int16_t lasttilehit;
extern uint16_t tilehit, pixx;
extern int16_t xtile, ytile, xtilestep, ytilestep;
extern int32_t xstep, ystep;
extern uint16_t postx, postwidth;

void SetupWalls(void);
void ScalePost(void);
void HitVertWall(void);
void HitHorizWall(void);
void HitHorizDoor(void);
void HitVertDoor(void);
void HitHorizPWall(void);
void HitVertPWall(void);
void AsmRefresh(void);
void WallRefresh(void);
void VGAClearScreen(void);

/* WL_DRAW frame (S4c) */
extern int32_t lasttimecount, frameon;
void DrawPlayerWeapon(void);
void ThreeDRefresh(void);

/* WL_SCALE: sprites; ScaleShape also records its calls, in order (cleared by the caller) */
typedef visobj_t scaledraw_t;
extern scaledraw_t scaledraws[MAXVISABLE];
extern int numscaledraws;

void ScaleShape(int xcenter, int shapenum, unsigned height);

/* scaler table (SetupScaling) and the column scaler of the compiled scalers */
extern int16_t maxscale, maxscaleshl2, stepbytwo;
void SetupScaling(int maxheight);
int ScalerHeight(int index);
void ScaleColumn(const byte *src, int height, byte *dest, int pitch);
int TexelWidth(int height, int src);
int MaskedColumns(int x, int width, int cols[12]);
void SimpleScaleShape(int xcenter, int shapenum, unsigned height);

/* the play loop (S5): WL_PLAY.C controls / palette / music / PlayLoop, WL_GAME.C PlayDemo */
extern byte orig_keys[NUMCODES]; /* Keyboard[], filled by the platform layer */
extern byte orig_lastscan;        /* LastScan */
void IN_ClearKeysDown(void);
extern boolean mouseenabled, joystickenabled, joypadenabled, joystickprogressive;
extern int16_t joystickport, mouseadjustment;
extern int16_t dirscan[4], buttonscan[NUMBUTTONS], buttonmouse[4], buttonjoy[4];
extern const byte *demoptr, *lastdemoptr;
extern int16_t damagecount, bonuscount;
extern boolean palshifted, screenfaded, startgame;

void SetTimeCount(int32_t count);
void CalcTics(void);
void PollControls(void);
void StartDemoRecording(byte *buf, size_t len);
byte *DemoRecordingEnd(void);
void CheckKeys(void);
void StopMusic(void);
void StartMusic(void);
void ClearPaletteShifts(void);
void UpdatePaletteShifts(void);
void FinishPaletteShifts(void);
void PlayLoop(void);
boolean PlayDemo(const byte *demo, size_t demosize);

/* the level loop (S5b): WL_GAME.C */
extern boolean ingame;
extern boolean abortgame; /* not in the original: leave GameLoop (Quit, a lost game, the platform) */
void DrawPlayBorder(void);
void DrawPlayScreen(void);
void Died(void);
void GameLoop(void);

/* sound placement (WL_GAME.C): SetSoundLoc fills leftchannel/rightchannel, 0-8 */
extern fixed globalsoundx, globalsoundy;
extern int leftchannel, rightchannel;
void SetSoundLoc(fixed gx, fixed gy);

/*
 * SaveTheGame / LoadTheGame (WL_MAIN.C), save format v2 as a byte stream.
 * SaveTheGame returns the size it needs (writing only what fits in cap; 0 if
 * an actor state cannot be named). LoadTheGame checks the whole stream, then
 * loads the level and restores it; the caller then sets playstate so
 * GameLoop restarts on it (loadedgame is set).
 */
size_t SaveTheGame(byte *buf, size_t cap);
boolean LoadTheGame(const byte *buf, size_t len);

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

/*
 * The 2D toolkit (P5a, orig_vh.c): ID_VH.C, ID_VL.C and ID_US_1.C drawing
 * into screenbuf. VWB_DrawPic rounds x down to 8 as the original;
 * fontnumber 0 is the small font, 1 the large.
 */
#define WHITE 15 /* ID_VH.H */
#define BLACK 0
#define SETFONTCOLOR(f, b) (fontcolor = (byte)(f), backcolor = (byte)(b))

extern const byte gamepal[768]; /* the shared id layer's game palette */
extern byte fontcolor, backcolor;
extern int16_t fontnumber;
extern int16_t px, py; /* where VW_DrawPropString draws */
extern uint16_t PrintX, PrintY, WindowX, WindowY, WindowW, WindowH;

void VWB_Bar(int x, int y, int width, int height, int color);
void VWB_Plot(int x, int y, int color);
void VWB_Hlin(int x1, int x2, int y, int color);
void VWB_Vlin(int y1, int y2, int x, int color);
void VWB_DrawPic(int x, int y, int chunknum);
void VW_MeasurePropString(const char *string, uint16_t *width, uint16_t *height);
void VW_DrawPropString(const char *string);
void VWB_DrawPropString(const char *string);
void US_Print(const char *s);
void US_PrintUnsigned(uint32_t n);
void US_PrintSigned(int32_t n);
void US_PrintCentered(const char *s);
void US_CPrintLine(const char *s);
void US_CPrint(const char *s);
void US_ClearWindow(void);
void VL_SetPalette(const byte *palette);
void VL_GetPalette(byte *palette);
void VL_FillPalette(int red, int green, int blue);
void VL_WaitVBL(int vbls);
void VL_FadeOut(int start, int end, int red, int green, int blue, int steps);
void VL_FadeIn(int start, int end, const byte *palette, int steps);
void VW_FadeOut(void); /* to black over 30 retraces */
void VW_FadeIn(void);  /* back to the game palette */
void VW_UpdateScreen(void);

/* the status bar (WL_AGENT.C), into screenbuf rows 160-199 */
void StatusDrawPic(unsigned x, unsigned y, unsigned picnum);
void LatchNumber(int x, int y, int width, int32_t number);
void DrawFace(void);
void DrawHealth(void);
void DrawLevel(void);
void DrawLives(void);
void DrawScore(void);
void DrawWeapon(void);
void DrawKeys(void);
void DrawAmmo(void);
void DrawGotGatling(void); /* StatusDrawPic (17,4,GOTGATLINGPIC) in GetBonus */

/* WL_INTER.C: "Get Psyched!" before a level */
void PreloadGraphics(void);
void ClearSplitVWB(void); /* the 2D window over the play area (WL_INTER.C) */

/* WL_MENU.C (P5a: the drawing the rest of the game uses) */
void DrawOutline(int x, int y, int w, int h, int color1, int color2);
void DrawWindow(int x, int y, int w, int h, int wcolor);
void Message(const char *string);

/*
 * The control panel (P5b, WL_MENU.C, ID_IN.C). The menus wait in loops as
 * the original did; each pass pumps input (IN_Pump: poll_input and idle)
 * and reads the clock (GetTimeCount; headless, every read is a tic). Not in
 * the original: orig_quit, set when the window closes or Quit is confirmed,
 * makes every wait give up and the menus back out.
 */
extern byte orig_lastascii; /* LastASCII */
extern boolean Paused;
extern boolean orig_quit;
int32_t GetTimeCount(void);
void IN_Pump(void);
void IN_Ack(void);

typedef enum {
    dir_North,
    dir_NorthEast,
    dir_East,
    dir_SouthEast,
    dir_South,
    dir_SouthWest,
    dir_West,
    dir_NorthWest,
    dir_None
} Direction;

typedef struct {
    boolean button0, button1, button2, button3;
    int16_t x, y;
    int8_t xaxis, yaxis;
    Direction dir;
} ControlInfo;

/* a menu routine gets 0, as the original's casts passed it (temp1) */
typedef struct {
    int16_t x, y, amount, curpos, indent;
} CP_iteminfo;

typedef struct {
    int16_t active;
    char string[36];
    void (*routine)(int temp1);
} CP_itemtype;

enum {
    newgame,
    soundmenu,
    control,
    loadgame,
    savegame,
    changeview,
    readthis,
    viewscores,
    backtodemo,
    quit
};

#define STR_VS "View Scores"
#define MENUSONG WONDERIN_MUS
#define INTROSONG NAZI_NOR_MUS

extern CP_itemtype MainMenu[];
extern CP_iteminfo MainItems;

void US_ControlPanel(byte scancode);
void SetupControlPanel(void);
void CleanupControlPanel(void);
void DrawMainMenu(void);
void DrawMenu(CP_iteminfo *item_i, CP_itemtype *items);
int HandleMenu(CP_iteminfo *item_i, CP_itemtype *items, void (*routine)(int w));
void ClearMScreen(void);
void DrawStripes(int y);
void DrawMenuGun(CP_iteminfo *iteminfo);
void WaitKeyUp(void);
void ReadAnyControl(ControlInfo *ci);
void TicDelay(int count);
void StartCPMusic(int song);
int Confirm(const char *string);
void CheckPause(void);
void ShootSnd(void);
void CP_NewGame(int temp1);
void CP_Sound(int temp1);
int CP_LoadGame(int quick);
int CP_SaveGame(int quick);
void CP_Control(int temp1);
void CP_ChangeView(int temp1);
void CP_ReadThis(int temp1);
void CP_ViewScores(int temp1);
void CP_Quit(void);
int CP_EndGame(void);
int CP_CheckQuick(unsigned scancode);

/* ID_US_1.C */
extern HighScore Scores[MaxScores];
void DrawHighScores(void); /* WL_INTER.C */
boolean US_LineInput(int x, int y, char *buf, const char *def, boolean escok, int maxchars,
                     int maxwidth);

void ShowViewSize(int width); /* WL_MAIN.C: the border for a view size, for Change View */

/* WL_TEXT.C: the articles (orig_text.c) */
boolean HelpAvailable(void); /* the pack has help of its own (the shareware) */
void HelpScreens(void);      /* Read This!, F1 */
void EndText(void);     /* after a victory: T_ENDART1 + episode */
