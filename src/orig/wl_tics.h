#pragma once

#include "orig_def.h"

/*
 * 70 Hz TimeCount (ID_SD.C timer ISR), driven by a monotonic counter such as
 * SDL_GetPerformanceCounter. The clock holds no platform calls so it can be
 * tested with synthetic counter values.
 *
 * The DOS PIT ran slightly fast (about 70.07-70.10 Hz depending on the sound
 * mode); this clock runs at exactly TICRATE. Wall-clock speed does not affect
 * demo playback, which advances a fixed number of tics per frame.
 */
typedef struct {
    uint64_t freq;   /* counter ticks per second, > 0 */
    uint64_t base;   /* counter value at which count == offset */
    longword offset; /* TimeCount at base */
} tic_clock_t;

void TicClock_Init(tic_clock_t *clock, uint64_t freq, uint64_t now);

/* TimeCount at counter value now (now >= base). Wraps like the 32-bit original. */
longword TicClock_Read(const tic_clock_t *clock, uint64_t now);

/* TimeCount = count, as the original code does when it resets the clock. */
void TicClock_Set(tic_clock_t *clock, longword count, uint64_t now);
