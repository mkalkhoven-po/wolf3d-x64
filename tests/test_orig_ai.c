/*
 * Actor AI, doors and pushwalls of the faithful engine (P4/S2).
 *
 * Without arguments: synthetic scenarios with hand-derived expectations
 * (reaction delay from the RNG table, death/pain state chains, door and
 * pushwall timing, sight lines) plus a golden trace hash that pins the whole
 * simulation, and the boss specials (throws, morph, death cam, BJ). With
 * --data DIR: every map runs for
 * 2100 tics on all four difficulties with the player standing still.
 */
#include "orig_testkit.h"
#include "wl_data.h"

/* ------------------------------------------------------------ state tables */

static void test_state_tables(void)
{
    int i, null_next = 0;

    CHECK(orig_numstates == 303);
    for (i = 0; i < orig_numstates; i++) {
        const statetype *s = orig_statelist[i];
        CHECK(s->tictime >= 0);
        CHECK(s->rotate >= 0 && s->rotate <= 2);
        CHECK(s->shapenum >= 0 && s->shapenum <= SPR_CHAINATK4);
        if (s->next) {
            CHECK(state_index(s->next) >= 0);
        } else {
            null_next++;
        }
    }
    /* s_smoke4, s_boom3 and s_deathcam end their chains (object removed) */
    CHECK(null_next == 3);

    CHECK(s_grdstand.think == T_Stand && s_grdstand.next == &s_grdstand);
    CHECK(s_grdpath4.next == &s_grdpath1 && s_grdpath1s.think == NULL);
    CHECK(s_grdpain.rotate == 2 && s_grdpain1.rotate == 2);
    CHECK(s_grdshoot2.action == T_Shoot && s_grddie1.action == A_DeathScream);
    CHECK(s_dogjump2.action == T_Bite && s_dogchase1.think == T_DogChase);
    CHECK(s_rocket.think == T_Projectile && s_rocket.action == A_Smoke);
    CHECK(s_ssshoot1.tictime == 20 && s_ofcshoot1.tictime == 6);
}

/* ------------------------------------------------------------- sight lines */

static void test_checkline(void)
{
    objtype *guard;

    empty_map();
    AT(plane1, 10, 10) = 19;  /* player, north */
    AT(plane1, 15, 10) = 108; /* guard */
    load_level(gd_hard);
    guard = first_actor();

    CHECK(CheckLine(guard));

    tilemap[12][10] = 1; /* wall in between */
    CHECK(!CheckLine(guard));

    doornum = 1; /* closed door in between */
    tilemap[12][10] = 0x80;
    doorposition[0] = 0;
    CHECK(!CheckLine(guard));
    /*
     * The intercept is the absolute y of the line in 1/256 tiles (10.5 tiles
     * = 2688 here), compared with the door position: the door stops blocking
     * sight once it is open at least that far, not when the line clears it.
     */
    doorposition[0] = 2687;
    CHECK(!CheckLine(guard));
    doorposition[0] = 2688;
    CHECK(CheckLine(guard));
    doorposition[0] = 0xffff;
    CHECK(CheckLine(guard));

    tilemap[12][10] = 0;
    guard->dir = east; /* looking away, player more than MINSIGHT behind */
    CHECK(!CheckSight(guard));
    guard->dir = west;
    CHECK(CheckSight(guard));
}

/* -------------------------------------------------------- reaction / chase */

static void test_guard_reaction(void)
{
    objtype *guard;
    int tic, reacted = 0, shot_tic = 0;

    empty_map();
    AT(plane1, 10, 10) = 19;      /* player */
    AT(plane1, 15, 10) = 108 + 2; /* guard facing west, towards the player */
    load_level(gd_hard);
    guard = first_actor();
    CHECK(guard->state == &s_grdstand && guard->dir == west);

    /*
     * No RNG used while spawning. The player thinks first each tic and its
     * UpdateFace draws rndtable[1] (8 > facecount 1, no face change), then
     * SightPlayer draws rndtable[2] = 109: reaction delay 1 + 109/4 = 28.
     */
    for (tic = 1; tic <= 40 && !reacted; tic++) {
        run_tic();
        if (tic == 1) {
            CHECK(guard->temp2 == 28);
        }
        if (guard->flags & FL_ATTACKMODE) {
            reacted = tic;
        }
    }
    CHECK(reacted == 29);
    CHECK(guard->state == &s_grdchase1 && guard->speed == SPDPATROL * 3);
    CHECK(guard->flags & FL_FIRSTATTACK);
    CHECK(count_sound(HALTSND) == 1);

    for (tic = 0; tic < 700 && !shot_tic; tic++) {
        run_tic();
        if (count_sound(NAZIFIRESND)) {
            shot_tic = tic + 1;
        }
    }
    CHECK(shot_tic > 0);
    CHECK(gamestate.health <= 100);
}

