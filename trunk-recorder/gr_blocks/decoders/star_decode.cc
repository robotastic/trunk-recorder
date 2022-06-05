
/*-
 * star_decode.c
 *  implementation of a 1600 Hz carrier, 400 bps PSK decoder
 *
 * Author: Matthew Kaufman (matthew@eeph.com)
 *
 * Copyright (c) 2012  Matthew Kaufman  All rights reserved.
 *
 *  This file is part of Matthew Kaufman's STAR Encoder/Decoder Library
 *
 *  The STAR Encoder/Decoder Library is free software; you can
 *  redistribute it and/or modify it under the terms of version 2 of
 *  the GNU General Public License as published by the Free Software
 *  Foundation.
 *
 *  If you cannot comply with the terms of this license, contact
 *  the author for alternative license arrangements or do not use
 *  or redistribute this software.
 *
 *  The STAR Encoder/Decoder Library is distributed in the hope
 *  that it will be useful, but WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 *  USA.
 *
 *  or see http://www.gnu.org/copyleft/gpl.html
 *
-*/

#include <stdlib.h> // for malloc

#include "star_decode.h"
#include "star_types.h"

static star_float_t _sintable[] = {
    0.000000, 0.024541, 0.049068, 0.073565, 0.098017, 0.122411, 0.146730, 0.170962,
    0.195090, 0.219101, 0.242980, 0.266713, 0.290285, 0.313682, 0.336890, 0.359895,
    0.382683, 0.405241, 0.427555, 0.449611, 0.471397, 0.492898, 0.514103, 0.534998,
    0.555570, 0.575808, 0.595699, 0.615232, 0.634393, 0.653173, 0.671559, 0.689541,
    0.707107, 0.724247, 0.740951, 0.757209, 0.773010, 0.788346, 0.803208, 0.817585,
    0.831470, 0.844854, 0.857729, 0.870087, 0.881921, 0.893224, 0.903989, 0.914210,
    0.923880, 0.932993, 0.941544, 0.949528, 0.956940, 0.963776, 0.970031, 0.975702,
    0.980785, 0.985278, 0.989177, 0.992480, 0.995185, 0.997290, 0.998795, 0.999699,
    1.000000, 0.999699, 0.998795, 0.997290, 0.995185, 0.992480, 0.989177, 0.985278,
    0.980785, 0.975702, 0.970031, 0.963776, 0.956940, 0.949528, 0.941544, 0.932993,
    0.923880, 0.914210, 0.903989, 0.893224, 0.881921, 0.870087, 0.857729, 0.844854,
    0.831470, 0.817585, 0.803208, 0.788346, 0.773010, 0.757209, 0.740951, 0.724247,
    0.707107, 0.689541, 0.671559, 0.653173, 0.634393, 0.615232, 0.595699, 0.575808,
    0.555570, 0.534998, 0.514103, 0.492898, 0.471397, 0.449611, 0.427555, 0.405241,
    0.382683, 0.359895, 0.336890, 0.313682, 0.290285, 0.266713, 0.242980, 0.219101,
    0.195090, 0.170962, 0.146730, 0.122411, 0.098017, 0.073565, 0.049068, 0.024541,
    0.000000, -0.024541, -0.049068, -0.073565, -0.098017, -0.122411, -0.146730, -0.170962,
    -0.195090, -0.219101, -0.242980, -0.266713, -0.290285, -0.313682, -0.336890, -0.359895,
    -0.382683, -0.405241, -0.427555, -0.449611, -0.471397, -0.492898, -0.514103, -0.534998,
    -0.555570, -0.575808, -0.595699, -0.615232, -0.634393, -0.653173, -0.671559, -0.689541,
    -0.707107, -0.724247, -0.740951, -0.757209, -0.773010, -0.788346, -0.803208, -0.817585,
    -0.831470, -0.844854, -0.857729, -0.870087, -0.881921, -0.893224, -0.903989, -0.914210,
    -0.923880, -0.932993, -0.941544, -0.949528, -0.956940, -0.963776, -0.970031, -0.975702,
    -0.980785, -0.985278, -0.989177, -0.992480, -0.995185, -0.997290, -0.998795, -0.999699,
    -1.000000, -0.999699, -0.998795, -0.997290, -0.995185, -0.992480, -0.989177, -0.985278,
    -0.980785, -0.975702, -0.970031, -0.963776, -0.956940, -0.949528, -0.941544, -0.932993,
    -0.923880, -0.914210, -0.903989, -0.893224, -0.881921, -0.870087, -0.857729, -0.844854,
    -0.831470, -0.817585, -0.803208, -0.788346, -0.773010, -0.757209, -0.740951, -0.724247,
    -0.707107, -0.689541, -0.671559, -0.653173, -0.634393, -0.615232, -0.595699, -0.575808,
    -0.555570, -0.534998, -0.514103, -0.492898, -0.471397, -0.449611, -0.427555, -0.405241,
    -0.382683, -0.359895, -0.336890, -0.313682, -0.290285, -0.266713, -0.242980, -0.219101,
    -0.195090, -0.170962, -0.146730, -0.122411, -0.098017, -0.073565, -0.049068, -0.024541};

