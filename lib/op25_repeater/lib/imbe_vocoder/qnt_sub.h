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

#ifndef _QNT_SUB
#define _QNT_SUB


#define CNST_0_5_Q0_16   0x8000


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
Word32 deqnt_by_step(Word16 qval, UWord16 step_size, Word16 bit_num);

//-----------------------------------------------------------------------------
//	PURPOSE:
//		Quantize by quantizer step size
//     
//
//  INPUT:
//		val          - value to be quantized 
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
Word16 qnt_by_step(Word16 val, UWord16 step_size, Word16 bit_num);

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
Word16 tbl_quant(Word16 val, Word16 *q_tbl, Word16 q_tbl_size);

#endif
