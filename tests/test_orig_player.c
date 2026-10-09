/*
 * Player code of the faithful engine (P4/S3, WL_AGENT.C).
 *
 * Without arguments: movement, turning and strafing with exact fixed-point
 * results, wall and actor collision, Cmd_Use (doors, locks, elevators,
 * pushwalls), every bonus item, the weapon frame tables, the hit test and
 * the victory run, plus a golden trace of a scripted walk. With --data DIR:
 * every map is played for 2100 tics with random input and the player must
 * never end up inside a wall or a blocking static.
 */
#include "orig_testkit.h"
#include "wl_data.h"

static int g_waits, g_gatling, g_bonus_flash;

static void count_wait(void)
{
    g_waits++;
}

static void count_gatling(void)
{
    g_gatling++;
}

static void count_bonus_flash(void)
{
    g_bonus_flash++;
}

/* Player on tile x,y facing east (info tile 20). */
static void place_player_east(int x, int y)
{
    AT(plane1, x, y) = 20;
}

static void run_tics(int n)
{
    while (n-- > 0) {
        run_tic();
    }
}

/* ------------------------------------------------------------- movement */

static void test_move_forward(void)
{
    empty_map();
    place_player_east(10, 10);
    load_level(gd_hard);
    CHECK(player->angle == 0 && player->state == &s_player);

    controly = -100; /* full speed forward for one tic */
    run_tic();
    /* Thrust (0, 100*MOVESCALE): FixedByFrac (15000, 0xffff) = 14999 */
    CHECK(player->x == (10l << TILESHIFT) + TILEGLOBAL / 2 + 14999);
    CHECK(player->y == (10l << TILESHIFT) + TILEGLOBAL / 2);
    CHECK(thrustspeed == 15000 && playerxmove == 14999 && playerymove == 0);
    CHECK(plux == (uint16_t)(player->x >> UNSIGNEDSHIFT));

    /* backwards is slower: 100*BACKMOVESCALE */
    controly = 100;
    run_tic();
    CHECK(player->x == (10l << TILESHIFT) + TILEGLOBAL / 2 + 14999 - 9999);
}

static void test_turning(void)
{
    empty_map();
    place_player_east(10, 10);
    load_level(gd_hard);

    controlx = 100; /* 100 / ANGLESCALE = 5 degrees per tic, clockwise */
    run_tic();
    CHECK(player->angle == 355 && anglefrac == 0);

    /* the remainder carries over; C division truncates towards zero */
    controlx = -30;
    run_tic();
    CHECK(player->angle == 356 && anglefrac == -10);
    run_tic();
    CHECK(player->angle == 358 && anglefrac == 0);
}

static void test_strafe(void)
{
    empty_map();
    place_player_east(10, 10);
    load_level(gd_hard);

    buttonstate[bt_strafe] = true;
    controlx = 100; /* angle - 90 = 270: south */
    run_tic();
    CHECK(player->angle == 0);
    CHECK(player->x == (10l << TILESHIFT) + TILEGLOBAL / 2);
    CHECK(player->y == (10l << TILESHIFT) + TILEGLOBAL / 2 + 14999);
}

static void test_wall_collision(void)
{
    empty_map();
    split_areas(12);
    place_player_east(10, 10);
    load_level(gd_hard);

    controly = -100;
    run_tics(10);
    /*
     * Steps of 14999 from 10.5 tiles: 763123 still keeps the player's box
     * (PLAYERSIZE 0x5800) out of tile 12, the next step would not.
     */
    CHECK(player->x == 763123);
    CHECK(count_sound(HITWALLSND) >= 1);

    /* sliding: moving diagonally along the wall keeps the y part */
    controly = 0;
    controlx = 0;
    player->angle = 45;
    controly = -100;
    run_tic();
    CHECK(player->x == 763123 && player->y < (10l << TILESHIFT) + TILEGLOBAL / 2);
}

static void test_actor_collision(void)
{
    empty_map();
    place_player_east(10, 10);
    AT(plane1, 12, 10) = 108; /* standing guard two tiles east */
    load_level(gd_hard);

    controly = -100;
    run_tics(10);
    /* stops once within MINACTORDIST (one tile) of the guard's centre */
    CHECK(player->x == 748124);
}

/* ------------------------------------------------------------------ use */

