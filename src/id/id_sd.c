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

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define MIX_RATE 22050
#define MIX_SAMPLES 1024
#define MAX_DIGI 64
/* Digitized samples in VSWAP are ~7000 Hz unsigned 8-bit PCM. */
#define DIGI_SRC_RATE 7000
#define DIGI_VOICE_GAIN 1.35f
#define MAX_VOICE 8
#define IMF_HZ 700.0 /* Wolf3D AdLib IMF rate (ID_SD) */

SDMode SoundMode = sdm_Off;
SMMode MusicMode = smm_Off;
SDSMode DigiMode = sds_Off;
boolean SD_Silent = false;
int DigiMap[LASTSOUND];

static boolean sd_started;
static SDL_AudioDeviceID audio_dev;
static SDL_mutex *audio_mx;

typedef struct {
    byte *data;
    int len;
} digi_sample_t;

typedef struct {
    const byte *data;
    int len;
    int pos; /* in source samples, fixed 16.16 */
    int active;
    int vol;
} voice_t;

static digi_sample_t digi[MAX_DIGI];
static int num_digi;
static voice_t voices[MAX_VOICE];

/* IMF music */
static byte *music_data;       /* full AUDIOT music chunk */
static int music_len;          /* total chunk bytes */
static int music_pos;          /* byte offset into values[] stream */
static int music_seq_len;      /* MusicGroup.length (bytes of values) */
static int music_remaining;    /* bytes left in values this loop */
static double music_tick_acc;  /* fractional AdLib timer ticks */
static long music_al_time;     /* alTimeCount analogue */
static long music_sq_time;     /* sqHackTime analogue */
static boolean music_on;
static int music_index_cur = -1;

/* Minimal AdLib channel state for pitch * volume */
static struct {
    int fnum;
    int block;
    int keyon;
    int vol; /* 0..63 where lower is louder on real HW — invert */
    double phase;
} opl_ch[9];

static byte opl_reg[256];

static longword time_count_base;

/* ---- digi table from last VSWAP page ---- */

