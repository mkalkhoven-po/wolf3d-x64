#include "wolf_present.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

void Wolf_EnableDpiAwareness(void)
{
#ifdef _WIN32
#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((DPI_AWARENESS_CONTEXT) - 4)
#endif
    HMODULE user32 = LoadLibraryA("user32.dll");
    if (user32) {
        typedef BOOL(WINAPI *SetDpiCtxFn)(DPI_AWARENESS_CONTEXT);
        SetDpiCtxFn set_ctx = (SetDpiCtxFn)(void (*)(void))GetProcAddress(
            user32, "SetProcessDpiAwarenessContext");
        if (set_ctx) {
            set_ctx(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
            FreeLibrary(user32);
            return;
        }
        FreeLibrary(user32);
    }
    {
        HMODULE shcore = LoadLibraryA("shcore.dll");
        if (shcore) {
            typedef HRESULT(WINAPI *SetDpiAwarenessFn)(int);
            SetDpiAwarenessFn set_aw = (SetDpiAwarenessFn)(void (*)(void))GetProcAddress(
                shcore, "SetProcessDpiAwareness");
            if (set_aw) {
                set_aw(2); /* PROCESS_PER_MONITOR_DPI_AWARE */
                FreeLibrary(shcore);
                return;
            }
            FreeLibrary(shcore);
        }
    }
    SetProcessDPIAware();
#endif
}

/*
 * Original VGA Wolf used ~20% taller pixels. We keep an internal 320×200
 * framebuffer but present it into a 4:3 destination (320×240 base).
 */
static void present_dst_rect(const wolf_present_t *pr, SDL_Rect *dst)
{
    int ww = 0, wh = 0;
    int out_w, out_h, scale;
    if (!pr || !pr->window || !dst) {
        return;
    }
    SDL_GetWindowSize(pr->window, &ww, &wh);
    if (ww < 1) {
        ww = 1;
    }
    if (wh < 1) {
        wh = 1;
    }
    {
        int sx = ww / 320;
        int sy = wh / 240;
        scale = sx < sy ? sx : sy;
        if (scale < 1) {
            scale = 1;
        }
    }
    out_w = 320 * scale;
    out_h = 240 * scale;
    dst->w = out_w;
    dst->h = out_h;
    dst->x = (ww - out_w) / 2;
    dst->y = (wh - out_h) / 2;
}

static void apply_present_hints(wolf_present_t *pr)
{
    if (!pr || !pr->renderer) {
        return;
    }
    SDL_RenderSetLogicalSize(pr->renderer, 0, 0);
    SDL_RenderSetIntegerScale(pr->renderer, SDL_FALSE);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
}

boolean Wolf_PresentCreate(wolf_present_t *pr, const char *title, int scale,
                           int fullscreen)
{
    int w, h;
    if (!pr) {
        return false;
    }
    memset(pr, 0, sizeof(*pr));
    pr->logical_w = SCREENWIDTH;
    pr->logical_h = SCREENHEIGHT;
    pr->scale = (scale < 1) ? 1 : (scale > 8 ? 8 : scale);
    pr->fullscreen = fullscreen ? 1 : 0;
    pr->running = 1;

    if (SDL_WasInit(SDL_INIT_VIDEO) == 0) {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
            fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
            return false;
        }
    }

    /* Client size is 4:3 (320×240 path), not square-pixel 320×200. */
    w = 320 * pr->scale;
    h = 240 * pr->scale;

    pr->window = SDL_CreateWindow(
        title ? title : "Wolf3D Win64",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        w,
        h,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE |
            (pr->fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0) |
            SDL_WINDOW_ALLOW_HIGHDPI);
    if (!pr->window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return false;
    }

    pr->renderer = SDL_CreateRenderer(
        pr->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!pr->renderer) {
        pr->renderer = SDL_CreateRenderer(pr->window, -1, 0);
    }
    if (!pr->renderer) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        Wolf_PresentDestroy(pr);
        return false;
    }

    apply_present_hints(pr);

    pr->texture = SDL_CreateTexture(pr->renderer, SDL_PIXELFORMAT_ARGB8888,
                                    SDL_TEXTUREACCESS_STREAMING, pr->logical_w,
                                    pr->logical_h);
    if (!pr->texture) {
        fprintf(stderr, "SDL_CreateTexture failed: %s\n", SDL_GetError());
        Wolf_PresentDestroy(pr);
        return false;
    }

    pr->pixels = (Uint32 *)malloc((size_t)pr->logical_w * (size_t)pr->logical_h *
                                  sizeof(Uint32));
    if (!pr->pixels) {
        Wolf_PresentDestroy(pr);
        return false;
    }
    return true;
}

