#pragma once

/*
 * A Yamaha YM3812 (OPL2, the AdLib chip) emulator written for this port.
 * It follows the chip's documented behaviour: a 10-bit phase per operator
 * read through log-sine and exponent tables, envelopes in 0.1875 dB steps
 * with the chip's rate rules, key scaling, tremolo and vibrato LFOs, two
 * operators per channel in FM or additive mode with feedback on the first.
 * Rhythm mode is not emulated: Wolfenstein 3-D never switches it on (its
 * sound code keeps register 0xBD at 0 apart from the LFO depth bits).
 *
 * The chip runs at 3.579545 MHz / 72; OPL2_Sample returns the next sample at
 * that rate. Not bit-exact with a real chip, but close in timing and timbre.
 */

#include <stdint.h>

#define OPL2_RATE 49716 /* samples per second */

typedef struct {
    /* registers */
    uint8_t am, vib, egt, ksr, mult; /* 0x20 */
    uint8_t ksl, tl;                 /* 0x40 */
    uint8_t ar, dr;                  /* 0x60 */
    uint8_t sl, rr;                  /* 0x80 */
    uint8_t ws;                      /* 0xE0 */

    /* state */
    uint32_t phase;  /* one cycle is 1 << 20 */
    int env;         /* attenuation: 0 (loudest) .. 511 (silent), 0.1875 dB steps */
    int stage;       /* attack, decay, sustain, release */
    uint32_t egfrac; /* envelope steps as 16.16 */
    int out[2];      /* last two outputs (feedback reads both) */
} opl2_op_t;

typedef struct {
    uint16_t fnum;  /* 10 bits */
    uint8_t block;  /* octave, 3 bits */
    uint8_t keyon;
    uint8_t fb;     /* feedback 0-7 for the first operator */
    uint8_t cnt;    /* 0: FM (op 0 modulates op 1), 1: both added */
    opl2_op_t op[2];
} opl2_ch_t;

typedef struct {
    opl2_ch_t ch[9];
    uint8_t wse;      /* 0x01 bit 5: waveform select enabled */
    uint8_t nts;      /* 0x08 bit 6: note select for key scaling */
    uint8_t amdepth;  /* 0xBD bit 7: tremolo 4.8 dB instead of 1 dB */
    uint8_t vibdepth; /* 0xBD bit 6: vibrato 14 cents instead of 7 */
    uint32_t clock;   /* samples since reset, drives the LFOs */
} opl2_t;

/* All registers zero, every operator silent. */
void OPL2_Reset(opl2_t *chip);

/* A write to register reg (0x00-0xF5); writes to unused registers are ignored. */
void OPL2_Write(opl2_t *chip, int reg, int val);

/* The next output sample: the sum of the nine channels (roughly +-37000 at most). */
int32_t OPL2_Sample(opl2_t *chip);

/*
 * The same sample split into channel 0 and the other eight. Wolfenstein 3-D
 * plays its AdLib sound effects on channel 0 and music on channels 1-8, so
 * the two can have their own volume.
 */
void OPL2_SampleSplit(opl2_t *chip, int32_t *first, int32_t *rest);
