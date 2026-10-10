#pragma once

#include <stdint.h>

/*
 * VGAGRAPH chunk layout. The chunk names are those of the registered 1.4
 * GFXV_WL6.H. The Apogee 1.4 shareware (WL1) has 12 more help pictures at the
 * start (19 instead of 7, for its "Read This!" screens) and is otherwise the
 * same, shifted by 12: every picture from C_OPTIONSPIC on has the same size at
 * +12 (checked against both packs), and so do the tile8 and extern chunks. It
 * is not the May 1992 GFXV_WL1.H in reference/original. The layout is chosen
 * in CA_OpenGraphFiles() and checked against the data there.
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
    int shift; /* added to the WL6 number of every chunk from C_OPTIONSPIC on */
} gfx_layout_t;

extern const gfx_layout_t gfx_layout_wl6;
extern const gfx_layout_t gfx_layout_wl1;
extern const gfx_layout_t *g_gfx;

#define NUMCHUNKS       (g_gfx->numchunks)
#define NUMPICS         (g_gfx->numpics)
#define STARTTILE8      (g_gfx->starttile8)
#define STARTEXTERNS    (g_gfx->startexterns)
#define GFXSHIFT        (g_gfx->shift)

#define NUMFONT         2
#define NUMTILE8        72

/* GFXV_WL6.H (registered 1.4). T_ENDART2..6 exist in WL6 only. */
#define H_BJPIC            3 /* WL6 only: the shareware has other help art */
#define H_CASTLEPIC        4 /* WL6 only: the shareware has other help art */
#define H_BLAZEPIC         5 /* WL6 only: the shareware has other help art */
#define H_TOPWINDOWPIC     6 /* WL6 only: the shareware has other help art */
#define H_LEFTWINDOWPIC    7 /* WL6 only: the shareware has other help art */
#define H_RIGHTWINDOWPIC   8 /* WL6 only: the shareware has other help art */
#define H_BOTTOMINFOPIC    9 /* WL6 only: the shareware has other help art */
#define C_OPTIONSPIC       (10 + GFXSHIFT)
#define C_CURSOR1PIC       (11 + GFXSHIFT)
#define C_CURSOR2PIC       (12 + GFXSHIFT)
#define C_NOTSELECTEDPIC   (13 + GFXSHIFT)
#define C_SELECTEDPIC      (14 + GFXSHIFT)
#define C_FXTITLEPIC       (15 + GFXSHIFT)
#define C_DIGITITLEPIC     (16 + GFXSHIFT)
#define C_MUSICTITLEPIC    (17 + GFXSHIFT)
#define C_MOUSELBACKPIC    (18 + GFXSHIFT)
#define C_BABYMODEPIC      (19 + GFXSHIFT)
#define C_EASYPIC          (20 + GFXSHIFT)
#define C_NORMALPIC        (21 + GFXSHIFT)
#define C_HARDPIC          (22 + GFXSHIFT)
#define C_LOADSAVEDISKPIC  (23 + GFXSHIFT)
#define C_DISKLOADING1PIC  (24 + GFXSHIFT)
#define C_DISKLOADING2PIC  (25 + GFXSHIFT)
#define C_CONTROLPIC       (26 + GFXSHIFT)
#define C_CUSTOMIZEPIC     (27 + GFXSHIFT)
#define C_LOADGAMEPIC      (28 + GFXSHIFT)
#define C_SAVEGAMEPIC      (29 + GFXSHIFT)
#define C_EPISODE1PIC      (30 + GFXSHIFT)
#define C_EPISODE2PIC      (31 + GFXSHIFT)
#define C_EPISODE3PIC      (32 + GFXSHIFT)
#define C_EPISODE4PIC      (33 + GFXSHIFT)
#define C_EPISODE5PIC      (34 + GFXSHIFT)
#define C_EPISODE6PIC      (35 + GFXSHIFT)
#define C_CODEPIC          (36 + GFXSHIFT)
#define C_TIMECODEPIC      (37 + GFXSHIFT)
#define C_LEVELPIC         (38 + GFXSHIFT)
#define C_NAMEPIC          (39 + GFXSHIFT)
#define C_SCOREPIC         (40 + GFXSHIFT)
#define C_JOY1PIC          (41 + GFXSHIFT)
#define C_JOY2PIC          (42 + GFXSHIFT)
#define L_GUYPIC           (43 + GFXSHIFT)
#define L_COLONPIC         (44 + GFXSHIFT)
#define L_NUM0PIC          (45 + GFXSHIFT)
#define L_NUM1PIC          (46 + GFXSHIFT)
#define L_NUM2PIC          (47 + GFXSHIFT)
#define L_NUM3PIC          (48 + GFXSHIFT)
#define L_NUM4PIC          (49 + GFXSHIFT)
#define L_NUM5PIC          (50 + GFXSHIFT)
#define L_NUM6PIC          (51 + GFXSHIFT)
#define L_NUM7PIC          (52 + GFXSHIFT)
#define L_NUM8PIC          (53 + GFXSHIFT)
#define L_NUM9PIC          (54 + GFXSHIFT)
#define L_PERCENTPIC       (55 + GFXSHIFT)
#define L_APIC             (56 + GFXSHIFT)
#define L_BPIC             (57 + GFXSHIFT)
#define L_CPIC             (58 + GFXSHIFT)
#define L_DPIC             (59 + GFXSHIFT)
#define L_EPIC             (60 + GFXSHIFT)
#define L_FPIC             (61 + GFXSHIFT)
#define L_GPIC             (62 + GFXSHIFT)
#define L_HPIC             (63 + GFXSHIFT)
#define L_IPIC             (64 + GFXSHIFT)
#define L_JPIC             (65 + GFXSHIFT)
#define L_KPIC             (66 + GFXSHIFT)
#define L_LPIC             (67 + GFXSHIFT)
#define L_MPIC             (68 + GFXSHIFT)
#define L_NPIC             (69 + GFXSHIFT)
#define L_OPIC             (70 + GFXSHIFT)
#define L_PPIC             (71 + GFXSHIFT)
#define L_QPIC             (72 + GFXSHIFT)
#define L_RPIC             (73 + GFXSHIFT)
#define L_SPIC             (74 + GFXSHIFT)
#define L_TPIC             (75 + GFXSHIFT)
#define L_UPIC             (76 + GFXSHIFT)
#define L_VPIC             (77 + GFXSHIFT)
#define L_WPIC             (78 + GFXSHIFT)
#define L_XPIC             (79 + GFXSHIFT)
#define L_YPIC             (80 + GFXSHIFT)
#define L_ZPIC             (81 + GFXSHIFT)
#define L_EXPOINTPIC       (82 + GFXSHIFT)
#define L_APOSTROPHEPIC    (83 + GFXSHIFT)
#define L_GUY2PIC          (84 + GFXSHIFT)
#define L_BJWINSPIC        (85 + GFXSHIFT)
#define STATUSBARPIC       (86 + GFXSHIFT)
#define TITLEPIC           (87 + GFXSHIFT)
#define PG13PIC            (88 + GFXSHIFT)
#define CREDITSPIC         (89 + GFXSHIFT)
#define HIGHSCORESPIC      (90 + GFXSHIFT)
#define KNIFEPIC           (91 + GFXSHIFT)
#define GUNPIC             (92 + GFXSHIFT)
#define MACHINEGUNPIC      (93 + GFXSHIFT)
#define GATLINGGUNPIC      (94 + GFXSHIFT)
#define NOKEYPIC           (95 + GFXSHIFT)
#define GOLDKEYPIC         (96 + GFXSHIFT)
#define SILVERKEYPIC       (97 + GFXSHIFT)
#define N_BLANKPIC         (98 + GFXSHIFT)
#define N_0PIC             (99 + GFXSHIFT)
#define N_1PIC             (100 + GFXSHIFT)
#define N_2PIC             (101 + GFXSHIFT)
#define N_3PIC             (102 + GFXSHIFT)
#define N_4PIC             (103 + GFXSHIFT)
#define N_5PIC             (104 + GFXSHIFT)
#define N_6PIC             (105 + GFXSHIFT)
#define N_7PIC             (106 + GFXSHIFT)
#define N_8PIC             (107 + GFXSHIFT)
#define N_9PIC             (108 + GFXSHIFT)
#define FACE1APIC          (109 + GFXSHIFT)
#define FACE1BPIC          (110 + GFXSHIFT)
#define FACE1CPIC          (111 + GFXSHIFT)
#define FACE2APIC          (112 + GFXSHIFT)
#define FACE2BPIC          (113 + GFXSHIFT)
#define FACE2CPIC          (114 + GFXSHIFT)
#define FACE3APIC          (115 + GFXSHIFT)
#define FACE3BPIC          (116 + GFXSHIFT)
#define FACE3CPIC          (117 + GFXSHIFT)
#define FACE4APIC          (118 + GFXSHIFT)
#define FACE4BPIC          (119 + GFXSHIFT)
#define FACE4CPIC          (120 + GFXSHIFT)
#define FACE5APIC          (121 + GFXSHIFT)
#define FACE5BPIC          (122 + GFXSHIFT)
#define FACE5CPIC          (123 + GFXSHIFT)
#define FACE6APIC          (124 + GFXSHIFT)
#define FACE6BPIC          (125 + GFXSHIFT)
#define FACE6CPIC          (126 + GFXSHIFT)
#define FACE7APIC          (127 + GFXSHIFT)
#define FACE7BPIC          (128 + GFXSHIFT)
#define FACE7CPIC          (129 + GFXSHIFT)
#define FACE8APIC          (130 + GFXSHIFT)
#define GOTGATLINGPIC      (131 + GFXSHIFT)
#define MUTANTBJPIC        (132 + GFXSHIFT)
#define PAUSEDPIC          (133 + GFXSHIFT)
#define GETPSYCHEDPIC      (134 + GFXSHIFT)
#define ORDERSCREEN        (136 + GFXSHIFT)
#define ERRORSCREEN        (137 + GFXSHIFT)
#define T_HELPART          (138 + GFXSHIFT)
#define T_DEMO0            (139 + GFXSHIFT)
#define T_DEMO1            (140 + GFXSHIFT)
#define T_DEMO2            (141 + GFXSHIFT)
#define T_DEMO3            (142 + GFXSHIFT)
#define T_ENDART1          (143 + GFXSHIFT)
#define T_ENDART2          (144 + GFXSHIFT)
#define T_ENDART3          (145 + GFXSHIFT)
#define T_ENDART4          (146 + GFXSHIFT)
#define T_ENDART5          (147 + GFXSHIFT)
#define T_ENDART6          (148 + GFXSHIFT)

/* Attract-mode demos (PlayDemo): byte map, word length incl. 4-byte header,
 * one pad byte, then 3 bytes per tic (buttons, dx, dy). */
#define NUMDEMOS        4

/* Original status strip height (WL_DEF.H). */
#define STATUSLINES 40

typedef struct {
    int16_t width;
    int16_t height;
} pictabletype;
