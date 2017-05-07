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
#include "v_uv_det.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "encode.h"
#include "imbe_vocoder.h"


#define CNST_0_5625_Q1_15   0x4800
#define CNST_0_45_Q1_15     0x3999
#define CNST_0_1741_Q1_15   0x164A
#define CNST_0_1393_Q1_15   0x11D5
#define CNST_0_99_Q1_15     0x7EB8
#define CNST_0_01_Q1_15     0x0148
#define CNST_0_0025_Q1_15   0x0051
#define CNST_0_25_Q1_15     0x2000
#define CNST_PI_4_Q1_15     0x6488
#define CNST_0_55_Q4_12     0x08CD







extern FILE *fp_in, *fp_out;
extern int frame_cnt;


void imbe_vocoder::pitch_ref_init(void)
{	
	v_zap(v_uv_dsn, NUM_BANDS_MAX);
	th_max = 0;
}

Word16 imbe_vocoder::voiced_sa_calc(Word32 num, Word16 den)
{
	Word16 tmp;
	Word32 L_tmp;

	L_tmp = L_mpy_ls(num, den);
	L_tmp = sqrt_l_exp(L_tmp, &tmp);				
	L_tmp = L_shr(L_tmp, tmp - 3);
	
	//L_tmp =0;
	//return (Word16)(2*256.0*sqrt(2*(double)num/(double)den));
	return extract_h(L_tmp);
}

Word16 imbe_vocoder::unvoiced_sa_calc(Word32 num, Word16 den)
{
	Word16 shift, tmp;
	Word32 L_tmp;

	shift = norm_s(den);
	tmp   = div_s(0x4000, shl(den, shift));
	L_tmp = L_shl(L_mpy_ls(num, tmp), shift + 2);
	L_tmp = sqrt_l_exp(L_tmp, &tmp);				
	L_tmp = L_shr(L_tmp, tmp - 2 + 6);
	L_tmp = L_mpy_ls(L_tmp, 0x4A76);

	//L_tmp =0;
	//return (Word16)(2*0.1454 * sqrt(2*256*(double)num/(double)den));
	return extract_h(L_tmp);
}

