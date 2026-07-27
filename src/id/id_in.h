#pragma once

#include "wl_types.h"

#define NumCodes 128

extern boolean Keyboard[NumCodes];
extern boolean MousePresent;
extern boolean JoysPresent[2];

void IN_Startup(void);
void IN_Shutdown(void);
void IN_ClearKeysDown(void);
void IN_WaitKey(void);
boolean IN_KeyDown(int scancode);
void IN_ReadControl(int player, void *info);
boolean IN_UserInput(longword delay);
longword IN_GetTimeCount(void);
void IN_Ack(void);
