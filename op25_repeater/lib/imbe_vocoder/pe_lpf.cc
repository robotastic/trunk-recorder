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
#include "pe_lpf.h"



static const Word16 lpf_coef[PE_LPF_ORD] =
{
	 -94,  -92,  185,   543,  288, -883, -1834, -495, 3891, 9141, 11512,        
	9141, 3891, -495, -1834, -883,  288,   543,  185,  -92,  -94
};


//-----------------------------------------------------------------------------
//	PURPOSE:
//		Low-pass filter for pitch estimator
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
void pe_lpf(Word16 *sigin, Word16 *sigout, Word16 *mem, Word16 len)
{
	Word16 i;
	Word32 L_sum;

	while(len--)
	{
		for(i = 0; i < PE_LPF_ORD - 1; i++)
			mem[i] = mem[i + 1];
		mem[PE_LPF_ORD - 1] = *sigin++;

		L_sum = 0;
		for(i = 0; i < PE_LPF_ORD; i++)
			L_sum = L_mac(L_sum, mem[i], lpf_coef[i]);

		*sigout++ = round(L_sum);
	}
}
