#include "opl2.h"

#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

enum { EG_ATTACK, EG_DECAY, EG_SUSTAIN, EG_RELEASE };

#define ENV_MAX 511

/*
 * The chip computes in the log domain: a quarter sine as -log2(sin) in
 * 1/256 octave units, and 2^x back out of it, both as 256-entry tables.
 */
static uint16_t logsin[256];
static uint16_t exptab[256];
static int tables_ready;

/* MULT register -> frequency multiplier times two (0.5, 1, 2, ... 15) */
static const uint8_t multx2[16] = {1, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 20, 24, 24, 30, 30};

/* key scale level attenuation for the top four fnum bits at block 7, 0.75 dB units */
static const uint8_t kslrom[16] = {0, 32, 40, 45, 48, 51, 53, 55, 56, 58, 59, 60, 61, 62, 63, 64};

/* KSL register -> shift of the 6 dB/octave curve: off, 3, 1.5 and 6 dB/octave */
static const uint8_t kslshift[4] = {8, 1, 2, 0};

static void InitTables(void)
{
    int i;

    for (i = 0; i < 256; i++) {
        logsin[i] = (uint16_t)lround(-log2(sin((i + 0.5) * M_PI / 512.0)) * 256.0);
        exptab[i] = (uint16_t)lround((pow(2.0, i / 256.0) - 1.0) * 1024.0);
    }
    tables_ready = 1;
}

/* v >> n rounding down, also for negative v */
static int Asr(int v, int n)
{
    return v >= 0 ? v >> n : -((-v - 1) >> n) - 1;
}

/* --------------------------------------------------------------- operators */

/* Output for a 10-bit phase and an attenuation in envelope units: +-4084 at most. */
static int OpOutput(int wse, int ws, int phase, int att)
{
    unsigned p = (unsigned)phase & 1023;
    unsigned q = p & 255;
    int neg = 0;
    int level, shift, mag;

    switch (wse ? ws & 3 : 0) {
    case 0: /* sine */
        if (p & 256) {
            q = 255 - q;
        }
        neg = (p & 512) != 0;
        break;
    case 1: /* half sine: the negative half is silent */
        if (p & 512) {
            return 0;
        }
        if (p & 256) {
            q = 255 - q;
        }
        break;
    case 2: /* absolute sine */
        if (p & 256) {
            q = 255 - q;
        }
        break;
    default: /* quarter sine: the rising quarter of each half, then silence */
        if (p & 256) {
            return 0;
        }
        break;
    }

    level = logsin[q] + (att << 3);
    shift = level >> 8;
    if (shift > 12) {
        return 0;
    }
    mag = ((exptab[255 - (level & 255)] + 1024) << 1) >> shift;
    return neg ? -mag : mag;
}

/* Envelope rate 0-63 for a 4-bit rate register, with key scaling. */
static int EffectiveRate(const opl2_t *chip, const opl2_ch_t *ch, const opl2_op_t *op, int reg)
{
    int ksv, rate;

    if (!reg) {
        return 0; /* a zero rate never moves */
    }
    ksv = (ch->block << 1) | ((ch->fnum >> (chip->nts ? 8 : 9)) & 1);
    rate = reg * 4 + (op->ksr ? ksv : ksv >> 2);
    return rate > 63 ? 63 : rate;
}

/*
 * Envelope steps due this sample. Rate 4r+l moves (4+l)/4 steps every
 * 2^(13-r) samples: one step per 4096 samples at rate 4, seven per sample
 * at rate 63. Spread evenly with a 16.16 accumulator.
 */
static int EnvelopeSteps(opl2_op_t *op, int rate)
{
    int steps;

    if (!rate) {
        return 0;
    }
    op->egfrac += (uint32_t)(4 + (rate & 3)) << ((rate >> 2) + 1);
    steps = (int)(op->egfrac >> 16);
    op->egfrac &= 0xffff;
    return steps;
}

