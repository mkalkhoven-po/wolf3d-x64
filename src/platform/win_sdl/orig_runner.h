#pragma once

/*
 * Runs the faithful engine (src/orig, ADR-002) in an SDL window:
 * wolf3d [--map N] [--skill 0-3] [--viewsize 4-20] [--demo 0-3] [--load 0-9]
 *        [--fast] [--frames N] [--screenshot N file.bmp]
 *
 * The engine's hooks are wired to the shared platform layer here: the 70 Hz
 * clock, DOS scan-code input, presenting the 320x200 frame with palette
 * shifts and FizzleFade, sound and music, the save slots (format v2) and the
 * high scores. Without game options it runs the title page and the
 * control panel (P5b); the intermission and the attract loop are P5c/P5d.
 */

#include "wl_types.h"

typedef struct {
    int map;      /* 0-59: episode * 10 + level */
    int skill;    /* gd_baby .. gd_hard */
    int viewsize; /* 4-20 */
    int demo;     /* -1: play; 0-3: watch that demo and quit */
    int load;     /* -1: new game; 0-9: start from that save slot */
    int menu;     /* the title page and the control panel instead of the options above */
    int fast;     /* no clock: run as fast as possible (smoke tests) */
    int maxframes;  /* >0: stop after that many frames (smoke tests) */
    int shotframe;  /* >0: save that frame as an 8-bit BMP to shotpath */
    const char *shotpath;
} orig_run_options_t;

/*
 * True when argv starts the game directly (any option above, or
 * --engine=orig) rather than the title screen and menu.
 */
boolean OrigRunner_Direct(int argc, char **argv);

void OrigRunner_Defaults(orig_run_options_t *opts);

/* Fill opts from argv; false (with a message) on a bad value or --engine=legacy. */
boolean OrigRunner_ParseArgs(int argc, char **argv, orig_run_options_t *opts);

/* The data files are open (maps, VSWAP, VGAGRAPH, audio). Returns 0 on success. */
int OrigRunner_Run(const orig_run_options_t *opts);

/* --smoke-only: 300 frames of demo 0 with all hooks, then a save v2 roundtrip. */
boolean OrigRunner_SmokeVerify(void);

/* The window was closed or Quit chosen during the last run. */
boolean OrigRunner_QuitRequested(void);
