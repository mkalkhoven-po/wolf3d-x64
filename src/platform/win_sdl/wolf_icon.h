#pragma once

/*
 * The window icon: the same picture as res/wolf3d-x64.ico (the exe's icon on
 * Windows), for SDL_SetWindowIcon on every platform. Drawn by
 * scripts/make_icon.py.
 */
#define WOLF_ICON_SIZE 32

extern const unsigned char wolf_icon_rgba[WOLF_ICON_SIZE * WOLF_ICON_SIZE * 4];
