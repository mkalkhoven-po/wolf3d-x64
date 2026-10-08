#include <stdio.h>

#include "id_us_rnd.h"
#include "wl_fixed.h"
#include "wl_tables.h"
#include "wl_tics.h"

static int g_failures;

#define CHECK(cond)                                                          \
    do {                                                                     \
        if (!(cond)) {                                                       \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);  \
            g_failures++;                                                    \
        }                                                                    \
    } while (0)

#define SIGNBIT ((fixed)0x80000000u)

static uint32_t fnv1a(const void *data, size_t n)
{
    const unsigned char *p = (const unsigned char *)data;
    uint32_t h = 2166136261u;
    size_t i;

    for (i = 0; i < n; i++) {
        h ^= p[i];
        h *= 16777619u;
    }
    return h;
}

static void test_fixed_by_frac(void)
{
    const fixed half = 0x8000;

    CHECK(FixedByFrac(3 * GLOBAL1, half) == 3 * GLOBAL1 / 2);
    CHECK(FixedByFrac(-3 * GLOBAL1, half) == -3 * GLOBAL1 / 2);
    /* sign-magnitude fraction */
    CHECK(FixedByFrac(3 * GLOBAL1, half | SIGNBIT) == -3 * GLOBAL1 / 2);
    CHECK(FixedByFrac(-3 * GLOBAL1, half | SIGNBIT) == 3 * GLOBAL1 / 2);
    /* magnitude is truncated: negative results round towards zero, not down */
    CHECK(FixedByFrac(3, half) == 1);
    CHECK(FixedByFrac(-3, half) == -1);
    CHECK(FixedByFrac(3, half | SIGNBIT) == -1);
    CHECK(FixedByFrac(-1, half) == 0);
    /* bits 16-30 of the fraction are ignored, so 1.0 multiplies by zero */
    CHECK(FixedByFrac(5 * GLOBAL1, 0x00018000) == FixedByFrac(5 * GLOBAL1, half));
    CHECK(FixedByFrac(5 * GLOBAL1, GLOBAL1) == 0);
    CHECK(FixedByFrac(5 * GLOBAL1, 0xffff) == 5 * GLOBAL1 - 5);
    /* largest magnitude */
    CHECK(FixedByFrac((fixed)0x80000000u, 0xffff) == -0x7fff8000);
    CHECK(FixedByFrac(0x7fffffff, 0xffff) == 0x7fff7fff);
}

static void test_fixed_by_frac_sweep(void)
{
    /* Against the original's two-step word multiply, written out directly. */
    uint32_t s = 12345u;
    int i;

    for (i = 0; i < 100000; i++) {
        uint32_t a, b, mag, hi, lo, frac, r;
        int neg;

        s = s * 1103515245u + 12345u;
        a = s;
        s = s * 1103515245u + 12345u;
        b = s;

        neg = (b & 0x80000000u) != 0;
        mag = a;
        if (a & 0x80000000u) {
            mag = 0u - a;
            neg = !neg;
        }
        hi = mag >> 16;
        lo = mag & 0xffffu;
        frac = b & 0xffffu;
        r = hi * frac + ((lo * frac) >> 16);
        if (neg) {
            r = 0u - r;
        }
        CHECK(FixedByFrac((fixed)a, (fixed)b) == (fixed)r);
        if (g_failures) {
            return;
        }
    }
}

static void test_tables(void)
{
    int i;

    BuildTables();

    /* Float accumulation of the angle makes these one below the exact value. */
    CHECK(sintable[0] == 0);
    CHECK(sintable[1] == 0x477);
    CHECK(sintable[30] == 0x7fff);
    CHECK(sintable[45] == 0xb504);
    CHECK(sintable[90] == 0xffff);
    CHECK(costable[0] == 0xffff);
    CHECK(costable[90] == SIGNBIT); /* sintable[180], see below */

    /* 180 and 360 end up as negative zero (written last with the sign bit) */
    CHECK(sintable[180] == SIGNBIT);
    CHECK(sintable[360] == SIGNBIT);
    CHECK(sintable[270] == (SIGNBIT | 0xffff));
    CHECK(sintable[450] == 0xffff);

    for (i = 0; i <= ANGLEQUAD; i++) {
        if (i > 0) {
            CHECK(sintable[ANGLES / 2 - i] == sintable[i]);
        }
        CHECK(sintable[ANGLES / 2 + i] == (sintable[i] | SIGNBIT));
        CHECK(((uint32_t)sintable[i] & 0x7fff0000u) == 0);
    }
    for (i = 1; i <= ANGLEQUAD; i++) {
        CHECK(sintable[i + ANGLES] == sintable[i]);
    }

    CHECK(finetangent[0] == 57);
    CHECK(finetangent[1] == 171);
    CHECK(finetangent[449] == 65421);
    CHECK(finetangent[450] == 65650);
    CHECK(finetangent[899] == 75098708);
    for (i = 1; i < FINEANGLES / 4; i++) {
        CHECK(finetangent[i] > finetangent[i - 1]);
    }

    /* Whole-table fingerprints; a compiler or libm that rounds differently fails here. */
    CHECK(fnv1a(sintable, sizeof(sintable)) == 0x5bea6c8eu);
    CHECK(fnv1a(finetangent, sizeof(finetangent)) == 0x7491d46du);
}

static void test_rnd(void)
{
    int i, first;

    CHECK(rndtable[0] == 0 && rndtable[1] == 8 && rndtable[255] == 249);
    CHECK(fnv1a(rndtable, sizeof(rndtable)) == 0xac34fbe5u);

    /* the index is advanced before the read: the first value is rndtable[1] */
    US_InitRndT(false);
    CHECK(US_RndT() == 8);
    CHECK(US_RndT() == 109);
    CHECK(US_RndT() == 220);

    US_InitRndT(false);
    for (i = 1; i < 256; i++) {
        CHECK(US_RndT() == rndtable[i]);
    }
    CHECK(US_RndT() == rndtable[0]); /* wraps */

    US_InitRndT(true);
    first = US_RndT();
    CHECK(first >= 0 && first <= 255);
}

static void test_tic_clock(void)
{
    tic_clock_t c;
    const uint64_t qpc = 10000000u; /* typical QueryPerformanceFrequency */
    const uint64_t start = 123456789u;

    TicClock_Init(&c, 1000, 0);
    CHECK(TicClock_Read(&c, 0) == 0);
    CHECK(TicClock_Read(&c, 14) == 0);
    CHECK(TicClock_Read(&c, 15) == 1);
    CHECK(TicClock_Read(&c, 999) == 69);
    CHECK(TicClock_Read(&c, 1000) == 70);

    /* a year of uptime at 10 MHz does not overflow or drift */
    TicClock_Init(&c, qpc, start);
    CHECK(TicClock_Read(&c, start + qpc * 3600u * 24u * 365u) ==
          (longword)(70u * 3600u * 24u * 365u));

    /* TimeCount = 500, then one more second */
    TicClock_Set(&c, 500, start + 42);
    CHECK(TicClock_Read(&c, start + 42) == 500);
    CHECK(TicClock_Read(&c, start + 42 + qpc) == 570);

    /* a counter behind base reads as base, never as a huge value */
    CHECK(TicClock_Read(&c, start) == 500);
}

int main(void)
{
    test_fixed_by_frac();
    test_fixed_by_frac_sweep();
    test_tables();
    test_rnd();
    test_tic_clock();

    if (g_failures) {
        fprintf(stderr, "%d check(s) failed\n", g_failures);
        return 1;
    }
    printf("test_orig_core: all checks passed\n");
    return 0;
}
