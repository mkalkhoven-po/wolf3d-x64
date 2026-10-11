#include "id_sd.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif
#include <SDL.h>

#include "id_in.h"
#include "id_pm.h"
#include "id_us.h"
#include "sd_adlib.h"

/*
 * The mixer runs at the OPL2's own rate; SDL converts to the device. The
 * AdLib side (music, AdLib-only effects) is sd_adlib.c; the digitized
 * effects play here on one Sound Blaster channel with the SB Pro's stereo
 * placement, as in the original.
 */
#define MIX_RATE SDA_RATE
#define MIX_SAMPLES 1024
#define MAX_DIGI 64
/* the SB time constant 256 - 1000000 / 7000 gives 1000000 / 142 Hz */
#define DIGI_RATE 7042
/* the chip sums nine channels of +-4084; x2 as in DOSBox, the usual reference for the mix */
#define OPL_GAIN (2.0f / 32768.0f)
#define DIGI_GAIN (1.0f / 128.0f)  /* 8-bit samples at full scale */

SDMode SoundMode = sdm_Off;
SMMode MusicMode = smm_Off;
SDSMode DigiMode = sds_Off;
boolean SD_Silent = false;
int DigiMap[LASTSOUND];

static boolean sd_started;
static boolean sd_upload; /* a shareware pack */
static SDL_AudioDeviceID audio_dev;
static SDL_mutex *audio_mx;

typedef struct {
    byte *data;
    int len;
} digi_sample_t;

static digi_sample_t digi[MAX_DIGI];
static int num_digi;

/* the digitized channel (SD_PlayDigitized, SDL_DigitizedDone) */
static const byte *digi_data;
static int digi_len;
static uint32_t digi_pos; /* 16.16 source samples */
static float digi_left, digi_right;
static int DigiNumber, DigiPriority;

/* SD_PositionSound for the next sound, and the position of the current one */
static int LeftPosition, RightPosition;
static boolean nextsoundpos, SoundPositioned;

static longword time_count_base;

#define LOCK()                                                                         \
    do {                                                                               \
        if (audio_mx) {                                                                \
            SDL_LockMutex(audio_mx);                                                   \
        }                                                                              \
    } while (0)
#define UNLOCK()                                                                       \
    do {                                                                               \
        if (audio_mx) {                                                                \
            SDL_UnlockMutex(audio_mx);                                                 \
        }                                                                              \
    } while (0)

/* ---- digi table from last VSWAP page ---- */

/*
 * wolfdigimap (WL_MAIN.C InitDigiMap): sound -> VSWAP digitized sample.
 * Sounds not listed play on the AdLib (pickups, treasure, menus...). The
 * shareware EXE was built with UPLOAD and maps only the first group.
 */
static void init_digimap(boolean upload)
{
    static const int upload_map[] = {
        HALTSND, 0, DOGBARKSND, 1, CLOSEDOORSND, 2, OPENDOORSND, 3,
        ATKMACHINEGUNSND, 4, ATKPISTOLSND, 5, ATKGATLINGSND, 6, SCHUTZADSND, 7,
        GUTENTAGSND, 8, MUTTISND, 9, BOSSFIRESND, 10, SSFIRESND, 11,
        DEATHSCREAM1SND, 12, DEATHSCREAM2SND, 13, DEATHSCREAM3SND, 13,
        TAKEDAMAGESND, 14, PUSHWALLSND, 15,
        LEBENSND, 20, NAZIFIRESND, 21, SLURPIESND, 22,
        YEAHSND, 32,
        LASTSOUND
    };
    /* "These are in all other episodes" */
    static const int full_map[] = {
        DOGDEATHSND, 16, AHHHGSND, 17, DIESND, 18, EVASND, 19,
        TOT_HUNDSND, 23, MEINGOTTSND, 24, SCHABBSHASND, 25, HITLERHASND, 26,
        SPIONSND, 27, NEINSOVASSND, 28, DOGATTACKSND, 29, LEVELDONESND, 30,
        MECHSTEPSND, 31,
        SCHEISTSND, 33,
        DEATHSCREAM4SND, 34, /* AIIEEE */
        DEATHSCREAM5SND, 35, /* DEE-DEE */
        DONNERSND, 36,       /* episode 4 boss dies */
        EINESND, 37,         /* episode 4 boss sighting */
        ERLAUBENSND, 38,     /* episode 6 boss sighting */
        DEATHSCREAM6SND, 39, /* FART */
        DEATHSCREAM7SND, 40, /* GASP */
        DEATHSCREAM8SND, 41, /* GUH-BOY! */
        DEATHSCREAM9SND, 42, /* AH GEEZ! */
        KEINSND, 43,         /* episode 5 boss sighting */
        MEINSND, 44,         /* episode 6 boss dies */
        ROSESND, 45,         /* episode 5 boss dies */
        LASTSOUND
    };
    int i;

    for (i = 0; i < LASTSOUND; ++i) {
        DigiMap[i] = -1;
    }
    for (i = 0; upload_map[i] != LASTSOUND; i += 2) {
        DigiMap[upload_map[i]] = upload_map[i + 1];
    }
    if (!upload) {
        for (i = 0; full_map[i] != LASTSOUND; i += 2) {
            DigiMap[full_map[i]] = full_map[i + 1];
        }
    }
}