/* ----------------------------------------------------------- pain / death */

static void test_damage_and_death(void)
{
    objtype *guard;
    statobj_t *clip;
    int tic;

    empty_map();
    AT(plane1, 10, 10) = 19;
    AT(plane1, 15, 10) = 108; /* facing east, away from the player */
    load_level(gd_hard);
    guard = first_actor();

    DamageActor(guard, 10); /* doubled: not in attack mode */
    CHECK(guard->hitpoints == 5);
    CHECK(guard->state == &s_grdpain); /* odd hitpoints */
    CHECK(guard->flags & FL_ATTACKMODE);
    CHECK(madenoise);

    DamageActor(guard, 10);
    CHECK(guard->hitpoints == -5);
    CHECK(guard->state == &s_grddie1 && guard->ticcount == 15);
    CHECK(gamestate.score == 100 && gamestate.killcount == 1);
    CHECK(!(guard->flags & FL_SHOOTABLE) && (guard->flags & FL_NONMARK));
    clip = laststatobj - 1;
    CHECK(clip->itemnumber == bo_clip2 && clip->tilex == 15 && clip->tiley == 10);

    for (tic = 1; tic <= 45; tic++) {
        run_tic();
        if (tic == 44) {
            CHECK(guard->state == &s_grddie3);
        }
    }
    CHECK(guard->state == &s_grddie4);
    CHECK(count_sound(DEATHSCREAM1SND) + count_sound(DEATHSCREAM2SND) +
              count_sound(DEATHSCREAM3SND) + count_sound(DEATHSCREAM4SND) +
              count_sound(DEATHSCREAM5SND) + count_sound(DEATHSCREAM7SND) +
              count_sound(DEATHSCREAM8SND) + count_sound(DEATHSCREAM9SND) ==
          1);

    /* SS drop a machine gun while the player only has the pistol */
    empty_map();
    AT(plane1, 10, 10) = 19;
    AT(plane1, 15, 10) = 126;
    load_level(gd_hard);
    KillActor(first_actor());
    CHECK((laststatobj - 1)->itemnumber == bo_machinegun);

    /* extra life every 40000 points */
    gamestate.score = 39950;
    GivePoints(100);
    CHECK(gamestate.lives == 4 && gamestate.nextextra == 80000);
    CHECK(count_sound(BONUS1UPSND) == 1);

    /* baby mode quarters damage; death sets playstate */
    gamestate.difficulty = gd_baby;
    gamestate.health = 100;
    TakeDamage(40, NULL);
    CHECK(gamestate.health == 90);
    gamestate.difficulty = gd_hard;
    TakeDamage(200, first_actor());
    CHECK(gamestate.health == 0 && playstate == ex_died && killerobj == first_actor());
}

/* ------------------------------------------------------------------ doors */

static void test_door_cycle(void)
{
    int tic;

    empty_map();
    split_areas(20);
    AT(plane0, 20, 10) = 90; /* vertical door between area 0 and 1 */
    AT(plane1, 10, 10) = 19;
    load_level(gd_hard);

    CHECK(doornum == 1 && !areabyplayer[1]);
    OpenDoor(0);

    for (tic = 1; tic <= 64; tic++) {
        MoveDoors();
        if (tic == 1) {
            CHECK(doorposition[0] == 1024 && areaconnect[0][1] == 1 && areabyplayer[1]);
            CHECK(count_sound(OPENDOORSND) == 1);
        }
    }
    CHECK(doorobjlist[0].action == dr_open && doorposition[0] == 0xffff);
    CHECK(actorat[20][10] == NULL);

    for (tic = 1; tic <= 300; tic++) {
        MoveDoors();
        if (tic == 299) {
            CHECK(doorobjlist[0].action == dr_open);
        }
    }
    CHECK(doorobjlist[0].action == dr_closing);
    CHECK(ACTORAT_VALUE(actorat[20][10]) == 0x80);
    CHECK(count_sound(CLOSEDOORSND) == 1);

    for (tic = 1; tic <= 64; tic++) {
        MoveDoors();
    }
    CHECK(doorobjlist[0].action == dr_closed && doorposition[0] == 0);
    CHECK(areaconnect[0][1] == 0 && !areabyplayer[1]);
}

