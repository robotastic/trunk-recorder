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
#include "aux_sub.h"
#include "math_sub.h"
#include "dsp_sub.h"
#include "tbls.h"
#include "pitch_ref.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>


#define PITCH_REF_FRAME  221
#define MIN_INDEX         50 




void pitch_ref(IMBE_PARAM *imbe_param, Cmplx16 *fft_buf)
{
	Word16 i, j, index_a_save, pitch_est, tmp, shift, index_wr, up_lim;
	Cmplx16 sp_rec[FFTLENGTH/2];
	Word32 fund_freq, fund_freq_2, fund_freq_acc_a, fund_freq_acc_b, fund_freq_acc, L_tmp, amp_re_acc, amp_im_acc, L_sum, L_diff_min;
	Word16 ha, hb, index_a, index_b, index_tbl[20], it_ind, re_tmp, im_tmp, pitch_cand=0;
	Word32 fund_freq_cand=0;

	
	pitch_est = shl(imbe_param->pitch, 7);                      // Convert to Q8.8
	pitch_est = sub(pitch_est, CNST_1_125_Q8_8);                // Sub 1.125 = 9/8

	L_diff_min = MAX_32;
	for(i = 0; i < 19; i++)
	{
		shift = norm_s(pitch_est);
		tmp = shl(pitch_est, shift);
		tmp = div_s(0x4000, tmp);
		fund_freq = L_shl(tmp, shift + 11);

		fund_freq_acc   = fund_freq;
		fund_freq_2     = L_shr(fund_freq, 1);
		fund_freq_acc_a = L_sub(fund_freq, fund_freq_2);
		fund_freq_acc_b = L_add(fund_freq, fund_freq_2);

        // Calculate upper limit for spectrum reconstruction
		up_lim = extract_h(L_shr((UWord32)CNST_0_9254_Q0_16 * pitch_est, 1));  // 0.9254/fund_freq
		up_lim = sub(up_lim, CNST_0_5_Q8_8);                                   // sub 0.5
		up_lim = up_lim & 0xFF00;                                              // extract fixed part
		up_lim = mult(up_lim, extract_h(fund_freq));
		up_lim = shr(up_lim, 1);
				
		index_b = 0;
		while(index_b <= up_lim)
		{
			ha = extract_h(fund_freq_acc_a);
			hb = extract_h(fund_freq_acc_b);
			index_a = (ha >> 8) + ((ha & 0xFF)?1:0);
			index_b = (hb >> 8) + ((hb & 0xFF)?1:0);

			if(index_b >= MIN_INDEX)
			{			
				L_tmp = L_shl(L_deposit_h(index_a), 8);
				L_tmp = L_sub(L_tmp, fund_freq_acc);
				L_tmp = L_add(L_tmp, 0x00020000);   // for rounding purpose
				L_tmp = L_shr(L_tmp, 2);

				index_a_save = index_a;
				it_ind = 0;

				amp_re_acc = amp_im_acc = 0;
				while(index_a < index_b)
				{
					index_wr = extract_h(L_tmp);
					if(index_wr < 0 && (L_tmp & 0xFFFF)) // truncating for negative number
						index_wr = add(index_wr, 1);
					index_wr = add(index_wr, 160);
					index_tbl[it_ind++] = index_wr;				
					if(index_wr >= 0 && index_wr <= 320)
					{
						amp_re_acc = L_mac(amp_re_acc, fft_buf[index_a].re, wr_sp[index_wr]);
						amp_im_acc = L_mac(amp_im_acc, fft_buf[index_a].im, wr_sp[index_wr]);
					}
	
					index_a++;
					L_tmp = L_add(L_tmp, 0x400000);
				}

				it_ind = 0;
				index_a = index_a_save;			
				while(index_a < index_b)
				{
					index_wr = index_tbl[it_ind++];
					if(index_wr < 0 || index_wr > 320)
					{
						sp_rec[index_a].im = sp_rec[index_a].re = 0;
					}
					else
					{	
						sp_rec[index_a].im = mult(mult(extract_h(amp_im_acc), wr_sp[index_wr]), 0x6666);
						sp_rec[index_a].re = mult(mult(extract_h(amp_re_acc), wr_sp[index_wr]), 0x6666);
					}
					
					index_a++;
				}
			}

			fund_freq_acc_a = L_add(fund_freq_acc_a, fund_freq);
			fund_freq_acc_b = L_add(fund_freq_acc_b, fund_freq);
			fund_freq_acc   = L_add(fund_freq_acc,   fund_freq);
		}

		L_sum = 0;
		for(j = MIN_INDEX; j <= up_lim; j++)
		{
			re_tmp = sub(fft_buf[j].re, sp_rec[j].re);
			im_tmp = sub(fft_buf[j].im, sp_rec[j].im);
			L_sum = L_mac(L_sum, re_tmp, re_tmp);
			L_sum = L_mac(L_sum, im_tmp, im_tmp);
		}

		if(L_sum < L_diff_min)
		{
			L_diff_min     = L_sum;
			pitch_cand     = pitch_est;				
			fund_freq_cand = fund_freq;
		}
		
		pitch_est = add(pitch_est,  CNST_0_125_Q8_8); // Add 0.125 = 1/8
	}

	imbe_param->ref_pitch = pitch_cand;		
	imbe_param->fund_freq = fund_freq_cand;
}










