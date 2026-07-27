#include "wl_menu_simple.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif
#include <SDL.h>

#include "id/id_ca.h"
#include "id/id_vh.h"
#include "id/id_vl.h"
#include "id/gfxv_wl6.h"

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
    VL_ClearScreen(0x18); /* dark blue-ish typical backdrop color index */
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
    VH_DrawString(40, 170, "UP/DOWN  ENTER  ESC QUIT", 6);
}

int Wolf_RunIntroMenu(void)
{
    ui_state_t state = UI_TITLE;
    int cursor = 0;
    int running = 1;
    int scale = 3;
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    SDL_Texture *texture = NULL;
    Uint32 *pixels = NULL;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    window = SDL_CreateWindow(
        "Wolf3D Win64",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        SCREENWIDTH * scale,
        SCREENHEIGHT * scale,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    renderer = SDL_CreateRenderer(window, -1,
                                  SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        renderer = SDL_CreateRenderer(window, -1, 0);
    }
    if (!renderer) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
    SDL_RenderSetLogicalSize(renderer, SCREENWIDTH, SCREENHEIGHT);

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                SDL_TEXTUREACCESS_STREAMING,
                                SCREENWIDTH, SCREENHEIGHT);
    if (!texture) {
        fprintf(stderr, "SDL_CreateTexture failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    pixels = (Uint32 *)malloc(SCREENPIXELS * sizeof(Uint32));
    if (!pixels) {
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    draw_title();

    while (running && state != UI_QUIT) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) {
                state = UI_QUIT;
                running = 0;
            } else if (ev.type == SDL_KEYDOWN) {
                SDL_Keycode k = ev.key.keysym.sym;
                if (state == UI_TITLE) {
                    if (k == SDLK_RETURN || k == SDLK_SPACE) {
                        state = UI_MENU;
                        cursor = 0;
                        draw_menu(cursor);
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
                    } else if (k == SDLK_RETURN) {
                        if (cursor == 0) {
                            /* New Game — stub for E4 */
                            VL_Bar(40, 150, 240, 20, 0);
                            VH_DrawString(50, 154, "NEW GAME - COMING IN E5!", 14);
                        } else if (cursor == 1) {
                            VL_Bar(40, 150, 240, 20, 0);
                            VH_DrawString(70, 154, "OPTIONS STUB", 14);
                        } else {
                            state = UI_QUIT;
                            running = 0;
                        }
                    }
                }
            }
        }

        /* Present framebuffer */
        if (framebuffer) {
            int i;
            for (i = 0; i < SCREENPIXELS; ++i) {
                byte idx = framebuffer[i];
                byte r = VL_VGA6to8(g_palette[idx * 3 + 0]);
                byte g = VL_VGA6to8(g_palette[idx * 3 + 1]);
                byte b = VL_VGA6to8(g_palette[idx * 3 + 2]);
                pixels[i] = (Uint32)(0xFF000000u | ((Uint32)r << 16) |
                                     ((Uint32)g << 8) | (Uint32)b);
            }
            SDL_UpdateTexture(texture, NULL, pixels, SCREENWIDTH * 4);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, texture, NULL, NULL);
            SDL_RenderPresent(renderer);
        } else {
            SDL_Delay(16);
        }
    }

    free(pixels);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    /* Do not SDL_Quit here if other subsystems need it — full quit at process end. */
    SDL_Quit();
    return 0;
}