static void test_guard_opens_door(void)
{
    objtype *guard;
    int tic;

    empty_map();
    split_areas(20);
    AT(plane0, 20, 10) = 90;
    AT(plane1, 10, 10) = 19;
    AT(plane1, 18, 10) = 112; /* patrol heading east, through the door */
    load_level(gd_hard);
    guard = first_actor();
    CHECK(guard->tilex == 19);

    for (tic = 0; tic < 400 && doorobjlist[0].action == dr_closed; tic++) {
        run_tic();
    }
    CHECK(doorobjlist[0].action != dr_closed); /* TryWalk asked for it */
}

/* --------------------------------------------------------------- pushwalls */

static int pushwall_distance(int tics_per_frame)
{
    int frames, x;

    empty_map();
    AT(plane0, 30, 30) = 5;
    AT(plane1, 30, 30) = PUSHABLETILE;
    AT(plane1, 29, 30) = 19;
    load_level(gd_hard);
    CHECK(gamestate.secrettotal == 1);

    PushWall(30, 30, di_east);
    CHECK(gamestate.secretcount == 1 && pwallstate == 1);
    CHECK(tilemap[30][30] == (5 | 0xc0) && tilemap[31][30] == 5);
    CHECK(AT(plane1, 30, 30) == 0 && count_sound(PUSHWALLSND) == 1);

    tics = (uint16_t)tics_per_frame;
    for (frames = 0; frames < 1000 && pwallstate; frames++) {
        MovePWalls();
    }
    for (x = 30; x < 40; x++) {
        if (tilemap[x][30]) {
            return x - 30;
        }
    }
    return -1;
}

static void test_pushwall(void)
{
    /*
     * MovePWalls stops once pwallstate > 256. With one tic per frame it lands
     * on exactly 256 first and moves once more, so the wall ends three tiles
     * away; with two tics per frame it skips 256 and ends two tiles away.
     * The original behaves the same, so the distance depends on frame rate.
     */
    CHECK(pushwall_distance(1) == 3);
    CHECK(pushwall_distance(2) == 2);
}

/* ---------------------------------------------------------------- patrols */

static void test_patrol_loop(void)
{
    objtype *guard;
    int tic, turns = 0;
    dirtype lastdir;
    static const dirtype order[4] = {north, west, south, east};

    empty_map();
    split_areas(40); /* player out of sight in area 1 */
    AT(plane1, 50, 50) = 19;
    AT(plane1, 11, 20) = 112;            /* patrol heading east */
    AT(plane1, 14, 20) = ICONARROWS + 2; /* north */
    AT(plane1, 14, 16) = ICONARROWS + 4; /* west */
    AT(plane1, 10, 16) = ICONARROWS + 6; /* south */
    AT(plane1, 10, 20) = ICONARROWS + 0; /* east */
    load_level(gd_hard);
    guard = first_actor();
    CHECK(guard->ticcount == rndtable[1] % 20);

    lastdir = guard->dir;
    CHECK(lastdir == east);
    for (tic = 0; tic < 6000 && turns < 8; tic++) {
        run_tic();
        if (guard->dir != lastdir) {
            CHECK(guard->dir == order[turns % 4]); /* turns follow the arrows */
            lastdir = guard->dir;
            turns++;
        }
        CHECK(guard->tiley >= 16 && guard->tiley <= 20 && guard->tilex >= 10 &&
              guard->tilex <= 14);
    }
    CHECK(turns == 8); /* two laps */
    CHECK(!(guard->flags & FL_ATTACKMODE));
    CHECK(guard->temp2 == 0); /* never sighted the player */
}

