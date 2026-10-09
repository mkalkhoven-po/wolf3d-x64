/*
 * The OPL2 emulator against the YM3812 data sheet: pitch from fnum, block
 * and MULT, envelope times, waveforms, total level and key scale level in
 * dB, FM and feedback, key off.
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "opl2.h"

static int failures;

#define CHECK(cond)                                                                    \
    do {                                                                               \
        if (!(cond)) {                                                                 \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);            \
            failures++;                                                                \
        }                                                                              \
    } while (0)

static opl2_t chip;

/* Channel 0 as a plain carrier: the modulator never attacks (AR 0), additive. */
static void plain_tone(int fnum, int block, int mult)
{
    OPL2_Reset(&chip);
    OPL2_Write(&chip, 0x01, 0x20);       /* WSE */
    OPL2_Write(&chip, 0x20, 0x20);       /* modulator: sustained, MULT 1 */
    OPL2_Write(&chip, 0x60, 0x00);       /* modulator: AR 0 never sounds */
    OPL2_Write(&chip, 0x23, 0x20 | mult); /* carrier: sustained */
    OPL2_Write(&chip, 0x43, 0x00);       /* full level */
    OPL2_Write(&chip, 0x63, 0xf0);       /* instant attack, no decay */
    OPL2_Write(&chip, 0x83, 0x00);
    OPL2_Write(&chip, 0xc0, 0x01);       /* additive */
    OPL2_Write(&chip, 0xa0, fnum & 0xff);
    OPL2_Write(&chip, 0xb0, 0x20 | (block << 2) | (fnum >> 8));
}

typedef struct {
    int crossings; /* sign changes */
    int peak;
    int min;
} stats_t;

static stats_t run(int samples)
{
    stats_t st = {0, 0, 0};
    int i, prev = 0;

    for (i = 0; i < samples; i++) {
        const int s = (int)OPL2_Sample(&chip);

        if ((s > 0 && prev < 0) || (s < 0 && prev > 0)) {
            st.crossings++;
        }
        if (s) {
            prev = s;
        }
        if (abs(s) > st.peak) {
            st.peak = abs(s);
        }
        if (s < st.min) {
            st.min = s;
        }
    }
    return st;
}

static double hz(int fnum, int block, double mult)
{
    return fnum * (double)OPL2_RATE / (double)(1 << (20 - block)) * mult;
}

static void test_silence(void)
{
    OPL2_Reset(&chip);
    CHECK(run(OPL2_RATE / 10).peak == 0);
}

static void test_pitch(void)
{
    stats_t st;

    plain_tone(577, 4, 1); /* 437.7 Hz */
    st = run(OPL2_RATE);
    printf("pitch: %d crossings for %.1f Hz, peak %d\n", st.crossings, hz(577, 4, 1),
           st.peak);
    CHECK(abs(st.crossings - (int)lround(2 * hz(577, 4, 1))) <= 2);
    CHECK(st.peak >= 4000 && st.peak <= 4100); /* a full-scale operator */

    plain_tone(577, 5, 1); /* an octave up */
    st = run(OPL2_RATE);
    CHECK(abs(st.crossings - (int)lround(2 * hz(577, 5, 1))) <= 2);

    plain_tone(577, 4, 3); /* MULT 3 */
    st = run(OPL2_RATE);
    CHECK(abs(st.crossings - (int)lround(2 * hz(577, 4, 3))) <= 2);

    plain_tone(577, 4, 0); /* MULT 0 is half */
    st = run(OPL2_RATE);
    CHECK(abs(st.crossings - (int)lround(2 * hz(577, 4, 0.5))) <= 2);
}

/* samples until the carrier's envelope reaches level (or the attack ends at 0) */
static int samples_until(int level, int falling, int skip)
{
    int n = 0;

    run(skip); /* let an instant attack land first */

    while (n < 60 * OPL2_RATE) {
        const int env = chip.ch[0].op[1].env;
        if (falling ? env >= level : env <= level) {
            break;
        }
        OPL2_Sample(&chip);
        n++;
    }
    return n;
}

