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

#ifndef _PE_LPF
#define _PE_LPF


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
void pe_lpf(Word16 *sigin, Word16 *sigout, Word16 *mem, Word16 len);


#endif