/* ------------------------------------------------------------ golden trace */

static uint32_t hash_world(uint32_t h)
{
    objtype *ob;

    for (ob = player; ob; ob = ob->next) {
        const int32_t v[10] = {ob->x, ob->y, ob->tilex, ob->tiley, ob->dir,
                               state_index(ob->state), ob->ticcount, ob->hitpoints, ob->flags,
                               ob->distance};
        h = fnv(h, v, sizeof(v));
    }
    h = fnv(h, &gamestate.health, sizeof(gamestate.health));
    h = fnv(h, doorposition, sizeof(doorposition));
    return h;
}

static uint32_t run_trace(void)
{
    uint32_t h = 2166136261u;
    int tic;

    empty_map();
    split_areas(40);
    AT(plane0, 40, 12) = 90;      /* door to area 1 */
    AT(plane1, 8, 12) = 19;       /* player */
    AT(plane1, 30, 12) = 108 + 2; /* guard facing the player */
    AT(plane1, 30, 20) = 134 + 0; /* standing dog: rewrites the SS patrol (quirk) */
    AT(plane1, 25, 30) = 138;     /* dog patrol east */
    AT(plane1, 35, 30) = ICONARROWS + 4;
    AT(plane1, 20, 30) = ICONARROWS + 0;
    AT(plane1, 50, 12) = 130 + 2; /* SS patrol west, through the door */
    AT(plane1, 28, 6) = 126 + 3;  /* SS standing, facing south */
    load_level(gd_hard);
    godmode = true; /* keep the player alive for the whole trace */

    for (tic = 0; tic < 1500; tic++) {
        run_tic();
        h = hash_world(h);
    }
    return h;
}

static void test_golden_trace(void)
{
    const uint32_t a = run_trace();
    const uint32_t b = run_trace();

    printf("golden trace: %08x\n", a);
    CHECK(a == b);
    CHECK(a == 0x9d67a71eu); /* regression pin: update only for an intended change */
}


/* ------------------------------------------------------------ boss specials */

/* the death cam's screen work: VW_WaitVBL (100), the 70-frame fizzle, IN_UserInput (300) */
static int g_pause, g_fizzle, g_seeagain;

static void count_pause(int vbls)
{
    g_pause += vbls == 100;
}

static void count_fizzle(int x, int y, int width, int height, int frames)
{
    g_fizzle += x == 0 && y == 0 && width == 320 && height == 200 - STATUSLINES && frames == 70;
}

static boolean count_seeagain(int32_t tics)
{
    g_seeagain += tics == 300;
    return false;
}

static objtype *find_class(classtype c)
{
    objtype *ob;

    for (ob = player->next; ob; ob = ob->next) {
        if (ob->obclass == c) {
            return ob;
        }
    }
    return NULL;
}

/* Run until an actor of class c exists; returns the tic it appeared or 0. */
static int run_until_class(classtype c, int maxtics)
{
    int tic;

    for (tic = 1; tic <= maxtics; tic++) {
        run_tic();
        if (find_class(c)) {
            return tic;
        }
    }
    return 0;
}

/*
 * Angle of Schabbs' first needle with the player at px,py. The angle comes
 * from atan2 kept in a float; the four axis directions and the diagonal
 * truncate to whole degrees here.
 */
static int needle_angle(int px, int py)
{
    objtype *needle;

    empty_map();
    AT(plane1, px, py) = 19;
    AT(plane1, 20, 20) = 196;
    load_level(gd_hard);
    godmode = true;
    FirstSighting(find_class(schabbobj)); /* ambush boss: wake it directly */
    find_class(schabbobj)->speed = 0;     /* stay on the tile centre while throwing */

    if (!run_until_class(needleobj, 2000)) {
        return -1;
    }
    needle = find_class(needleobj);
    CHECK(needle->speed == 0x2000 && needle->flags == FL_NONMARK && needle->active);
    /* appended to the list, so it already ran once: s_needle1 lasts one tic */
    CHECK(needle->state == &s_needle2);
    return needle->angle;
}

