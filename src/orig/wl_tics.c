#include "wl_tics.h"

void TicClock_Init(tic_clock_t *clock, uint64_t freq, uint64_t now)
{
    clock->freq = freq ? freq : 1;
    clock->base = now;
    clock->offset = 0;
}

longword TicClock_Read(const tic_clock_t *clock, uint64_t now)
{
    const uint64_t elapsed = now >= clock->base ? now - clock->base : 0;
    /* Split so elapsed * TICRATE cannot overflow for any realistic counter. */
    const uint64_t tics = (elapsed / clock->freq) * TICRATE +
                          (elapsed % clock->freq) * TICRATE / clock->freq;
    return clock->offset + (longword)tics;
}

void TicClock_Set(tic_clock_t *clock, longword count, uint64_t now)
{
    clock->base = now;
    clock->offset = count;
}
