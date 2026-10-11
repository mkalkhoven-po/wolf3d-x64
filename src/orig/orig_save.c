/*
 * SaveTheGame / LoadTheGame (WL_MAIN.C): save format v2.
 *
 * The original wrote memory images: gamestate, LevelRatios, tilemap,
 * actorat, areaconnect, areabyplayer, the actor list, the statics, the doors
 * and the pushwall, with near pointers in them (actor states, actorat
 * entries). This writes the same contents field by field, little-endian,
 * with pointers as indices: a state by its place in orig_statelist, an actor
 * by its place in the list (the player first). It adds what the original
 * left out so a loaded game continues exactly: the US_RndT index (the
 * original re-randomized it on load), killerobj and LastAttacker.
 *
 * The engine only builds and reads the byte stream; the platform layer puts
 * it in a file with its own header (pack, description, checksum).
 */

#include <string.h>

#include "id_us_rnd.h"
#include "orig.h"

#define SAVE_VERSION 2
#define NUMLEVELRATIOS 8 /* LevelRatios: each floor's intermission tally */

/* ------------------------------------------------------------ the stream */

typedef struct {
    byte *buf;
    size_t cap, len;
} writer_t;

typedef struct {
    const byte *buf;
    size_t len, pos;
    boolean bad;
} reader_t;

static void W8(writer_t *w, unsigned v)
{
    if (w->len < w->cap) {
        w->buf[w->len] = (byte)v;
    }
    w->len++; /* counts on past the end, so the caller learns the size */
}

static void W16(writer_t *w, unsigned v)
{
    W8(w, v & 0xff);
    W8(w, (v >> 8) & 0xff);
}

static void W32(writer_t *w, uint32_t v)
{
    W16(w, v & 0xffff);
    W16(w, (v >> 16) & 0xffff);
}

static void WBytes(writer_t *w, const void *data, size_t n)
{
    const byte *p = (const byte *)data;
    size_t i;

    for (i = 0; i < n; i++) {
        W8(w, p[i]);
    }
}

static unsigned R8(reader_t *r)
{
    if (r->pos >= r->len) {
        r->bad = true;
        return 0;
    }
    return r->buf[r->pos++];
}

static unsigned R16(reader_t *r)
{
    const unsigned lo = R8(r);
    return lo | (R8(r) << 8);
}

static int16_t RS16(reader_t *r)
{
    return (int16_t)R16(r);
}

static int32_t RS32(reader_t *r)
{
    const uint32_t lo = R16(r);
    return (int32_t)(lo | ((uint32_t)R16(r) << 16));
}

static void RBytes(reader_t *r, void *data, size_t n)
{
    if (r->len - r->pos < n || r->pos > r->len) {
        r->bad = true;
        memset(data, 0, n);
        return;
    }
    memcpy(data, r->buf + r->pos, n);
    r->pos += n;
}

/* ------------------------------------------------------- pointers, indices */

/* the place of an actor in the list (player 0), -1 for none */
static int ObjIndex(const objtype *ob)
{
    const objtype *check;
    int i = 0;

    if (!ob) {
        return -1;
    }
    for (check = player; check; check = check->next, i++) {
        if (check == ob) {
            return i;
        }
    }
    return -1; /* not in the list (removed): saved as none */
}

static int StateIndex(const statetype *state)
{
    int i;

    if (!state) {
        return -1;
    }
    if (state == &s_player) {
        return orig_numstates;
    }
    if (state == &s_attack) {
        return orig_numstates + 1;
    }
    for (i = 0; i < orig_numstates; i++) {
        if (orig_statelist[i] == state) {
            return i;
        }
    }
    return -2; /* unknown: refused by SaveTheGame */
}

static statetype *StateFromIndex(int index)
{
    if (index == orig_numstates) {
        return &s_player;
    }
    if (index == orig_numstates + 1) {
        return &s_attack;
    }
    if (index >= 0 && index < orig_numstates) {
        return orig_statelist[index];
    }
    return NULL;
}

/* ---------------------------------------------------------------- saving */

