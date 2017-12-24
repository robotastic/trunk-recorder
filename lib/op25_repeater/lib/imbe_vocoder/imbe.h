/*
 * Project 25 IMBE Encoder/Decoder Fixed-Point implementation
 * Developed by Pavel Yazev E-mail: pyazev@gmail.com
 * Version 1.0 (c) Copyright 2009
 * 
 * This is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * The software is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this; see the file COPYING.  If not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Boston, MA
 * 02110-1301, USA.
 */


#ifndef _IMBE
#define _IMBE

#include "typedef.h"

#define FRAME             160   // Number samples in frame
#define NUM_HARMS_MAX      56   // Maximum number of harmonics
#define NUM_HARMS_MIN       9   // Minimum number of harmonics
#define NUM_BANDS_MAX      12   // Maximum number of bands
#define MAX_BLOCK_LEN      10   // Maximum length of block used during spectral amplitude encoding
#define NUM_PRED_RES_BLKS   6   // Number of Prediction Residual Blocks
#define PE_LPF_ORD         21   // Order of Pitch estimation LPF 
#define PITCH_EST_FRAME   301   // Pitch estimation frame size


#define B_NUM           (NUM_HARMS_MAX - 1)


typedef struct 
{
	Word16 e_p;
	Word16 pitch;                 // Q14.2
	Word16 ref_pitch;             // Q8.8 
	Word32 fund_freq;
	Word16 num_harms;
	Word16 num_bands;
	Word16 v_uv_dsn[NUM_HARMS_MAX];
	Word16 b_vec[NUM_HARMS_MAX + 3];
	Word16 bit_alloc[B_NUM + 4];
	Word16 sa[NUM_HARMS_MAX];
	Word16 l_uv;
	Word16 div_one_by_num_harm;
	Word16 div_one_by_num_harm_sh;
} IMBE_PARAM;

typedef struct  
{
	Word16 re;
	Word16 im;
} Cmplx16;

#if 0
void decode_init(IMBE_PARAM *imbe_param);
void decode(IMBE_PARAM *imbe_param, Word16 *frame_vector, Word16 *snd);
#endif

#endif