//=============================================================================
//
// Voiced/Unvoiced Determination & Spectral Amplitudes Estimation
//
//=============================================================================
void imbe_vocoder::v_uv_det(IMBE_PARAM *imbe_param, Cmplx16 *fft_buf)
{
	Word16 i, j, index_a_save, tmp, index_wr;
	Word32 fund_freq, fund_freq_2, fund_freq_acc_a, fund_freq_acc_b, fund_freq_acc, fund_fr_acc, L_tmp, amp_re_acc, amp_im_acc;
	Word16 ha, hb, index_a, index_b, index_tbl[30], it_ind, re_tmp, im_tmp, re_tmp2, im_tmp2, sc_coef;
	Word32 Sw_sum, M_num[NUM_HARMS_MAX], M_num_sum, M_den_sum, D_num, D_den, th_lf, th_hf, th0, fund_fr_step, M_fcn_num, M_fcn_den; 
	Word16 sp_rec_re, sp_rec_im, M_fcn;
	Word16 band_cnt, num_harms_cnt, uv_harms_cnt,  Dk;
	Word16 num_harms, num_bands, dsn_thr=0;
	Word16 thr[NUM_BANDS_MAX], M_den[NUM_HARMS_MAX], b1_vec;


	fund_freq = imbe_param->fund_freq;

	tmp = shr( add( shr(imbe_param->ref_pitch, 1),  CNST_0_25_Q8_8), 8);     // fix(pitch_cand / 2 + 0.5)
	num_harms = extract_h((UWord32)CNST_0_9254_Q0_16 * tmp);                 // fix(0.9254 * fix(pitch_cand / 2 + 0.5))
	if(num_harms < NUM_HARMS_MIN)
		num_harms = NUM_HARMS_MIN;
	else if(num_harms > NUM_HARMS_MAX)
		num_harms = NUM_HARMS_MAX;

	if(num_harms <= 36)
		num_bands = extract_h((UWord32)(num_harms + 2) * CNST_0_33_Q0_16);   // fix((L+2)/3)
	else
		num_bands = NUM_BANDS_MAX;

	imbe_param->num_harms = num_harms;
	imbe_param->num_bands = num_bands;

	//=========================================================================
	//
	// M(th) function calculation
	//
	//=========================================================================
	for(j = 0, th_lf = 0; j < 64; j++)
	{
		th_lf = L_mac(th_lf, fft_buf[j].re, fft_buf[j].re);
		th_lf = L_mac(th_lf, fft_buf[j].im, fft_buf[j].im);
	}
	for(j = 64, th_hf = 0; j < 128; j++)
	{
		th_hf = L_mac(th_hf, fft_buf[j].re, fft_buf[j].re);
		th_hf = L_mac(th_hf, fft_buf[j].im, fft_buf[j].im);
	}
	th0 = L_add(th_lf, th_hf);

	if(th0 > th_max)
		th_max = L_shr(L_add(th_max, th0), 1);
	else
		th_max = L_add(L_mpy_ls(th_max, CNST_0_99_Q1_15), L_mpy_ls(th0, CNST_0_01_Q1_15));

	M_fcn_num = L_add(th0, L_mpy_ls(th_max, CNST_0_0025_Q1_15));
	M_fcn_den = L_add(th0, L_mpy_ls(th_max, CNST_0_01_Q1_15));
	if(M_fcn_den == 0)
		M_fcn = CNST_0_25_Q1_15;     
	else
	{
		tmp       = norm_l(M_fcn_den);
		M_fcn_den = L_shl(M_fcn_den, tmp);
		M_fcn_num = L_shl(M_fcn_num, tmp);		

		M_fcn = div_s(extract_h(M_fcn_num), extract_h(M_fcn_den));

		if(th_lf < (L_tmp= L_add(L_shl(th_hf, 2), th_hf)))           // compare th_lf < 5*th_hf
		{
			tmp       = norm_l(L_tmp);
			M_fcn_den = L_shl(L_tmp, tmp);
			th_lf     = L_shl(th_lf, tmp);		

			tmp   = div_s(extract_h(th_lf), extract_h(M_fcn_den));	
			L_tmp = sqrt_l_exp(L_deposit_h(tmp),  &tmp);
			if(tmp)
				L_tmp = L_shr(L_tmp, tmp);
			M_fcn = mult(M_fcn, extract_h(L_tmp));
		}
	}
	// ========================================================================
	fund_fr_step  = L_shl(L_mpy_ls(fund_freq, CNST_PI_4_Q1_15), 2);  // mult by PI

	uv_harms_cnt    = 0;
	b1_vec          = 0;
	band_cnt        = 0;
	num_harms_cnt   = 0;
	Sw_sum          = 0;	
	D_num = D_den   = 0;

	fund_fr_acc     = 0;
	fund_freq_acc   = fund_freq;
	fund_freq_2     = L_shr(fund_freq, 1);
	fund_freq_acc_a = L_sub(fund_freq, fund_freq_2);
	fund_freq_acc_b = L_add(fund_freq, fund_freq_2);		
	for(j = 0; j < num_harms; j++)
	{
		ha = extract_h(fund_freq_acc_a);
		hb = extract_h(fund_freq_acc_b);
		index_a = (ha >> 8) + ((ha & 0xFF)?1:0);
		index_b = (hb >> 8) + ((hb & 0xFF)?1:0);

		L_tmp = L_shl(L_deposit_h(index_a), 8);
		L_tmp = L_sub(L_tmp, fund_freq_acc);
		L_tmp = L_add(L_tmp, 0x00020000);   // for rounding purpose
		L_tmp = L_shr(L_tmp, 2);

		index_a_save = index_a;
		it_ind = 0;

		// =========== v/uv determination threshold function ==
		if(num_harms_cnt == 0)   // calculate one time per band
		{
			if(imbe_param->e_p > CNST_0_55_Q4_12 && band_cnt >= 1)
				dsn_thr = 0;
			else if(v_uv_dsn[band_cnt] == 1)
				dsn_thr = mult(M_fcn, sub(CNST_0_5625_Q1_15, mult(CNST_0_1741_Q1_15, extract_h(fund_fr_acc))));
			else
				dsn_thr = mult(M_fcn, sub(CNST_0_45_Q1_15,   mult(CNST_0_1393_Q1_15, extract_h(fund_fr_acc))));

			fund_fr_acc = L_add(fund_fr_acc, fund_fr_step);

			thr[band_cnt] = dsn_thr;
		}
		// ====================================================

		M_den_sum  = 0;
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
				M_den_sum  = L_add(M_den_sum, mult(wr_sp[index_wr], wr_sp[index_wr]));
			}

			index_a++;
			L_tmp = L_add(L_tmp, 0x400000);
		}
		sc_coef = div_s(0x4000, extract_l(L_shr(M_den_sum, 1)));
		im_tmp2 = mult(extract_h(amp_im_acc), sc_coef);
		re_tmp2 = mult(extract_h(amp_re_acc), sc_coef);

		M_num_sum = 0;		
		it_ind    = 0;
		index_a   = index_a_save;	
		while(index_a < index_b)
		{
			index_wr = index_tbl[it_ind++];
			if(index_wr < 0 || index_wr > 320)
				sp_rec_re = sp_rec_im = 0;
			else
			{	
				sp_rec_im = mult( im_tmp2, wr_sp[index_wr]);
				sp_rec_re = mult( re_tmp2, wr_sp[index_wr]);
			}

			re_tmp = sub(fft_buf[index_a].re, sp_rec_re);
			im_tmp = sub(fft_buf[index_a].im, sp_rec_im);
			D_num = L_mac(D_num, re_tmp, re_tmp);
			D_num = L_mac(D_num, im_tmp, im_tmp);

			M_num_sum = L_mac(M_num_sum, fft_buf[index_a].re, fft_buf[index_a].re);
			M_num_sum = L_mac(M_num_sum, fft_buf[index_a].im, fft_buf[index_a].im);
	
			index_a++;
		}

		M_den[j] = sc_coef;  
		M_num[j] = M_num_sum;
		D_den    = L_add(D_den, M_num_sum);

		if(++num_harms_cnt == 3 && band_cnt < num_bands - 1)
		{	
			b1_vec <<= 1;

			if(D_den > D_num && D_den != 0)
			{			
				tmp = norm_l(D_den);
				Dk  = div_s(extract_h(L_shl(D_num, tmp)), extract_h(L_shl(D_den, tmp)));
			}
			else
				Dk = MAX_16;

			if(Dk < dsn_thr)
			{
				// voiced band
				v_uv_dsn[band_cnt] = 1;
				b1_vec |= 1;
				i = j - 2;
				while(i <= j)
				{
					imbe_param->sa[i] = voiced_sa_calc(M_num[i], M_den[i]); 
					imbe_param->v_uv_dsn[i] = 1;
					i++;
				}				
			}
			else
			{
				// unvoiced band
				v_uv_dsn[band_cnt] = 0;
				i = j - 2;
				while(i <= j)
				{
					imbe_param->sa[i] = unvoiced_sa_calc(M_num[i], index_b - index_a_save);
					imbe_param->v_uv_dsn[i] = 0;
					uv_harms_cnt++;
					i++;
				}
			}
	
			D_num = D_den = 0;
			num_harms_cnt = 0; 
			band_cnt++;
		}


		fund_freq_acc_a = L_add(fund_freq_acc_a, fund_freq);
		fund_freq_acc_b = L_add(fund_freq_acc_b, fund_freq);
		fund_freq_acc   = L_add(fund_freq_acc,   fund_freq);
	}

	if(num_harms_cnt)
	{
		b1_vec <<= 1;
		if(D_den > D_num && D_den != 0)
		{			
			tmp = norm_l(D_den);
			Dk= div_s(extract_h(L_shl(D_num, tmp)), extract_h(L_shl(D_den, tmp)));
		}
		else
			Dk = MAX_16;

		if(Dk < dsn_thr)
		{
			// voiced band
			v_uv_dsn[band_cnt] = 1;
			b1_vec |= 1;

			i = num_harms - num_harms_cnt;
			while(i < num_harms)
			{
				imbe_param->sa[i] = voiced_sa_calc(M_num[i], M_den[i]);
				imbe_param->v_uv_dsn[i] = 1;
				i++;
			}
		}
		else
		{
			// unvoiced band
			v_uv_dsn[band_cnt] = 0;
			i = num_harms - num_harms_cnt;
			while(i < num_harms)
			{
				imbe_param->sa[i] = unvoiced_sa_calc(M_num[i], index_b - index_a_save);  
				imbe_param->v_uv_dsn[i] = 0;
				uv_harms_cnt++;
				i++;
			}
		}
	}

	imbe_param->l_uv = uv_harms_cnt;


	imbe_param->b_vec[1] = b1_vec;                                       // Save encoded voiced/unvoiced decision
	imbe_param->b_vec[0] = shr( sub(imbe_param->ref_pitch, 0x1380), 7);  // Pitch encode  fix(2*pitch - 39)

}