static void test_envelope_times(void)
{
    double ms;

    /* decay at DR 4: the data sheet gives 0-96 dB in 39280 ms / 8 */
    plain_tone(577, 0, 1);
    OPL2_Write(&chip, 0x63, 0xf4);
    OPL2_Write(&chip, 0x83, 0xf0); /* SL 15: decay all the way */
    OPL2_Write(&chip, 0xb0, 0x00);
    OPL2_Write(&chip, 0xb0, 0x20 | (577 >> 8));
    ms = samples_until(496, 1, 2) * 1000.0 / OPL2_RATE * 512.0 / 496.0;
    printf("decay DR 4: 0-96 dB in %.0f ms (data sheet %.0f)\n", ms, 39280.0 / 8);
    CHECK(fabs(ms - 39280.0 / 8) < 39280.0 / 8 * 0.10);

    /* release at RR 6 after key off, from full level */
    plain_tone(577, 0, 1);
    OPL2_Write(&chip, 0x83, 0x06);
    run(100);
    OPL2_Write(&chip, 0xb0, 577 >> 8); /* key off */
    ms = samples_until(511, 1, 0) * 1000.0 / OPL2_RATE;
    printf("release RR 6: 0-96 dB in %.0f ms (data sheet %.0f)\n", ms, 39280.0 / 32);
    CHECK(fabs(ms - 39280.0 / 32) < 39280.0 / 32 * 0.10);

    /* attack at AR 4: the data sheet gives 2826 ms / 8 */
    plain_tone(577, 0, 1);
    OPL2_Write(&chip, 0x63, 0x40);
    OPL2_Write(&chip, 0xb0, 0x00);
    OPL2_Write(&chip, 0xb0, 0x20 | (577 >> 8));
    ms = samples_until(0, 0, 0) * 1000.0 / OPL2_RATE;
    printf("attack AR 4: %.0f ms (data sheet %.0f)\n", ms, 2826.0 / 8);
    CHECK(fabs(ms - 2826.0 / 8) < 2826.0 / 8 * 0.20);

    /* key scale rate: the same DR is faster at a higher block */
    {
        int slow, fast;

        plain_tone(577, 0, 1);
        OPL2_Write(&chip, 0x23, 0x30); /* KSR on */
        OPL2_Write(&chip, 0x63, 0xf4);
        OPL2_Write(&chip, 0x83, 0xf0);
        OPL2_Write(&chip, 0xb0, 0x00);
        OPL2_Write(&chip, 0xb0, 0x20 | (577 >> 8));
        slow = samples_until(256, 1, 2);
        plain_tone(577, 7, 1);
        OPL2_Write(&chip, 0x23, 0x30);
        OPL2_Write(&chip, 0x63, 0xf4);
        OPL2_Write(&chip, 0x83, 0xf0);
        OPL2_Write(&chip, 0xb0, 0x00);
        OPL2_Write(&chip, 0xb0, 0x20 | (7 << 2) | (577 >> 8));
        fast = samples_until(256, 1, 2);
        CHECK(fast * 8 < slow);
    }

    /* a percussive sound (EGT 0) fades after the decay, a sustained one holds */
    plain_tone(577, 4, 1);
    OPL2_Write(&chip, 0x23, 0x01); /* EGT off */
    OPL2_Write(&chip, 0x83, 0x58); /* SL 5, RR 8 */
    OPL2_Write(&chip, 0x63, 0xf8);
    OPL2_Write(&chip, 0xb0, 0x00);
    OPL2_Write(&chip, 0xb0, 0x20 | (4 << 2) | (577 >> 8));
    run(OPL2_RATE);
    CHECK(chip.ch[0].op[1].env == 511);
    plain_tone(577, 4, 1);
    OPL2_Write(&chip, 0x83, 0x58);
    OPL2_Write(&chip, 0x63, 0xf8);
    OPL2_Write(&chip, 0xb0, 0x00);
    OPL2_Write(&chip, 0xb0, 0x20 | (4 << 2) | (577 >> 8));
    run(OPL2_RATE);
    CHECK(chip.ch[0].op[1].env >= 80 && chip.ch[0].op[1].env < 96); /* held at SL 5 */
}

static void test_key_off(void)
{
    plain_tone(577, 4, 1);
    OPL2_Write(&chip, 0x83, 0x0f); /* fast release */
    run(1000);
    OPL2_Write(&chip, 0xb0, (4 << 2) | (577 >> 8));
    run(OPL2_RATE / 10);
    CHECK(run(1000).peak == 0);
}

static void test_levels(void)
{
    int full, tl8, ksl0, ksl3;

    plain_tone(577, 4, 1);
    full = run(OPL2_RATE / 10).peak;
    plain_tone(577, 4, 1);
    OPL2_Write(&chip, 0x43, 8); /* TL 8: 6 dB */
    tl8 = run(OPL2_RATE / 10).peak;
    printf("levels: full %d, TL 8 %d\n", full, tl8);
    CHECK(fabs((double)tl8 / full - 0.5) < 0.02);

    plain_tone(1023, 7, 1);
    ksl0 = run(OPL2_RATE / 10).peak;
    plain_tone(1023, 7, 1);
    OPL2_Write(&chip, 0x43, 0xc0); /* KSL 3: 6 dB/octave, 42 dB at the top */
    ksl3 = run(OPL2_RATE / 10).peak;
    printf("levels: KSL 0 %d, KSL 3 %d\n", ksl0, ksl3);
    CHECK(ksl3 * 100 < ksl0 && ksl3 * 200 > ksl0);
}

