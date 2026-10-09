#pragma once

#include "wl_types.h"

/*
 * What is left of the legacy input manager: a millisecond-based tick count
 * for SD_GetTimeCount. The game's input is the faithful engine's (orig_keys,
 * PollControls, IN_UserInput through the runner).
 */

void IN_Startup(void);
void IN_Shutdown(void);
longword IN_GetTimeCount(void);