static star_u32_t _star_crc(star_u32_t input) {
  star_u32_t crcsr;
  star_int_t bit;
  star_u32_t cur;
  star_int_t inv;

  crcsr = 0x2a;
  for (bit = 6; bit < (21 + 6); bit++) {
    cur = (input >> (32 - bit)) & 0x01;
    inv = cur ^ (0x01 & (crcsr >> 5));
    crcsr <<= 1;
    if (inv)
      crcsr ^= 0x2f;
  }

  return crcsr & 0x3f;
}

static void _reset_decoder(star_decoder_t *dec, star_int_t num) {

  dec->phsr[num] = 0;
  dec->phstate[num] = 0;
  dec->lastbit[num] = 0;
  dec->rbit[num] = 0;
  dec->accum[num] = 0.0;
  dec->bitsr[num] = 0;
  dec->bitstate[num] = 0;
}

static void _bitshift(star_decoder_t *dec, star_int_t num, star_int_t b) {
  star_int_t i;

  dec->bitsr[num] <<= 1;
  dec->bitsr[num] |= b;

  switch (dec->bitstate[num]) {
  case 0:
    if ((dec->bitsr[num] & 0x001f) == 0x0e) {
      dec->bitstate[num] = 1;
      dec->bitcount[num] = 32 - 5;
    } else if ((dec->bitsr[num] & 0x001f) == 0x11) {
      dec->bitsr[num] ^= 0x1f;
      dec->bitstate[num] = 1;
      dec->bitcount[num] = 32 - 5;
      dec->rbit[num] ^= 1;
    }
    break;
  case 1:
    if (--dec->bitcount[num] == 0) {
      dec->bits0[num] = dec->bitsr[num];
      dec->bitstate[num] = 2;
      dec->bitcount[num] = 32;
    }
    break;
  case 2:
    if (--dec->bitcount[num] == 0) {
      dec->bits1[num] = dec->bitsr[num];
      dec->bitstate[num] = 3;
      dec->bitcount[num] = 16;
    }
    break;
  case 3:
    if (--dec->bitcount[num] == 0) {
      dec->bits2[num] = dec->bitsr[num];
      dec->bits2[num] <<= 16;
      dec->bitstate[num] = 4; // unless we all get reset after success of this one

      // XXX TODO: smarter logic in here about making the best of the bits we get in error cases
      if ((dec->bits0[num] & 0x07ff0000) == ((~(dec->bits1[num])) & 0x07ff0000)) // 1fff0000 mod rbit
      {

        if (_star_crc(dec->bits0[num]) == (dec->bits0[num] & 0x3f) && ((dec->bits0[num] & 0x3f) == (dec->bits1[num] & 0x3f))) {
          // printf("decoder %d got %08x %08x %04x\n",num,dec->bits0[num],dec->bits1[num],dec->bits2[num] >> 16);
          // printf("unit id %d\n",(dec->bits0[num] >> 16) & 0x07ff);

          dec->valid = 1;
          dec->lastBits0 = dec->bits0[num];

          for (i = 0; i < NDEC; i++)
            _reset_decoder(dec, i);
        }
      }
    }
    break;
  case 4:
    // we get here if this decoder processed all the bits it could and it wasn't a success
    _reset_decoder(dec, num);
    break;
  }
}

static void _ph_shift(star_decoder_t *dec, star_int_t num, star_int_t ph) {
  star_int_t x;
  dec->phsr[num] <<= 1;
  dec->phsr[num] |= ph;

  switch (dec->phstate[num]) {
  case 0:
    if ((dec->phsr[num] & 0xffffffff) == 0xf0f0f0f0) // relax to allow even shorter preamble?
    {
      dec->phstate[num] = 1;
      dec->bitstate[num] = 0;
    }
    break;
  case 1:
    dec->phstate[num] = 2;
    break;
  case 2:
    dec->phstate[num] = 3;
    break;
  case 3:
    dec->phstate[num] = 4;
    break;
  case 4:
    x = dec->phsr[num] & 0x01;
    x += (dec->phsr[num] >> 1) & 0x01;
    x += (dec->phsr[num] >> 2) & 0x01;
    x += (dec->phsr[num] >> 3) & 0x01;

    if (x > 2)
      dec->thisbit[num] = 0x01;
    else
      dec->thisbit[num] = 0x00;

    if (dec->thisbit[num] ^ dec->lastbit[num])
      dec->rbit[num] ^= 0x01;
    _bitshift(dec, num, dec->rbit[num]);
    dec->lastbit[num] = dec->thisbit[num];
    dec->phstate[num] = 1;
    break;
  }
}

static void _process_sample_per(star_decoder_t *dec, star_int_t num, star_float_t s) {
  star_int_t ofs;

  dec->theta[num] += (TWOPI * 1600.0) / dec->sampleRate;

  ofs = (star_int_t)(dec->theta[num] * (256.0 / TWOPI));
  dec->accum[num] += (_sintable[ofs] * s);

  if (dec->theta[num] >= TWOPI) {
    if (dec->accum[num] < 0.0)
      _ph_shift(dec, num, 0);
    else
      _ph_shift(dec, num, 1);

    dec->theta[num] -= TWOPI;
    dec->accum[num] = 0.0;
  }
}

