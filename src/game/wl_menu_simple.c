#include "wl_menu_simple.h"

#include <stdio.h>
#include <string.h>

#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif
#include <SDL.h>

#include "id/audio_wl6.h"
#include "id/gfxv_wl6.h"
#include "id/id_ca.h"
#include "id/id_sd.h"
#include "id/id_vh.h"
#include "id/id_vl.h"
#include "wl_game.h"
#include "wolf_present.h"
#include "wolf_userdata.h"

typedef enum {
    UI_TITLE = 0,
    UI_MENU,
    UI_QUIT
} ui_state_t;

static const char *menu_items[] = {
    "NEW GAME",
    "OPTIONS",
    "QUIT",
};
#define MENU_COUNT 3

static void draw_title(void)
{
    VL_ClearScreen(0);
    VL_SetPalette(gamepal);
    CA_CacheGrChunk(TITLEPIC);
    if (grsegs[TITLEPIC] && pictable) {
        int w = pictable[TITLEPIC - STARTPICS].width;
        int h = pictable[TITLEPIC - STARTPICS].height;
        int x = (SCREENWIDTH - w) / 2;
        int y = (SCREENHEIGHT - h) / 2;
        if (x < 0) {
            x = 0;
        }
        if (y < 0) {
            y = 0;
        }
        VWB_DrawPic(x, y, TITLEPIC);
    } else {
        VH_DrawString(80, 90, "TITLEPIC MISSING", 15);
    }
    VH_DrawString(70, 185, "PRESS ENTER FOR MENU", 14);
}

static void draw_menu(int cursor)
{
    int i;
    char line[64];
    VL_ClearScreen(0x18);
    VL_SetPalette(gamepal);

    if (pictable) {
        CA_CacheGrChunk(C_OPTIONSPIC);
        if (grsegs[C_OPTIONSPIC]) {
            VWB_DrawPic(84, 0, C_OPTIONSPIC);
        }
        CA_CacheGrChunk(C_MOUSELBACKPIC);
        if (grsegs[C_MOUSELBACKPIC]) {
            VWB_DrawPic(112, 184, C_MOUSELBACKPIC);
        }
    }

    VH_DrawString(110, 60, "MAIN MENU", 15);
    for (i = 0; i < MENU_COUNT; ++i) {
        int y = 90 + i * 18;
        int color = (i == cursor) ? 14 : 7;
        if (i == cursor) {
            VH_DrawString(90, y, ">", 14);
        }
        VH_DrawString(110, y, menu_items[i], color);
    }
    snprintf(line, sizeof(line), "SCALE x%d  %s", g_config.scale,
             g_config.fullscreen ? "FULLSCREEN" : "WINDOW");
    VH_DrawString(40, 160, line, 7);
    VH_DrawString(40, 175, "UP/DOWN ENTER  F11 FS  +/- SCALE  ESC", 6);
}

int Wolf_RunIntroMenu(void)
{
    ui_state_t state = UI_TITLE;
    int cursor = 0;
    int running = 1;
    wolf_present_t pr;

    if (!Wolf_PresentCreate(&pr, "Wolf3D Win64", g_config.scale,
                            g_config.fullscreen)) {
        return 1;
    }

    draw_title();
    if (g_config.music_on) {
        SD_StartMusic(INTROCW3_MUS);
    }

    while (running && state != UI_QUIT) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) {
                state = UI_QUIT;
                running = 0;
            } else if (ev.type == SDL_WINDOWEVENT &&
                       (ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
                        ev.window.event == SDL_WINDOWEVENT_RESIZED)) {
                Wolf_PresentHandleResize(&pr);
            } else if (ev.type == SDL_KEYDOWN) {
                SDL_Keycode k = ev.key.keysym.sym;
                if (k == SDLK_F11) {
                    if (Wolf_PresentToggleFullscreen(&pr)) {
                        g_config.fullscreen = pr.fullscreen;
                        (void)Wolf_ConfigSave();
                        if (state == UI_MENU) {
                            draw_menu(cursor);
                        } else {
                            draw_title();
                        }
                    }
                } else if (k == SDLK_EQUALS || k == SDLK_PLUS || k == SDLK_KP_PLUS) {
                    if (Wolf_PresentSetScale(&pr, pr.scale + 1)) {
                        g_config.scale = pr.scale;
                        (void)Wolf_ConfigSave();
                        if (state == UI_MENU) {
                            draw_menu(cursor);
                        }
                    }
                } else if (k == SDLK_MINUS || k == SDLK_KP_MINUS) {
                    if (Wolf_PresentSetScale(&pr, pr.scale - 1)) {
                        g_config.scale = pr.scale;
                        (void)Wolf_ConfigSave();
                        if (state == UI_MENU) {
                            draw_menu(cursor);
                        }
                    }
                } else if (state == UI_TITLE) {
                    if (k == SDLK_RETURN || k == SDLK_SPACE) {
                        state = UI_MENU;
                        cursor = 0;
                        draw_menu(cursor);
                        if (g_config.music_on) {
                            SD_StartMusic(WARMARCH_MUS);
                        }
                    } else if (k == SDLK_ESCAPE) {
                        state = UI_QUIT;
                        running = 0;
                    }
                } else if (state == UI_MENU) {
                    if (k == SDLK_UP) {
                        cursor = (cursor + MENU_COUNT - 1) % MENU_COUNT;
                        draw_menu(cursor);
                    } else if (k == SDLK_DOWN) {
                        cursor = (cursor + 1) % MENU_COUNT;
                        draw_menu(cursor);
                    } else if (k == SDLK_ESCAPE) {
                        state = UI_TITLE;
                        draw_title();
                        if (g_config.music_on) {
                            SD_StartMusic(INTROCW3_MUS);
                        }
                    } else if (k == SDLK_RETURN) {
                        if (cursor == 0) {
                            g_config.scale = pr.scale;
                            g_config.fullscreen = pr.fullscreen;
                            (void)Wolf_ConfigSave();
                            Wolf_PresentDestroy(&pr);
                            SDL_QuitSubSystem(SDL_INIT_VIDEO);
                            {
                                int grc = Wolf_RunGame();
                                if (grc != 0) {
                                    return grc;
                                }
                            }
                            if (!Wolf_PresentCreate(&pr, "Wolf3D Win64",
                                                    g_config.scale,
                                                    g_config.fullscreen)) {
                                return 1;
                            }
                            state = UI_MENU;
                            draw_menu(cursor);
                            if (g_config.music_on) {
                                SD_StartMusic(WARMARCH_MUS);
                            }
                        } else if (cursor == 1) {
                            VL_Bar(40, 145, 240, 28, 0);
                            VH_DrawString(50, 148, "OPTIONS IN CONFIG.INI", 14);
                            VH_DrawString(50, 160, "(AppData scale/mouse/audio)", 7);
                        } else {
                            state = UI_QUIT;
                            running = 0;
                        }
                    }
                }
            }
        }

        Wolf_PresentFrame(&pr);
        SDL_Delay(1);
    }

    g_config.scale = pr.scale;
    g_config.fullscreen = pr.fullscreen;
    (void)Wolf_ConfigSave();
    Wolf_PresentDestroy(&pr);
    return 0;
}
