/*-
 * fsync_decode.c
 *   Decodes a specific format of 1200 BPS MSK data burst
 *   from input audio samples.
 *
 * Author: Matthew Kaufman (matthew@eeph.com)
 *
 * Copyright (c) 2012, 2013, 2014  Matthew Kaufman  All rights reserved.
 *
 *  This file is part of Matthew Kaufman's fsync Encoder/Decoder Library
 *
 *  The fsync Encoder/Decoder Library is free software; you can
 *  redistribute it and/or modify it under the terms of version 2 of
 *  the GNU General Public License as published by the Free Software
 *  Foundation.
 *
 *  If you cannot comply with the terms of this license, contact
 *  the author for alternative license arrangements or do not use
 *  or redistribute this software.
 *
 *  The fsync Encoder/Decoder Library is distributed in the hope
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

#include "fsync_decode.h"
#include <stdlib.h>

static float _sintable[] = {
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

static int _fsync_crc(int word1, int word2) {

  int paritybit = 0;
  int crcsr = 0;
  int bit;
  int cur;
  int invert;

  for (bit = 0; bit < 48; bit++) {
    if (bit < 32) {
      cur = (word1 >> (31 - bit)) & 0x01;
    } else {
      cur = (word2 >> (31 - (bit - 32))) & 0x01;
    }
    if (cur)
      paritybit ^= 1;
    invert = cur ^ (0x01 & (crcsr >> 15));
    if (invert) {
      crcsr ^= 0x6815;
    }
    crcsr <<= 1;
  }

  for (bit = 48; bit < 63; bit++) {
    cur = (word2 >> (31 - (bit - 32))) & 0x01;
    if (cur)
      paritybit ^= 1;
  }

  crcsr ^= 0x0002;
  crcsr += paritybit;
  return crcsr & 0xffff;
}

static int pcheck[] = {0x4045, 0x2067, 0x1076, 0x083b, 0x0458, 0x022c, 0x0116, 0x008b};
static int rpcheck[] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x17, 0x2e, 0x5c, 0xb8, 0x67, 0xce, 0x8b};

static int _fsync2_ecc_repair(int input) {
  int result = 0;
  int i, j;
  int s;

  for (i = 0; i < 8; i++) {
    s = 0;
    for (j = 0; j < 15; j++) {
      s += ((pcheck[i] >> j) & 1) * ((input >> j) & 1);
    }
    if (s & 1) {
      result += 1 << i;
    }
  }

  if (result == 0)
    return input;

  for (i = 0; i < 15; i++) {
    if (result == rpcheck[i]) {
      return input ^ (1 << (14 - i));
    }
  }
  return -1;
}

fsync_decoder_t *fsync_decoder_new(int sampleRate) {
  fsync_decoder_t *decoder;
  fsync_int_t i;

  decoder = (fsync_decoder_t *)malloc(sizeof(fsync_decoder_t));
  if (!decoder)
    return (fsync_decoder_t *)0L;

  decoder->hyst = 3.0 / 256.0;
  decoder->incr = (1200.0 * TWOPI) / ((fsync_float_t)sampleRate);
  decoder->actives = 0;
  decoder->level = 0;

  for (i = 0; i < FSYNC_ND; i++) {
    decoder->th[i] = 0.0 + (((fsync_float_t)i) * (TWOPI / (fsync_float_t)FSYNC_ND_12));
    while (decoder->th[i] >= TWOPI)
      decoder->th[i] -= TWOPI;
    decoder->thx[i] = decoder->th[i];
    decoder->zc[i] = 0;
    decoder->xorb[i] = 0;
    decoder->shstate[i] = 0;
    decoder->shcount[i] = 0;
    decoder->fs2state[i] = 0;
  }

  decoder->callback = (fsync_decoder_callback_t)0L;
  return decoder;
}

static void _dispatch(fsync_decoder_t *decoder, int x) {
  unsigned char payload[128];
  int paylen = 0;
  int aflag;
  int fleetflag;
  fsync_u8_t m0, m1;
  fsync_u8_t *msg = decoder->message[x];
  fsync_int_t msglen = decoder->msglen[x];
  fsync_int_t from_fleet, from_unit, to_fleet, to_unit;

  if (msg[0] & 0x01)
    aflag = 1;
  else
    aflag = 0;

  if (msg[1] & 0x01)
    fleetflag = 1;
  else
    fleetflag = 0;

  m0 = msg[0] & 0xfe;
  m1 = msg[1] & 0xfe;

  from_fleet = msg[2] + 99;
  from_unit = (msg[3] << 4) + ((msg[4] >> 4) & 0x0f) + 999;
  to_unit = ((msg[4] << 8) & 0xf00) + msg[5] + 999;

  if (fleetflag) {
    if (msglen < 7)
      return; // cannot dispatch, not enough data
    to_fleet = msg[6] + 99;
  } else {
    to_fleet = from_fleet;
  }

  if (from_fleet == 99)
    from_fleet = -1;
  if (to_fleet == 99)
    to_fleet = -1;
  if (to_unit == 999)
    to_unit = -1;
  if (from_unit == 999)
    from_unit = -1;

  if (m1 == 0x42) {
    int i, offset;

    if (msglen < 11)
      return; // truncated before length field, cannot dispatch

    paylen = (msg[9] << 8) + msg[10];

    // XXX unsure if shuffle is appropriate for GPS message (subcmd 0xf4)

    for (i = 0; i < paylen; i++) {
      offset = 11 + (6 * ((i / 6) + 1)) - (i % 6);
      if (offset >= msglen)
        return; // truncated message (XXX could instead dispatch what we have but with a truncated flag
      if ((unsigned int)i >= sizeof(payload))
        return; // should never happen unless paylen is corrupted

      payload[i] = msg[offset];
    }
  } else {
    paylen = 0;
  }

  if (decoder->callback)
    (decoder->callback)((int)m1, (int)m0, (int)from_fleet, (int)from_unit, (int)to_fleet, (int)to_unit, (int)aflag, payload, paylen, (unsigned char *)msg, (int)msglen, decoder->callback_context, decoder->is_fs2[x], x < FSYNC_ND_12 ? 0 : 1);
}

static void _procbits(fsync_decoder_t *decoder, int x) {
  int crc;

  crc = _fsync_crc(decoder->word1[x], decoder->word2[x]);

  if (crc == (int)(decoder->word2[x] & 0x0000ffff)) // valid fs crc?
  {
    decoder->message[x][decoder->msglen[x]++] = (decoder->word1[x] >> 24) & 0xff;
    decoder->message[x][decoder->msglen[x]++] = (decoder->word1[x] >> 16) & 0xff;
    decoder->message[x][decoder->msglen[x]++] = (decoder->word1[x] >> 8) & 0xff;
    decoder->message[x][decoder->msglen[x]++] = (decoder->word1[x] >> 0) & 0xff;
    decoder->message[x][decoder->msglen[x]++] = (decoder->word2[x] >> 24) & 0xff;
    decoder->message[x][decoder->msglen[x]++] = (decoder->word2[x] >> 16) & 0xff;
    // followed by 15 bits of crc and 1 parity bit (already checked)
    decoder->is_fs2[x] = 0;
    // go get next word, if there is one
    decoder->shstate[x] = 1;
    decoder->shcount[x] = 32;

#if 0
		// and abort rest

		// we no longer abort the rest, as we give each decoder a shot
		// at each component of a multi-part message

		for(i=0; i<FSYNC_ND; i++)
		{
			if(i != x)
			{
				decoder->shstate[i] = 0;
				decoder->fs2state[i] = 0;
			}
		}
#endif
  } else {
    // fs2?
    // (swap is deliberate)
    int ec1 = _fsync2_ecc_repair((decoder->word1[x]) & 0x0000ffff);
    int ec2 = _fsync2_ecc_repair((decoder->word1[x] >> 16) & 0x0000ffff);
    int ec3 = _fsync2_ecc_repair((decoder->word2[x]) & 0x0000ffff);
    int ec4 = _fsync2_ecc_repair((decoder->word2[x] >> 16) & 0x0000ffff);

    if (ec1 != -1 && ec2 != -1 && ec3 != -1 && ec4 != -4) {
      switch (decoder->fs2state[x]) {
      case 0:
        decoder->fs2w1[x] = (((ec1 >> 8) & 0xf) << 28) + (((ec2 >> 8) & 0xf) << 24) + (((ec3 >> 8) & 0xf) << 20) + (((ec4 >> 8) & 0xf) << 16);
        decoder->fs2state[x] = 1;
        decoder->shstate[x] = 1;
        decoder->shcount[x] = 32;
        break;
      case 1:
        decoder->fs2w1[x] |= (((ec1 >> 8) & 0xf) << 12) + (((ec2 >> 8) & 0xf) << 8) + (((ec3 >> 8) & 0xf) << 4) + (((ec4 >> 8) & 0xf));
        decoder->fs2state[x] = 2;
        decoder->shstate[x] = 1;
        decoder->shcount[x] = 32;
        break;
      case 2:
        decoder->fs2w2[x] = (((ec1 >> 8) & 0xf) << 28) + (((ec2 >> 8) & 0xf) << 24) + (((ec3 >> 8) & 0xf) << 20) + (((ec4 >> 8) & 0xf) << 16);
        decoder->fs2state[x] = 3;
        decoder->shstate[x] = 1;
        decoder->shcount[x] = 32;
        break;
      case 3:
        decoder->fs2w2[x] |= (((ec1 >> 8) & 0xf) << 12) + (((ec2 >> 8) & 0xf) << 8) + (((ec3 >> 8) & 0xf) << 4) + (((ec4 >> 8) & 0xf));

        crc = _fsync_crc(decoder->fs2w1[x], decoder->fs2w2[x]);

        if (crc == (decoder->fs2w2[x] & 0x0000ffff)) // valid fs crc?
        {
          decoder->message[x][decoder->msglen[x]++] = (decoder->fs2w1[x] >> 24) & 0xff;
          decoder->message[x][decoder->msglen[x]++] = (decoder->fs2w1[x] >> 16) & 0xff;
          decoder->message[x][decoder->msglen[x]++] = (decoder->fs2w1[x] >> 8) & 0xff;
          decoder->message[x][decoder->msglen[x]++] = (decoder->fs2w1[x] >> 0) & 0xff;
          decoder->message[x][decoder->msglen[x]++] = (decoder->fs2w2[x] >> 24) & 0xff;
          decoder->message[x][decoder->msglen[x]++] = (decoder->fs2w2[x] >> 16) & 0xff;

          decoder->is_fs2[x] = 1;

          decoder->fs2state[x] = 0;
          decoder->shstate[x] = 1;
          decoder->shcount[x] = 32;
        } else {
          // refactor (below)
          decoder->actives--;
          if (decoder->msglen[x] > 0) {
            if (decoder->actives) {
              // see below
            } else {
              _dispatch(decoder, x);
            }
          }
          decoder->shstate[x] = 0;
          decoder->fs2state[x] = 0;
        }
        break;
      default:
        decoder->fs2state[x] = 0;
        break;
      }
    } else {
      decoder->actives--;
      if (decoder->msglen[x] > 0) {
        if (decoder->actives) {
          // others are working, might decode a longer successful run
          // XXX this approach is a little sensitive to one of them re-syncing on something
        } else {
          _dispatch(decoder, x);
        }
      }
      decoder->shstate[x] = 0;
      decoder->fs2state[x] = 0;
    }
  }
}

static int _onebits(fsync_u32_t n) {
  int i = 0;
  while (n) {
    ++i;
    n &= (n - 1);
  }
  return i;
}

static void _shiftin(fsync_decoder_t *decoder, int x) {
  int bit = decoder->xorb[x];

  decoder->synchigh[x] <<= 1;
  if (decoder->synclow[x] & 0x80000000)
    decoder->synchigh[x] |= 1;

  decoder->synclow[x] <<= 1;
  if (bit)
    decoder->synclow[x] |= 1;

  switch (decoder->shstate[x]) {
  case 0:
    // sync 23eb or can also be 0x052B?
    if (_onebits(decoder->synchigh[x] ^ 0xaaaa23eb) < FSYNC_GDTHRESH || _onebits(decoder->synchigh[x] ^ 0xaaaa052b) < FSYNC_GDTHRESH) {
      decoder->actives++;
      // note, we do sync on the trailing 32 bits and skip state 1 here to ensure we can catch back-to-back messages with the same decoder phase
      decoder->word1[x] = decoder->synclow[x];
      decoder->shstate[x] = 2;
      decoder->shcount[x] = 32;
      decoder->msglen[x] = 0;
      decoder->fs2state[x] = 0;
    }
    return;
  case 1:
    if (--decoder->shcount[x] <= 0) {
      decoder->word1[x] = decoder->synclow[x];
      decoder->shcount[x] = 32;
      decoder->shstate[x] = 2;
    }

    return;
  case 2:
    if (--decoder->shcount[x] <= 0) {
      decoder->word2[x] = decoder->synclow[x];
      _procbits(decoder, x);
    }
    return;

  default:
    decoder->shstate[x] = 0; // should never happen
    decoder->fs2state[x] = 0;
    return;
  }
}

#ifdef ZEROCROSSING
static void _zcproc(fsync_decoder_t *decoder, int x) {
  // XXX optimize the 2400 case
  if (x >= FSYNC_ND_12) {
    switch (decoder->zc[x]) {
    case 1:
      decoder->xorb[x] = 1;
      break;
    case 2:
      decoder->xorb[x] = 0;
      break;
    default:
      return;
    }
  } else {
    switch (decoder->zc[x]) {
    case 2:
    case 4:
      decoder->xorb[x] = 1;
      break;
    case 3:
      decoder->xorb[x] = 0;
      break;
    default:
      return;
    }
  }
  _shiftin(decoder, x);
}
#endif

int fsync_decoder_process_samples(fsync_decoder_t *decoder,
                                  fsync_sample_t *samples,
                                  int numSamples) {
  fsync_int_t i, j;
  fsync_sample_t sample;
  fsync_float_t value;

#ifdef ZEROCROSSING
  fsync_int_t k;
  fsync_float_t delta;
#endif

  if (!decoder)
    return -1;

  for (i = 0; i < numSamples; i++) {
    sample = samples[i];

#if defined(FSYNC_SAMPLE_FORMAT_U8)
    value = (((fsync_float_t)sample) - 128.0) / 256;
#elif defined(FSYNC_SAMPLE_FORMAT_U16)
    value = (((fsync_float_t)sample) - 32768.0) / 65536.0;
#elif defined(FSYNC_SAMPLE_FORMAT_S16)
    value = ((fsync_float_t)sample) / 65536.0;
#elif defined(FSYNC_SAMPLE_FORMAT_FLOAT)
    value = sample;
#else
#error "no known sample format set"
#endif

#ifdef ZEROCROSSING

#ifdef DIFFERENTIATOR
    delta = value - decoder->lastvalue;
    decoder->lastvalue = value;

    if (decoder->level == 0) {
      if (delta > decoder->hyst) {
        for (k = 0; k < FSYNC_ND; k++)
          decoder->zc[k]++;
        decoder->level = 1;
      }
    } else {
      if (delta < (-1 * decoder->hyst)) {
        for (k = 0; k < FSYNC_ND; k++)
          decoder->zc[k]++;
        decoder->level = 0;
      }
    }
#else  /* DIFFERENTIATOR */
    if (decoder->level == 0) {
      if (s > decoder->hyst) {
        for (k = 0; k < FSYNC_ND; k++)
          decoder->zc[k]++;
        decoder->level = 1;
      }
    } else {
      if (s < (-1.0 * decoder->hyst)) {
        for (k = 0; k < FSYNC_ND; k++)
          decoder->zc[k]++;
        decoder->level = 0;
      }
    }
