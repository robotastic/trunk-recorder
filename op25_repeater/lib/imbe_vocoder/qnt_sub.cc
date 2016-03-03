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
#include "qnt_sub.h"


//-----------------------------------------------------------------------------
//	PURPOSE:
//		Dequantize by quantizer step size
//     
//
//  INPUT:
//		qval         - quantized value
//      step_size    - step size used to quantize in unsigned Q0.16 format
//      bit_num      - the number of bits
//
//	OUTPUT:
//		None
//
//	RETURN:
//		Quantized Value in signed (bit_num).16 format
//
//-----------------------------------------------------------------------------
Word32 deqnt_by_step(Word16 qval, UWord16 step_size, Word16 bit_num)
{
	Word32 res;

	if(bit_num == 0)
		return (Word32)0;

	res = (Word32)step_size * (qval -  (1 << (bit_num - 1)));
	res = L_add(res, ((Word32)step_size * CNST_0_5_Q0_16) >> 16);

	return res;
}



//-----------------------------------------------------------------------------
//	PURPOSE:
//		Quantize by quantizer step size
//     
//
//  INPUT:
//		val          - value to be quantized in Q5.10 format
//      step_size    - step size used to quantize in unsigned Q0.16 format
//      bit_num      - the number of bits
//
//	OUTPUT:
//		None
//
//	RETURN:
//		Quantized Value in integer
//
//-----------------------------------------------------------------------------
Word16 qnt_by_step(Word16 val, UWord16 step_size, Word16 bit_num)
{
	Word16 index, min_val, max_val;
	Word16 q_index, shift, tmp;

	shift = norm_s(step_size);
	
	tmp = div_s(0x4000, shl(step_size, shift));     // Remark: To get result in Qxx.16 format it is necessary left shift tmp by (shift + 3) 
	q_index = shr_r(mult(val, tmp), sub(9, shift)); // q_index here is rounded to the nearest integer
	
	max_val = 1 << (bit_num - 1);
	min_val = negate(max_val);

	if(q_index < min_val)
		index = 0;
	else if(q_index >= max_val)
		index = (1 << bit_num) - 1;	
	else
		index = max_val + q_index;
			
	return index;
}

//-----------------------------------------------------------------------------
//	PURPOSE:
//		Quantize by table
//     
//
//  INPUT:
//		val          - value to be quantized 
//      q_tbl        - pointer to table
//      q_tbl_size   - size of table
//
//	OUTPUT:
//		None
//
//	RETURN:
//		Quantized Value in integer
//
//-----------------------------------------------------------------------------
Word16 tbl_quant(Word16 val, Word16 *q_tbl, Word16 q_tbl_size)
{
	Word16 min_index, max_index, index;

	min_index = 0;
	max_index = q_tbl_size - 1;

	if(val >= q_tbl[max_index])
		return max_index;

	if(val <= q_tbl[min_index])
		return min_index;

	while(max_index - min_index != 1)
	{
		index = min_index + ((max_index - min_index) >> 1);            

		if(q_tbl[index] > val)
			max_index = index;
		else
			min_index = index;
	}

	if(q_tbl[max_index] - val <= val - q_tbl[min_index])
		return max_index;
	else
		return min_index;
}
