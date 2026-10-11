/*
 * The game pad mapping (pad_map.c): dead zones, speeds against the
 * keyboard's, directions, the turn curve, the D-pad, weapon stepping, and
 * the pad in the control panel.
 */

#include <stdio.h>
#include <stdlib.h>

#include "pad_map.h"

static int failures;

#define CHECK(cond)                                                                    \
    do {                                                                               \
        if (!(cond)) {                                                                 \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);            \
            failures++;                                                                \
        }                                                                              \
    } while (0)

static pad_controls_t map(int lx, int ly, int rx, unsigned buttons, int rt)
{
    pad_state_t now = {0, 0, 0, 0, 0, 0, 0};
    pad_controls_t c;

    now.lx = lx;
    now.ly = ly;
    now.rx = rx;
    now.rt = rt;
    now.buttons = buttons;
    Pad_Map(&now, NULL, 2, &c);
    return c;
}

static void test_stick(void)
{
    int x, y;

    Pad_Stick(5000, -5000, PAD_LEFT_DEADZONE, &x, &y); /* 7071 < 7849: inside */
    CHECK(x == 0 && y == 0);
    Pad_Stick(32767, 0, PAD_LEFT_DEADZONE, &x, &y);
    CHECK(x == 1000 && y == 0);
    Pad_Stick(-32768, 0, PAD_LEFT_DEADZONE, &x, &y);
    CHECK(x == -1000 && y == 0);
    Pad_Stick(32767, 32767, PAD_LEFT_DEADZONE, &x, &y); /* a corner: still at most full */
    CHECK(x > 600 && x <= 1000 && x == y);
    Pad_Stick(7849 + (32767 - 7849) / 2, 0, PAD_LEFT_DEADZONE, &x, &y);
    CHECK(x == 500); /* half way between the dead zone and full tilt */
}

static void test_moves(void)
{
    pad_controls_t c;

    /* nothing pressed: nothing happens */
    c = map(0, 0, 0, 0, 0);
    CHECK(!c.controlx && !c.controly && !c.controlstrafe && !c.attack && !c.use && !c.run);

    /* full forward walks at the keyboard's walk speed: 35 per tic, forward negative */
    c = map(0, -32768, 0, 0, 0);
    CHECK(c.controly == -35 * 2 && c.controlstrafe == 0 && c.controlx == 0);
    /* the left stick click runs: 70 per tic */
    c = map(0, -32768, 0, PAD_LSTICK, 0);
    CHECK(c.controly == -70 * 2 && c.run);
    /* left stick right strafes right, with no turn */
    c = map(32767, 0, 0, 0, 0);
    CHECK(c.controlstrafe == 35 * 2 && c.controlx == 0 && c.controly == 0);
    /* right stick right turns right (controlx positive), up to the running turn rate */
    c = map(0, 0, 32767, 0, 0);
    CHECK(c.controlx == 70 * 2 && c.controlstrafe == 0);
    c = map(0, 0, -32768, 0, 0);
    CHECK(c.controlx == -70 * 2);
    /* both at once: the point of twin sticks */
    c = map(32767, -32768, 32767, 0, 0);
    CHECK(c.controlstrafe > 0 && c.controly < 0 && c.controlx > 0);
}

static void test_turn_curve(void)
{
    int v, last = -1;

    /* finer near the centre, rising all the way */
    for (v = PAD_RIGHT_DEADZONE + 500; v <= 32767; v += 1000) {
        const pad_controls_t c = map(0, 0, v, 0, 0);
        CHECK(c.controlx >= last);
        last = c.controlx;
    }
    {
        const pad_controls_t half = map(0, 0, PAD_RIGHT_DEADZONE + (32767 - PAD_RIGHT_DEADZONE) / 2, 0, 0);
        CHECK(half.controlx < 70 * 2 / 2); /* less than half rate at half tilt */
        CHECK(half.controlx > 70 * 2 / 4);
    }
}