static void WriteGamestate(writer_t *w, const gametype *g)
{
    W16(w, (uint16_t)g->difficulty);
    W16(w, (uint16_t)g->mapon);
    W32(w, (uint32_t)g->oldscore);
    W32(w, (uint32_t)g->score);
    W32(w, (uint32_t)g->nextextra);
    W16(w, (uint16_t)g->lives);
    W16(w, (uint16_t)g->health);
    W16(w, (uint16_t)g->ammo);
    W16(w, (uint16_t)g->keys);
    W16(w, (uint16_t)g->bestweapon);
    W16(w, (uint16_t)g->weapon);
    W16(w, (uint16_t)g->chosenweapon);
    W16(w, (uint16_t)g->faceframe);
    W16(w, (uint16_t)g->attackframe);
    W16(w, (uint16_t)g->attackcount);
    W16(w, (uint16_t)g->weaponframe);
    W16(w, (uint16_t)g->episode);
    W16(w, (uint16_t)g->secretcount);
    W16(w, (uint16_t)g->treasurecount);
    W16(w, (uint16_t)g->killcount);
    W16(w, (uint16_t)g->secrettotal);
    W16(w, (uint16_t)g->treasuretotal);
    W16(w, (uint16_t)g->killtotal);
    W32(w, (uint32_t)g->TimeCount);
    W32(w, (uint32_t)g->killx);
    W32(w, (uint32_t)g->killy);
    W8(w, g->victoryflag ? 1 : 0);
}

static void WriteObj(writer_t *w, const objtype *ob)
{
    W16(w, (uint16_t)ob->active);
    W16(w, (uint16_t)ob->ticcount);
    W16(w, (uint16_t)ob->obclass);
    W16(w, (uint16_t)StateIndex(ob->state));
    W8(w, ob->flags);
    W32(w, (uint32_t)ob->distance);
    W16(w, (uint16_t)ob->dir);
    W32(w, (uint32_t)ob->x);
    W32(w, (uint32_t)ob->y);
    W16(w, ob->tilex);
    W16(w, ob->tiley);
    W8(w, ob->areanumber);
    W16(w, (uint16_t)ob->viewx);
    W16(w, ob->viewheight);
    W32(w, (uint32_t)ob->transx);
    W32(w, (uint32_t)ob->transy);
    W16(w, (uint16_t)ob->angle);
    W16(w, (uint16_t)ob->hitpoints);
    W32(w, (uint32_t)ob->speed);
    W16(w, (uint16_t)ob->temp1);
    W16(w, (uint16_t)ob->temp2);
    W16(w, (uint16_t)ob->temp3);
}

size_t SaveTheGame(byte *buf, size_t cap)
{
    writer_t w = {buf, cap, 0};
    const objtype *ob;
    int x, y, i, count = 0;

    for (ob = player; ob; ob = ob->next) {
        if (StateIndex(ob->state) == -2) {
            return 0; /* a state the save format cannot name */
        }
        count++;
    }

    W16(&w, SAVE_VERSION);
    WriteGamestate(&w, &gamestate);
    for (i = 0; i < NUMLEVELRATIOS; i++) {
        W16(&w, (uint16_t)LevelRatios[i].kill);
        W16(&w, (uint16_t)LevelRatios[i].secret);
        W16(&w, (uint16_t)LevelRatios[i].treasure);
        W32(&w, (uint32_t)LevelRatios[i].time);
    }

    WBytes(&w, tilemap, sizeof(tilemap));
    for (x = 0; x < MAPSIZE; x++) {
        for (y = 0; y < MAPSIZE; y++) {
            /* a tile value (wall, door | 0x80, 1 for a blocking static) or an actor */
            const objtype *check = actorat[x][y];
            W16(&w, ISPOINTER(check) ? (uint16_t)(-1 - ObjIndex(check))
                                     : (uint16_t)ACTORAT_VALUE(check));
        }
    }
    WBytes(&w, areaconnect, sizeof(areaconnect));
    for (i = 0; i < NUMAREAS; i++) {
        W8(&w, areabyplayer[i] ? 1 : 0);
    }

    W16(&w, (uint16_t)count);
    for (ob = player; ob; ob = ob->next) {
        WriteObj(&w, ob);
    }

    W16(&w, (uint16_t)(laststatobj - statobjlist));
    for (i = 0; i < MAXSTATS; i++) {
        /* all of them, as the original: a reused slot keeps old flags (bo_clip2) */
        const statobj_t *st = &statobjlist[i];
        W8(&w, st->tilex);
        W8(&w, st->tiley);
        W8(&w, st->visspot ? 1 : 0);
        W16(&w, (uint16_t)st->shapenum);
        W8(&w, st->flags);
        W8(&w, st->itemnumber);
    }

    for (i = 0; i < MAXDOORS; i++) {
        W16(&w, doorposition[i]);
    }
    for (i = 0; i < MAXDOORS; i++) {
        const doorobj_t *door = &doorobjlist[i];
        W8(&w, door->tilex);
        W8(&w, door->tiley);
        W8(&w, door->vertical ? 1 : 0);
        W8(&w, door->lock);
        W8(&w, (unsigned)door->action);
        W16(&w, (uint16_t)door->ticcount);
    }

    W16(&w, pwallstate);
    W16(&w, pwallx);
    W16(&w, pwally);
    W16(&w, (uint16_t)pwalldir);
    W16(&w, pwallpos);

    /* not in the original */
    W8(&w, (unsigned)US_GetRndIndex());
    W16(&w, (uint16_t)ObjIndex(killerobj));
    W16(&w, (uint16_t)ObjIndex(LastAttacker));
    W16(&w, (uint16_t)gotgatgun);

    return w.len;
}

