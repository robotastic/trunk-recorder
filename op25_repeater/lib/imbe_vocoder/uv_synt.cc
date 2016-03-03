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
#include "dsp_sub.h"
#include "math_sub.h"
#include "uv_synt.h"
#include "rand_gen.h"
#include "tbls.h"
#include "encode.h"
#include "imbe_vocoder.h"




void imbe_vocoder::uv_synt_init(void)
{
	fft_init();
	v_zap(uv_mem, 105);
}






void imbe_vocoder::uv_synt(IMBE_PARAM *imbe_param, Word16 *snd)
{
	Cmplx16 Uw[FFTLENGTH];
	Word16 i, index_a, index_b, index_aux, ha, hb, *v_uv_dsn_ptr, *sa_ptr, sa;
    Word32 fund_freq, fund_freq_2, fund_freq_acc_a, fund_freq_acc_b;

	sa_ptr       = imbe_param->sa;
	v_uv_dsn_ptr = imbe_param->v_uv_dsn;
	fund_freq    = imbe_param->fund_freq;
	fund_freq_2  = L_shr(fund_freq, 1);

	v_zap((Word16 *)&Uw, 2 * FFTLENGTH);

	fund_freq_acc_a = L_sub(fund_freq, fund_freq_2);
	fund_freq_acc_b = L_add(fund_freq, fund_freq_2);

	for(i = 0; i < imbe_param->num_harms; i++)
	{
		ha = extract_h(fund_freq_acc_a);
		hb = extract_h(fund_freq_acc_b);
		index_a = (ha >> 8) + ((ha & 0xFF)?1:0);
		index_b = (hb >> 8) + ((hb & 0xFF)?1:0);

		index_aux = 256 - index_a;

		sa = shl(*sa_ptr, 3);

		if(*v_uv_dsn_ptr++)
		{
			while(index_a < index_b)
			{
				Uw[index_a].re   = Uw[index_a].im   = 0;
				Uw[index_aux].re = Uw[index_aux].im = 0;
				index_a++;
				index_aux--;
			}
		}
		else
		{			
			while(index_a < index_b)
			{
				Uw[index_a].re = mult(sa, rand_gen());
				Uw[index_a].im = mult(sa, rand_gen());
				//Uw[index_a].re = sa;
				//Uw[index_a].im = sa;

				Uw[index_aux].re = Uw[index_a].re;
				Uw[index_aux].im = negate(Uw[index_a].im);
				index_a++;
				index_aux--;
			}
		}

		fund_freq_acc_a = L_add(fund_freq_acc_a, fund_freq);
		fund_freq_acc_b = L_add(fund_freq_acc_b, fund_freq);
		sa_ptr++;
	}

/*
	j = 128;
	for(i = 0; i < 128; i++)
		Uw_tmp[j++] = Uw[i];

	j = 128;
	for(i = 0; i < 128; i++)
		Uw_tmp[i] = Uw[j++];

	for(i = 0; i < 256; i++)
		Uw[i] = Uw_tmp[i];
*/


	fft((Word16 *)&Uw, FFTLENGTH, -1);

	for(i = 0; i < 105; i++)
		snd[i] = uv_mem[i];

	index_aux = 73;
	for(i = 105; i < FRAME; i++)
		snd[i] = shl(Uw[index_aux++].re, 3);


	// Weighted Overlap Add Algorithm
	index_aux = 24;
	index_a   = 0;
	index_b   = 48;
	for(i = 56; i < 105; i++)
	{
		snd[i] = extract_h(L_add(L_mult(snd[i], ws[index_b]), L_mult(shl(Uw[index_aux].re, 3), ws[index_a])));

		index_aux++;
		index_a++;
		index_b--;
	}
	
	index_aux = 128;
	for(i = 0; i < 105; i++)
		uv_mem[i] = shl(Uw[index_aux++].re, 3);
}

