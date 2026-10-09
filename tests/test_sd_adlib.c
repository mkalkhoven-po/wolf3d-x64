/*
 * The AdLib half of the sound manager (sd_adlib.c): effects stepped at
 * 140 Hz on channel 0 and ending by themselves, the 700 Hz music sequencer
 * with its delays and loop, music off. With --data DIR: every AdLib effect
 * and song in the pack's AUDIOT.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "audio_wl6.h"
#include "sd_adlib.h"
#include "wl_data.h"

static int failures;

#define CHECK(cond)                                                                    \
    do {                                                                               \
        if (!(cond)) {                                                                 \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);            \
            failures++;                                                                \
        }                                                                              \
    } while (0)

static int32_t buf[SDA_RATE];
static long g_ticks; /* 700 Hz ticks rendered since SDA_Startup */

static void startup(void)
{
    SDA_Startup();
    g_ticks = 0;
}

/* Render until t more timer ticks have run; returns the largest |sample|. */
static int32_t ticks(long t)
{
    int32_t peak = 0;
    /* tick k runs before the sample that reaches k * SDA_RATE / 700 */
    long from = (g_ticks * SDA_RATE + SDA_TICKRATE - 1) / SDA_TICKRATE;
    long to = ((g_ticks + t) * SDA_RATE + SDA_TICKRATE - 1) / SDA_TICKRATE;

    g_ticks += t;
    while (from < to) {
        const int n = to - from > SDA_RATE ? SDA_RATE : (int)(to - from);
        int i;

        SDA_Render(buf, n);
        for (i = 0; i < n; i++) {
            const int32_t a = buf[i] < 0 ? -buf[i] : buf[i];
            if (a > peak) {
                peak = a;
            }
        }
        from += n;
    }
    return peak;
}

static const opl2_ch_t *ch(int c)
{
    return &SDA_Chip()->ch[c];
}

static void test_effect(void)
{
    /* AdLibSound: length 4, priority 7, an instrument, block 3, four bytes */
    static const byte sound[] = {
        4,    0,    0,    0,    7,    0,                         /* SoundCommon */
        0x21, 0x21, 0x10, 0x00, 0xf0, 0xf0, 0x0f, 0x0f, 0, 0, 0, /* Instrument */
        0,    0,    0,    0,    0,                               /* (Muse bytes) */
        3,                                                       /* block */
        100,  0,    150,  200,                                   /* data */
    };
    byte bad[sizeof(sound)];

    startup();
    CHECK(SDA_PlaySound(sound, sizeof(sound), 5, 7));
    CHECK(SDA_SoundPlaying() == 5 && SDA_SoundPriority() == 7);
    CHECK(ch(0)->op[0].mult == 1 && ch(0)->op[0].tl == 0x10 && ch(0)->op[1].sl == 0);

    ticks(4); /* effects run on every fifth tick */
    CHECK(!ch(0)->keyon);
    ticks(1);
    CHECK(ch(0)->keyon && ch(0)->fnum == 100 && ch(0)->block == 3);
    CHECK(ticks(5) > 1000); /* sounding until the zero byte */
    CHECK(!ch(0)->keyon);
    ticks(5);
    CHECK(ch(0)->keyon && ch(0)->fnum == 150);
    ticks(4);
    CHECK(SDA_SoundPlaying() == 5);
    ticks(1); /* the fourth byte is the last */
    CHECK(SDA_SoundPlaying() == 0 && SDA_SoundPriority() == 0);
    CHECK(!ch(0)->keyon);

    /* stopped early */
    CHECK(SDA_PlaySound(sound, sizeof(sound), 5, 7));
    ticks(5);
    SDA_StopSound();
    CHECK(SDA_SoundPlaying() == 0 && SDA_SoundPriority() == 0 && !ch(0)->keyon);

    /* "Bad instrument": no sustain on either operator */
    memcpy(bad, sound, sizeof(bad));
    bad[12] = bad[13] = 0;
    CHECK(!SDA_PlaySound(bad, sizeof(bad), 5, 7));
    /* a length past the chunk plays what is there */
    memcpy(bad, sound, sizeof(bad));
    bad[0] = 200;
    CHECK(SDA_PlaySound(bad, sizeof(bad), 6, 1));
    ticks(5 * 4);
    CHECK(SDA_SoundPlaying() == 0);
}

static void test_music(void)
{
    /* MusicGroup: 16 bytes of (register, value, delay) */
    static const byte song[] = {
        16,   0,                  /* length */
        0xa1, 0x40, 0, 0,         /* fnum low, no delay */
        0xb1, 0x2d, 3, 0,         /* key on, block 3; next event 3 ticks later */
        0xb1, 0x0d, 2, 0,         /* key off */
        0xbd, 0x00, 1, 0,         /* then the song loops */
    };
    static const byte broken[] = {200, 0, 0xa2, 0x10, 0, 0, 0xb2, 0x30, 9};

    startup();
    SDA_StartMusic(song, sizeof(song));
    CHECK(SDA_MusicActive());
    ticks(1); /* the first two events run at once */
    CHECK(ch(1)->keyon && ch(1)->fnum == 0x140 && ch(1)->block == 3);
    ticks(2);
    CHECK(ch(1)->keyon);
    ticks(1); /* 3 ticks after the key on */
    CHECK(!ch(1)->keyon);
    ticks(2); /* the last event, then the loop starts over */
    CHECK(!ch(1)->keyon);
    ticks(1);
    CHECK(ch(1)->keyon);

    SDA_MusicOff();
    CHECK(!SDA_MusicActive() && !ch(1)->keyon);
    ticks(20);
    CHECK(!ch(1)->keyon);

    /* a length past the chunk is cut to the whole events in it */
    SDA_StartMusic(broken, sizeof(broken));
    ticks(1);
    CHECK(ch(2)->fnum == 0x10 && !ch(2)->keyon);
    SDA_MusicOff();
}