/* --------------------------------------------------------------- loading */

/* everything is read and checked here before the engine is touched */
static struct {
    gametype game;
    byte tilemap[MAPSIZE][MAPSIZE];
    int16_t actorat[MAPSIZE][MAPSIZE];
    byte areaconnect[NUMAREAS][NUMAREAS];
    byte areabyplayer[NUMAREAS];
    int count;
    objtype objs[MAXACTORS];
    int laststat;
    statobj_t stats[MAXSTATS];
    byte statvis[MAXSTATS];
    uint16_t doorposition[MAXDOORS];
    doorobj_t doors[MAXDOORS];
    uint16_t pwallstate, pwallx, pwally, pwallpos;
    int16_t pwalldir;
    int rndindex, killer, attacker;
    int16_t gotgatgun;
    LRstruct ratios[NUMLEVELRATIOS];
} sv;

static boolean ReadSave(reader_t *r)
{
    gametype *g = &sv.game;
    int x, y, i;

    if (R16(r) != SAVE_VERSION) {
        return false;
    }

    memset(g, 0, sizeof(*g));
    g->difficulty = RS16(r);
    g->mapon = RS16(r);
    g->oldscore = RS32(r);
    g->score = RS32(r);
    g->nextextra = RS32(r);
    g->lives = RS16(r);
    g->health = RS16(r);
    g->ammo = RS16(r);
    g->keys = RS16(r);
    g->bestweapon = (weapontype)RS16(r);
    g->weapon = (weapontype)RS16(r);
    g->chosenweapon = (weapontype)RS16(r);
    g->faceframe = RS16(r);
    g->attackframe = RS16(r);
    g->attackcount = RS16(r);
    g->weaponframe = RS16(r);
    g->episode = RS16(r);
    g->secretcount = RS16(r);
    g->treasurecount = RS16(r);
    g->killcount = RS16(r);
    g->secrettotal = RS16(r);
    g->treasuretotal = RS16(r);
    g->killtotal = RS16(r);
    g->TimeCount = RS32(r);
    g->killx = RS32(r);
    g->killy = RS32(r);
    g->victoryflag = R8(r) != 0;
    for (i = 0; i < NUMLEVELRATIOS; i++) {
        sv.ratios[i].kill = RS16(r);
        sv.ratios[i].secret = RS16(r);
        sv.ratios[i].treasure = RS16(r);
        sv.ratios[i].time = RS32(r);
    }
    /* demos keep the absolute map in mapon with episode 0, so check the sum */
    if (g->episode < 0 || g->mapon < 0 || g->mapon + 10 * g->episode >= 60 ||
        g->difficulty < gd_baby || g->difficulty > gd_hard) {
        return false;
    }

    RBytes(r, sv.tilemap, sizeof(sv.tilemap));
    for (x = 0; x < MAPSIZE; x++) {
        for (y = 0; y < MAPSIZE; y++) {
            sv.actorat[x][y] = RS16(r);
        }
    }
    RBytes(r, sv.areaconnect, sizeof(sv.areaconnect));
    RBytes(r, sv.areabyplayer, sizeof(sv.areabyplayer));

    sv.count = (int)R16(r);
    if (sv.count < 1 || sv.count > MAXACTORS) {
        return false;
    }
    for (i = 0; i < sv.count; i++) {
        objtype *ob = &sv.objs[i];
        int state;

        memset(ob, 0, sizeof(*ob));
        ob->active = (activetype)RS16(r);
        ob->ticcount = RS16(r);
        ob->obclass = (classtype)RS16(r);
        state = RS16(r);
        ob->state = StateFromIndex(state);
        if (state != -1 && !ob->state) {
            return false;
        }
        ob->flags = (byte)R8(r);
        ob->distance = RS32(r);
        ob->dir = (dirtype)RS16(r);
        ob->x = RS32(r);
        ob->y = RS32(r);
        ob->tilex = (uint16_t)R16(r);
        ob->tiley = (uint16_t)R16(r);
        ob->areanumber = (byte)R8(r);
        ob->viewx = RS16(r);
        ob->viewheight = (uint16_t)R16(r);
        ob->transx = RS32(r);
        ob->transy = RS32(r);
        ob->angle = RS16(r);
        ob->hitpoints = RS16(r);
        ob->speed = RS32(r);
        ob->temp1 = RS16(r);
        ob->temp2 = RS16(r);
        ob->temp3 = RS16(r);
        if (ob->tilex >= MAPSIZE || ob->tiley >= MAPSIZE || (unsigned)ob->dir > nodir) {
            return false;
        }
    }
    for (x = 0; x < MAPSIZE; x++) {
        for (y = 0; y < MAPSIZE; y++) {
            if (sv.actorat[x][y] < -sv.count) {
                return false; /* an actor that is not in the list */
            }
        }
    }

    sv.laststat = (int)R16(r);
    if (sv.laststat > MAXSTATS) {
        return false;
    }
    for (i = 0; i < MAXSTATS; i++) {
        statobj_t *st = &sv.stats[i];

        st->tilex = (byte)R8(r);
        st->tiley = (byte)R8(r);
        sv.statvis[i] = (byte)R8(r);
        st->shapenum = RS16(r);
        st->flags = (byte)R8(r);
        st->itemnumber = (byte)R8(r);
        if (st->tilex >= MAPSIZE || st->tiley >= MAPSIZE) {
            return false;
        }
    }

    for (i = 0; i < MAXDOORS; i++) {
        sv.doorposition[i] = (uint16_t)R16(r);
    }
    for (i = 0; i < MAXDOORS; i++) {
        doorobj_t *door = &sv.doors[i];

        door->tilex = (byte)R8(r);
        door->tiley = (byte)R8(r);
        door->vertical = R8(r) != 0;
        door->lock = (byte)R8(r);
        door->action = (dooraction_t)R8(r);
        door->ticcount = RS16(r);
        if (door->tilex >= MAPSIZE || door->tiley >= MAPSIZE || door->action > dr_closing) {
            return false;
        }
    }

    sv.pwallstate = (uint16_t)R16(r);
    sv.pwallx = (uint16_t)R16(r);
    sv.pwally = (uint16_t)R16(r);
    sv.pwalldir = RS16(r);
    sv.pwallpos = (uint16_t)R16(r);
    if (sv.pwallx >= MAPSIZE || sv.pwally >= MAPSIZE) {
        return false;
    }

    sv.rndindex = (int)R8(r);
    sv.killer = RS16(r);
    sv.attacker = RS16(r);
    sv.gotgatgun = RS16(r);
    if (sv.killer >= sv.count || sv.attacker >= sv.count) {
        return false;
    }
    return !r->bad && r->pos == r->len;
}

