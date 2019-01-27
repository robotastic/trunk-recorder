/*
 * Copyright (C) 2010 mbelib Author
 * GPG Key ID: 0xEA5EFE2C (9E7A 5527 9CDC EBF7 BF1B  D772 4F98 E863 EA5E FE2C)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _MBELIB_H
#define _MBELIB_H

#define MBELIB_VERSION "1.2.3"

/* #include "config.h" */
#include <stdio.h>
#include <math.h>

struct mbe_parameters
{
  float w0;
  int L;
  int K;
  int Vl[57];
  float Ml[57];
  float log2Ml[57];
  float PHIl[57];
  float PSIl[57];
  float gamma;
  int un;
  int repeat;
};

struct mbe_tones
{
  int ID;
  int AD;
  int n;
};

typedef struct mbe_parameters mbe_parms;
typedef struct mbe_tones mbe_tone;

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Prototypes from mbelib.c
 */
void mbe_printVersion (char *str);
void mbe_moveMbeParms (mbe_parms * cur_mp, mbe_parms * prev_mp);
void mbe_useLastMbeParms (mbe_parms * cur_mp, mbe_parms * prev_mp);
void mbe_initMbeParms (mbe_parms * cur_mp, mbe_parms * prev_mp, mbe_parms * prev_mp_enhanced);
void mbe_initToneParms (mbe_tone * tone_mp);
void mbe_spectralAmpEnhance (mbe_parms * cur_mp);
void mbe_synthesizeSilencef (float *aout_buf);
void mbe_synthesizeSilence (short *aout_buf);
void mbe_synthesizeSpeechf (float *aout_buf, mbe_parms * cur_mp, mbe_parms * prev_mp, int uvquality);
void mbe_synthesizeSpeech (short *aout_buf, mbe_parms * cur_mp, mbe_parms * prev_mp, int uvquality);
void mbe_floattoshort (float *float_buf, short *aout_buf);

#ifdef __cplusplus
}
#endif
#endif
