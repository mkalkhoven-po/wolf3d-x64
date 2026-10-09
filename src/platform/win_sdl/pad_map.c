#include "pad_map.h"

#include <math.h>

/* the keyboard's speeds per tic (WL_PLAY.C BASEMOVE, RUNMOVE) */
#define BASEMOVE 35
#define RUNMOVE 70
#define PADTURN RUNMOVE /* full right stick turns as fast as running with the arrows */

void Pad_Stick(int x, int y, int deadzone, int *outx, int *outy)
{
    const double mag = sqrt((double)x * x + (double)y * y);
    double scale;

    if (mag <= deadzone) {
        *outx = *outy = 0;
        return;
    }
    /* rescale so the edge of the dead zone is 0 and full tilt is 1000 */
    scale = (mag > 32767.0 ? 32767.0 - deadzone : mag - deadzone) / (32767.0 - deadzone) / mag;
    *outx = (int)lround(x * scale * 1000.0);
    *outy = (int)lround(y * scale * 1000.0);
    *outx = *outx > 1000 ? 1000 : *outx < -1000 ? -1000 : *outx;
    *outy = *outy > 1000 ? 1000 : *outy < -1000 ? -1000 : *outy;
}

/* finer near the centre: 40% linear, 60% squared */
static int TurnCurve(int v)
{
    const double t = v / 1000.0;
    return (int)lround((0.4 * t + 0.6 * t * fabs(t)) * 1000.0);
}

void Pad_Map(const pad_state_t *now, const pad_state_t *before, int tics, pad_controls_t *out)
{
    const unsigned pressed = now->buttons & ~(before ? before->buttons : 0);
    int lx, ly, rx, ry, speed;

    Pad_Stick(now->lx, now->ly, PAD_LEFT_DEADZONE, &lx, &ly);
    Pad_Stick(now->rx, now->ry, PAD_RIGHT_DEADZONE, &rx, &ry);
    (void)ry;

    out->run = (now->buttons & PAD_LSTICK) != 0;
    out->attack = now->rt > PAD_TRIGGER_PRESS || (now->buttons & PAD_A);
    out->use = (now->buttons & PAD_X) != 0;
    out->weaponstep = (pressed & PAD_RB) ? 1 : (pressed & PAD_LB) ? -1 : 0;

    speed = out->run ? RUNMOVE : BASEMOVE;
    out->controly = ly * speed * tics / 1000;
    out->controlstrafe = lx * speed * tics / 1000;
    out->controlx = TurnCurve(rx) * PADTURN * tics / 1000;

    /* the D-pad works as the arrow keys */
    if (now->buttons & PAD_UP) {
        out->controly -= speed * tics;
    }
    if (now->buttons & PAD_DOWN) {
        out->controly += speed * tics;
    }
    if (now->buttons & PAD_LEFT) {
        out->controlx -= speed * tics;
    }
    if (now->buttons & PAD_RIGHT) {
        out->controlx += speed * tics;
    }
}

int Pad_NextWeapon(int weapon, int bestweapon, int step)
{
    int next = weapon;

    if (!step || bestweapon < 1) {
        return weapon;
    }
    next += step;
    if (next > bestweapon) {
        next = 0;
    } else if (next < 0) {
        next = bestweapon;
    }
    return next;
}
