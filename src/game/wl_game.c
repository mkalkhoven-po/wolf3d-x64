#include "wl_game.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif
#include <SDL.h>

#include "id/id_vh.h"
#include "id/id_vl.h"
#include "wl_draw.h"
#include "wl_play.h"
#include "id/id_sd.h"
#include "id/audio_wl6.h"

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

static void draw_hud(void)
{
    char line[128];
    player_t *p = &g_level.player;
    const char *wname = "KNIFE";
    if (p->weapon == 1) {
        wname = "PISTOL";
    } else if (p->weapon == 2) {
        wname = "MGUN";
    } else if (p->weapon == 3) {
        wname = "CHAIN";
    }
    snprintf(line, sizeof(line), "HP %d  AMMO %d  LIVES %d  SCORE %d  %s  MAP %d",
             p->health, p->ammo, p->lives, p->score, wname, g_level.mapnum);
    VH_DrawString(2, SCREENHEIGHT - 18, line, 15);
    VH_DrawString(2, SCREENHEIGHT - 9, "W/A/S/D MOVE  ARROWS TURN  CTRL SHOOT  E DOOR  ESC",
                  7);
    if (p->dead) {
        if (p->lives >= 0) {
            VH_DrawString(90, 80, "YOU DIED - PRESS R", 12);
        } else {
            VH_DrawString(100, 80, "GAME OVER", 12);
        }
    }
    if (g_level.level_complete) {
        VH_DrawString(70, 70, "LEVEL COMPLETE!", 14);
    }
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
    boolean keys[12];
    int start_map = 0;

    if (SDL_WasInit(SDL_INIT_VIDEO) == 0) {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
            fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
            return 1;
        }
    }

    Wolf_ResetPlayerStats();
    if (!Wolf_SetupLevel(start_map)) {
        fprintf(stderr, "Failed to setup map %d\n", start_map);
        return 1;
    }

    window = SDL_CreateWindow(
        "Wolf3D Win64 — E6 Combat",
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
    SD_StartMusic(GETTHEM_MUS);
    printf("E6 combat: WASD/arrows, CTRL/LMB shoot, E door/elevator, R respawn, ESC exit\n");

    while (running) {
        SDL_Event ev;
        Uint32 now;
        float dt;
        const Uint8 *state;

        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) {
                running = 0;
            } else if (ev.type == SDL_KEYDOWN) {
                SDL_Keycode k = ev.key.keysym.sym;
                if (k == SDLK_ESCAPE) {
                    running = 0;
                } else if (k == SDLK_e || k == SDLK_SPACE) {
                    Wolf_TryUse();
                } else if (k == SDLK_r && g_level.player.dead) {
                    if (g_level.player.lives >= 0) {
                        int map = g_level.mapnum;
                        int lives = g_level.player.lives;
                        int score = g_level.player.score;
                        Wolf_SetupLevel(map);
                        g_level.player.lives = lives;
                        g_level.player.score = score;
                        g_level.player.health = 100;
                        g_level.player.ammo = 8;
                        g_level.player.dead = false;
                        printf("Respawned on map %d lives=%d\n", map, lives);
                    } else {
                        printf("No lives left.\n");
                    }
                } else if (k == SDLK_n && g_level.level_complete) {
                    int next = g_level.mapnum + 1;
                    if (next >= SHAREWARE_MAPS) {
                        printf("Shareware episode complete! Score=%d\n",
                               g_level.player.score);
                        VH_DrawString(40, 100, "EPISODE COMPLETE - ESC", 14);
                    } else {
                        if (!Wolf_SetupLevel(next)) {
                            fprintf(stderr, "Failed next map %d\n", next);
                            running = 0;
                        } else {
                            printf("Entered map %d\n", next);
                        }
                    }
                }
            } else if (ev.type == SDL_MOUSEBUTTONDOWN) {
                if (ev.button.button == SDL_BUTTON_LEFT) {
                    Wolf_TryShoot();
                }
            }
        }

        /* Auto-advance after elevator */
        if (g_level.level_complete && g_level.elevator_ready) {
            /* wait for N key — flashed on HUD */
        }

        state = SDL_GetKeyboardState(NULL);
        keys[0] = state[SDL_SCANCODE_W] || state[SDL_SCANCODE_UP];
        keys[1] = state[SDL_SCANCODE_S] || state[SDL_SCANCODE_DOWN];
        keys[2] = state[SDL_SCANCODE_LEFT];
        keys[3] = state[SDL_SCANCODE_RIGHT];
        keys[4] = state[SDL_SCANCODE_A];
        keys[5] = state[SDL_SCANCODE_D];
        keys[6] = state[SDL_SCANCODE_LCTRL] || state[SDL_SCANCODE_RCTRL];
        keys[7] = state[SDL_SCANCODE_1];
        keys[8] = state[SDL_SCANCODE_2];
        keys[9] = state[SDL_SCANCODE_3];
        keys[10] = state[SDL_SCANCODE_4];

        now = SDL_GetTicks();
        dt = (float)(now - last_ticks) / 1000.0f;
        last_ticks = now;
        if (dt > 0.05f) {
            dt = 0.05f;
        }

        Wolf_UpdateLevel(dt, keys);
        Wolf_DrawWorld();
        draw_hud();
        present(renderer, texture, pixels);
    }

    free(pixels);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    return 0;
}