#endif /* DIFFERENTIATOR */

    for (j = 0; j < FSYNC_ND; j++) {
      if (j < FSYNC_ND_12)
        decoder->th[j] += decoder->incr;
      else
        decoder->th[j] += 2 * (decoder->incr); // 2400

      if (decoder->th[j] >= TWOPI) {
        _zcproc(decoder, j);
        decoder->th[j] -= TWOPI;
        decoder->zc[j] = 0;
      }
    }
#else /* ZEROCROSSING */

    for (j = 0; j < FSYNC_ND; j++) {
      int th_zero, th_one;
      if (j < FSYNC_ND_12) {
        th_zero = (int)((256.0 / TWOPI) * decoder->thx[j]);
        th_one = (int)((256.0 / TWOPI) * decoder->th[j]);
      } else {
        th_zero = (int)((256.0 / TWOPI) * decoder->th[j]);
        th_one = (int)((256.0 / TWOPI) * decoder->thx[j]);
      }

      decoder->accum0[j] += _sintable[th_zero] * value;
      decoder->accum1[j] += _sintable[th_one] * value;

      decoder->accum0i[j] += _sintable[(th_zero + 64) % 256] * value;
      decoder->accum1i[j] += _sintable[(th_one + 64) % 256] * value;
      //

      if (j < FSYNC_ND_12) {
        decoder->th[j] += decoder->incr;
        decoder->thx[j] += 1.5 * decoder->incr;
      } else {
        decoder->th[j] += 2 * (decoder->incr);
        decoder->thx[j] += decoder->incr;
      }

      while (decoder->thx[j] >= TWOPI)
        decoder->thx[j] -= TWOPI;

      if (decoder->th[j] >= TWOPI) {
        if (
            (decoder->accum0[j] * decoder->accum0[j]) + (decoder->accum0i[j] * decoder->accum0i[j]) > (decoder->accum1[j] * decoder->accum1[j]) + (decoder->accum1i[j] * decoder->accum1i[j]))
          decoder->xorb[j] = 0;
        else
          decoder->xorb[j] = 1;

        decoder->accum0[j] = decoder->accum1[j] = 0.0;
        decoder->accum0i[j] = decoder->accum1i[j] = 0.0;

        _shiftin(decoder, j);

        decoder->th[j] -= TWOPI;
      }
    }

#endif
  }

  // XXX callback only -- no longer return 1 if good and have a way to get message. ok?

  return 0;
}

int fsync_decoder_end_samples(fsync_decoder_t *decoder) {
  int i, j;

  if (!decoder)
    return -1;

  for (i = 0; i < 70; i++) {
    for (j = 0; j < FSYNC_ND; j++) {
      _shiftin(decoder, j);
    }
  }

  return 0;
}

int fsync_decoder_set_callback(fsync_decoder_t *decoder, fsync_decoder_callback_t callback_function, void *context) {
  if (!decoder)
    return -1;

  decoder->callback = callback_function;
  decoder->callback_context = context;

  return 0;
}