void Wolf_PresentDestroy(wolf_present_t *pr)
{
    if (!pr) {
        return;
    }
    if (pr->pixels) {
        free(pr->pixels);
        pr->pixels = NULL;
    }
    if (pr->texture) {
        SDL_DestroyTexture(pr->texture);
        pr->texture = NULL;
    }
    if (pr->renderer) {
        SDL_DestroyRenderer(pr->renderer);
        pr->renderer = NULL;
    }
    if (pr->window) {
        SDL_DestroyWindow(pr->window);
        pr->window = NULL;
    }
}

void Wolf_PresentFrame(wolf_present_t *pr)
{
    int i, n;
    SDL_Rect dst;

    if (!pr || !pr->renderer || !pr->texture || !pr->pixels || !framebuffer) {
        return;
    }
    n = pr->logical_w * pr->logical_h;
    for (i = 0; i < n; ++i) {
        byte idx = framebuffer[i];
        byte r = VL_VGA6to8(g_palette[idx * 3 + 0]);
        byte g = VL_VGA6to8(g_palette[idx * 3 + 1]);
        byte b = VL_VGA6to8(g_palette[idx * 3 + 2]);
        pr->pixels[i] = (Uint32)(0xFF000000u | ((Uint32)r << 16) |
                                 ((Uint32)g << 8) | (Uint32)b);
    }
    SDL_UpdateTexture(pr->texture, NULL, pr->pixels, pr->logical_w * 4);
    SDL_SetRenderDrawColor(pr->renderer, 0, 0, 0, 255);
    SDL_RenderClear(pr->renderer);
    present_dst_rect(pr, &dst);
    SDL_RenderCopy(pr->renderer, pr->texture, NULL, &dst);
    SDL_RenderPresent(pr->renderer);
}

boolean Wolf_PresentToggleFullscreen(wolf_present_t *pr)
{
    if (!pr || !pr->window) {
        return false;
    }
    if (pr->fullscreen) {
        if (SDL_SetWindowFullscreen(pr->window, 0) != 0) {
            return false;
        }
        pr->fullscreen = 0;
        SDL_SetWindowSize(pr->window, 320 * pr->scale, 240 * pr->scale);
        SDL_SetWindowPosition(pr->window, SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED);
    } else {
        if (SDL_SetWindowFullscreen(pr->window, SDL_WINDOW_FULLSCREEN_DESKTOP) !=
            0) {
            return false;
        }
        pr->fullscreen = 1;
    }
    apply_present_hints(pr);
    return true;
}

boolean Wolf_PresentSetScale(wolf_present_t *pr, int scale)
{
    if (!pr || !pr->window) {
        return false;
    }
    if (scale < 1) {
        scale = 1;
    }
    if (scale > 8) {
        scale = 8;
    }
    pr->scale = scale;
    if (!pr->fullscreen) {
        SDL_SetWindowSize(pr->window, 320 * pr->scale, 240 * pr->scale);
        SDL_SetWindowPosition(pr->window, SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED);
    }
    apply_present_hints(pr);
    return true;
}

void Wolf_PresentHandleResize(wolf_present_t *pr)
{
    apply_present_hints(pr);
}