static boolean load_digi_table(void)
{
    int i;
    word *list;
    const byte *page;
    int entries;
    int pg;

    if (!PMPages || ChunksInFile < 2 || PMSoundStart >= ChunksInFile) {
        return false;
    }

    page = (const byte *)PM_GetPage((int)ChunksInFile - 1);
    if (!page) {
        return false;
    }

    /* Count usable digi entries (same logic as original SDL_SetupDigi). */
    list = (word *)page;
    pg = (int)PMSoundStart;
    entries = 0;
    for (i = 0; i < (int)(PMPageSize / (sizeof(word) * 2)); ++i) {
        word start = list[i * 2 + 0];
        word length = list[i * 2 + 1];
        int pages_used;
        if (pg >= (int)ChunksInFile - 1) {
            break;
        }
        (void)start;
        pages_used = (int)((length + (PMPageSize - 1)) / PMPageSize);
        if (pages_used <= 0) {
            break;
        }
        pg += pages_used;
        entries++;
        if (entries >= MAX_DIGI) {
            break;
        }
    }

    num_digi = entries;
    for (i = 0; i < entries; ++i) {
        word start = list[i * 2 + 0];
        word length = list[i * 2 + 1];
        int left = (int)length;
        int pagenum = (int)PMSoundStart + (int)start;
        byte *buf;
        int written = 0;

        if (length == 0) {
            digi[i].data = NULL;
            digi[i].len = 0;
            continue;
        }
        buf = (byte *)malloc(length);
        if (!buf) {
            digi[i].data = NULL;
            digi[i].len = 0;
            continue;
        }
        while (left > 0 && pagenum < (int)ChunksInFile - 1) {
            const byte *src = (const byte *)PM_GetPage(pagenum);
            int n = left > (int)PMPageSize ? (int)PMPageSize : left;
            if (!src) {
                break;
            }
            memcpy(buf + written, src, (size_t)n);
            written += n;
            left -= n;
            pagenum++;
        }
        digi[i].data = buf;
        digi[i].len = written;
    }

    printf("Audio digi: %d samples from VSWAP\n", num_digi);
    return num_digi > 0;
}

/* ---- AUDIOT: AdLib effects and music ---- */

static byte *audiot_blob;
static long audiot_size;
static long *audiot_starts;
static int audiot_chunks;

