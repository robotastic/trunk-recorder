/*-
 * fsync_decode.h
 *  header for fsync_decode.c
 *
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

#ifndef _FSYNC_DECODE_H_
#define _FSYNC_DECODE_H_

#include "fsync_types.h"

#ifndef TWOPI
#define TWOPI (2.0 * 3.1415926535)
#endif

#define FSYNC_ND 10
#define FSYNC_ND_12 5

#define FSYNC_GDTHRESH 4  // "good bits" threshold

#define DIFFERENTIATOR

// #define ZEROCROSSING /* turn off for better correlator method */


typedef void (*fsync_decoder_callback_t)(int cmd, int subcmd, int from_fleet, int from_unit, int to_fleet, int to_unit, int allflag, unsigned char *payload, int payload_len, unsigned char *raw_msg, int raw_msg_len, void *context, int is_fsync2, int is_2400);


typedef struct {
	fsync_float_t hyst;
	fsync_float_t incr;
	fsync_float_t th[FSYNC_ND];
	fsync_float_t thx[FSYNC_ND];
	fsync_int_t level;
	fsync_float_t lastvalue;
	fsync_float_t accum0[FSYNC_ND];
	fsync_float_t accum1[FSYNC_ND];
	fsync_float_t accum0i[FSYNC_ND];
	fsync_float_t accum1i[FSYNC_ND];
	fsync_int_t zc[FSYNC_ND];
	fsync_int_t xorb[FSYNC_ND];
	fsync_u32_t synclow[FSYNC_ND];
	fsync_u32_t synchigh[FSYNC_ND];
	fsync_int_t shstate[FSYNC_ND];
	fsync_int_t shcount[FSYNC_ND];
	fsync_int_t fs2state[FSYNC_ND];
	fsync_int_t fs2w1[FSYNC_ND];
	fsync_int_t fs2w2[FSYNC_ND];
	fsync_int_t is_fs2[FSYNC_ND];
	fsync_u32_t word1[FSYNC_ND];
	fsync_u32_t word2[FSYNC_ND];
	fsync_u8_t message[FSYNC_ND][1536];
	fsync_int_t msglen[FSYNC_ND];
	fsync_int_t actives;
	fsync_decoder_callback_t callback;
	void *callback_context;
} fsync_decoder_t;
	


/*
 fsync_decoder_new
 create a new fsync_decoder object

  parameters: int sampleRate - the sampling rate in Hz

  returns: an fsync_decoder object or null if failure

*/
fsync_decoder_t * fsync_decoder_new(int sampleRate);

/*
 fsync_decoder_process_samples
 process incoming samples using an fsync_decoder object

 parameters: fsync_decoder_t *decoder - pointer to the decoder object
             fsync_sample_t *samples - pointer to samples (in format set in fsync_types.h)
             int numSamples - count of the number of samples in buffer

 returns: 0 if more samples are needed (usual case)
         -1 if an error occurs
*/
 
int fsync_decoder_process_samples(fsync_decoder_t *decoder,
                                fsync_sample_t *samples,
                                int numSamples);


/*
 fsync_decoder_end_samples
 notify decoder that no more samples will be forthcoming for some time
 (used to flush any pending messages, may invoke callback)

 returns: -1 if error, 0 otherwise
*/

int fsync_decoder_end_samples(fsync_decoder_t *decoder);

/*
 fsync_decoder_set_callback
 set a callback function to be called upon successful decode
 if this is set, the functions fsync_decoder_get_packet and fsync_decoder_get_double_packet
 will no longer be functional, instead the callback function is called immediately when
 a successful decode happens (from within the context of fsync_decoder_process_samples)

 last parameter of callback will be the (void *)context set here

 returns: -1 if error, 0 otherwise
 */

int fsync_decoder_set_callback(fsync_decoder_t *decoder, fsync_decoder_callback_t callback_function, void *context);

#endif
