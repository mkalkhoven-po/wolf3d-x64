#pragma once

#include <stdint.h>

/* WL6 VGA graph chunk IDs (from GFXV_WL6.H). */

#define STRUCTPIC     0
#define STARTFONT     1
#define STARTPICS     3

#define C_OPTIONSPIC  10
#define C_CURSOR1PIC  11
#define C_CURSOR2PIC  12
#define C_NOTSELECTEDPIC 13
#define C_SELECTEDPIC 14
#define C_MOUSELBACKPIC 18

#define STATUSBARPIC  86
#define TITLEPIC      87
#define PG13PIC       88
#define CREDITSPIC    89
#define HIGHSCORESPIC 90

#define KNIFEPIC      91
#define GUNPIC        92
#define MACHINEGUNPIC 93
#define GATLINGGUNPIC 94
#define NOKEYPIC      95
#define GOLDKEYPIC    96
#define SILVERKEYPIC  97
#define N_BLANKPIC    98
#define N_0PIC        99
#define N_1PIC        100
#define N_2PIC        101
#define N_3PIC        102
#define N_4PIC        103
#define N_5PIC        104
#define N_6PIC        105
#define N_7PIC        106
#define N_8PIC        107
#define N_9PIC        108

#define FACE1APIC     109
#define FACE1BPIC     110
#define FACE1CPIC     111
#define FACE2APIC     112
#define FACE8APIC     130
#define GOTGATLINGPIC 131
#define MUTANTBJPIC   132
#define PAUSEDPIC     133
#define GETPSYCHEDPIC 134

#define NUMCHUNKS     149
#define NUMPICS       132
#define STARTTILE8    135
#define STARTEXTERNS  136

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