static void test_use_doors(void)
{
    empty_map();
    split_areas(12);
    AT(plane0, 12, 10) = 90;     /* door */
    AT(plane0, 12, 12) = 90 + 2; /* gold-locked door */
    place_player_east(11, 10);
    load_level(gd_hard);

    buttonstate[bt_use] = true;
    run_tic();
    CHECK(doorobjlist[0].action == dr_opening && buttonheld[bt_use]);

    /* holding the button doesn't operate it again */
    run_tic();
    CHECK(count_sound(DONOTHINGSND) == 1);

    /* locked without the key */
    clear_input();
    player->y += 2l << TILESHIFT;
    player->tiley += 2;
    buttonstate[bt_use] = true;
    run_tic();
    CHECK(doorobjlist[1].action == dr_closed && count_sound(NOWAYSND) == 1);

    GiveKey(0);
    buttonheld[bt_use] = false;
    run_tic();
    CHECK(doorobjlist[1].action == dr_opening);
}

static void test_use_elevator(void)
{
    /* area 1 floor: a normal elevator */
    empty_map();
    AT(plane0, 12, 10) = ELEVATORTILE;
    AT(plane0, 11, 10) = AREATILE + 1;
    place_player_east(11, 10);
    load_level(gd_hard);
    orig_hooks.wait_sound_done = count_wait;
    g_waits = 0;

    buttonstate[bt_use] = true;
    run_tic();
    CHECK(playstate == ex_completed && tilemap[12][10] == ELEVATORTILE + 1);
    CHECK(count_sound(LEVELDONESND) == 1 && g_waits == 1);

    /*
     * Standing on ALTELEVATORTILE leads to the secret level. That value is
     * also AREATILE, the first area's floor tile, so any elevator used from
     * area 0 goes to the secret level.
     */
    empty_map();
    AT(plane0, 12, 10) = ELEVATORTILE;
    place_player_east(11, 10);
    load_level(gd_hard);
    buttonstate[bt_use] = true;
    run_tic();
    CHECK(playstate == ex_secretlevel);

    /* facing north or south at a switch does nothing */
    empty_map();
    AT(plane0, 11, 9) = ELEVATORTILE;
    AT(plane1, 11, 10) = 19; /* facing north */
    load_level(gd_hard);
    buttonstate[bt_use] = true;
    run_tic();
    CHECK(playstate == ex_stillplaying && count_sound(DONOTHINGSND) == 1);
}

static void test_use_pushwall(void)
{
    empty_map();
    AT(plane0, 12, 10) = 5;
    AT(plane1, 12, 10) = PUSHABLETILE;
    place_player_east(11, 10);
    load_level(gd_hard);

    buttonstate[bt_use] = true;
    run_tic();
    CHECK(pwallstate != 0 && pwalldir == di_east && gamestate.secretcount == 1);
    CHECK(tilemap[13][10] == 5);

    /*
     * Using the wall again while it slides: its tile reads as door 0x40 | 5.
     * The original operated a door past the end of doorobjlist; here nothing.
     */
    CHECK(tilemap[12][10] == (5 | 0xc0) && doornum == 0);
    buttonheld[bt_use] = false;
    run_tic();
    CHECK(buttonheld[bt_use] && doornum == 0 && gamestate.secretcount == 1);
}

/* ---------------------------------------------------------------- items */

static statobj_t *drop(int itemtype)
{
    PlaceItemType(itemtype, 20, 20);
    return laststatobj - 1;
}

