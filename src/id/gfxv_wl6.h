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
#define TITLEPIC      87
#define CREDITSPIC    89
#define STATUSBARPIC  86
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

typedef struct {
    int16_t width;
    int16_t height;
} pictabletype;