static void Envelope(const opl2_t *chip, const opl2_ch_t *ch, opl2_op_t *op)
{
    int rate, steps, sustain;

    switch (op->stage) {
    case EG_ATTACK:
        rate = EffectiveRate(chip, ch, op, op->ar);
        if (rate >= 60) {
            op->env = 0; /* the top rates attack instantly */
        } else {
            /* exponential: each step closes an eighth of the distance */
            steps = EnvelopeSteps(op, rate);
            while (steps-- > 0 && op->env > 0) {
                op->env -= (op->env + 8) >> 3;
            }
        }
        if (op->env <= 0) {
            op->env = 0;
            op->stage = EG_DECAY;
            op->egfrac = 0;
        }
        break;

    case EG_DECAY:
        sustain = (op->sl == 15 ? 31 : op->sl) << 4; /* 3 dB per SL step, 15 = 93 dB */
        if (op->env >= sustain) {
            op->stage = EG_SUSTAIN;
            break;
        }
        op->env += EnvelopeSteps(op, EffectiveRate(chip, ch, op, op->dr));
        if (op->env > ENV_MAX) {
            op->env = ENV_MAX;
        }
        break;

    case EG_SUSTAIN:
        /* a sustained sound holds until key off, a percussive one keeps fading */
        if (op->egt) {
            break;
        }
        /* fall through */
    default:
        op->env += EnvelopeSteps(op, EffectiveRate(chip, ch, op, op->rr));
        if (op->env > ENV_MAX) {
            op->env = ENV_MAX;
        }
        break;
    }
}

static int Tremolo(const opl2_t *chip)
{
    /* a 3.7 Hz triangle over 210 steps of 64 samples */
    int pos = (int)((chip->clock >> 6) % 210);
    int tri = pos < 105 ? pos : 209 - pos;

    return chip->amdepth ? tri >> 2 : tri >> 4; /* 4.875 or 1.125 dB */
}

static int Attenuation(const opl2_ch_t *ch, const opl2_op_t *op, int tremolo)
{
    int ksl = kslrom[ch->fnum >> 6] * 4 - (8 - ch->block) * 32;
    int att;

    if (ksl < 0) {
        ksl = 0;
    }
    att = op->env + op->tl * 4 + (ksl >> kslshift[op->ksl]) + (op->am ? tremolo : 0);
    return att > ENV_MAX ? ENV_MAX : att;
}

static uint32_t PhaseStep(const opl2_t *chip, const opl2_ch_t *ch, const opl2_op_t *op)
{
    int fnum = ch->fnum;

    if (op->vib) {
        /* a 6.1 Hz wave in eight steps of 1024 samples, scaled by the top fnum bits */
        const int pos = (int)((chip->clock >> 10) & 7);
        const int range = fnum >> 7;
        int delta = (pos & 3) == 0 ? 0 : (pos & 3) == 2 ? range : range >> 1;

        if (!chip->vibdepth) {
            delta >>= 1;
        }
        fnum += (pos & 4) ? -delta : delta;
    }
    return (((uint32_t)fnum << ch->block) * multx2[op->mult]) >> 1;
}

/* ------------------------------------------------------------------ chip */

void OPL2_Reset(opl2_t *chip)
{
    int c, o;

    if (!tables_ready) {
        InitTables();
    }
    memset(chip, 0, sizeof(*chip));
    for (c = 0; c < 9; c++) {
        for (o = 0; o < 2; o++) {
            chip->ch[c].op[o].env = ENV_MAX;
            chip->ch[c].op[o].stage = EG_RELEASE;
        }
    }
}

static void KeyOn(opl2_op_t *op)
{
    op->phase = 0;
    op->stage = EG_ATTACK;
    op->egfrac = 0;
}

