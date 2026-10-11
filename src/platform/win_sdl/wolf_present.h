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
    SDL_Texture *prescaled; /* fill mode: the frame at a whole multiple, smoothed down to the window */
    int prescale_x, prescale_y;
    Uint32 *pixels;
    int logical_w;
    int logical_h;
    int scale;
    int fullscreen;
    int fill;    /* 0: whole multiples only (black bars); 1: fill the window at 4:3, sharp bilinear */
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

/* Fill the window at 4:3 (sharp bilinear) instead of whole multiples only. */
void Wolf_PresentSetFill(wolf_present_t *pr, int fill);

/* Apply config aspect: keep 4:3 logical size; black bars on resize. */
void Wolf_PresentHandleResize(wolf_present_t *pr);