static boolean load_audiot(const wolf_data_dir_t *dir)
{
    char path[WOLF_PATH_MAX];
    FILE *f;
    long fsz;
    int i;

    Wolf_MakeDataPath(dir, "AUDIOHED", path, sizeof(path));
    f = fopen(path, "rb");
    if (!f) {
        return false;
    }
    fseek(f, 0, SEEK_END);
    fsz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (fsz < 8) {
        fclose(f);
        return false;
    }
    audiot_chunks = (int)(fsz / 4);
    audiot_starts = (long *)malloc((size_t)audiot_chunks * sizeof(long));
    if (!audiot_starts) {
        fclose(f);
        return false;
    }
    for (i = 0; i < audiot_chunks; ++i) {
        byte b[4];
        if (fread(b, 1, 4, f) != 4) {
            break;
        }
        audiot_starts[i] =
            (long)(b[0] | (b[1] << 8) | (b[2] << 16) | ((long)b[3] << 24));
    }
    fclose(f);

    /* AdLib effects at STARTADLIBSOUNDS, music up to STARTMUSIC + LASTMUSIC */
    if (audiot_chunks < STARTMUSIC + LASTMUSIC + 1) {
        fprintf(stderr, "SD: AUDIOHED has %d entries, expected %d - no AdLib\n",
                audiot_chunks, STARTMUSIC + LASTMUSIC + 1);
        free(audiot_starts);
        audiot_starts = NULL;
        audiot_chunks = 0;
        return false;
    }

    Wolf_MakeDataPath(dir, "AUDIOT", path, sizeof(path));
    f = fopen(path, "rb");
    if (!f) {
        return false;
    }
    fseek(f, 0, SEEK_END);
    fsz = ftell(f);
    fseek(f, 0, SEEK_SET);
    audiot_blob = (byte *)malloc((size_t)fsz + 1);
    if (!audiot_blob || fread(audiot_blob, 1, (size_t)fsz, f) != (size_t)fsz) {
        fclose(f);
        free(audiot_blob);
        audiot_blob = NULL;
        return false;
    }
    fclose(f);
    audiot_size = fsz;
    printf("Audio AUDIOT loaded: %ld bytes, %d headers\n", fsz, audiot_chunks);
    return true;
}

/* An AUDIOT chunk, NULL if absent. */
static const byte *audio_chunk(int chunk, size_t *size)
{
    long start, end;

    if (!audiot_blob || chunk < 0 || chunk + 1 >= audiot_chunks) {
        return NULL;
    }
    start = audiot_starts[chunk];
    end = audiot_starts[chunk + 1];
    if (start < 0 || end <= start || end > audiot_size) {
        return NULL;
    }
    *size = (size_t)(end - start);
    return audiot_blob + start;
}

/* ---- mixer ---- */

/* the settings page's volumes (not in the original), as gains */
static float music_gain = 1.0f, effects_gain = 1.0f;

/* 0 is silent, then 3 dB a step up to 9 at full volume */
static float LevelGain(int level)
{
    if (level <= 0) {
        return 0.0f;
    }
    if (level >= SD_MAXVOLUME) {
        return 1.0f;
    }
    return powf(10.0f, -3.0f * (float)(SD_MAXVOLUME - level) / 20.0f);
}

void SD_SetVolumes(int music, int effects)
{
    LOCK();
    music_gain = LevelGain(music);
    effects_gain = LevelGain(effects);
    UNLOCK();
}

static void mix_callback(void *userdata, Uint8 *stream, int len)
{
    static int32_t fxopl[MIX_SAMPLES], musopl[MIX_SAMPLES];
    float *out = (float *)stream;
    int frames = len / (int)(2 * sizeof(float));
    const uint32_t step = (uint32_t)(((uint64_t)DIGI_RATE << 16) / MIX_RATE);
    (void)userdata;

    SDL_LockMutex(audio_mx);
    while (frames > 0) {
        const int n = frames > MIX_SAMPLES ? MIX_SAMPLES : frames;
        int i;

        SDA_RenderSplit(fxopl, musopl, n);
        for (i = 0; i < n; i++) {
            float l = ((float)fxopl[i] * effects_gain + (float)musopl[i] * music_gain) * OPL_GAIN;
            float r = l;

            if (digi_data) {
                /* linear interpolation between the 7 kHz samples */
                const int s = (int)(digi_pos >> 16);
                const float frac = (float)(digi_pos & 0xffff) / 65536.0f;
                const int a = digi_data[s] - 128;
                const int b = (s + 1 < digi_len ? digi_data[s + 1] : 128) - 128;
                const float d = ((float)a + (float)(b - a) * frac) * DIGI_GAIN * effects_gain;

                l += d * digi_left;
                r += d * digi_right;
                digi_pos += step;
                if ((int)(digi_pos >> 16) >= digi_len) {
                    digi_data = NULL; /* SDL_DigitizedDone */
                    DigiNumber = DigiPriority = 0;
                    SoundPositioned = false;
                }
            }
            out[0] = l > 1.0f ? 1.0f : l < -1.0f ? -1.0f : l;
            out[1] = r > 1.0f ? 1.0f : r < -1.0f ? -1.0f : r;
            out += 2;
        }
        frames -= n;
    }
    SDL_UnlockMutex(audio_mx);
}