static void test_buttons(void)
{
    pad_state_t before = {0, 0, 0, 0, 0, 0, 0}, now = before;
    pad_controls_t c;

    c = map(0, 0, 0, 0, 32767);
    CHECK(c.attack); /* right trigger */
    c = map(0, 0, 0, 0, 3000);
    CHECK(!c.attack); /* resting on it */
    c = map(0, 0, 0, PAD_A, 0);
    CHECK(c.attack);
    c = map(0, 0, 0, PAD_X, 0);
    CHECK(c.use && !c.attack);

    /* weapon steps only on the frame the bumper goes down */
    now.buttons = PAD_RB;
    Pad_Map(&now, &before, 1, &c);
    CHECK(c.weaponstep == 1);
    Pad_Map(&now, &now, 1, &c);
    CHECK(c.weaponstep == 0);
    now.buttons = PAD_LB;
    Pad_Map(&now, &before, 1, &c);
    CHECK(c.weaponstep == -1);

    /* the D-pad as the arrow keys */
    c = map(0, 0, 0, PAD_UP, 0);
    CHECK(c.controly == -35 * 2);
    c = map(0, 0, 0, PAD_LEFT, 0);
    CHECK(c.controlx == -35 * 2);
}

static void test_next_weapon(void)
{
    CHECK(Pad_NextWeapon(1, 3, 1) == 2);
    CHECK(Pad_NextWeapon(3, 3, 1) == 0); /* wraps to the knife */
    CHECK(Pad_NextWeapon(0, 3, -1) == 3);
    CHECK(Pad_NextWeapon(1, 1, 1) == 0); /* only knife and pistol */
    CHECK(Pad_NextWeapon(0, 1, -1) == 1);
    CHECK(Pad_NextWeapon(2, 3, 0) == 2);
}

/* the menus read the pad like the original's joystick: past SENSITIVE (60) moves */
static void test_menu(void)
{
    pad_state_t pad = {0, 0, 0, 0, 0, 0, 0};
    int dx, dy, b;

    Pad_Menu(&pad, &dx, &dy, &b);
    CHECK(dx == 0 && dy == 0 && b == 0);

    pad.ly = -32768; /* stick up */
    Pad_Menu(&pad, &dx, &dy, &b);
    CHECK(dy < -60 && dy >= -127 && dx == 0);

    pad.ly = 5000; /* inside the dead zone */
    Pad_Menu(&pad, &dx, &dy, &b);
    CHECK(dy == 0);

    pad.buttons = PAD_DOWN | PAD_LEFT | PAD_A | PAD_B;
    Pad_Menu(&pad, &dx, &dy, &b);
    CHECK(dx == -127 && dy == 127 && b == 3);

    pad.buttons = PAD_X | PAD_START; /* not menu buttons */
    Pad_Menu(&pad, &dx, &dy, &b);
    CHECK(b == 0);
}

/* the settings page's pad turning: the default as before, slower at 0, faster at 9 */
static void test_sensitivity(void)
{
    const int normal = map(0, 0, 32767, 0, 0).controlx;
    int slow, fast;

    Pad_SetSensitivity(0);
    slow = map(0, 0, 32767, 0, 0).controlx;
    Pad_SetSensitivity(9);
    fast = map(0, 0, 32767, 0, 0).controlx;
    Pad_SetSensitivity(PAD_SENS_DEFAULT);
    CHECK(map(0, 0, 32767, 0, 0).controlx == normal);
    CHECK(slow > 0 && slow < normal && fast > normal);
    CHECK(slow == normal * 40 / 100 && fast == normal * 148 / 100);
    Pad_SetSensitivity(99); /* clamped */
    CHECK(map(0, 0, 32767, 0, 0).controlx == fast);
    Pad_SetSensitivity(PAD_SENS_DEFAULT);
}

int main(void)
{
    test_stick();
    test_moves();
    test_turn_curve();
    test_buttons();
    test_next_weapon();
    test_menu();
    test_sensitivity();
    if (failures) {
        fprintf(stderr, "%d check(s) failed\n", failures);
        return 1;
    }
    printf("test_pad_map: all checks passed\n");
    return 0;
}
