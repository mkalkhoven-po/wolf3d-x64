#pragma once

#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif
#include <SDL.h>

#include "wl_types.h"
#include "id/id_vl.h"

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    Uint32 *pixels;
    int logical_w;
    int logical_h;
    int scale;
    int fullscreen;
    int running;
} wolf_present_t;

/* DPI awareness early (Windows). Safe to call multiple times. */
void Wolf_EnableDpiAwareness(void);

/* Create window at integer * scale of 320x200 with nearest-neighbor present. */
boolean Wolf_PresentCreate(wolf_present_t *pr, const char *title, int scale, int fullscreen);
void Wolf_PresentDestroy(wolf_present_t *pr);

/* Upload 8-bit framebuffer via palette and present with letterboxed integer scale. */
void Wolf_PresentFrame(wolf_present_t *pr);

/* Toggle fullscreen (desktop). Updates pr->fullscreen. */
boolean Wolf_PresentToggleFullscreen(wolf_present_t *pr);

/* Change integer scale 1..8 while windowed. */
boolean Wolf_PresentSetScale(wolf_present_t *pr, int scale);

/* Apply config aspect: keep 4:3 logical size; black bars on resize. */
void Wolf_PresentHandleResize(wolf_present_t *pr);