/* SDL_PositionSBP: the SB Pro voice volume is 15 - position per side */
static void set_digi_position(int leftpos, int rightpos)
{
    digi_left = (float)(15 - leftpos) / 15.0f;
    digi_right = (float)(15 - rightpos) / 15.0f;
}

/* SD_PlayDigitized */
static void play_digitized(int which, int leftpos, int rightpos)
{
    if (which < 0 || which >= num_digi || !digi[which].data || digi[which].len <= 0) {
        return;
    }
    digi_data = digi[which].data;
    digi_len = digi[which].len;
    digi_pos = 0;
    set_digi_position(leftpos, rightpos);
}

/* ---- public API ---- */

void SD_Startup(void)
{
    init_digimap(false);
    digi_data = NULL;
    DigiNumber = DigiPriority = 0;
    LeftPosition = RightPosition = 0;
    nextsoundpos = SoundPositioned = false;
    SoundMode = sdm_Off;
    MusicMode = smm_Off;
    DigiMode = sds_Off;
    SD_Silent = false;
    sd_started = false;
    audio_dev = 0;
    audio_mx = NULL;
}

void SD_SetSilent(boolean silent)
{
    /* no sound effects; music has its own switch (MusicMode) */
    SD_Silent = silent;
    if (silent) {
        SD_StopSound();
    }
}

boolean SD_AdLibPresent(void)
{
    return audio_dev != 0 && audiot_blob != NULL;
}

boolean SD_DigiPresent(void)
{
    return audio_dev != 0 && num_digi > 0;
}

/* SD_SetSoundMode: the AdLib effects on or off (no PC speaker here) */
void SD_SetSoundMode(SDMode mode)
{
    LOCK();
    SDA_StopSound();
    UNLOCK();
    SoundMode = mode == sdm_AdLib && SD_AdLibPresent() ? sdm_AdLib : sdm_Off;
}

/* SD_SetDigiDevice: digitized sound through the Sound Blaster, or off */
void SD_SetDigiDevice(SDSMode mode)
{
    LOCK();
    digi_data = NULL; /* SD_StopDigitized */
    DigiNumber = DigiPriority = 0;
    SoundPositioned = false;
    UNLOCK();
    DigiMode = mode == sds_SoundBlaster && SD_DigiPresent() ? sds_SoundBlaster : sds_Off;
}

/* SD_SetMusicMode: off stops the music; AdLib takes the next SD_StartMusic */
void SD_SetMusicMode(SMMode mode)
{
    SD_MusicOff();
    MusicMode = mode == smm_AdLib && SD_AdLibPresent() ? smm_AdLib : smm_Off;
}

boolean SD_SetupFromData(const wolf_data_dir_t *dir)
{
    SDL_AudioSpec want, have;

    if (sd_started) {
        return true;
    }

    if (!audio_mx) {
        audio_mx = SDL_CreateMutex();
    }

    sd_upload = strcmp(dir->extension, ".WL1") == 0; /* the shareware EXE: UPLOAD */
    init_digimap(sd_upload);
    if (!load_digi_table()) {
        fprintf(stderr, "SD: digi table load failed - silent digi\n");
    }
    if (!load_audiot(dir)) {
        fprintf(stderr, "SD: AUDIOT load failed - no AdLib effects or music\n");
    }
    SDA_Startup();

    if (SDL_WasInit(SDL_INIT_AUDIO) == 0) {
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
            fprintf(stderr, "SD: SDL_INIT_AUDIO failed: %s - silent\n",
                    SDL_GetError());
            SD_Silent = true;
            sd_started = true;
            return true; /* still OK — silent fallback */
        }
    }

    SDL_zero(want);
    want.freq = MIX_RATE;
    want.format = AUDIO_F32SYS;
    want.channels = 2;
    want.samples = MIX_SAMPLES;
    want.callback = mix_callback;
    want.userdata = NULL;

    /* no allowed changes: SDL converts our rate and format to the device's */
    audio_dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (!audio_dev) {
        fprintf(stderr, "SD: OpenAudioDevice failed: %s - silent\n", SDL_GetError());
        SD_Silent = true;
        sd_started = true;
        return true;
    }

    DigiMode = num_digi > 0 ? sds_SoundBlaster : sds_Off;
    SoundMode = audiot_blob ? sdm_AdLib : sdm_Off;
    MusicMode = audiot_blob ? smm_AdLib : smm_Off;
    SDL_PauseAudioDevice(audio_dev, 0);
    sd_started = true;
    printf("SD: audio device OK rate=%d (mixing at %d) digi=%d adlib=%d\n", have.freq,
           MIX_RATE, num_digi, (int)(SoundMode == sdm_AdLib));
    return true;
}