/* ------------------------------------------------------------- real data */

static byte *read_file(const wolf_data_dir_t *dir, const char *name, long *size)
{
    char path[WOLF_PATH_MAX];
    FILE *f;
    byte *data;

    Wolf_MakeDataPath(dir, name, path, sizeof(path));
    f = fopen(path, "rb");
    if (!f) {
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    *size = ftell(f);
    fseek(f, 0, SEEK_SET);
    data = (byte *)malloc((size_t)*size);
    if (data && fread(data, 1, (size_t)*size, f) != (size_t)*size) {
        free(data);
        data = NULL;
    }
    fclose(f);
    return data;
}

static int run_pack(int argc, char **argv)
{
    wolf_data_dir_t dir;
    byte *hed, *aud;
    long hedsize = 0, audsize = 0;
    int n, audible = 0, longest = 0, absent = 0;

    memset(&dir, 0, sizeof(dir));
    if (!Wolf_ResolveDataDir(argc, argv, &dir)) {
        return 1;
    }
    hed = read_file(&dir, "AUDIOHED", &hedsize);
    aud = read_file(&dir, "AUDIOT", &audsize);
    CHECK(hed && aud);
    if (!hed || !aud) {
        return 1;
    }
    CHECK(hedsize / 4 >= STARTMUSIC + LASTMUSIC + 1);

#define START(c) ((long)(hed[(c) * 4] | (hed[(c) * 4 + 1] << 8) | ((long)hed[(c) * 4 + 2] << 16)))

    /* every AdLib effect plays for exactly length * 5 ticks */
    for (n = 0; n < NUMSOUNDS; n++) {
        const long start = START(STARTADLIBSOUNDS + n), end = START(STARTADLIBSOUNDS + n + 1);
        const byte *s = aud + start;
        long length;
        int32_t peak;

        CHECK(start >= 0 && end > start + 23 && end <= audsize);
        if (!(start >= 0 && end > start + 23 && end <= audsize)) {
            continue;
        }
        length = s[0] | (s[1] << 8) | ((long)s[2] << 16);
        CHECK(length > 0 && length <= end - start - 23);
        startup();
        if (!SDA_PlaySound(s, (size_t)(end - start), n, s[4] | (s[5] << 8))) {
            fprintf(stderr, "FAIL: AdLib sound %d did not play\n", n);
            failures++;
            continue;
        }
        peak = ticks(length * 5 - 1);
        CHECK(SDA_SoundPlaying() == n || (n == 0 && SDA_SoundPlaying() == 0));
        ticks(1);
        CHECK(SDA_SoundPlaying() == 0 && !ch(0)->keyon);
        if (peak > 500) {
            audible++;
        }
        if (length > longest) {
            longest = (int)length;
        }
    }
    printf("%s: %d AdLib effects, %d audible, longest %.2f s\n", dir.extension, NUMSOUNDS,
           audible, longest / 140.0);
    CHECK(audible >= NUMSOUNDS - 2);

    /*
     * Every song sounds within 5 s and stays inside 16 bits after OPL_GAIN.
     * Shareware leaves out the songs it never plays: an empty MusicGroup.
     */
    for (n = 0; n < LASTMUSIC; n++) {
        const long start = START(STARTMUSIC + n), end = START(STARTMUSIC + n + 1);
        int32_t peak;
        int c, keyed = 0;

        CHECK(start >= 0 && end > start + 2 && end <= audsize);
        if (!(start >= 0 && end > start + 2 && end <= audsize)) {
            continue;
        }
        if (!(aud[start] | aud[start + 1])) {
            printf("  music %2d: not in this pack\n", n);
            absent++;
            continue;
        }
        startup();
        SDA_StartMusic(aud + start, (size_t)(end - start));
        peak = ticks(SDA_TICKRATE * 5);
        for (c = 1; c < 9; c++) {
            keyed += ch(c)->keyon;
        }
        printf("  music %2d: peak %5d, %d channels keyed at 5 s\n", n, (int)peak, keyed);
        CHECK(peak > 500 && peak < 32768 / 2); /* OPL_GAIN in id_sd.c is 2x */
        CHECK(!ch(0)->keyon); /* channel 0 belongs to the effects */
    }
    /* shareware has the 11 songs of episode 1 and the menus; WL6 all 27 */
    CHECK(absent == (strcmp(dir.extension, ".WL1") == 0 ? 16 : 0));

    free(hed);
    free(aud);
    return 0;
}

int main(int argc, char **argv)
{
    test_effect();
    test_music();
    if (argc > 1 && run_pack(argc, argv)) {
        return 1;
    }

    if (failures) {
        fprintf(stderr, "%d check(s) failed\n", failures);
        return 1;
    }
    printf("test_sd_adlib: all checks passed\n");
    return 0;
}