static void test_items(void)
{
    statobj_t *s;

    empty_map();
    place_player_east(10, 10);
    load_level(gd_hard);
    orig_hooks.draw_got_gatling = count_gatling;
    orig_hooks.start_bonus_flash = count_bonus_flash;
    g_gatling = g_bonus_flash = 0;

    /* health items are left when not needed */
    s = drop(bo_firstaid);
    GetBonus(s);
    CHECK(s->shapenum != -1 && g_bonus_flash == 0);
    gamestate.health = 50;
    GetBonus(s);
    CHECK(gamestate.health == 75 && s->shapenum == -1 && g_bonus_flash == 1);
    GetBonus(drop(bo_food));
    GetBonus(drop(bo_alpo));
    CHECK(gamestate.health == 89);
    GetBonus(drop(bo_gibs)); /* only at 10 or less */
    CHECK(gamestate.health == 89);
    gamestate.health = 10;
    GetBonus(drop(bo_gibs));
    CHECK(gamestate.health == 11 && count_sound(SLURPIESND) == 1);

    /* treasure */
    GetBonus(drop(bo_cross));
    GetBonus(drop(bo_chalice));
    GetBonus(drop(bo_bible));
    GetBonus(drop(bo_crown));
    CHECK(gamestate.score == 6600 && gamestate.treasurecount == 4);

    /* keys */
    GetBonus(drop(bo_key2));
    CHECK(gamestate.keys == 2 && count_sound(GETKEYSND) == 1);

    /* ammo caps at 99 and full ammo leaves clips lying */
    GetBonus(drop(bo_clip));
    CHECK(gamestate.ammo == 16);
    GetBonus(drop(bo_clip2));
    CHECK(gamestate.ammo == 20);
    gamestate.ammo = 99;
    s = drop(bo_clip);
    GetBonus(s);
    CHECK(s->shapenum != -1);

    /* weapons */
    gamestate.ammo = 10;
    GetBonus(drop(bo_machinegun));
    CHECK(gamestate.bestweapon == wp_machinegun && gamestate.weapon == wp_machinegun);
    CHECK(gamestate.ammo == 16);
    GetBonus(drop(bo_chaingun));
    CHECK(gamestate.bestweapon == wp_chaingun && gotgatgun == 1 && g_gatling == 1);
    CHECK(facecount == 0);
    GetBonus(drop(bo_machinegun)); /* a worse gun only gives ammo */
    CHECK(gamestate.weapon == wp_chaingun && gamestate.ammo == 28);

    /* one up */
    gamestate.health = 50;
    GetBonus(drop(bo_fullheal));
    CHECK(gamestate.health == 100 && gamestate.lives == 4 && gamestate.ammo == 53);
    CHECK(gamestate.treasurecount == 5);
}

/* --------------------------------------------------------------- weapons */

static void test_pistol_frames(void)
{
    int tic;

    empty_map();
    place_player_east(10, 10);
    load_level(gd_hard);

    buttonstate[bt_attack] = true; /* held the whole time */
    run_tic();
    CHECK(player->state == &s_attack && gamestate.attackcount == 6);
    CHECK(gamestate.weaponframe == 1);

    /* frames of 6 tics: ready, fire, recover, done */
    for (tic = 2; tic <= 25; tic++) {
        run_tic();
        if (tic == 12) {
            CHECK(gamestate.ammo == 8);
        }
        if (tic == 13) {
            CHECK(gamestate.ammo == 7 && count_sound(ATKPISTOLSND) == 1);
        }
        if (tic == 24) {
            CHECK(player->state == &s_attack);
        }
    }
    CHECK(player->state == &s_player && gamestate.weaponframe == 0);

    /* semi-automatic: the button has to be released before the next shot */
    run_tics(30);
    CHECK(gamestate.ammo == 7);
}

static void test_auto_weapons(void)
{
    empty_map();
    place_player_east(10, 10);
    load_level(gd_hard);
    gamestate.bestweapon = gamestate.weapon = gamestate.chosenweapon = wp_machinegun;

    buttonstate[bt_attack] = true;
    run_tics(1 + 12 * 6); /* shots on tic 13, then every 12 while held */
    CHECK(gamestate.ammo == 8 - 6 && player->state == &s_attack);
    CHECK(count_sound(ATKMACHINEGUNSND) == 6);

    /* chain gun: two shots per 12 tics */
    load_level(gd_hard);
    gamestate.bestweapon = gamestate.weapon = gamestate.chosenweapon = wp_chaingun;
    buttonstate[bt_attack] = true;
    run_tics(1 + 6 + 12 * 2); /* frame 3 loops straight back into a shot */
    CHECK(count_sound(ATKGATLINGSND) >= 4);

    /* out of ammo: the knife comes out after the attack */
    load_level(gd_hard);
    gamestate.ammo = 1;
    buttonstate[bt_attack] = true;
    run_tics(30);
    CHECK(gamestate.ammo == 0 && gamestate.weapon == wp_knife);
    buttonheld[bt_attack] = false;
    run_tics(13);
    CHECK(count_sound(ATKKNIFESND) == 1);

    /* number keys switch to owned weapons only */
    load_level(gd_hard);
    buttonstate[bt_readymachinegun] = true;
    run_tic();
    CHECK(gamestate.weapon == wp_pistol);
    gamestate.bestweapon = wp_machinegun;
    run_tic();
    CHECK(gamestate.weapon == wp_machinegun && gamestate.chosenweapon == wp_machinegun);
}