void SD_Shutdown(void)
{
    int i;
    if (audio_dev) {
        SDL_CloseAudioDevice(audio_dev);
        audio_dev = 0;
    }
    digi_data = NULL;
    for (i = 0; i < MAX_DIGI; ++i) {
        free(digi[i].data);
        digi[i].data = NULL;
        digi[i].len = 0;
    }
    num_digi = 0;
    free(audiot_blob);
    audiot_blob = NULL;
    audiot_size = 0;
    free(audiot_starts);
    audiot_starts = NULL;
    audiot_chunks = 0;
    SoundMode = sdm_Off;
    MusicMode = smm_Off;
    DigiMode = sds_Off;
    if (audio_mx) {
        SDL_DestroyMutex(audio_mx);
        audio_mx = NULL;
    }
    sd_started = false;
}

void SD_PositionSound(int leftvol, int rightvol)
{
    LeftPosition = leftvol;
    RightPosition = rightvol;
    nextsoundpos = true;
}

void SD_SetPosition(int leftpos, int rightpos)
{
    if (leftpos < 0 || leftpos > 15 || rightpos < 0 || rightpos > 15 ||
        (leftpos == 15 && rightpos == 15)) {
        return; /* the original quits: "Illegal position" */
    }
    LOCK();
    if (DigiMode == sds_SoundBlaster) {
        set_digi_position(leftpos, rightpos);
    }
    UNLOCK();
}

boolean SD_SoundPositioned(void)
{
    boolean positioned;

    LOCK();
    positioned = SoundPositioned;
    UNLOCK();
    return positioned;
}

/*
 * SD_PlaySound: digitized when the sound has one, otherwise the AdLib
 * effect. The two channels keep separate priorities; either is only
 * replaced by a sound of at least the same priority. True when a
 * digitized sound started (the caller then tracks its position).
 */
boolean SD_PlaySound(word sound)
{
    const byte *s;
    size_t size = 0;
    int priority;
    int lp = LeftPosition, rp = RightPosition;
    boolean ispos = nextsoundpos;
    boolean played = false;

    LeftPosition = RightPosition = 0;
    nextsoundpos = false;

    if (sound >= LASTSOUND || SD_Silent || !audio_dev) {
        return false;
    }
    s = audio_chunk(STARTADLIBSOUNDS + sound, &size);
    if (!s || size < 6) {
        return false; /* the original quits: "Uncached sound" */
    }
    priority = s[4] | (s[5] << 8);

    LOCK();
    if (DigiMode != sds_Off && DigiMap[sound] >= 0 && DigiMap[sound] < num_digi &&
        digi[DigiMap[sound]].data) {
        if (priority >= DigiPriority) {
            play_digitized(DigiMap[sound], lp, rp);
            SoundPositioned = ispos;
            DigiNumber = sound;
            DigiPriority = priority;
            played = true;
        }
    } else if (SoundMode == sdm_AdLib && priority >= SDA_SoundPriority()) {
        (void)SDA_PlaySound(s, size, sound, priority);
    }
    UNLOCK();
    return played;
}

