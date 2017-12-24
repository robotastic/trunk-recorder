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


#ifndef _DSP_SUB
#define _DSP_SUB


#define CNST_0_5_Q1_15  0x4000
#define CNST_0_5_Q5_11  0x0400
#define CNST_1_0_Q1_15  0x7FFF


//-----------------------------------------------------------------------------
//	PURPOSE:
//				Perform inverse DCT
//
//
//  INPUT:
//              in     -  pointer to input data
//              m_lim  -  input data's size
//              i_lim  -  result's size
//              out    -  pointer to save result 
//
//	OUTPUT:
//		None
//
//	RETURN:
//		        Saved in out result of conversion
//
//-----------------------------------------------------------------------------
void idct(Word16 *in, Word16 m_lim, Word16 i_lim, Word16 *out);


//-----------------------------------------------------------------------------
//	PURPOSE:
//				Perform DCT
//
//
//  INPUT:
//              in     -  pointer to input data
//              m_lim  -  input data's size
//              i_lim  -  result's size
//              out    -  pointer to save result 
//
//	OUTPUT:
//		None
//
//	RETURN:
//		        Saved in out result of conversion
//
//-----------------------------------------------------------------------------
void dct(Word16 *in, Word16 m_lim, Word16 i_lim, Word16 *out);

#define FFTLENGTH 256


void fft_init(void);
void fft(Word16 *datam1, Word16 nn, Word16 isign);

void c_fft(Word16 * farray_ptr);

#endif
