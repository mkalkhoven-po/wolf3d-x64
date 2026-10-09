#include "wolf_pad.h"

#include <stdio.h>
#include <string.h>

static SDL_GameController *g_pad;
static int g_stickdir; /* the menu's left stick: -1 up, 1 down, 0 centred */

static void OpenFirstPad(void)
{
    int i;

    for (i = 0; i < SDL_NumJoysticks() && !g_pad; i++) {
        if (SDL_IsGameController(i)) {
            g_pad = SDL_GameControllerOpen(i);
            if (g_pad) {
                printf("Game pad: %s\n", SDL_GameControllerName(g_pad));
            }
        }
    }
}

void Wolf_PadInit(void)
{
    if (SDL_WasInit(SDL_INIT_GAMECONTROLLER) == 0 &&
        SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) != 0) {
        fprintf(stderr, "Game pad support unavailable: %s\n", SDL_GetError());
        return;
    }
    OpenFirstPad();
}

void Wolf_PadShutdown(void)
{
    if (g_pad) {
        SDL_GameControllerClose(g_pad);
        g_pad = NULL;
    }
    if (SDL_WasInit(SDL_INIT_GAMECONTROLLER)) {
        SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
    }
}

void Wolf_PadEvent(const SDL_Event *ev)
{
    if (ev->type == SDL_CONTROLLERDEVICEADDED && !g_pad) {
        OpenFirstPad();
    } else if (ev->type == SDL_CONTROLLERDEVICEREMOVED && g_pad &&
               ev->cdevice.which ==
                   SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(g_pad))) {
        printf("Game pad removed\n");
        SDL_GameControllerClose(g_pad);
        g_pad = NULL;
        OpenFirstPad(); /* another one, if there is */
    }
}

boolean Wolf_PadRead(pad_state_t *state)
{
    static const struct {
        SDL_GameControllerButton button;
        unsigned bit;
    } buttons[] = {
        {SDL_CONTROLLER_BUTTON_A, PAD_A},
        {SDL_CONTROLLER_BUTTON_B, PAD_B},
        {SDL_CONTROLLER_BUTTON_X, PAD_X},
        {SDL_CONTROLLER_BUTTON_Y, PAD_Y},
        {SDL_CONTROLLER_BUTTON_LEFTSHOULDER, PAD_LB},
        {SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, PAD_RB},
        {SDL_CONTROLLER_BUTTON_BACK, PAD_BACK},
        {SDL_CONTROLLER_BUTTON_START, PAD_START},
        {SDL_CONTROLLER_BUTTON_LEFTSTICK, PAD_LSTICK},
        {SDL_CONTROLLER_BUTTON_RIGHTSTICK, PAD_RSTICK},
        {SDL_CONTROLLER_BUTTON_DPAD_UP, PAD_UP},
        {SDL_CONTROLLER_BUTTON_DPAD_DOWN, PAD_DOWN},
        {SDL_CONTROLLER_BUTTON_DPAD_LEFT, PAD_LEFT},
        {SDL_CONTROLLER_BUTTON_DPAD_RIGHT, PAD_RIGHT},
    };
    size_t i;

    memset(state, 0, sizeof(*state));
    if (!g_pad) {
        return false;
    }
    state->lx = SDL_GameControllerGetAxis(g_pad, SDL_CONTROLLER_AXIS_LEFTX);
    state->ly = SDL_GameControllerGetAxis(g_pad, SDL_CONTROLLER_AXIS_LEFTY);
    state->rx = SDL_GameControllerGetAxis(g_pad, SDL_CONTROLLER_AXIS_RIGHTX);
    state->ry = SDL_GameControllerGetAxis(g_pad, SDL_CONTROLLER_AXIS_RIGHTY);
    state->lt = SDL_GameControllerGetAxis(g_pad, SDL_CONTROLLER_AXIS_TRIGGERLEFT);
    state->rt = SDL_GameControllerGetAxis(g_pad, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
    for (i = 0; i < sizeof(buttons) / sizeof(buttons[0]); i++) {
        if (SDL_GameControllerGetButton(g_pad, buttons[i].button)) {
            state->buttons |= buttons[i].bit;
        }
    }
    return true;
}

SDL_Keycode Wolf_PadMenuKey(const SDL_Event *ev)
{
    if (ev->type == SDL_CONTROLLERBUTTONDOWN) {
        switch (ev->cbutton.button) {
        case SDL_CONTROLLER_BUTTON_DPAD_UP:
            return SDLK_UP;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
            return SDLK_DOWN;
        case SDL_CONTROLLER_BUTTON_A:
        case SDL_CONTROLLER_BUTTON_START:
            return SDLK_RETURN;
        case SDL_CONTROLLER_BUTTON_B:
            return SDLK_ESCAPE;
        default:
            return 0;
        }
    }
    if (ev->type == SDL_CONTROLLERAXISMOTION && ev->caxis.axis == SDL_CONTROLLER_AXIS_LEFTY) {
        /* one step each time the stick is pushed past half way */
        const int v = ev->caxis.value;
        const int dir = v < -16000 ? -1 : v > 16000 ? 1 : 0;

        if (dir && dir != g_stickdir) {
            g_stickdir = dir;
            return dir < 0 ? SDLK_UP : SDLK_DOWN;
        }
        if (v > -8000 && v < 8000) {
            g_stickdir = 0;
        }
    }
    return 0;
}