void SD_StopSound(void)
{
    LOCK();
    digi_data = NULL; /* SD_StopDigitized */
    DigiNumber = DigiPriority = 0;
    SDA_StopSound();
    SoundPositioned = false;
    UNLOCK();
}

word SD_SoundPlaying(void)
{
    int playing;

    LOCK();
    playing = SDA_SoundPlaying(); /* the AdLib effect only, as in the original */
    UNLOCK();
    return (word)playing;
}

void SD_WaitSoundDone(void)
{
    /* the original spins; give up after 10 s in case the device stalled */
    const Uint32 start = SDL_GetTicks();

    while (SD_SoundPlaying() && SDL_GetTicks() - start < 10000) {
        SDL_Delay(1);
    }
}

void SD_StartMusic(word music_index)
{
    const byte *chunk;
    size_t size = 0;

    if (MusicMode != smm_AdLib || music_index >= LASTMUSIC) {
        return;
    }
    chunk = audio_chunk(STARTMUSIC + (int)music_index, &size);
    if (!chunk) {
        return;
    }
    LOCK();
    SDA_StartMusic(chunk, size);
    UNLOCK();
}

void SD_MusicOn(void)
{
    if (MusicMode != smm_AdLib) {
        return;
    }
    LOCK();
    SDA_MusicOn();
    UNLOCK();
}

void SD_MusicOff(void)
{
    LOCK();
    SDA_MusicOff();
    UNLOCK();
}

boolean SD_MusicPlaying(void)
{
    boolean active;

    LOCK();
    active = SDA_MusicActive();
    UNLOCK();
    return active;
}

longword SD_GetTimeCount(void)
{
    return IN_GetTimeCount() + time_count_base;
}

void SD_SetTimeCount(longword count)
{
    time_count_base = count;
}

int SD_NumDigi(void)
{
    return num_digi;
}

boolean SD_HasDigiFor(word sound)
{
    return sound < LASTSOUND && DigiMap[sound] >= 0 && DigiMap[sound] < num_digi &&
           digi[DigiMap[sound]].data != NULL;
}

/* channels of the chip with a note keyed on */
static int keyed_channels(void)
{
    const opl2_t *chip;
    int ch, keyed = 0;

    LOCK();
    chip = SDA_Chip();
    for (ch = 0; ch < 9; ++ch) {
        if (chip->ch[ch].keyon) {
            keyed++;
        }
    }
    UNLOCK();
    return keyed;
}

/* the loudest sample the mixer gives over a quarter of a second (the smoke check) */
static float mix_peak(void)
{
    static float buf[2 * 1024];
    float peak = 0.0f;
    int pass, i;

    for (pass = 0; pass < MIX_RATE / 4 / 1024; pass++) {
        mix_callback(NULL, (Uint8 *)buf, (int)sizeof(buf));
        for (i = 0; i < 2 * 1024; i++) {
            const float v = buf[i] < 0 ? -buf[i] : buf[i];
            peak = v > peak ? v : peak;
        }
    }
    return peak;
}