/* -------------------------------------------------------------- hit test */

static objtype *visible_guard(int x, int y, fixed transx)
{
    objtype *ob;

    for (ob = player->next; ob; ob = ob->next) {
        if (ob->tilex == x && ob->tiley == y) {
            ob->flags |= FL_VISABLE; /* the renderer sets these (S4) */
            ob->viewx = centerx;
            ob->transx = transx;
            return ob;
        }
    }
    return NULL;
}

static void test_gun_attack(void)
{
    objtype *closer, *farther;

    empty_map();
    place_player_east(10, 10);
    AT(plane1, 12, 10) = 108;
    load_level(gd_hard);
    centerx = 151; /* full-width view: viewwidth 304 */
    shootdelta = 30;

    closer = visible_guard(12, 10, 2 * TILEGLOBAL);
    GunAttack(player);
    /* distance 2: rndtable[1] / 6 = 1, doubled while not in attack mode */
    CHECK(closer->hitpoints == 23 && (closer->flags & FL_ATTACKMODE));
    CHECK(closer->state == &s_grdpain && madenoise);

    /* outside the aim band */
    closer->viewx = centerx + shootdelta;
    GunAttack(player);
    CHECK(closer->hitpoints == 23);

    /*
     * A nearer target behind a wall hides a farther clear one: the search
     * keeps the nearest distance seen, so the second pass finds nothing.
     */
    empty_map();
    place_player_east(10, 10);
    AT(plane1, 12, 10) = 108;
    AT(plane1, 14, 11) = 108;
    AT(plane0, 11, 10) = 1;
    load_level(gd_hard);
    closer = visible_guard(12, 10, 2 * TILEGLOBAL);
    farther = visible_guard(14, 11, 4 * TILEGLOBAL);
    GunAttack(player);
    CHECK(closer->hitpoints == 25 && farther->hitpoints == 25);

    /* knife: only up to 0x18000 away */
    empty_map();
    place_player_east(10, 10);
    AT(plane1, 11, 10) = 108;
    load_level(gd_hard);
    closer = visible_guard(11, 10, 0x18001);
    KnifeAttack(player);
    CHECK(closer->hitpoints == 25);
    closer->transx = 0x18000;
    US_RndT(); /* rndtable[1] >> 4 would be 0 */
    KnifeAttack(player);
    CHECK(closer->hitpoints == 25 - 2 * (rndtable[2] >> 4));
}

/* --------------------------------------------------------------- victory */

static void test_victory(void)
{
    int tic;

    empty_map();
    place_player_east(11, 10);
    AT(plane1, 12, 10) = EXITTILE;
    load_level(gd_hard);

    controly = -100;
    for (tic = 0; tic < 10 && !gamestate.victoryflag; tic++) {
        run_tic();
    }
    CHECK(gamestate.victoryflag && player->tilex == 12);
    CHECK(first_actor() && first_actor()->obclass == bjobj);

    /* VictorySpin: turn to 270 at 3 per tic, back off to five tiles north */
    clear_input();
    run_tics(100);
    CHECK(player->angle == 270);
    CHECK(player->y == ((10l - 5) << TILESHIFT) - 0x3000);

    for (tic = 0; tic < 2000 && playstate != ex_victorious; tic++) {
        run_tic();
    }
    CHECK(playstate == ex_victorious && count_sound(YEAHSND) == 1);
}

/* ---------------------------------------------------------- golden trace */

static uint32_t run_walk(void)
{
    uint32_t h = 2166136261u;
    int tic;
    objtype *ob;

    empty_map();
    split_areas(30);
    AT(plane0, 30, 20) = 90;
    place_player_east(10, 20);
    AT(plane1, 40, 20) = 108 + 2; /* guard behind the door, facing west */
    AT(plane1, 20, 25) = 23 + 29; /* cross (pickup needs the renderer) */
    load_level(gd_hard);
    godmode = true;

    for (tic = 0; tic < 1200; tic++) {
        clear_input();
        buttonheld[bt_attack] = tic > 400 && tic < 460;
        if (tic < 250) {
            controly = -60;
        } else if (tic < 260) {
            buttonstate[bt_use] = true;
        } else if (tic < 300) {
            controlx = 40;
        } else if (tic < 400) {
            controly = -100;
            buttonstate[bt_run] = true;
        } else if (tic < 460) {
            buttonstate[bt_attack] = true;
        } else {
            buttonstate[bt_strafe] = true;
            controlx = (tic / 50) % 2 ? 80 : -80;
            controly = -30;
        }
        run_tic();
        for (ob = player; ob; ob = ob->next) {
            const int32_t v[7] = {ob->x, ob->y, ob->angle, state_index(ob->state),
                                  ob->ticcount, ob->hitpoints, ob->flags};
            h = fnv(h, v, sizeof(v));
        }
        h = fnv(h, &gamestate, sizeof(gamestate));
        h = fnv(h, doorposition, sizeof(doorposition));
    }
    return h;
}

