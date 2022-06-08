
/*-
 * star_decode.h
 *  header for star_decode.c
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

#ifndef __STAR_DECODE_H__
#define __STAR_DECODE_H__

#include "star_common.h"
#include "star_types.h"

#define NDEC 4
#define THINCR (TWOPI / 8)

/*
 callback function is called based on format set when callback installed
*/

typedef void (*star_decoder_callback_t)(int unitID, int tag, int status, int message, void *context);

typedef struct {
  star_float_t sampleRate;
  star_u32_t phsr[NDEC];
  star_int_t phstate[NDEC];
  star_int_t lastbit[NDEC];
  star_int_t thisbit[NDEC];
  star_int_t rbit[NDEC];
  star_float_t theta[NDEC];
  star_float_t accum[NDEC];
  star_u32_t bitsr[NDEC];
  star_int_t bitstate[NDEC];
  star_u32_t bits0[NDEC];
  star_u32_t bits1[NDEC];
  star_u32_t bits2[NDEC];
  star_int_t bitcount[NDEC];
  star_u32_t lastBits0;
  star_int_t valid;
  star_decoder_callback_t callback;
  star_format callback_format;
  void *callback_context;
} star_decoder_t;

/*
 star_decoder_new
  create a new star_decoder_t object

  parameters: int sampleRate - the sampling rate in Hz

  returns: a star_decoder_t object or null if failure
 */

star_decoder_t *star_decoder_new(int sampleRate);

/*
 star_decoder_process_samples
  process incoming samples using a star_decoder_t object

  parameters: star_deocder_t *decoder - pointer to the decoder object
              star_sample_t *samples  - pointer to samples (in format set in star_types.h)
              int sampleCount         - count of number of samples in the buffer

  returns: 0 if more samples are needed
          -1 if an error occurs
           1 if a decoded packet is available to read (assuming no callback set)
*/

int star_decoder_process_samples(star_decoder_t *decoder, star_sample_t *samples, int sampleCount); // 8 bit version

/*
 star_decoder_get
  retrieve last successfully decoded packet from decoder object

  parameters: star_decoder_t *decoder - pointer to the decoder object
              star_format format      - format to apply to interpret bits
              int *unitID             - pointer to where to store "unid ID"
              int *tag                - pointer to where to store "tag"
              int *status             - pointer to where to store "status"
              int *message            - pointer to where to store "message"

  returns -1 if failure, 0 otherwise
*/

int star_decoder_get(star_decoder_t *decoder, star_format format, int *unitID, int *tag, int *status, int *message);

/*
 star_decoder_set_callback
  set a callback function to be called upon successful decode
  if this is set, the function star_decoder_get will no longer be functional,
  instead the callback function is called immediately when a successful decode happens
  from within the calling context of star_decoder_process_samples()
  the callback function will use the format set with callback_format

  the callback function will be called with (void *)context as its last parameter

  returns -1 if failure, 0 otherwise
*/

int star_decoder_set_callback(star_decoder_t *decoder, star_format callback_format, star_decoder_callback_t callbackFunction, void *context);

#endif
