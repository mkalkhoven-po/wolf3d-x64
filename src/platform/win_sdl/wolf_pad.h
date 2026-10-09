#pragma once

/*
 * The game pad through SDL's GameController API (XInput pads on Windows,
 * PlayStation and others through SDL's mapping database). The first pad
 * connected is used; pads can be plugged in and out at any time.
 */

#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif
#include <SDL.h>

#include "pad_map.h"
#include "wl_types.h"

/* Starts SDL's game controller support and opens a pad if one is there. */
void Wolf_PadInit(void);
void Wolf_PadShutdown(void);

/* Hot plugging: call with every SDL event (the others are ignored). */
void Wolf_PadEvent(const SDL_Event *ev);

/* The pad's state now; false (and a neutral state) when none is connected. */
boolean Wolf_PadRead(pad_state_t *state);

/*
 * The menu on the pad: up/down from the D-pad or left stick, A to choose,
 * B to go back, Start to choose. 0 when the event means nothing there.
 */
SDL_Keycode Wolf_PadMenuKey(const SDL_Event *ev);