static void test_waveforms(void)
{
    stats_t sine, st;

    plain_tone(577, 4, 1);
    sine = run(OPL2_RATE / 10);
    CHECK(sine.min < -4000);

    plain_tone(577, 4, 1);
    OPL2_Write(&chip, 0xe3, 1); /* half sine */
    st = run(OPL2_RATE / 10);
    CHECK(st.min == 0 && st.peak > 4000);

    plain_tone(577, 4, 1);
    OPL2_Write(&chip, 0xe3, 2); /* absolute sine */
    st = run(OPL2_RATE / 10);
    CHECK(st.min == 0 && st.peak > 4000);

    plain_tone(577, 4, 1);
    OPL2_Write(&chip, 0xe3, 3); /* quarter sine */
    st = run(OPL2_RATE / 10);
    CHECK(st.min == 0 && st.peak > 4000);

    plain_tone(577, 4, 1);
    OPL2_Write(&chip, 0x01, 0x00); /* WSE off: always a sine */
    OPL2_Write(&chip, 0xe3, 1);
    st = run(OPL2_RATE / 10);
    CHECK(st.min < -4000);
}

static void test_fm_and_feedback(void)
{
    stats_t pure, fm, fb;

    plain_tone(577, 4, 1);
    pure = run(OPL2_RATE);

    /* FM: a full-level modulator at MULT 3 adds partials, more sign changes */
    plain_tone(577, 4, 1);
    OPL2_Write(&chip, 0x20, 0x23);
    OPL2_Write(&chip, 0x60, 0xf0);
    OPL2_Write(&chip, 0x40, 0x10);
    OPL2_Write(&chip, 0xc0, 0x00);
    fm = run(OPL2_RATE);
    printf("fm: %d crossings (pure %d), peak %d\n", fm.crossings, pure.crossings, fm.peak);
    CHECK(fm.crossings > pure.crossings * 2);
    CHECK(fm.peak <= 4100);

    /* feedback 7 on the modulator alone: a saw-like wave, same period */
    plain_tone(577, 4, 1);
    OPL2_Write(&chip, 0x40, 0x00);
    OPL2_Write(&chip, 0x60, 0xf0);
    OPL2_Write(&chip, 0x63, 0x00); /* carrier never sounds */
    OPL2_Write(&chip, 0xc0, 0x0f); /* FB 7, additive */
    fb = run(OPL2_RATE);
    printf("feedback 7: %d crossings, peak %d\n", fb.crossings, fb.peak);
    CHECK(fb.peak > 2000 && fb.peak <= 4100);
    CHECK(fb.crossings >= pure.crossings);
}

/* Writes to every register stay inside the chip (0x16-0x1F of each group are holes). */
static void test_all_registers(void)
{
    static struct {
        opl2_t chip;
        unsigned char guard[256];
    } box;
    int reg, i, clean = 1;

    OPL2_Reset(&box.chip);
    memset(box.guard, 0, sizeof(box.guard));
    for (reg = 0; reg < 256; reg++) {
        OPL2_Write(&box.chip, reg, 0xff);
    }
    for (i = 0; i < (int)sizeof(box.guard); i++) {
        clean &= box.guard[i] == 0;
    }
    CHECK(clean);
    CHECK(box.chip.ch[8].op[1].mult == 15 && box.chip.ch[8].op[1].ws == 3);
    for (i = 0; i < 1000; i++) {
        OPL2_Sample(&box.chip);
    }
}

static void test_determinism(void)
{
    unsigned long h1 = 5381, h2 = 5381;
    int i;

    plain_tone(345, 3, 2);
    OPL2_Write(&chip, 0x20, 0xe2); /* tremolo, vibrato */
    OPL2_Write(&chip, 0xbd, 0xc0);
    OPL2_Write(&chip, 0xc0, 0x06);
    for (i = 0; i < 20000; i++) {
        h1 = h1 * 33 + (unsigned long)OPL2_Sample(&chip);
    }
    plain_tone(345, 3, 2);
    OPL2_Write(&chip, 0x20, 0xe2);
    OPL2_Write(&chip, 0xbd, 0xc0);
    OPL2_Write(&chip, 0xc0, 0x06);
    for (i = 0; i < 20000; i++) {
        h2 = h2 * 33 + (unsigned long)OPL2_Sample(&chip);
    }
    CHECK(h1 == h2);
}

int main(void)
{
    test_silence();
    test_pitch();
    test_envelope_times();
    test_key_off();
    test_levels();
    test_waveforms();
    test_fm_and_feedback();
    test_all_registers();
    test_determinism();

    if (failures) {
        fprintf(stderr, "%d check(s) failed\n", failures);
        return 1;
    }
    printf("test_opl2: all checks passed\n");
    return 0;
}