static objtype *ObjFromIndex(objtype *const *list, int index)
{
    return index >= 0 ? list[index] : NULL;
}

boolean LoadTheGame(const byte *buf, size_t len)
{
    static objtype *list[MAXACTORS];
    reader_t r = {buf, len, 0, false};
    int x, y, i;

    if (!buf || !ReadSave(&r)) {
        return false;
    }

    gamestate = sv.game;
    memcpy(LevelRatios, sv.ratios, sizeof(LevelRatios));
    loadedgame = true; /* SetupGameLevel keeps the level totals */
    if (!SetupGameLevel()) {
        return false;
    }

    memcpy(tilemap, sv.tilemap, sizeof(tilemap));
    memcpy(areaconnect, sv.areaconnect, sizeof(areaconnect));
    for (i = 0; i < NUMAREAS; i++) {
        areabyplayer[i] = sv.areabyplayer[i] != 0;
    }

    /* the actors in their saved order, keeping the list links */
    InitActorList();
    for (i = 0; i < sv.count; i++) {
        objtype *next, *prev;

        if (i) {
            GetNewActor();
        }
        list[i] = i ? new : player;
        next = list[i]->next;
        prev = list[i]->prev;
        *list[i] = sv.objs[i];
        list[i]->next = next;
        list[i]->prev = prev;
    }

    for (x = 0; x < MAPSIZE; x++) {
        for (y = 0; y < MAPSIZE; y++) {
            const int16_t v = sv.actorat[x][y];
            actorat[x][y] = v < 0 ? list[-1 - v] : ACTORAT_TILE((uint16_t)v);
        }
    }

    laststatobj = &statobjlist[sv.laststat];
    for (i = 0; i < MAXSTATS; i++) {
        statobjlist[i] = sv.stats[i];
        statobjlist[i].visspot =
            sv.statvis[i] ? &spotvis[sv.stats[i].tilex][sv.stats[i].tiley] : NULL;
    }
    memcpy(doorposition, sv.doorposition, sizeof(doorposition));
    memcpy(doorobjlist, sv.doors, sizeof(doorobjlist));

    pwallstate = sv.pwallstate;
    pwallx = sv.pwallx;
    pwally = sv.pwally;
    pwalldir = sv.pwalldir;
    pwallpos = sv.pwallpos;

    US_SetRndIndex(sv.rndindex);
    killerobj = ObjFromIndex(list, sv.killer);
    LastAttacker = ObjFromIndex(list, sv.attacker);
    gotgatgun = sv.gotgatgun;
    return true;
}
