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
#include "tbls.h"
#include "dsp_sub.h"
#include "math_sub.h"
#include "encode.h"
#include "imbe_vocoder.h"

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
void imbe_vocoder::idct(Word16 *in, Word16 m_lim, Word16 i_lim, Word16 *out)
{
	UWord16 angl_step, angl_intl, angl_intl_2;
	UWord16 angl_acc;
	Word32  sum;
	Word16  i, m;

	if(m_lim == 1)
	{
		angl_intl   = CNST_0_5_Q1_15;
		angl_intl_2 = CNST_1_0_Q1_15;
	}
	else
	{
		angl_intl   = div_s ((Word16) CNST_0_5_Q5_11, m_lim << 11); // calculate 0.5/m_lim
		angl_intl_2 = shl(angl_intl, 1);
	}

	angl_step = angl_intl;
	for(i = 0; i < i_lim; i++)
	{
		sum = 0;
		angl_acc = angl_step;
		for(m = 1; m < m_lim; m++)
		{
			sum = L_add(sum, L_shr( L_mult(in[m], cos_fxp(angl_acc)), 7));
			angl_acc += angl_step;			
		}
		sum = L_add(sum, L_shr( L_deposit_h(in[0]), 8));
		out[i] = extract_l(L_shr_r (sum, 8)); 
		angl_step += angl_intl_2; 
	}
}

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
void imbe_vocoder::dct(Word16 *in, Word16 m_lim, Word16 i_lim, Word16 *out)
{
	UWord16 angl_step, angl_intl, angl_intl_2, angl_begin;
	UWord16 angl_acc;
	Word32  sum;
	Word16  i, m;

	if(m_lim == 1)
	{
		angl_intl   = CNST_0_5_Q1_15;
		angl_intl_2 = CNST_1_0_Q1_15;
	}
	else
	{
		angl_intl   = div_s ((Word16) CNST_0_5_Q5_11, m_lim << 11); // calculate 0.5/m_lim
		angl_intl_2 = shl(angl_intl, 1);
	}

	// Calculate first coefficient
	sum = 0;
	for(m = 0; m < m_lim; m++)
		sum = L_add(sum, L_deposit_l(in[m]));
	out[0] = extract_l(L_mpy_ls(sum, angl_intl_2));

	// Calculate the others coefficients
	angl_begin = angl_intl;
	angl_step  = angl_intl_2;
	for(i = 1; i < i_lim; i++)
	{
		sum = 0;
		angl_acc = angl_begin;
		for(m = 0; m < m_lim; m++)
		{
			sum = L_add(sum, L_deposit_l(mult(in[m], cos_fxp(angl_acc))));
			angl_acc += angl_step;			
		}
		out[i] = extract_l(L_mpy_ls(sum, angl_intl_2));

		angl_step  += angl_intl_2;  
		angl_begin += angl_intl;
	}
}




void imbe_vocoder::fft_init(void)
{
	Word16 i, fft_len2, shift, step, theta;

	fft_len2 = shr(FFTLENGTH, 1);
	shift    = norm_s(fft_len2);
	step     = shl(2, shift);
	theta    = 0;

	for(i = 0; i <= fft_len2; i++) 
	{
		wr_array[i] = cos_fxp(theta);    
		wi_array[i] = sin_fxp(theta);    
		if(i >= (fft_len2 - 1))
			theta = ONE_Q15;
		else
			theta = add(theta, step);
	}
}


//  	Subroutine FFT: Fast Fourier Transform 		
// ***************************************************************
// * Replaces data by its DFT, if isign is 1, or replaces data   *
// * by inverse DFT times nn if isign is -1.  data is a complex  *
// * array of length nn, input as a real array of length 2*nn.   *
// * nn MUST be an integer power of two.  This is not checked    *
// * The real part of the number should be in the zeroeth        *
// * of data , and the imaginary part should be in the next      *
// * element.  Hence all the real parts should have even indeces *
// * and the imaginary parts, odd indeces.			             *
// *                                                             * 
// * Data is passed in an array starting in position 0, but the  *
// * code is copied from Fortran so uses an internal pointer     *
// * which accesses position 0 as position 1, etc.		         *
// *                                                             *
// * This code uses e+jwt sign convention, so isign should be    *
// * reversed for e-jwt.                                         *
// ***************************************************************
//
// Q values:
// datam1 - Q14
// isign  - Q15 

#define	SWAP(a,b) temp1 = (a);(a) = (b); (b) = temp1

void imbe_vocoder::fft(Word16 *datam1, Word16 nn, Word16 isign)
{
	Word16 n, mmax, m, j, istep, i;
	Word16 wr, wi, temp1;
	Word32 L_tempr, L_tempi;
	Word16 *data;
	Word32 L_temp1, L_temp2;
	Word16 index, index_step;

	//  Use pointer indexed from 1 instead of 0	
	data = &datam1[-1];

	n = shl(nn,1);
	j = 1;
	for( i = 1; i < n; i+=2 ) 
	{
		if ( j > i) 
		{
			SWAP(data[j],data[i]);    
			SWAP(data[j+1],data[i+1]);   
		}
		m = nn;
		while ( m >= 2 && j > m ) 
		{
			j = sub(j,m);
			m = shr(m,1);
		}
		j = add(j,m);
	}
	mmax = 2;

	// initialize index step 
	index_step = nn;

	while ( n > mmax) 
	{
		istep = shl(mmax,1);  // istep = 2 * mmax 

		index = 0;
		index_step = shr(index_step,1);

		wr = ONE_Q15;
		wi = 0;
		for ( m = 1; m < mmax; m+=2) 
		{
			for ( i = m; i <= n; i += istep) 
			{
				j = i + mmax;

				// tempr = wr * data[j] - wi * data[j+1] 
				L_temp1 = L_shr(L_mult(wr,data[j]),1);
				L_temp2 = L_shr(L_mult(wi,data[j+1]),1);
				L_tempr = L_sub(L_temp1,L_temp2);

				// tempi = wr * data[j+1] + wi * data[j] 
				L_temp1 = L_shr(L_mult(wr,data[j+1]),1);
				L_temp2 = L_shr(L_mult(wi,data[j]),1);
				L_tempi = L_add(L_temp1,L_temp2);


				// data[j] = data[i] - tempr 
				L_temp1 = L_shr(L_deposit_h(data[i]),1);
				data[j] = round(L_sub(L_temp1,L_tempr));

				// data[i] += tempr 
				data[i] = round(L_add(L_temp1,L_tempr));

				// data[j+1] = data[i+1] - tempi 
				L_temp1 = L_shr(L_deposit_h(data[i+1]),1);
				data[j+1] = round(L_sub(L_temp1,L_tempi));

				// data[i+1] += tempi 
				data[i+1] = round(L_add(L_temp1,L_tempi));
			}
			index = add(index,index_step);
			wr = wr_array[index];
			if (isign < 0)
				wi = negate(wi_array[index]);
			else
				wi = wi_array[index];
		}
		mmax = istep;
	}
} 