static void test_boss_throwers(void)
{
    int tic;

    CHECK(needle_angle(10, 20) == 180); /* west */
    CHECK(needle_angle(30, 20) == 0);   /* east */
    CHECK(needle_angle(20, 10) == 90);  /* north */
    CHECK(needle_angle(20, 30) == 270); /* south */
    CHECK(needle_angle(30, 10) == 45);  /* north-east */
    CHECK(count_sound(SCHABBSTHROWSND) >= 1);

    /* a needle reaches the player and hurts: 20-51 points on hard */
    empty_map();
    AT(plane1, 10, 20) = 19;
    AT(plane1, 20, 20) = 196;
    load_level(gd_hard);
    FirstSighting(find_class(schabbobj));
    for (tic = 0; tic < 2000 && gamestate.health == 100; tic++) {
        run_tic();
    }
    CHECK(gamestate.health < 100 && gamestate.health >= 100 - 51);

    /* Gift throws rockets that leave smoke */
    empty_map();
    AT(plane1, 10, 20) = 19;
    AT(plane1, 20, 20) = 215;
    load_level(gd_hard);
    godmode = true;
    FirstSighting(find_class(giftobj));
    find_class(giftobj)->speed = 0;
    CHECK(run_until_class(rocketobj, 2000) > 0);
    CHECK(find_class(rocketobj)->angle == 180 && count_sound(MISSILEFIRESND) >= 1);
    CHECK(run_until_class(inertobj, 50) > 0); /* A_Smoke */

    /* fake Hitler breathes fire */
    empty_map();
    AT(plane1, 10, 20) = 19;
    AT(plane1, 20, 20) = 160;
    load_level(gd_hard);
    godmode = true;
    FirstSighting(find_class(fakeobj));
    CHECK(run_until_class(fireobj, 2000) > 0);
    CHECK(find_class(fireobj)->speed == 0x1200 && find_class(fireobj)->flags == FL_NEVERMARK);
    CHECK(count_sound(FLAMETHROWERSND) >= 1);
}

static void test_hitler_morph(void)
{
    objtype *mecha, *hitler;
    int tic;

    empty_map();
    AT(plane1, 10, 20) = 19;
    AT(plane1, 20, 20) = 178;
    load_level(gd_hard);
    godmode = true;
    mecha = find_class(mechahitlerobj);
    FirstSighting(mecha);
    for (tic = 0; tic < 200; tic++) {
        run_tic();
    }
    CHECK(count_sound(MECHSTEPSND) >= 1);

    KillActor(mecha);
    CHECK(mecha->state == &s_mechadie1 && count_sound(SCHEISTSND) == 0);

    /* die1, die2 and die3 are 10 tics each; A_HitlerMorph ends die3 */
    for (tic = 1; tic <= 30; tic++) {
        run_tic();
        if (tic == 29) {
            CHECK(find_class(realhitlerobj) == NULL);
        }
    }
    hitler = find_class(realhitlerobj);
    CHECK(hitler != NULL);
    CHECK(hitler && hitler->hitpoints == 900 && hitler->speed == SPDPATROL * 5);
    CHECK(hitler && (hitler->flags & FL_SHOOTABLE) && (hitler->flags & FL_ATTACKMODE));
    CHECK(hitler && hitler->state != NULL && hitler->dir == mecha->dir);
    CHECK(mecha->state == &s_mechadie4 && count_sound(SCHEISTSND) == 1);
}

