#pragma once

/*
 * A game pad (XInput layout) mapped to the engine's controls, twin-stick
 * style: the left stick walks and strafes, the right stick turns, both
 * analog and at once (the strafe goes through controlstrafe, which is not in
 * the original; see orig.h). No SDL here, so it can be tested on its own;
 * wolf_pad.c reads the pad, orig_runner.c applies the result.
 *
 *   Left stick   walk / strafe        Right stick  turn
 *   RT or A      fire                 X            open / use
 *   Left click   run (hold)           LB / RB      previous / next weapon
 *   D-pad        as the arrow keys    Start        Esc (menu)   Back  pause
 */

#include <stdint.h>

enum {
    PAD_A = 1 << 0,
    PAD_B = 1 << 1,
    PAD_X = 1 << 2,
    PAD_Y = 1 << 3,
    PAD_LB = 1 << 4,
    PAD_RB = 1 << 5,
    PAD_BACK = 1 << 6,
    PAD_START = 1 << 7,
    PAD_LSTICK = 1 << 8,
    PAD_RSTICK = 1 << 9,
    PAD_UP = 1 << 10,
    PAD_DOWN = 1 << 11,
    PAD_LEFT = 1 << 12,
    PAD_RIGHT = 1 << 13
};

typedef struct {
    int lx, ly, rx, ry; /* sticks, -32768..32767; up is negative */
    int lt, rt;         /* triggers, 0..32767 */
    unsigned buttons;   /* PAD_* */
} pad_state_t;

typedef struct {
    int controlx;     /* turn: add to the engine's controlx */
    int controly;     /* walk: add to controly (forward is negative) */
    int controlstrafe; /* strafe: controlstrafe (right is positive) */
    int attack, use, run;
    int weaponstep;   /* -1 or +1 on the frame LB or RB goes down, else 0 */
} pad_controls_t;

/* XInput's recommended dead zones, and the trigger's press point */
#define PAD_LEFT_DEADZONE 7849
#define PAD_RIGHT_DEADZONE 8689
#define PAD_TRIGGER_PRESS 8000

/* A stick (x, y) past a round dead zone, scaled to -1000..1000 per axis. */
void Pad_Stick(int x, int y, int deadzone, int *outx, int *outy);

/*
 * The controls for one frame of tics: walking and strafing at the keyboard's
 * walk speed at full tilt (run speed with run held), turning up to the
 * keyboard's run turn rate on a curve that is finer near the centre.
 * before is the previous frame's state, for the weapon buttons' edges.
 */
void Pad_Map(const pad_state_t *now, const pad_state_t *before, int tics, pad_controls_t *out);

/* The settings page's pad sensitivity (not in the original): the right stick's turn speed. */
#define PAD_SENS_DEFAULT 5
#define PAD_SENS_MAX 9
void Pad_SetSensitivity(int level);

/* The next weapon to ready for a weapon step (0 knife .. 3 chain gun). */
int Pad_NextWeapon(int weapon, int bestweapon, int step);

/*
 * The pad in the control panel, read like the original's joystick: the left
 * stick or the D-pad as dx, dy (-127..127; up is negative), A as button 0
 * (select) and B as button 1 (back).
 */
void Pad_Menu(const pad_state_t *now, int *dx, int *dy, int *buttons);
