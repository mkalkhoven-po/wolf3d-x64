#pragma once

#include <stdint.h>

/*
 * VGAGRAPH chunk layout. The numbering differs per pack, so the IDs below are
 * resolved at runtime from the layout chosen in CA_OpenGraphFiles().
 * WL6: registered 1.4 (GFXV_WL6.H). WL1: Apogee 1.4 shareware, checked against
 * the data (VGAHEAD size, pictable size, TITLEPIC 320x200 etc.).
 */

#define STRUCTPIC     0
#define STARTFONT     1
#define STARTPICS     3

/* Fixed upper bound for chunk-indexed arrays across all supported packs. */
#define NUMCHUNKS_MAX 160

typedef struct {
    const char *name;
    int numchunks;
    int numpics;
    int starttile8;
    int startexterns;

    int c_optionspic, c_cursor1pic, c_cursor2pic, c_notselectedpic, c_selectedpic,
        c_mouselbackpic;
    int statusbarpic, titlepic, pg13pic, creditspic, highscorespic;
    int knifepic, gunpic, machinegunpic, gatlinggunpic;
    int nokeypic, goldkeypic, silverkeypic;
    int n_blankpic, n_0pic;
    int face1apic, face8apic;
    int gotgatlingpic, mutantbjpic, pausedpic, getpsychedpic;
    int t_demo0; /* T_DEMO0..T_DEMO3 are consecutive extern chunks */
} gfx_layout_t;

extern const gfx_layout_t gfx_layout_wl6;
extern const gfx_layout_t gfx_layout_wl1;
extern const gfx_layout_t *g_gfx;

#define NUMCHUNKS       (g_gfx->numchunks)
#define NUMPICS         (g_gfx->numpics)
#define STARTTILE8      (g_gfx->starttile8)
#define STARTEXTERNS    (g_gfx->startexterns)

#define C_OPTIONSPIC    (g_gfx->c_optionspic)
#define C_CURSOR1PIC    (g_gfx->c_cursor1pic)
#define C_CURSOR2PIC    (g_gfx->c_cursor2pic)
#define C_NOTSELECTEDPIC (g_gfx->c_notselectedpic)
#define C_SELECTEDPIC   (g_gfx->c_selectedpic)
#define C_MOUSELBACKPIC (g_gfx->c_mouselbackpic)

#define STATUSBARPIC    (g_gfx->statusbarpic)
#define TITLEPIC        (g_gfx->titlepic)
#define PG13PIC         (g_gfx->pg13pic)
#define CREDITSPIC      (g_gfx->creditspic)
#define HIGHSCORESPIC   (g_gfx->highscorespic)

/* Weapon pics are consecutive: knife, pistol, machine gun, gatling. */
#define KNIFEPIC        (g_gfx->knifepic)
#define GUNPIC          (g_gfx->gunpic)
#define MACHINEGUNPIC   (g_gfx->machinegunpic)
#define GATLINGGUNPIC   (g_gfx->gatlinggunpic)
#define NOKEYPIC        (g_gfx->nokeypic)
#define GOLDKEYPIC      (g_gfx->goldkeypic)
#define SILVERKEYPIC    (g_gfx->silverkeypic)

/* Status digits N_0PIC..N_9PIC are consecutive. */
#define N_BLANKPIC      (g_gfx->n_blankpic)
#define N_0PIC          (g_gfx->n_0pic)
#define N_1PIC          ((g_gfx->n_0pic) + 1)
#define N_2PIC          ((g_gfx->n_0pic) + 2)
#define N_3PIC          ((g_gfx->n_0pic) + 3)
#define N_4PIC          ((g_gfx->n_0pic) + 4)
#define N_5PIC          ((g_gfx->n_0pic) + 5)
#define N_6PIC          ((g_gfx->n_0pic) + 6)
#define N_7PIC          ((g_gfx->n_0pic) + 7)
#define N_8PIC          ((g_gfx->n_0pic) + 8)
#define N_9PIC          ((g_gfx->n_0pic) + 9)

/* Face frames: FACE<n>A/B/C are consecutive from FACE1APIC. */
#define FACE1APIC       (g_gfx->face1apic)
#define FACE1BPIC       ((g_gfx->face1apic) + 1)
#define FACE1CPIC       ((g_gfx->face1apic) + 2)
#define FACE2APIC       ((g_gfx->face1apic) + 3)
#define FACE8APIC       (g_gfx->face8apic)
#define GOTGATLINGPIC   (g_gfx->gotgatlingpic)
#define MUTANTBJPIC     (g_gfx->mutantbjpic)
#define PAUSEDPIC       (g_gfx->pausedpic)
#define GETPSYCHEDPIC   (g_gfx->getpsychedpic)

/* Attract-mode demos (PlayDemo): byte map, word length incl. 4-byte header,
 * one pad byte, then 3 bytes per tic (buttons, dx, dy). */
#define T_DEMO0         (g_gfx->t_demo0)
#define NUMDEMOS        4

/* Control-panel colours (WL_MENU.H) */
#define MENU_BORDCOLOR  0x29
#define MENU_BORD2COLOR 0x23
#define MENU_BKGDCOLOR  0x2d
#define MENU_STRIPE     0x2c
#define MENU_TEXTCOLOR  0x17
#define MENU_HIGHLIGHT  0x13

#define MENU_X 76
#define MENU_Y 55
#define MENU_W 178
#define MENU_H (13 * 9 + 6)

/* Original status strip height (WL_DEF.H). */
#define STATUSLINES 40

typedef struct {
    int16_t width;
    int16_t height;
} pictabletype;