static void test_death_cam(void)
{
    objtype *gift;
    int tic, cam_tic = 0, done_tic = 0;

    empty_map();
    AT(plane1, 10, 20) = 19;
    AT(plane1, 20, 20) = 215;
    load_level(gd_hard);
    orig_hooks.wait_vbl = count_pause;
    orig_hooks.fizzle = count_fizzle;
    orig_hooks.user_input = count_seeagain;
    g_pause = g_fizzle = g_seeagain = 0;
    gift = find_class(giftobj);
    CHECK(s_giftdie2.tictime == 140); /* digitized sound on */

    KillActor(gift);
    CHECK(gamestate.killx == player->x && gamestate.killy == player->y);
    CHECK(gamestate.score == 5000);

    for (tic = 1; tic <= 1000 && !done_tic; tic++) {
        run_tic();
        if (!cam_tic && gamestate.victoryflag) {
            cam_tic = tic;
        }
        if (playstate == ex_victorious) {
            done_tic = tic;
        }
    }

    /* die1 1 + die2 140 + die3..5 10 each + die6 20 */
    CHECK(cam_tic == 191);
    CHECK(g_fizzle == 1 && g_seeagain == 1 && g_pause == 2);
    CHECK(player->state == &s_deathcam && fizzlein);
    CHECK(player->angle == 0); /* the boss is due east of where it died */
    /* first try, 0x14000 back: costable[0] is 0xffff, so 0x13ffe rather than 1.25 tiles */
    CHECK(player->x == gift->x - 0x13ffe && player->y == gift->y);
    CHECK(player->tilex == 19 && plux == (uint16_t)(player->x >> UNSIGNEDSHIFT));
    /* the replay runs the death chain again and ends the level */
    CHECK(done_tic > cam_tic);
    CHECK(count_sound(DONNERSND) == 2);
}

static void test_bj_victory(void)
{
    objtype *bj;
    int tic, yeah_tic = 0;

    empty_map();
    AT(plane1, 20, 40) = 19;
    load_level(gd_hard);

    SpawnBJVictory();
    bj = find_class(bjobj);
    CHECK(bj && bj->tilex == 20 && bj->tiley == 41 && bj->temp1 == 6);
    CHECK(bj && bj->x == player->x && bj->y == player->y && bj->dir == north);

    /* VictorySpin (S3) backs the player off five tiles; stand in for it */
    player->y -= 10l << TILESHIFT;
    player->tiley -= 10;

    for (tic = 1; tic <= 2000 && playstate != ex_victorious; tic++) {
        run_tic();
        if (!yeah_tic && count_sound(YEAHSND)) {
            yeah_tic = tic;
        }
    }
    CHECK(playstate == ex_victorious);
    CHECK(yeah_tic > 0 && count_sound(YEAHSND) == 1);
    CHECK(bj->state == &s_bjjump4);
    CHECK(bj->tiley <= 35); /* ran six tiles north of its spawn tile */
}

/* -------------------------------------------------------------- real data */

/*
 * Put the player on a free floor tile 3-6 tiles from ob with a clear line
 * to it, so a boss gets to use its attack on a real map. False if none.
 */
