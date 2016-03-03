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
#include "aux_sub.h"
#include "basic_op.h"
#include "math_sub.h"
#include "dc_rmv.h"


#define CNST_0_99_Q1_15   0x7EB8


//-----------------------------------------------------------------------------
//	PURPOSE:
//		High-pass filter to remove DC
//     
//
//  INPUT:
//		*sigin  - pointer to input signal buffer
//      *sigout - pointer to output signal buffer
//      *mem    - pointer to filter's memory element
//       len    - number of input signal samples
//
//	OUTPUT:
//		None
//
//	RETURN:
//       Saved filter state in mem
//
//-----------------------------------------------------------------------------
void dc_rmv(Word16 *sigin, Word16 *sigout, Word32 *mem, Word16 len)
{
	Word32 L_tmp, L_mem;

	L_mem = *mem;
	while(len--)
	{
		L_tmp = L_deposit_h(*sigin++);
		L_mem = L_add(L_mem, L_tmp);
		*sigout++ = round(L_mem);
		L_mem = L_mpy_ls(L_mem, CNST_0_99_Q1_15);
		L_mem = L_sub(L_mem, L_tmp);
	}
	*mem = L_mem;
}
