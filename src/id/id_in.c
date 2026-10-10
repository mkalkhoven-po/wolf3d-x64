#include "id_in.h"

#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif
#include <SDL.h>

static longword g_start_ticks;

void IN_Startup(void)
{
    if (SDL_WasInit(SDL_INIT_TIMER) == 0) {
        (void)SDL_InitSubSystem(SDL_INIT_TIMER);
    }
    g_start_ticks = (longword)SDL_GetTicks();
}

void IN_Shutdown(void)
{
}

longword IN_GetTimeCount(void)
{
    /* about 70 Hz from milliseconds */
    const longword now = (longword)SDL_GetTicks();
    return (now - g_start_ticks) / 14u;
}