void OPL2_Write(opl2_t *chip, int reg, int val)
{
    reg &= 0xff;
    val &= 0xff;

    if (reg == 0x01) {
        chip->wse = (uint8_t)((val >> 5) & 1);
        return;
    }
    if (reg == 0x08) {
        chip->nts = (uint8_t)((val >> 6) & 1);
        return;
    }
    if (reg == 0xbd) {
        chip->amdepth = (uint8_t)((val >> 7) & 1);
        chip->vibdepth = (uint8_t)((val >> 6) & 1);
        return; /* rhythm mode bits: not emulated */
    }

    if ((reg >= 0x20 && reg <= 0x95) || (reg >= 0xe0 && reg <= 0xf5)) {
        /* operator registers: three channels per group of eight offsets */
        const int off = reg & 0x1f;
        opl2_op_t *op;

        if (off > 0x15 || (off & 7) > 5) {
            return; /* no operator there */
        }
        op = &chip->ch[(off >> 3) * 3 + (off & 7) % 3].op[(off & 7) / 3];
        switch (reg & 0xe0) {
        case 0x20:
            op->am = (uint8_t)((val >> 7) & 1);
            op->vib = (uint8_t)((val >> 6) & 1);
            op->egt = (uint8_t)((val >> 5) & 1);
            op->ksr = (uint8_t)((val >> 4) & 1);
            op->mult = (uint8_t)(val & 15);
            break;
        case 0x40:
            op->ksl = (uint8_t)(val >> 6);
            op->tl = (uint8_t)(val & 63);
            break;
        case 0x60:
            op->ar = (uint8_t)(val >> 4);
            op->dr = (uint8_t)(val & 15);
            break;
        case 0x80:
            op->sl = (uint8_t)(val >> 4);
            op->rr = (uint8_t)(val & 15);
            break;
        default: /* 0xe0 */
            op->ws = (uint8_t)(val & 3);
            break;
        }
        return;
    }

    if (reg >= 0xa0 && reg <= 0xa8) {
        opl2_ch_t *ch = &chip->ch[reg - 0xa0];
        ch->fnum = (uint16_t)((ch->fnum & 0x300) | val);
        return;
    }
    if (reg >= 0xb0 && reg <= 0xb8) {
        opl2_ch_t *ch = &chip->ch[reg - 0xb0];
        const uint8_t keyon = (uint8_t)((val >> 5) & 1);

        ch->fnum = (uint16_t)((ch->fnum & 0xff) | ((val & 3) << 8));
        ch->block = (uint8_t)((val >> 2) & 7);
        if (keyon && !ch->keyon) {
            KeyOn(&ch->op[0]);
            KeyOn(&ch->op[1]);
        } else if (!keyon && ch->keyon) {
            ch->op[0].stage = EG_RELEASE;
            ch->op[1].stage = EG_RELEASE;
        }
        ch->keyon = keyon;
        return;
    }
    if (reg >= 0xc0 && reg <= 0xc8) {
        opl2_ch_t *ch = &chip->ch[reg - 0xc0];
        ch->fb = (uint8_t)((val >> 1) & 7);
        ch->cnt = (uint8_t)(val & 1);
    }
}

int32_t OPL2_Sample(opl2_t *chip)
{
    const int tremolo = Tremolo(chip);
    int32_t sum = 0;
    int c;

    for (c = 0; c < 9; c++) {
        opl2_ch_t *ch = &chip->ch[c];
        opl2_op_t *mod = &ch->op[0];
        opl2_op_t *car = &ch->op[1];
        int fb = 0, mo, co;

        Envelope(chip, ch, mod);
        Envelope(chip, ch, car);
        if (mod->env >= ENV_MAX && car->env >= ENV_MAX && mod->stage == EG_RELEASE &&
            car->stage == EG_RELEASE) {
            mod->out[0] = mod->out[1] = 0;
            continue; /* silent until the next key on */
        }

        if (ch->fb) {
            fb = Asr(mod->out[0] + mod->out[1], 9 - ch->fb);
        }
        mo = OpOutput(chip->wse, mod->ws, (int)(mod->phase >> 10) + fb,
                      Attenuation(ch, mod, tremolo));
        mod->out[1] = mod->out[0];
        mod->out[0] = mo;

        if (ch->cnt) {
            co = OpOutput(chip->wse, car->ws, (int)(car->phase >> 10),
                          Attenuation(ch, car, tremolo));
            sum += mo + co;
        } else {
            co = OpOutput(chip->wse, car->ws, (int)(car->phase >> 10) + mo,
                          Attenuation(ch, car, tremolo));
            sum += co;
        }

        mod->phase = (mod->phase + PhaseStep(chip, ch, mod)) & 0xfffff;
        car->phase = (car->phase + PhaseStep(chip, ch, car)) & 0xfffff;
    }
    chip->clock++;
    return sum;
}
