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
#include "ch_encode.h"
#include "aux_sub.h"


void encode_frame_vector(IMBE_PARAM *imbe_param, Word16 *frame_vector)
{
	Word16 bit_stream[EN_BIT_STREAM_LEN], index0, bit_thr, bit_mask, i;
	Word16 vec_num, num_harms, num_bands, tmp;
	Word16 *ba_ptr, *b_ptr;

	num_harms = imbe_param->num_harms;
	num_bands = imbe_param->num_bands;

	v_zap(frame_vector, 8);

	// Unpack bit allocation table's item
	get_bit_allocation(num_harms, imbe_param->bit_alloc);

	// Priority Scanning
	index0   = 0;
	ba_ptr   = imbe_param->bit_alloc;
	bit_thr  = (num_harms == 0xb)?9:ba_ptr[0];
	bit_mask = shl(1, bit_thr - 1);

	while(index0 < EN_BIT_STREAM_LEN - (num_bands - 3))
	{
		b_ptr = &imbe_param->b_vec[3];
		for(i = 0; i < num_harms - 1; i++)
			if(bit_thr && bit_thr <= ba_ptr[i])
				bit_stream[index0++] = (b_ptr[i] & bit_mask)?1:0;

		bit_thr--;
		bit_mask = shr(bit_mask, 1);
	}

	frame_vector[0] = shl(imbe_param->b_vec[0] & 0xFC, 4) | imbe_param->b_vec[2] & 0x38;

	index0 = 0;
	frame_vector[0] |= (bit_stream[index0++])?4:0;
	frame_vector[0] |= (bit_stream[index0++])?2:0;
	frame_vector[0] |= (bit_stream[index0++])?1:0;

	for(vec_num = 1; vec_num <= 3; vec_num++)
	{
		tmp = 0;
		for(i = 0; i < 12; i++)
		{
			tmp <<= 1;
			tmp |= bit_stream[index0++];
		}
		frame_vector[vec_num] = tmp;
	}

	index0 -= num_bands + 2;

	bit_mask = shl(1, num_bands - 1);
	for(i = 0; i < num_bands; i++)
	{
		bit_stream[index0++] = (imbe_param->b_vec[1] & bit_mask)?1:0;
		bit_mask >>= 1;
	}

	bit_stream[index0++] = (imbe_param->b_vec[2] & 0x04)?1:0;
	bit_stream[index0++] = (imbe_param->b_vec[2] & 0x02)?1:0;

	index0 -= num_bands + 2;

	for(vec_num = 4; vec_num <= 6; vec_num++)
	{
		tmp = 0;
		for(i = 0; i < 11; i++)
		{
			tmp <<= 1;
			tmp |= bit_stream[index0++];
		}
		frame_vector[vec_num] = tmp;
	}

	frame_vector[7] = shl(imbe_param->b_vec[0] & 0x03, 1) | shl(imbe_param->b_vec[2] & 0x01, 3);
	frame_vector[7] |= (bit_stream[index0++])?0x40:0;
	frame_vector[7] |= (bit_stream[index0++])?0x20:0;
	frame_vector[7] |= (bit_stream[index0++])?0x10:0;
	frame_vector[7] |= (imbe_param->b_vec[num_harms + 2])?0x01:0;
}
