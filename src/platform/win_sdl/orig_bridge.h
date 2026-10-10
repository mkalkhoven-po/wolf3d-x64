#pragma once

/*
 * What the faithful-engine runner needs from the shared platform layer
 * (window, palette, sound, data), as plain functions. The engine draws its
 * own pictures and text (orig_vh.c). The legacy headers and src/orig/orig.h
 * define some of the same names (sound enums, sprite numbers), so the two
 * sides meet only here.
 */

#include <stddef.h>

#include "wl_types.h"

/* window (config scale / fullscreen); false if SDL video fails */
boolean Bridge_OpenWindow(const char *title);
void Bridge_CloseWindow(void);
boolean Bridge_ToggleFullscreen(void);

byte *Bridge_Framebuffer(void); /* the 320x200 8-bit frame the window shows */
void Bridge_Present(void);
void Bridge_SetPalette(const byte *palette); /* 768 bytes, 6-bit VGA */
const byte *Bridge_GamePalette(void);

boolean Bridge_PlaySound(int sound); /* true: a digitized sound started */
int Bridge_SoundPlaying(void);        /* the AdLib effect playing, 0 when none */
void Bridge_PositionSound(int left, int right);    /* 0-15 per side, for the next sound */
void Bridge_SetSoundPosition(int left, int right); /* for the placed sound playing */
boolean Bridge_SoundPositioned(void);
void Bridge_WaitSoundDone(void);
void Bridge_StartMusic(int song);
void Bridge_MusicOff(void);

const byte *Bridge_Page(int page); /* VSWAP page */
int Bridge_SpriteStart(void);
const byte *Bridge_DemoLump(int demo, size_t *length);
boolean Bridge_IsShareware(void); /* a WL1 pack */
boolean Bridge_MapExists(int map); /* 0-59, in this pack's MAPHEAD */
int Bridge_MouseLook(void);       /* config: mouse turns */