static void init_digimap(void)
{
    int i;
    /*
     * Official wolfdigimap (WL_MAIN.C InitDigiMap) — sounds NOT in this list
     * have DigiMap=-1 and must not fall back to unrelated digi (e.g. pistol).
     * Pickups like GETAMMOSND / BONUS* are AdLib-only in the original.
     */
    static const int map[] = {
        HALTSND, 0, DOGBARKSND, 1, CLOSEDOORSND, 2, OPENDOORSND, 3,
        ATKMACHINEGUNSND, 4, ATKPISTOLSND, 5, ATKGATLINGSND, 6, SCHUTZADSND, 7,
        GUTENTAGSND, 8, MUTTISND, 9, BOSSFIRESND, 10, SSFIRESND, 11,
        DEATHSCREAM1SND, 12, DEATHSCREAM2SND, 13, DEATHSCREAM3SND, 13,
        TAKEDAMAGESND, 14, PUSHWALLSND, 15, DOGDEATHSND, 16, AHHHGSND, 17,
        DIESND, 18, EVASND, 19, LEBENSND, 20, NAZIFIRESND, 21, SLURPIESND, 22,
        TOT_HUNDSND, 23, MEINGOTTSND, 24, SCHABBSHASND, 25, HITLERHASND, 26,
        SPIONSND, 27, NEINSOVASSND, 28, DOGATTACKSND, 29, LEVELDONESND, 30,
        /* mech step 31 and yeah 32 exist on some packs — omit if no enum */
        LASTSOUND
    };
    for (i = 0; i < LASTSOUND; ++i) {
        DigiMap[i] = -1;
    }
    for (i = 0; map[i] != LASTSOUND; i += 2) {
        if (map[i] >= 0 && map[i] < LASTSOUND) {
            DigiMap[map[i]] = map[i + 1];
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

        if (length == 0 || length > 256 * 1024) {
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

/* ---- AUDIOT music ---- */

static byte *audiot_blob;
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
            (long)(b[0] | (b[1] << 8) | (b[2] << 16) | (b[3] << 24));
    }
    fclose(f);

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
    printf("Audio AUDIOT loaded: %ld bytes, %d headers\n", fsz, audiot_chunks);
    return true;
}

static void free_music(void)
{
    free(music_data);
    music_data = NULL;
    music_len = 0;
    music_pos = 0;
    music_seq_len = 0;
    music_remaining = 0;
    music_tick_acc = 0;
    music_al_time = 0;
    music_sq_time = 0;
    music_on = false;
    music_index_cur = -1;
}

/* ---- OPL register write stub for IMF ---- */

static void al_out(byte reg, byte val)
{
    int ch;
    opl_reg[reg] = val;

    /* Key on / fnum for 9 melodic channels packed in A0-B8 / freql mod */
    if (reg >= 0xA0 && reg <= 0xA8) {
        ch = reg - 0xA0;
        opl_ch[ch].fnum = (opl_ch[ch].fnum & 0x300) | val;
    } else if (reg >= 0xB0 && reg <= 0xB8) {
        ch = reg - 0xB0;
        opl_ch[ch].fnum = (opl_ch[ch].fnum & 0xFF) | ((val & 3) << 8);
        opl_ch[ch].block = (val >> 2) & 7;
        opl_ch[ch].keyon = (val & 0x20) ? 1 : 0;
        if (!opl_ch[ch].keyon) {
            opl_ch[ch].phase = 0;
        }
    } else if (reg >= 0x40 && reg <= 0x55) {
        /* total level of op — approximate channel volume via carrier ops */
        int op = reg - 0x40;
        static const int op_to_ch[22] = {
            0, 1, 2, 0, 1, 2, 3, 4, 5, 3, 4, 5, 6, 7, 8, 6, 7, 8, 0, 0, 0, 0
        };
        if (op < 18) {
            ch = op_to_ch[op];
            /* carriers rough 3,4,5,9,10,11,... — treat all as volume tags */
            opl_ch[ch].vol = 63 - (val & 63);
        }
    }
}

static word rd_le16(const byte *p)
{
    return (word)(p[0] | (p[1] << 8));
}

/*
 * Original ID_SD SDL_ALService (MusicGroup stream):
 *   while (len && sqHackTime <= alTimeCount) {
 *     cmd = *ptr++; delay = *ptr++;  // words
 *     alOut(cmd&0xff, cmd>>8);
 *     sqHackTime = alTimeCount + delay;
 *     len -= 4;
 *   }
 *   alTimeCount++;
 * AdLib ISR runs ~700 Hz.
 */
static void imf_advance(double samples_at_mix)
{
    double ticks = samples_at_mix * (IMF_HZ / (double)MIX_RATE);
    music_tick_acc += ticks;

    while (music_on && music_data && music_seq_len > 0 && music_tick_acc >= 1.0) {
        /* Drain all events due at this alTimeCount. */
        while (music_remaining >= 4 && music_sq_time <= music_al_time) {
            word cmd, delay;
            byte reg, val;
            if (music_pos + 4 > music_len) {
                music_remaining = 0;
                break;
            }
            cmd = rd_le16(music_data + music_pos);
            delay = rd_le16(music_data + music_pos + 2);
            music_pos += 4;
            music_remaining -= 4;
            reg = (byte)(cmd & 0xFF);
            val = (byte)((cmd >> 8) & 0xFF);
            al_out(reg, val);
            music_sq_time = music_al_time + (long)delay;
        }

        music_al_time += 1;
        music_tick_acc -= 1.0;

        if (music_remaining < 4) {
            /* Loop MusicGroup values (original rewinds pointer). */
            music_pos = (music_seq_len > 0 && music_len >= music_seq_len + 2)
                            ? 2
                            : 0;
            music_remaining = music_seq_len;
            music_al_time = 0;
            music_sq_time = 0;
        }
    }
}

static float opl_sample(void)
{
    int ch;
    float sum = 0.0f;
    for (ch = 0; ch < 9; ++ch) {
        double freq, step, s;
        if (!opl_ch[ch].keyon || opl_ch[ch].fnum == 0) {
            continue;
        }
        /* AdLib formula-ish */
        freq = (double)opl_ch[ch].fnum * (14318180.0 / 288.0) /
               (double)(1 << (20 - opl_ch[ch].block));
        if (freq < 20.0 || freq > 4000.0) {
            continue;
        }
        step = freq / (double)MIX_RATE;
        opl_ch[ch].phase += step;
        if (opl_ch[ch].phase >= 1.0) {
            opl_ch[ch].phase -= floor(opl_ch[ch].phase);
        }
        s = (opl_ch[ch].phase < 0.5) ? 1.0 : -1.0;
        sum += (float)(s * ((opl_ch[ch].vol > 0 ? opl_ch[ch].vol : 32) / 63.0) * 0.22);
    }
    return sum;
}

/* ---- mixer ---- */

static void mix_callback(void *userdata, Uint8 *stream, int len)
{
    int samples = len / (int)sizeof(float); /* we use F32 stereo? mono F32 */
    float *out = (float *)stream;
    int i, v;
    (void)userdata;

    memset(stream, 0, (size_t)len);

    if (SD_Silent) {
        return;
    }

    SDL_LockMutex(audio_mx);

    if (music_on && MusicMode == smm_AdLib) {
        imf_advance((double)samples);
    }

    for (i = 0; i < samples; ++i) {
        float mix = 0.0f;

        if (music_on && MusicMode == smm_AdLib) {
            mix += opl_sample();
        }

        for (v = 0; v < MAX_VOICE; ++v) {
            voice_t *vc = &voices[v];
            int spos;
            int sample_u8;
            float s;
            if (!vc->active || !vc->data) {
                continue;
            }
            spos = vc->pos >> 16;
            if (spos >= vc->len) {
                vc->active = 0;
                continue;
            }
            sample_u8 = vc->data[spos];
            s = ((float)sample_u8 - 128.0f) / 128.0f;
            mix += s * (vc->vol / 128.0f) * 0.85f * DIGI_VOICE_GAIN;
            /* step source at DIGI_SRC_RATE → MIX_RATE */
            vc->pos += (int)((DIGI_SRC_RATE << 16) / MIX_RATE);
        }

        if (mix > 1.0f) {
            mix = 1.0f;
        }
        if (mix < -1.0f) {
            mix = -1.0f;
        }
        out[i] = mix;
    }

    SDL_UnlockMutex(audio_mx);
}

static void play_digi_index(int which, int vol)
{
    int v;
    if (which < 0 || which >= num_digi || !digi[which].data || digi[which].len <= 0) {
        return;
    }
    if (SD_Silent || DigiMode == sds_Off) {
        return;
    }
    SDL_LockMutex(audio_mx);
    for (v = 0; v < MAX_VOICE; ++v) {
        if (!voices[v].active) {
            break;
        }
    }
    if (v == MAX_VOICE) {
        v = 0; /* steal */
    }
    voices[v].data = digi[which].data;
    voices[v].len = digi[which].len;
    voices[v].pos = 0;
    voices[v].vol = vol;
    voices[v].active = 1;
    SDL_UnlockMutex(audio_mx);
}

/* ---- public API ---- */

void SD_Startup(void)
{
    int i;
    init_digimap();
    for (i = 0; i < MAX_VOICE; ++i) {
        voices[i].active = 0;
    }
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
    SD_Silent = silent;
    /* Do not permanently clobber device modes — silent only mutes the mixer. */
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

    if (!load_digi_table()) {
        fprintf(stderr, "SD: digi table load failed — silent digi\n");
    }
    if (!load_audiot(dir)) {
        fprintf(stderr, "SD: AUDIOT load failed — no music bank\n");
    }

    if (SDL_WasInit(SDL_INIT_AUDIO) == 0) {
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
            fprintf(stderr, "SD: SDL_INIT_AUDIO failed: %s — silent\n",
                    SDL_GetError());
            SD_Silent = true;
            sd_started = true;
            return true; /* still OK — silent fallback */
        }
    }

    SDL_zero(want);
    want.freq = MIX_RATE;
    want.format = AUDIO_F32SYS;
    want.channels = 1;
    want.samples = MIX_SAMPLES;
    want.callback = mix_callback;
    want.userdata = NULL;

    audio_dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (!audio_dev) {
        fprintf(stderr, "SD: OpenAudioDevice failed: %s — silent\n", SDL_GetError());
        SD_Silent = true;
        DigiMode = sds_Off;
        MusicMode = smm_Off;
        sd_started = true;
        return true;
    }

    DigiMode = sds_SoundBlaster;
    SoundMode = sdm_AdLib; /* digi preferred for mapped effects */
    MusicMode = smm_AdLib;
    SDL_PauseAudioDevice(audio_dev, 0);
    sd_started = true;
    printf("SD: audio device OK rate=%d silent=%d digi=%d\n", have.freq,
           (int)SD_Silent, num_digi);
    return true;
}

void SD_Shutdown(void)
{
    int i;
    if (audio_dev) {
        SDL_CloseAudioDevice(audio_dev);
        audio_dev = 0;
    }
    free_music();
    for (i = 0; i < MAX_DIGI; ++i) {
        free(digi[i].data);
        digi[i].data = NULL;
        digi[i].len = 0;
    }
    num_digi = 0;
    free(audiot_blob);
    audiot_blob = NULL;
    free(audiot_starts);
    audiot_starts = NULL;
    audiot_chunks = 0;
    if (audio_mx) {
        SDL_DestroyMutex(audio_mx);
        audio_mx = NULL;
    }
    sd_started = false;
}

void SD_PlaySound(word sound)
{
    int digi_idx;
    if (sound >= LASTSOUND) {
        return;
    }
    digi_idx = DigiMap[sound];
    if (digi_idx >= 0 && digi_idx < num_digi && digi[digi_idx].data) {
        play_digi_index(digi_idx, 140);
        return;
    }
    /*
     * No DigiMap entry (pickups/treasure/etc.): original plays AdLib, not digi.
     * Do NOT fall back to pistol digi — that made ammo pickups sound like shots.
     */
    (void)sound;
}

void SD_StopSound(void)
{
    int v;
    if (!audio_mx) {
        return;
    }
    SDL_LockMutex(audio_mx);
    for (v = 0; v < MAX_VOICE; ++v) {
        voices[v].active = 0;
    }
    SDL_UnlockMutex(audio_mx);
}

void SD_WaitSoundDone(void)
{
    /* non-blocking port — no wait */
}

void SD_StartMusic(word music_index)
{
    int chunk;
    long start, end, len;
    free_music();
    if (SD_Silent || MusicMode == smm_Off || !audiot_blob || !audiot_starts) {
        return;
    }
    chunk = STARTMUSIC + (int)music_index;
    if (chunk + 1 >= audiot_chunks) {
        return;
    }
    start = audiot_starts[chunk];
    end = audiot_starts[chunk + 1];
    if (end <= start || start < 0) {
        return;
    }
    len = end - start;
    music_data = (byte *)malloc((size_t)len);
    if (!music_data) {
        return;
    }
    memcpy(music_data, audiot_blob + start, (size_t)len);
    music_len = (int)len;
    /* MusicGroup (ID_SD.H): first word = byte length of values[], then
         * pairs of words (cmd, delay). cmd low=reg, high=val. */
        music_pos = 0;
        music_seq_len = 0;
    if (music_len >= 6) {
        word nest = (word)(music_data[0] | (music_data[1] << 8));
        if (nest >= 4 && (int)nest + 2 <= music_len && (nest % 4) == 0) {
            music_seq_len = (int)nest;
            music_pos = 2; /* start of values[] */
        } else {
            /* Fallback: whole chunk is the values stream. */
            music_seq_len = music_len & ~3;
            music_pos = 0;
        }
    }
    music_remaining = music_seq_len;
    music_tick_acc = 0;
    music_al_time = 0;
    music_sq_time = 0;
    music_on = true;
    music_index_cur = (int)music_index;
    memset(opl_ch, 0, sizeof(opl_ch));
    memset(opl_reg, 0, sizeof(opl_reg));
    /* Default operator volume so notes sound before first level write. */
    {
        int ch;
        for (ch = 0; ch < 9; ++ch) {
            opl_ch[ch].vol = 48;
        }
    }
    printf("SD: music start index=%u chunk=%d bytes=%ld seq_len=%d values_off=%d\n",
           music_index, chunk, len, music_seq_len, music_pos);
}

void SD_MusicOn(void)
{
    if (music_data) {
        music_on = true;
    }
}

void SD_MusicOff(void)
{
    music_on = false;
}

boolean SD_SoundPlaying(void)
{
    int v;
    boolean any = false;
    if (!audio_mx) {
        return false;
    }
    SDL_LockMutex(audio_mx);
    for (v = 0; v < MAX_VOICE; ++v) {
        if (voices[v].active) {
            any = true;
            break;
        }
    }
    SDL_UnlockMutex(audio_mx);
    return any;
}

boolean SD_MusicPlaying(void)
{
    return music_on && music_data != NULL;
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

boolean SD_AudioSmokeVerify(void)
{
    boolean ok = true;
    printf("Audio smoke: silent=%d digiMode=%d musicMode=%d num_digi=%d\n",
           (int)SD_Silent, (int)DigiMode, (int)MusicMode, num_digi);
    printf("  digi pistol=%d door=%d open_mapped=%d\n",
           (int)SD_HasDigiFor(ATKPISTOLSND), (int)SD_HasDigiFor(OPENDOORSND),
           DigiMap[OPENDOORSND]);

    /* Silent path must not crash */
    {
        boolean old = SD_Silent;
        SD_SetSilent(true);
        SD_PlaySound(ATKPISTOLSND);
        SD_StartMusic(CORNER_MUS);
        SD_MusicOff();
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

    if (!SD_Silent && MusicMode == smm_AdLib && audiot_blob) {
            SD_StartMusic(GETTHEM_MUS);
            if (!SD_MusicPlaying() || music_seq_len < 4 || music_pos != 2) {
                fprintf(stderr,
                        "Audio smoke: music failed (playing=%d seq_len=%d off=%d)\n",
                        (int)SD_MusicPlaying(), music_seq_len, music_pos);
                ok = false;
            } else {
                int max_keyed = 0;
                int sample;
                /* Sample several mix windows — notes are not continuous. */
                for (sample = 0; sample < 12; ++sample) {
                    int ch, keyed = 0;
                    SDL_Delay(40);
                    for (ch = 0; ch < 9; ++ch) {
                        if (opl_ch[ch].keyon && opl_ch[ch].fnum != 0) {
                            keyed++;
                        }
                    }
                    if (keyed > max_keyed) {
                        max_keyed = keyed;
                    }
                }
                printf("  music start/stop: OK seq_len=%d values_off=%d max_keyed=%d\n",
                       music_seq_len, music_pos, max_keyed);
                if (max_keyed == 0) {
                    fprintf(stderr,
                            "Audio smoke: music stream advanced but soft-OPL never key-on\n");
                    ok = false;
                }
                SD_MusicOff();
            }
        }

    return ok;
}
