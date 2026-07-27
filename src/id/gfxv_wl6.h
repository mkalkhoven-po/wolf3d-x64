#pragma once

#include <stdint.h>

/* WL6 VGA graph chunk IDs (from GFXV_WL6.H). */

#define STRUCTPIC     0
#define STARTFONT     1
#define STARTPICS     3
#define TITLEPIC      87
#define CREDITSPIC    89
#define C_OPTIONSPIC  10
#define C_CURSOR1PIC  11
#define C_CURSOR2PIC  12
#define C_MOUSELBACKPIC 18
#define STATUSBARPIC  86
#define GETPSYCHEDPIC 134

#define NUMCHUNKS     149
#define NUMPICS       132
#define STARTTILE8    135
#define STARTEXTERNS  136

typedef struct {
    int16_t width;
    int16_t height;
} pictabletype;
