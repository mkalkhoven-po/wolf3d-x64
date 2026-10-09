#pragma once

/*
 * Runs the faithful engine (src/orig, ADR-002) in an SDL window:
 * wolf3d --engine=orig [--map N] [--skill 0-3] [--viewsize 4-20] [--demo 0-3]
 *        [--fast] [--frames N] [--screenshot N file.bmp]
 *
 * The engine's hooks are wired to the shared platform layer here: the 70 Hz
 * clock, DOS scan-code input, presenting the 320x200 frame with palette
 * shifts and FizzleFade, the status bar, sound and music. Menus, the
 * intermission and the attract loop are P5; until then Esc quits.
 */

#include "wl_types.h"

typedef struct {
    int map;      /* 0-59: episode * 10 + level */
    int skill;    /* gd_baby .. gd_hard */
    int viewsize; /* 4-20 */
    int demo;     /* -1: play; 0-3: watch that demo and quit */
    int fast;     /* no clock: run as fast as possible (smoke tests) */
    int maxframes;  /* >0: stop after that many frames (smoke tests) */
    int shotframe;  /* >0: save that frame as an 8-bit BMP to shotpath */
    const char *shotpath;
} orig_run_options_t;

/* True when argv asks for the faithful engine (--engine=orig). */
boolean OrigRunner_Requested(int argc, char **argv);

/* Fill opts from argv; false (with a message) on a bad value. */
boolean OrigRunner_ParseArgs(int argc, char **argv, orig_run_options_t *opts);

/* The data files are open (maps, VSWAP, VGAGRAPH, audio). Returns 0 on success. */
int OrigRunner_Run(const orig_run_options_t *opts);
