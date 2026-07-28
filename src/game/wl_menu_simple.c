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

/* Subset of the original main menu (full DOS list is longer). */
static const char *menu_items[] = {
    "New Game",
    "Sound",
    "Control",
    "Load Game",
    "Change View",
    "View Scores",
    "Quit",
};
#define MENU_COUNT 7

static void draw_stripes(int y)
{
    int x;
    /* Thin metal rail under its Options banner (simplified DrawStripes). */
    for (x = 0; x < SCREENWIDTH; ++x) {
        VL_Plot(x, y, MENU_STRIPE);
        VL_Plot(x, y + 1, MENU_BORD2COLOR);
    }
}

static void draw_window(int x, int y, int w, int h, int color)
{
    /* Filled panel with 2-tone border like DrawWindow in WL_MENU.C. */
    VL_Bar(x, y, w, h, color);
    /* Outer border */
    VL_Bar(x, y, w, 1, MENU_BORDCOLOR);
    VL_Bar(x, y + h - 1, w, 1, MENU_BORDCOLOR);
    VL_Bar(x, y, 1, h, MENU_BORDCOLOR);
    VL_Bar(x + w - 1, y, 1, h, MENU_BORDCOLOR);
    /* Inner highlight */
    VL_Bar(x + 1, y + 1, w - 2, 1, MENU_BORD2COLOR);
    VL_Bar(x + 1, y + 1, 1, h - 2, MENU_BORD2COLOR);
}

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

    VL_ClearScreen(0); /* classic ClearMScreen default */
    VL_SetPalette(gamepal);

    /* Blue-ish top/bottom frames (approximation of striped backdrop). */
    VL_Bar(0, 0, SCREENWIDTH, 16, MENU_BORDCOLOR);
    VL_Bar(0, SCREENHEIGHT - 8, SCREENWIDTH, 8, MENU_BORDCOLOR);
    draw_stripes(10);

    if (pictable) {
        CA_CacheGrChunk(C_OPTIONSPIC);
        if (grsegs[C_OPTIONSPIC]) {
            VWB_DrawPic(84, 0, C_OPTIONSPIC);
        }
    }

    /* Red control panel window (MENU_X/Y/W/H style). */
    draw_window(MENU_X - 8, MENU_Y - 3, MENU_W, MENU_H, MENU_BKGDCOLOR);

    for (i = 0; i < MENU_COUNT; ++i) {
        int y = MENU_Y + i * 13;
        int color = (i == cursor) ? MENU_HIGHLIGHT : MENU_TEXTCOLOR;
        int mark_x = MENU_X + 4;
        int text_x = MENU_X + 24;

        if (pictable) {
            int mark = (i == cursor) ? C_SELECTEDPIC : C_NOTSELECTEDPIC;
            CA_CacheGrChunk(mark);
            if (grsegs[mark]) {
                VWB_DrawPic(mark_x, y, mark);
            } else if (i == cursor) {
                VH_DrawString(mark_x, y + 2, "*", MENU_HIGHLIGHT);
            }
        } else if (i == cursor) {
            VH_DrawString(mark_x, y + 2, "*", MENU_HIGHLIGHT);
        }
        VH_DrawString(text_x, y + 2, menu_items[i], color);
    }

    if (pictable) {
        CA_CacheGrChunk(C_MOUSELBACKPIC);
        if (grsegs[C_MOUSELBACKPIC]) {
            VWB_DrawPic(112, 184, C_MOUSELBACKPIC);
        }
        /* Gun cursor on active row (original uses C_CURSOR*). */
        CA_CacheGrChunk(C_CURSOR1PIC);
        if (grsegs[C_CURSOR1PIC]) {
            VWB_DrawPic(MENU_X - 16, MENU_Y + cursor * 13 - 2, C_CURSOR1PIC);
        }
    }

    snprintf(line, sizeof(line), "SCALE x%d  %s", g_config.scale,
             g_config.fullscreen ? "FULLSCREEN" : "WINDOW");
    VH_DrawString(8, 192, line, 7);
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
                        SD_PlaySound(MOVEGUN1SND);
                    } else if (k == SDLK_DOWN) {
                        cursor = (cursor + 1) % MENU_COUNT;
                        draw_menu(cursor);
                        SD_PlaySound(MOVEGUN2SND);
                    } else if (k == SDLK_ESCAPE) {
                        state = UI_TITLE;
                        draw_title();
                        if (g_config.music_on) {
                            SD_StartMusic(INTROCW3_MUS);
                        }
                    } else if (k == SDLK_RETURN) {
                        SD_PlaySound(SELECTITEMSND);
                        if (cursor == 0) { /* New Game */
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
                        } else if (cursor == 6) {
                            state = UI_QUIT;
                            running = 0;
                        } else {
                            /* NYI stubs for Sound/Control/Load/View. */
                            VL_Bar(40, 140, 240, 16, 0);
                            VH_DrawString(50, 142, "NOT IMPLEMENTED YET", 14);
                            Wolf_PresentFrame(&pr);
                            SDL_Delay(450);
                            draw_menu(cursor);
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
