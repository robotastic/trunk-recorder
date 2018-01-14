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
#include "basic_op.h"
#include "imbe.h"
#include "aux_sub.h"
#include "tbls.h"

//-----------------------------------------------------------------------------
//	PURPOSE:
//		Return pointer to bit allocation array 
//      according to the number of harmonics
//
//  INPUT:
//		num_harms - The number of harmonics
//
//	OUTPUT:
//		None
//
//	RETURN:
//		Pointer to bits allocation array 
//
//-----------------------------------------------------------------------------
const UWord16 *get_bit_allocation_arr(Word16 num_harms)
{
	Word16 offset_in_word, index;

	if(num_harms == NUM_HARMS_MIN)
		return &bit_allocation_tbl[0];
	else
	{
		index = num_harms - NUM_HARMS_MIN - 1;
		offset_in_word = bit_allocation_offset_tbl[index >> 2] + ((3 + (index >> 2)) * (index & 0x3));
		return &bit_allocation_tbl[offset_in_word];
	}
}

//-----------------------------------------------------------------------------
//	PURPOSE:
//		Unpack bit allocation table's item 
//
//  INPUT:
//		num_harms - The number of harmonics
//      ptr       - Pointer to buffer to place bit allocation data
//
//	OUTPUT:
//		Unpacked bit allocation table
//
//	RETURN:
//		None 
//
//-----------------------------------------------------------------------------
void get_bit_allocation(Word16 num_harms, Word16 *ptr)
{
	const UWord16 *bat_ptr;
	Word16 i, tmp;

	bat_ptr = get_bit_allocation_arr(num_harms);

	for(i = 0; i < num_harms - 1; i += 4)
	{
		tmp = *bat_ptr++;
		ptr[3] = tmp & 0xF; tmp >>= 4;
		ptr[2] = tmp & 0xF; tmp >>= 4;
		ptr[1] = tmp & 0xF; tmp >>= 4;
		ptr[0] = tmp & 0xF; 
		ptr += 4;
	}
}

//-----------------------------------------------------------------------------
//	PURPOSE:
//		Set the elements of a 16 bit input vector to zero.
//
//  INPUT:
//		vec       - Pointer to vector
//      n         - size of vec
//
//	OUTPUT:
//		None
//
//	RETURN:
//		None 
//
//-----------------------------------------------------------------------------
void v_zap(Word16 *vec, Word16 n)
{
	while(n--)
		*vec++ = 0;
}

//-----------------------------------------------------------------------------
//	PURPOSE:
//		Copy the contents of one 16 bit input vector to another
//
//  INPUT:
//		vec1      - Pointer to the destination vector
//		vec2      - Pointer to the source vector
//      n         - size of data should be copied
//
//	OUTPUT:
//		Copy of the source vector
//
//	RETURN:
//		None 
//
//-----------------------------------------------------------------------------
void v_equ(Word16 *vec1, Word16 *vec2, Word16 n)
{
	while(n--)
		*vec1++ = *vec2++;
}

//-----------------------------------------------------------------------------
//	PURPOSE:
//			Compute the sum of square magnitude of a 16 bit input vector
//		    with saturation and truncation.  Output is a 32 bit number.
//
//	INPUT:
//		vec       - Pointer to the vector
//      n         - size of input vectors
//
//	OUTPUT:
//		none
//
//	RETURN:
//		32 bit long signed integer result  
//
//-----------------------------------------------------------------------------
Word32 L_v_magsq(Word16 *vec, Word16 n)
{
	Word32 L_magsq = 0;

	while(n--)
	{
		L_magsq = L_mac(L_magsq, *vec, *vec);
		vec++;
	}
	return L_magsq;
} 

//-----------------------------------------------------------------------------
//	PURPOSE:
//		Copy the contents of one 16 bit input vector to another with shift
//
//  INPUT:
//		vec1      - Pointer to the destination vector
//		vec2      - Pointer to the source vector
//      scale     - right shift factor
//      n         - size of data should be copied
//
//	OUTPUT:
//		Copy of the source vector
//
//	RETURN:
//		None 
//
//-----------------------------------------------------------------------------
void v_equ_shr(Word16 *vec1, Word16 *vec2, Word16 scale, Word16 n)
{
	while(n--)
		*vec1++ = shr(*vec2++,scale);   
}

