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

#include "typedef.h"
#include "globals.h"
#include "imbe.h"
#include "basic_op.h"
#include "ch_decode.h"
#include "aux_sub.h"
#include <iostream>


void decode_frame_vector(IMBE_PARAM *imbe_param, Word16 *frame_vector)
{
	Word16 bit_stream[BIT_STREAM_LEN];
	Word16 i, vec_num, tmp, tmp1, tmp2, bit_thr, shift;
	Word16 *b_ptr, *ba_ptr, index0;
	Word32 L_tmp;

	imbe_param->b_vec[0] = (shr(frame_vector[0], 4) & 0xFC) | (shr(frame_vector[7], 1) & 0x3);

	if (imbe_param->b_vec[0] < 0 || imbe_param->b_vec[0] > 207)
		return; // If we return here IMBE parameters from previous frame will be used (frame repeating)

	tmp = ((imbe_param->b_vec[0] & 0xFF) << 1) + 0x4F;                                      // Convert b_vec[0] to unsigned Q15.1 format and add 39.5

	//imbe_param->ff = 4./((double)imbe_param->b_vec[0] + 39.5);

	// Calculate fundamental frequency with higher precession
	shift = norm_s(tmp);
	tmp1  = tmp << shift;

	tmp2 = div_s(0x4000, tmp1);
	imbe_param->fund_freq = L_shr(L_deposit_h(tmp2), 11 - shift);

	L_tmp = L_sub(0x40000000, L_mult(tmp1, tmp2));
	tmp2  = div_s(extract_l(L_shr(L_tmp, 2)), tmp1);
	L_tmp = L_shr(L_deposit_l(tmp2), 11 - shift - 2);
	imbe_param->fund_freq = L_add(imbe_param->fund_freq, L_tmp);

	//printf("%X %X \n", imbe_param->fund_freq, (Word32)(imbe_param->ff * (double)((UWord32)1<<31)));

	tmp = (tmp + 0x2) >> 3;                                                                 // Calculate (b0 + 39.5 + 1)/4
	imbe_param->num_harms   = ((UWord32)CNST_0_9254_Q0_16 * tmp) >> 16;

	if(imbe_param->num_harms <= 36)
		imbe_param->num_bands = extract_h((UWord32)(imbe_param->num_harms + 2) * CNST_0_33_Q0_16);   // fix((L+2)/3)
	else
		imbe_param->num_bands = NUM_BANDS_MAX;

	// Convert input vector (from b_3 to b_L+1) to bit stream
	bit_stream[0] = (frame_vector[0] & 0x4)?1:0;
	bit_stream[1] = (frame_vector[0] & 0x2)?1:0;
	bit_stream[2] = (frame_vector[0] & 0x1)?1:0;

	bit_stream[BIT_STREAM_LEN - 3] = (frame_vector[7] & 0x40)?1:0;
	bit_stream[BIT_STREAM_LEN - 2] = (frame_vector[7] & 0x20)?1:0;
	bit_stream[BIT_STREAM_LEN - 1] = (frame_vector[7] & 0x10)?1:0;


	index0 = 3 + 3 * 12 - 1;
	for(vec_num = 3; vec_num >= 1;  vec_num--)
	{
		tmp = frame_vector[vec_num];
		for(i = 0; i < 12; i++)
		{
			bit_stream[index0] = (tmp & 0x1)?1:0;
			tmp >>= 1;
			index0--;
		}
	}

	index0 = 3 + 3 * 12 + 3 * 11 - 1;
	for(vec_num = 6; vec_num >= 4;  vec_num--)
	{
		tmp = frame_vector[vec_num];
		for(i = 0; i < 11; i++)
		{
			bit_stream[index0] = (tmp & 0x1)?1:0;
			tmp >>= 1;
			index0--;
		}
	}

	// Rebuild b1
	index0 = 3 + 3 * 12;
	tmp = 0;
	for(i = 0; i < imbe_param->num_bands; i++)
		tmp = (tmp << 1) | bit_stream[index0++];

	imbe_param->b_vec[1] = tmp;

	// Rebuild b2
	tmp = 0;
	tmp |= bit_stream[index0++] << 1;
	tmp |= bit_stream[index0++];
	imbe_param->b_vec[2] = (frame_vector[0] & 0x38) | (tmp << 1) | (shr(frame_vector[7], 3) & 0x01);

	// Shift the rest of sequence
	tmp = imbe_param->num_bands + 2;            // shift
	for(; index0 < BIT_STREAM_LEN; index0++)
		bit_stream[index0 - tmp] = bit_stream[index0];

    // Priority ReScanning
	b_ptr = &imbe_param->b_vec[3];
	ba_ptr = imbe_param->bit_alloc;
	for(i = 0; i < B_NUM; i++)
		ba_ptr[i] = b_ptr[i] = 0;


	// Unpack bit allocation table's item
	get_bit_allocation(imbe_param->num_harms, imbe_param->bit_alloc);

	index0 = 0;
	bit_thr = (imbe_param->num_harms == 0xb)?9:ba_ptr[0];

	while (index0 < BIT_STREAM_LEN - imbe_param->num_bands - 2)
	{
		for(i = 0; i < imbe_param->num_harms - 1; i++)
			if(bit_thr && bit_thr <= ba_ptr[i])
				b_ptr[i] = (b_ptr[i] << 1) | bit_stream[index0++];
		bit_thr--;
		if (bit_thr < 0) {
			std::cout << "Weird Error - imploder malfunction" << std::endl;
			break;
		}
	}

	// Synchronization Bit Decoding
	imbe_param->b_vec[imbe_param->num_harms + 2] = frame_vector[7] & 1;
}


void v_uv_decode(IMBE_PARAM *imbe_param)
{
	Word16 num_harms;
	Word16 num_bands;
	Word16 vu_vec, *p_v_uv_dsn, mask, i, uv_cnt;

	num_harms = imbe_param->num_harms;
    num_bands = imbe_param->num_bands;
	vu_vec    = imbe_param->b_vec[1];

	p_v_uv_dsn = imbe_param->v_uv_dsn;

	mask = 1 << (num_bands - 1);

	v_zap(p_v_uv_dsn, NUM_HARMS_MAX);

	i = 0; uv_cnt = 0;
	while(num_harms--)
	{
		if(vu_vec & mask)
			*p_v_uv_dsn++ = 1;
		else
		{
			*p_v_uv_dsn++ = 0;
			uv_cnt++;
		}

		if(++i == 3)
		{
			if(num_bands > 1)
			{
				num_bands--;
				mask >>= 1;
			}
			i = 0;
		}
	}
	imbe_param->l_uv = uv_cnt;
}
