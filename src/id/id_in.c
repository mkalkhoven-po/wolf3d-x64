#include "id_in.h"

#include <string.h>

#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif
#include <SDL.h>

boolean Keyboard[NumCodes];
boolean MousePresent;
boolean JoysPresent[2];

static longword g_start_ticks;

void IN_Startup(void)
{
    if (SDL_WasInit(SDL_INIT_TIMER) == 0) {
        if (SDL_InitSubSystem(SDL_INIT_TIMER) != 0) {
            /* Timer optional for stubs; keep zero baseline. */
            g_start_ticks = 0;
        }
    }
    memset(Keyboard, 0, sizeof(Keyboard));
    MousePresent = false;
    JoysPresent[0] = JoysPresent[1] = false;
    g_start_ticks = (longword)SDL_GetTicks();
}

void IN_Shutdown(void)
{
}

void IN_ClearKeysDown(void)
{
    memset(Keyboard, 0, sizeof(Keyboard));
}

void IN_WaitKey(void)
{
    /* Polled from platform loop later; stub returns immediately. */
}

boolean IN_KeyDown(int scancode)
{
    if (scancode < 0 || scancode >= NumCodes) {
        return false;
    }
    return Keyboard[scancode];
}

void IN_ReadControl(int player, void *info)
{
    (void)player;
    (void)info;
}

boolean IN_UserInput(longword delay)
{
    (void)delay;
    return false;
}

longword IN_GetTimeCount(void)
{
    /* Approximate original TickBase scaling later; raw ms/14-ish is fine for stub. */
    const longword now = (longword)SDL_GetTicks();
    return (now - g_start_ticks) / 14u;
}

void IN_Ack(void)
{
}
