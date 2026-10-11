#include "sd_adlib.h"

#include <string.h>

/* register groups and channel 0's operators (ID_SD.H, ID_SD.C) */
#define alChar 0x20
#define alScale 0x40
#define alAttack 0x60
#define alSus 0x80
#define alWave 0xe0
#define alFreqL 0xa0
#define alFreqH 0xb0
#define alFeedCon 0xc0
#define alEffects 0xbd
#define sqMaxTracks 10
#define MODIFIER0 0
#define CARRIER0 3

/* AdLibSound: SoundCommon (length, priority), Instrument (16 bytes), block, data */
#define ADLIB_INST 6
#define ADLIB_BLOCK 22
#define ADLIB_DATA 23

static opl2_t chip;

/* effects (ID_SD.C / ID_SD_A.ASM) */
static const byte *alSound;
static word alLengthLeft;
static byte alBlock;
static int SoundNumber, SoundPriority;

/* the sequencer */
static const byte *sqHack, *sqHackPtr;
static word sqHackLen, sqHackSeqLen;
static long sqHackTime, alTimeCount;
static boolean sqActive;

/* the 700 Hz timer */
static int tickfrac; /* SDA_TICKRATE per sample, a tick per SDA_RATE */
static int count_fx;

static void alOut(int reg, int val)
{
    OPL2_Write(&chip, reg, val);
}

static word ReadWord(const byte *p)
{
    return (word)(p[0] | (p[1] << 8));
}

/* SDL_AlSetFXInst: inst is the 16-byte Instrument, NULL for alZeroInst */
static void SetFXInst(const byte *inst)
{
    static const byte zero[16];

    if (!inst) {
        inst = zero;
    }
    alOut(MODIFIER0 + alChar, inst[0]);
    alOut(MODIFIER0 + alScale, inst[2]);
    alOut(MODIFIER0 + alAttack, inst[4]);
    alOut(MODIFIER0 + alSus, inst[6]);
    alOut(MODIFIER0 + alWave, inst[8]);
    alOut(CARRIER0 + alChar, inst[1]);
    alOut(CARRIER0 + alScale, inst[3]);
    alOut(CARRIER0 + alAttack, inst[5]);
    alOut(CARRIER0 + alSus, inst[7]);
    alOut(CARRIER0 + alWave, inst[9]);
    alOut(alFeedCon, 0); /* nConn is ignored, as in the original */
}

static void ALStopSound(void)
{
    alSound = NULL;
    alOut(alFreqH + 0, 0);
}

void SDA_Startup(void)
{
    int i;

    OPL2_Reset(&chip);
    for (i = 1; i <= 0xf5; i++) { /* SDL_DetectAdLib: zero all the registers */
        alOut(i, 0);
    }
    alOut(1, 0x20); /* WSE=1 */
    alOut(8, 0);    /* CSM=0, SEL=0 */

    alOut(alEffects, 0); /* SDL_StartAL */
    SetFXInst(NULL);

    alSound = NULL;
    alLengthLeft = 0;
    SoundNumber = SoundPriority = 0;
    sqHack = sqHackPtr = NULL;
    sqHackLen = sqHackSeqLen = 0;
    sqHackTime = alTimeCount = 0;
    sqActive = false;
    tickfrac = 0;
    count_fx = 0;
}

boolean SDA_PlaySound(const byte *chunk, size_t size, int number, int priority)
{
    const byte *inst;
    longword length;

    if (!chunk || size <= ADLIB_DATA) {
        return false;
    }
    length = (longword)chunk[0] | ((longword)chunk[1] << 8) | ((longword)chunk[2] << 16) |
             ((longword)chunk[3] << 24);
    if (length > size - ADLIB_DATA) {
        length = (longword)(size - ADLIB_DATA); /* not in the original: stay in the chunk */
    }
    inst = chunk + ADLIB_INST;
    if (!length || !(inst[6] | inst[7])) {
        return false; /* the original quits: zero length or "Bad instrument" */
    }

    ALStopSound();
    alLengthLeft = (word)(length > 0xffff ? 0xffff : length);
    alSound = chunk + ADLIB_DATA;
    alBlock = (byte)(((chunk[ADLIB_BLOCK] & 7) << 2) | 0x20);
    SetFXInst(NULL);
    SetFXInst(inst);

    SoundNumber = number;
    SoundPriority = priority;
    return true;
}

void SDA_StopSound(void)
{
    ALStopSound();
    SoundNumber = SoundPriority = 0;
}

int SDA_SoundPlaying(void)
{
    return alSound ? SoundNumber : 0;
}

int SDA_SoundPriority(void)
{
    return SoundPriority;
}

void SDA_MusicOff(void)
{
    int i;

    alOut(alEffects, 0);
    for (i = 0; i < sqMaxTracks; i++) {
        alOut(alFreqH + i + 1, 0);
    }
    sqActive = false;
}

void SDA_MusicOn(void)
{
    sqActive = true;
}

boolean SDA_MusicActive(void)
{
    return sqActive;
}

void SDA_StartMusic(const byte *chunk, size_t size)
{
    word length;

    SDA_MusicOff();
    if (!chunk || size < 2) {
        return;
    }
    length = ReadWord(chunk);
    if (length > size - 2 || (length & 3)) {
        length = (word)((size - 2) & ~(size_t)3); /* not in the original: stay in the chunk */
    }
    sqHackPtr = sqHack = chunk + 2;
    sqHackSeqLen = sqHackLen = length;
    sqHackTime = 0;
    alTimeCount = 0;
    SDA_MusicOn();
}

/* DOFX in ID_SD_A.ASM: the next byte of the effect, at 140 Hz */
static void SoundService(void)
{
    byte s;

    if (!alSound) {
        return;
    }
    s = *alSound;
    if (s) {
        alOut(alFreqL + 0, s);
        alOut(alFreqH + 0, alBlock);
    } else {
        alOut(alFreqH + 0, 0);
    }
    alSound++;
    if (!--alLengthLeft) {
        alSound = NULL;
        SoundNumber = SoundPriority = 0;
        alOut(alFreqH + 0, 0);
    }
}

/* the sequencer in SDL_t0FastAsmService, every 700 Hz tick */
static void MusicService(void)
{
    if (!sqActive) {
        return;
    }
    while (sqHackLen && sqHackTime <= alTimeCount) {
        const word w = ReadWord(sqHackPtr);

        sqHackTime = alTimeCount + ReadWord(sqHackPtr + 2);
        sqHackPtr += 4;
        alOut(w & 0xff, w >> 8);
        sqHackLen -= 4;
    }
    alTimeCount++;
    if (!sqHackLen) {
        sqHackPtr = sqHack;
        sqHackLen = sqHackSeqLen;
        alTimeCount = sqHackTime = 0;
    }
}

static void TimerService(void)
{
    if (++count_fx >= 5) {
        count_fx = 0;
        SoundService();
    }
    MusicService();
}

void SDA_Render(int32_t *out, int frames)
{
    int i;

    for (i = 0; i < frames; i++) {
        int32_t effects, music;

        SDA_RenderSplit(&effects, &music, 1);
        out[i] = effects + music;
    }
}

void SDA_RenderSplit(int32_t *effects, int32_t *music, int frames)
{
    int i;

    for (i = 0; i < frames; i++) {
        tickfrac += SDA_TICKRATE;
        if (tickfrac >= SDA_RATE) {
            tickfrac -= SDA_RATE;
            TimerService();
        }
        OPL2_SampleSplit(&chip, &effects[i], &music[i]);
    }
}

const opl2_t *SDA_Chip(void)
{
    return &chip;
}