static void _process_sample(star_decoder_t *dec, star_float_t s) {
  star_int_t i;
  for (i = 0; i < NDEC; i++) {
    _process_sample_per(dec, i, s);
  }
}

// public API

star_decoder_t *star_decoder_new(int sampleRate) {
  star_decoder_t *dec;
  star_int_t i;

  dec = (star_decoder_t *)malloc(sizeof(star_decoder_t));

  if (dec) {

    dec->sampleRate = (star_float_t)sampleRate;
    dec->valid = 0;
    for (i = 0; i < NDEC; i++) {
      dec->theta[i] = THINCR * ((star_float_t)i);
      _reset_decoder(dec, i);
    }

    dec->callback = (star_decoder_callback_t)0;
  }
  return dec;
}

int star_decoder_process_samples(star_decoder_t *decoder, star_sample_t *samples, int sampleCount) {
  star_int_t i;
  star_float_t value;
  star_sample_t sample;

  if (!decoder)
    return -1;

  for (i = 0; i < sampleCount; i++) {
    sample = samples[i];

#if defined(STAR_SAMPLE_FORMAT_U8)
    value = (((star_float_t)sample) - 128.0) / 256;
#elif defined(STAR_SAMPLE_FORMAT_U16)
    value = (((star_float_t)sample) - 32768.0) / 65536.0;
#elif defined(STAR_SAMPLE_FORMAT_S16)
    value = ((star_float_t)sample) / 65536.0;
#elif defined(STAR_SAMPLE_FORMAT_FLOAT)
    value = sample;
#else
#error "no known sample format set"
#endif

    _process_sample(decoder, value);

    if (decoder->valid && decoder->callback) {
      int unitID, tag, status, message;

      if (-1 != star_decoder_get(decoder, decoder->callback_format, &unitID, &tag, &status, &message)) {
        decoder->callback(unitID, tag, status, message, decoder->callback_context);
      }
    }
  }
  return decoder->valid;
}

int star_decoder_get(star_decoder_t *decoder, star_format format, int *_unitID, int *_tag, int *_status, int *_message) {
  if (!decoder)
    return -1;

  if (decoder->valid) {
    int unitID = (decoder->lastBits0 >> 16) & 0x07ff;
    int tag = (decoder->lastBits0 >> 14) & 0x03;
    int status = (decoder->lastBits0 >> 10) & 0x0f;
    int message = (decoder->lastBits0 >> 6) & 0x0f;

    switch (format) {
    case star_format_1: // t1, t2, and s1 do not contribute to unit ID - original format, IDs to 2047
      break;

    case star_format_1_4095: // t1 = 2048, t2 used for mobile/portable, s1 ignored
      if (tag & 0x02)
        unitID += 2048;
      status &= 0x07;
      break;
    case star_format_1_16383: // t1 = 8192, t2 = 4096, s1 = 2048 used to allow unit IDs to 16383 -- most typical
      if (tag & 0x02)
        unitID += 8192;
      if (tag & 0x01)
        unitID += 4096;
      if (status & 0x08)
        unitID += 2048;
      status &= 0x07;
      tag = 0;
      break;
    case star_format_sys: // t1,t2 used for system ID, s1 = 2048
      if (status & 0x08)
        unitID += 2048;
      status &= 0x07;
      break;
    case star_format_2: // t1 = 4096, t2 = mobile/portable, s1 = 2048
      if (tag & 0x02)
        unitID += 4096;
      tag &= 0x01;
      if (status & 0x08)
        unitID += 2048;
      status &= 0x07;
      break;
    case star_format_3: // t1 = 4096, t2 = 8192, s1 = 2048
      if (tag & 0x02)
        unitID += 4096;
      if (tag & 0x01)
        unitID += 8192;
      tag = 0;
      if (status & 0x08)
        unitID += 2048;
      status &= 0x07;
      break;
    case star_format_4: // t1 = 4096, t2 = 2048, s1 = 8192
      if (tag & 0x02)
        unitID += 4096;
      if (tag & 0x01)
        unitID += 2048;
      tag = 0;
      if (status & 0x08)
        unitID += 8192;
      status &= 0x07;
      break;
    default:
      return -1;
    }

    if (_unitID)
      *_unitID = unitID;
    if (_tag)
      *_tag = tag;
    if (_status)
      *_status = status;
    if (_message)
      *_message = message;

    decoder->valid = 0;
  } else {
    return -1;
  }

  return 0;
}

int star_decoder_set_callback(star_decoder_t *decoder, star_format callback_format, star_decoder_callback_t callbackFunction, void *context) {
  if (!decoder)
    return -1;

  decoder->callback = callbackFunction;
  decoder->callback_format = callback_format;
  decoder->callback_context = context;

  return 0;
}
