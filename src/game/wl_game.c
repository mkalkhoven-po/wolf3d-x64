#include "wl_game.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif
#include <SDL.h>

#include "id/id_vl.h"
#include "id/id_vh.h"
#include "wl_draw.h"
#include "wl_play.h"

/* UTF-8 present framebuffer using existing palette. */
static void present(SDL_Renderer *renderer, SDL_Texture *texture, Uint32 *pixels)
{
    int i;
    if (!framebuffer || !pixels) {
        return;
    }
    for (i = 0; i < SCREENPIXELS; ++i) {
        byte idx = framebuffer[i];
        byte r = VL_VGA6to8(g_palette[idx * 3 + 0]);
        byte g = VL_VGA6to8(g_palette[idx * 3 + 1]);
        byte b = VL_VGA6to8(g_palette[idx * 3 + 2]);
        pixels[i] = (Uint32)(0xFF000000u | ((Uint32)r << 16) | ((Uint32)g << 8) |
                             (Uint32)b);
    }
    SDL_UpdateTexture(texture, NULL, pixels, SCREENWIDTH * 4);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

int Wolf_RunGame(void)
{
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    SDL_Texture *texture = NULL;
    Uint32 *pixels = NULL;
    int running = 1;
    int scale = 3;
    Uint32 last_ticks;
    boolean keys[8];

    if (SDL_WasInit(SDL_INIT_VIDEO) == 0) {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
            fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
            return 1;
        }
    }

    if (!Wolf_SetupLevel(0)) {
        fprintf(stderr, "Failed to setup map 0\n");
        return 1;
    }

    window = SDL_CreateWindow(
        "Wolf3D Win64 — E5",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        SCREENWIDTH * scale,
        SCREENHEIGHT * scale,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return 1;
    }

    renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        renderer = SDL_CreateRenderer(window, -1, 0);
    }
    if (!renderer) {
        SDL_DestroyWindow(window);
        return 1;
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
    SDL_RenderSetLogicalSize(renderer, SCREENWIDTH, SCREENHEIGHT);

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                SDL_TEXTUREACCESS_STREAMING, SCREENWIDTH,
                                SCREENHEIGHT);
    if (!texture) {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        return 1;
    }

    pixels = (Uint32 *)malloc(SCREENPIXELS * sizeof(Uint32));
    if (!pixels) {
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        return 1;
    }

    memset(keys, 0, sizeof(keys));
    last_ticks = SDL_GetTicks();
    printf("Game loop: WASD/arrows move, SPACE/E open door, ESC quit to OS\n");

    while (running) {
        SDL_Event ev;
        Uint32 now;
        float dt;
        const Uint8 *state;

        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) {
                running = 0;
            } else if (ev.type == SDL_KEYDOWN) {
                if (ev.key.keysym.sym == SDLK_ESCAPE) {
                    running = 0;
                } else if (ev.key.keysym.sym == SDLK_SPACE ||
                           ev.key.keysym.sym == SDLK_e) {
                    Wolf_TryUse();
                }
            }
        }

        state = SDL_GetKeyboardState(NULL);
        keys[0] = state[SDL_SCANCODE_W] || state[SDL_SCANCODE_UP];
        keys[1] = state[SDL_SCANCODE_S] || state[SDL_SCANCODE_DOWN];
        keys[2] = state[SDL_SCANCODE_LEFT];
        keys[3] = state[SDL_SCANCODE_RIGHT];
        keys[4] = state[SDL_SCANCODE_A];
        keys[5] = state[SDL_SCANCODE_D];

        now = SDL_GetTicks();
        dt = (float)(now - last_ticks) / 1000.0f;
        last_ticks = now;
        if (dt > 0.05f) {
            dt = 0.05f;
        }

        Wolf_UpdateLevel(dt, keys);
        Wolf_DrawWorld();
        /* HUD text */
        VH_DrawString(4, SCREENHEIGHT - 16, "WASD MOVE  ARROWS TURN  SPACE DOOR  ESC",
                      15);
        present(renderer, texture, pixels);
    }

    free(pixels);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    return 0;
}
