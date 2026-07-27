#pragma once

/*
 * Fixed-width types for the modern port (E2).
 * Replaces Borland `int`/`long`/`far`/`memptr = void _seg *` assumptions.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef uint8_t  byte;
typedef uint16_t word;
typedef uint32_t longword;
typedef int32_t  fixed;   /* 16.16 fixed-point */
typedef void    *memptr;

#ifndef __cplusplus
typedef bool boolean;
#endif

/* DOS far/near are flat pointers now (transitional only). */
#ifndef far
#define far
#endif
#ifndef near
#define near
#endif
#ifndef huge
#define huge
#endif
#ifndef _seg
#define _seg
#endif