boolean SD_AudioSmokeVerify(void)
{
    boolean ok = true;
    printf("Audio smoke: silent=%d digiMode=%d soundMode=%d musicMode=%d num_digi=%d\n",
           (int)SD_Silent, (int)DigiMode, (int)SoundMode, (int)MusicMode, num_digi);
    printf("  digi pistol=%d door=%d open_mapped=%d\n",
           (int)SD_HasDigiFor(ATKPISTOLSND), (int)SD_HasDigiFor(OPENDOORSND),
           DigiMap[OPENDOORSND]);

    /* the full game's extra screams are digitized; the shareware EXE plays them on the AdLib */
    if (num_digi > 0) {
        const boolean screams = SD_HasDigiFor(DEATHSCREAM4SND) && SD_HasDigiFor(DEATHSCREAM9SND) &&
                                SD_HasDigiFor(SCHEISTSND);
        printf("  digi extra screams=%d (shareware=%d)\n", (int)screams, (int)sd_upload);
        if (screams == sd_upload || !SD_HasDigiFor(DEATHSCREAM1SND) || !SD_HasDigiFor(YEAHSND)) {
            fprintf(stderr, "Audio smoke: digitized sound map does not match the pack\n");
            ok = false;
        }
    }

    /* Silent path must not crash */
    {
        boolean old = SD_Silent;
        SD_SetSilent(true);
        SD_PlaySound(ATKPISTOLSND);
        SD_PlaySound(GETAMMOSND);
        SD_SetSilent(old);
        printf("  silent fallback play: OK\n");
    }

    if (!SD_Silent && DigiMode != sds_Off) {
        if (!SD_HasDigiFor(ATKPISTOLSND) || !SD_HasDigiFor(OPENDOORSND)) {
            fprintf(stderr, "Audio smoke: expected weapon/door digi\n");
            ok = false;
        } else {
            SD_PlaySound(ATKPISTOLSND);
            SDL_Delay(30);
            SD_PlaySound(OPENDOORSND);
            SDL_Delay(30);
            printf("  digi play triggered (non-silent)\n");
        }
    }

    if (!SD_Silent && SoundMode == sdm_AdLib) {
        /* an AdLib-only effect: plays on channel 0 and ends by itself */
        Uint32 start;

        SD_PlaySound(BONUS1SND);
        if (SD_SoundPlaying() != BONUS1SND) {
            fprintf(stderr, "Audio smoke: AdLib effect did not start\n");
            ok = false;
        }
        start = SDL_GetTicks();
        SD_WaitSoundDone();
        printf("  AdLib effect: %u ms\n", (unsigned)(SDL_GetTicks() - start));
        if (SD_SoundPlaying()) {
            fprintf(stderr, "Audio smoke: AdLib effect never ended\n");
            ok = false;
        }
    }

    if (MusicMode == smm_AdLib) {
        int max_keyed = 0;
        int sample;

        SD_StartMusic(GETTHEM_MUS);
        /* Sample several mix windows — notes are not continuous. */
        for (sample = 0; sample < 12; ++sample) {
            int keyed;
            SDL_Delay(40);
            keyed = keyed_channels();
            if (keyed > max_keyed) {
                max_keyed = keyed;
            }
        }
        printf("  music: playing=%d max_keyed=%d\n", (int)SD_MusicPlaying(), max_keyed);
        if (!SD_MusicPlaying() || max_keyed == 0) {
            fprintf(stderr, "Audio smoke: music never keyed a note\n");
            ok = false;
        }
        SD_MusicOff();
    }

    /*
     * The Sound menu's switches (CP_Sound): with digitized sound off a sound
     * falls back to its AdLib version, as in the original; with the effects
     * off nothing of them plays.
     */
    if (!SD_Silent && SoundMode == sdm_AdLib && DigiMode != sds_Off) {
        const SDSMode olddigi = DigiMode;

        SD_SetDigiDevice(sds_Off);
        SD_PlaySound(ATKPISTOLSND);
        if (SD_SoundPlaying() != ATKPISTOLSND) {
            fprintf(stderr, "Audio smoke: digitized off, the pistol did not play on the AdLib\n");
            ok = false;
        }
        SD_StopSound();
        SD_SetSoundMode(sdm_Off);
        SD_PlaySound(ATKPISTOLSND);
        if (SD_SoundPlaying()) {
            fprintf(stderr, "Audio smoke: effects off, an AdLib effect still played\n");
            ok = false;
        }
        SD_SetSoundMode(sdm_AdLib);
        SD_SetDigiDevice(olddigi);
        printf("  sound switches: digitized off -> AdLib, effects off -> none\n");
    }

    /* the volume sliders, through the mixer itself: silent at 0, heard at full */
    if (!SD_Silent && MusicMode == smm_AdLib) {
        float peak0, peakfull;

        SD_StartMusic(GETTHEM_MUS);
        SDL_Delay(100);
        SD_SetVolumes(0, 0);
        peak0 = mix_peak();
        SD_SetVolumes(SD_MAXVOLUME, SD_MAXVOLUME);
        peakfull = mix_peak();
        SD_MusicOff();
        printf("  volumes: peak %.3f at 0, %.3f at full\n", peak0, peakfull);
        if (peak0 != 0.0f || peakfull <= 0.0f) {
            fprintf(stderr, "Audio smoke: the volumes do not set the level\n");
            ok = false;
        }
    }

    return ok;
}
