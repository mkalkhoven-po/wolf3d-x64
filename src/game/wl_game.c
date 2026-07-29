#include "wl_game.h"

#include <stdio.h>
#include <string.h>

#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif
#include <SDL.h>

#include "id/audio_wl6.h"
#include "id/id_sd.h"
#include "id/id_vh.h"
#include "wl_draw.h"
#include "wl_play.h"
#include "wolf_present.h"
#include "wolf_userdata.h"

/* Episode 1 map songs from original WL_PLAY.C (mapon 0..9). */
static const int episode1_songs[] = {
    GETTHEM_MUS, SEARCHN_MUS, POW_MUS, SUSPENSE_MUS, GETTHEM_MUS,
    SEARCHN_MUS, POW_MUS, SUSPENSE_MUS, WARMARCH_MUS, CORNER_MUS};

static void draw_hud(void)
{
    char line[160];
    player_t *p = &g_level.player;
    const char *wname = "KNIFE";
    if (p->weapon == 1) {
        wname = "PISTOL";
    } else if (p->weapon == 2) {
        wname = "MGUN";
    } else if (p->weapon == 3) {
        wname = "CHAIN";
    }
    snprintf(line, sizeof(line),
             "HP %d  AMMO %d  LIVES %d  SCORE %d  %s  MAP %d  x%d%s",
             p->health, p->ammo, p->lives, p->score, wname, g_level.mapnum,
             g_config.scale, g_config.fullscreen ? " FS" : "");
    VH_DrawString(2, SCREENHEIGHT - 18, line, 15);
    VH_DrawString(2, SCREENHEIGHT - 9,
                  "WASD  CTRL SHOOT  E DOOR  F5 SAVE  F9 LOAD  F11 FS  ESC", 7);
    if (p->dead) {
        if (p->lives >= 0) {
            VH_DrawString(90, 80, "YOU DIED - PRESS R", 12);
        } else {
            VH_DrawString(100, 80, "GAME OVER", 12);
        }
    }
    if (g_level.level_complete) {
        VH_DrawString(70, 70, "LEVEL COMPLETE! N=NEXT", 14);
    }
}

int Wolf_RunGame(void)
{
    wolf_present_t pr;
    int running = 1;
    Uint32 last_ticks;
    boolean keys[12];
    int start_map = 0;

    Wolf_ResetPlayerStats();
    if (!Wolf_SetupLevel(start_map)) {
        fprintf(stderr, "Failed to setup map %d\n", start_map);
        return 1;
    }

    if (!Wolf_PresentCreate(&pr, "Wolf3D Win64", g_config.scale,
                            g_config.fullscreen)) {
        return 1;
    }

    memset(keys, 0, sizeof(keys));
    last_ticks = SDL_GetTicks();
    if (g_config.music_on) {
        int track = episode1_songs[0];
        if (start_map >= 0 && start_map < 10) {
            track = episode1_songs[start_map];
        }
        SD_StartMusic((word)track);
    }
    printf("E8: F5 save / F9 load / F11 fullscreen / +/- scale, ESC exit\n");

    while (running) {
        SDL_Event ev;
        Uint32 now;
        float dt;
        const Uint8 *state;

        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) {
                running = 0;
            } else if (ev.type == SDL_WINDOWEVENT &&
                       (ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
                        ev.window.event == SDL_WINDOWEVENT_RESIZED)) {
                Wolf_PresentHandleResize(&pr);
            } else if (ev.type == SDL_KEYDOWN) {
                SDL_Keycode k = ev.key.keysym.sym;
                if (k == SDLK_ESCAPE) {
                    running = 0;
                } else if (k == SDLK_e || k == SDLK_SPACE) {
                    Wolf_TryUse();
                } else if (k == SDLK_F11) {
                    if (Wolf_PresentToggleFullscreen(&pr)) {
                        g_config.fullscreen = pr.fullscreen;
                        (void)Wolf_ConfigSave();
                    }
                } else if (k == SDLK_EQUALS || k == SDLK_PLUS || k == SDLK_KP_PLUS) {
                    if (Wolf_PresentSetScale(&pr, pr.scale + 1)) {
                        g_config.scale = pr.scale;
                        (void)Wolf_ConfigSave();
                    }
                } else if (k == SDLK_MINUS || k == SDLK_KP_MINUS) {
                    if (Wolf_PresentSetScale(&pr, pr.scale - 1)) {
                        g_config.scale = pr.scale;
                        (void)Wolf_ConfigSave();
                    }
                } else if (k == SDLK_F5) {
                    (void)Wolf_SaveGame(0);
                } else if (k == SDLK_F9) {
                    (void)Wolf_LoadGame(0);
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
                        Wolf_HighScoresTryInsert(g_level.player.score, 0,
                                                 g_level.mapnum, "BJ");
                        VH_DrawString(40, 100, "EPISODE COMPLETE - ESC", 14);
                    } else {
                        if (!Wolf_SetupLevel(next)) {
                                                    fprintf(stderr, "Failed next map %d\n", next);
                                                    running = 0;
                                                } else {
                                                    if (g_config.music_on) {
                                                        int track = episode1_songs[next < 10 ? next : 0];
                                                        SD_StartMusic((word)track);
                                                    }
                                                    printf("Entered map %d\n", next);
                                                }
                    }
                }
            } else if (ev.type == SDL_MOUSEBUTTONDOWN) {
                if (ev.button.button == SDL_BUTTON_LEFT) {
                    Wolf_TryShoot();
                }
            } else if (ev.type == SDL_MOUSEMOTION && g_config.mouse_look) {
                            /* Mouse right → turn right (angle increases). */
                            float turn = (float)ev.motion.xrel *
                                         (0.0025f * (float)g_config.mouse_sens);
                            g_level.player.angle += turn;
                        }
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
        Wolf_PresentFrame(&pr);
    }

    g_config.scale = pr.scale;
    g_config.fullscreen = pr.fullscreen;
    (void)Wolf_ConfigSave();
    Wolf_PresentDestroy(&pr);
    return 0;
}