static boolean place_player_near(objtype *ob)
{
    int dist, d;
    static const int step[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

    for (dist = 3; dist <= 6; dist++) {
        for (d = 0; d < 4; d++) {
            const int x = ob->tilex + step[d][0] * dist;
            const int y = ob->tiley + step[d][1] * dist;
            uint16_t tile;

            if (x < 1 || y < 1 || x > MAPSIZE - 2 || y > MAPSIZE - 2 || tilemap[x][y] ||
                actorat[x][y]) {
                continue;
            }
            tile = *(mapsegs[0] + farmapylookup[y] + x);
            if (tile < AREATILE || tile >= AREATILE + NUMAREAS) {
                continue;
            }
            player->x = ((fixed)x << TILESHIFT) + TILEGLOBAL / 2;
            player->y = ((fixed)y << TILESHIFT) + TILEGLOBAL / 2;
            player->tilex = (uint16_t)x;
            player->tiley = (uint16_t)y;
            player->areanumber = (byte)(tile - AREATILE);
            update_plu();
            ConnectAreas();
            if (CheckLine(ob)) {
                return true;
            }
        }
    }
    return false;
}

static boolean is_thrower(const objtype *ob)
{
    return ob->obclass == schabbobj || ob->obclass == giftobj || ob->obclass == fatobj ||
           ob->obclass == fakeobj;
}

static int run_pack(int argc, char **argv)
{
    wolf_data_dir_t dir;
    int mapnum, difficulty, runs = 0;
    long sightings = 0, shots = 0, thrown = 0, duel_throws = 0;
    int duels = 0;

    memset(&dir, 0, sizeof(dir));
    if (!Wolf_ResolveDataDir(argc, argv, &dir)) {
        return 1;
    }
    CA_Startup();
    if (!CA_OpenMapFiles(&dir)) {
        fprintf(stderr, "FAIL: could not open maps in %s\n", dir.path);
        return 1;
    }

    for (mapnum = 0; mapnum < NUMMAPS; mapnum++) {
        if (!mapheaderseg[mapnum]) {
            continue;
        }
        for (difficulty = gd_baby; difficulty <= gd_hard; difficulty++) {
            int tic;
            objtype *ob;

            NewGame(difficulty, mapnum / 10);
            gamestate.mapon = (int16_t)(mapnum % 10);
            demoplayback = true;
            CHECK(SetupGameLevel());
            demoplayback = false;
            update_plu();
            pwallstate = 0;
            playstate = ex_stillplaying;
            godmode = true;
            tics = 1;
            install_hooks();

            /*
             * Bosses wait in ambush in rooms the player can't see from the
             * start. Wake and activate them (the renderer activates visible
             * actors in the original) so their attacks run on real maps.
             */
            for (ob = player->next; ob; ob = ob->next) {
                switch (ob->obclass) {
                case bossobj:
                case gretelobj:
                case schabbobj:
                case giftobj:
                case fatobj:
                case fakeobj:
                case mechahitlerobj:
                    FirstSighting(ob);
                    ob->active = ac_yes;
                    break;
                default:
                    break;
                }
            }

            for (tic = 0; tic < 2100; tic++) {
                run_tic();
                for (ob = player->next; ob; ob = ob->next) {
                    if (ob->tilex >= MAPSIZE || ob->tiley >= MAPSIZE || ob->x < 0 ||
                        ob->y < 0 || ob->x >= (64l << TILESHIFT) ||
                        ob->y >= (64l << TILESHIFT)) {
                        fprintf(stderr, "FAIL: map %d diff %d tic %d: actor left the map\n",
                                mapnum, difficulty, tic);
                        g_failures++;
                        return 0;
                    }
                }
            }
            for (ob = player->next; ob; ob = ob->next) {
                sightings += (ob->flags & FL_ATTACKMODE) != 0;
            }
            shots += count_sound(NAZIFIRESND) + count_sound(SSFIRESND) +
                     count_sound(BOSSFIRESND);
            thrown += count_sound(MISSILEFIRESND) + count_sound(FLAMETHROWERSND) +
                      count_sound(SCHABBSTHROWSND);
            runs++;

            /* duel: the player next to each throwing boss on the map */
            NewGame(difficulty, mapnum / 10);
            gamestate.mapon = (int16_t)(mapnum % 10);
            demoplayback = true;
            CHECK(SetupGameLevel());
            demoplayback = false;
            for (ob = player->next; ob; ob = ob->next) {
                if (is_thrower(ob)) {
                    break;
                }
            }
            if (!ob || !place_player_near(ob)) {
                continue;
            }
            FirstSighting(ob);
            ob->active = ac_yes;
            pwallstate = 0;
            playstate = ex_stillplaying;
            godmode = true;
            install_hooks();
            for (tic = 0; tic < 700; tic++) {
                run_tic();
            }
            duel_throws += count_sound(MISSILEFIRESND) + count_sound(FLAMETHROWERSND) +
                           count_sound(SCHABBSTHROWSND);
            duels++;
        }
    }
    printf("%d map/difficulty runs of 2100 tics; %ld actors alerted, %ld shots fired, "
           "%ld boss projectiles\n",
           runs, sightings, shots, thrown);
    printf("%d boss duels of 700 tics; %ld boss projectiles\n", duels, duel_throws);
    CHECK(runs > 0);
    return 0;
}

int main(int argc, char **argv)
{
    InitGame();

    if (argc > 1) {
        if (run_pack(argc, argv)) {
            return 1;
        }
    } else {
        test_state_tables();
        test_checkline();
        test_guard_reaction();
        test_damage_and_death();
        test_door_cycle();
        test_guard_opens_door();
        test_pushwall();
        test_patrol_loop();
        test_golden_trace();
        test_boss_throwers();
        test_hitler_morph();
        test_death_cam();
        test_bj_victory();
    }

    mapsegs[0] = mapsegs[1] = NULL;
    if (g_failures) {
        fprintf(stderr, "%d check(s) failed\n", g_failures);
        return 1;
    }
    printf("test_orig_ai: all checks passed\n");
    return 0;
}