static void test_golden_walk(void)
{
    const uint32_t a = run_walk();
    const uint32_t b = run_walk();

    printf("golden walk: %08x\n", a);
    CHECK(a == b);
    CHECK(a == 0xd9d726c3u); /* regression pin: update only for an intended change */
}

/* --------------------------------------------------------------- fuzzing */

static uint32_t g_lcg = 12345u;

static int rnd(int n)
{
    g_lcg = g_lcg * 1103515245u + 12345u;
    return (int)((g_lcg >> 16) % (uint32_t)n);
}

static int run_pack(int argc, char **argv)
{
    wolf_data_dir_t dir;
    int mapnum, runs = 0, ended = 0;
    long tiles_walked = 0, doors_used = 0;

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
        int tic, d, lastx, lasty;

        if (!mapheaderseg[mapnum]) {
            continue;
        }
        NewGame(gd_hard, mapnum / 10);
        gamestate.mapon = (int16_t)(mapnum % 10);
        demoplayback = true;
        CHECK(SetupGameLevel());
        demoplayback = false;
        clear_input();
        update_plu();
        pwallstate = 0;
        playstate = ex_stillplaying;
        godmode = true;
        tics = 1;
        install_hooks();
        lastx = player->tilex;
        lasty = player->tiley;

        for (tic = 0; tic < 2100 && playstate == ex_stillplaying; tic++) {
            int t;

            if (tic % 35 == 0) {
                controlx = (int16_t)(rnd(201) - 100);
                controly = (int16_t)(rnd(201) - 120); /* mostly forward */
                buttonstate[bt_strafe] = rnd(4) == 0;
                buttonstate[bt_attack] = rnd(6) == 0;
            }
            buttonstate[bt_use] = rnd(10) == 0;
            if (!buttonstate[bt_use]) {
                buttonheld[bt_use] = false;
            }
            if (!buttonstate[bt_attack]) {
                buttonheld[bt_attack] = false;
            }
            run_tic();

            t = tilemap[player->tilex][player->tiley];
            if (player->tilex >= MAPSIZE || player->tiley >= MAPSIZE ||
                (t && !(t & 0x80)) || ACTORAT_VALUE(actorat[player->tilex][player->tiley]) == 1) {
                fprintf(stderr, "FAIL: map %d tic %d: player inside a wall at %u,%u\n", mapnum,
                        tic, player->tilex, player->tiley);
                g_failures++;
                return 0;
            }
            if (player->tilex != lastx || player->tiley != lasty) {
                tiles_walked++;
                lastx = player->tilex;
                lasty = player->tiley;
            }
        }
        for (d = 0; d < doornum; d++) {
            doors_used += doorobjlist[d].action != dr_closed;
        }
        ended += playstate != ex_stillplaying;
        runs++;
    }
    printf("%d maps played for up to 2100 tics with random input: %ld tile changes, "
           "%ld doors left open, %d levels ended early\n",
           runs, tiles_walked, doors_used, ended);
    CHECK(runs > 0 && tiles_walked > 0);
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
        test_move_forward();
        test_turning();
        test_strafe();
        test_wall_collision();
        test_actor_collision();
        test_use_doors();
        test_use_elevator();
        test_use_pushwall();
        test_items();
        test_pistol_frames();
        test_auto_weapons();
        test_gun_attack();
        test_victory();
        test_golden_walk();
    }

    mapsegs[0] = mapsegs[1] = NULL;
    if (g_failures) {
        fprintf(stderr, "%d check(s) failed\n", g_failures);
        return 1;
    }
    printf("test_orig_player: all checks passed\n");
    return 0;
}
