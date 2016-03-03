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
#include "v_synt.h"
#include "rand_gen.h"
#include "tbls.h"
#include "encode.h"
#include "imbe_vocoder.h"






#define CNST_0_1_Q1_15 0x0CCD



void imbe_vocoder::v_synt_init(void)
{
	Word16 i;

	for(i = 0; i < NUM_HARMS_MAX; i++)
	{
		ph_mem[i] = L_deposit_h(rand_gen());
		vu_dsn_prev[i] = 0;
	}

	num_harms_prev3 = 0;
	fund_freq_prev = 0;
}


void imbe_vocoder::v_synt(IMBE_PARAM *imbe_param, Word16 *snd)
{
	Word32 L_tmp, L_tmp1, fund_freq, L_snd[FRAME], L_ph_acc, L_ph_step;
	Word32 L_ph_acc_aux, L_ph_step_prev, L_amp_acc, L_amp_step, L_ph_step_aux;
	Word16 num_harms, i, j, *vu_dsn, *sa, *s_ptr, *s_ptr_aux, num_harms_max, num_harms_max_4;
	UWord32 ph_mem_prev[NUM_HARMS_MAX], dph[NUM_HARMS_MAX];
	Word16 num_harms_inv, num_harms_sh, num_uv;
	Word16 freq_flag;


	fund_freq = imbe_param->fund_freq;
	num_harms = imbe_param->num_harms;
	vu_dsn    = imbe_param->v_uv_dsn;
	sa        = imbe_param->sa;
	num_harms_inv = imbe_param->div_one_by_num_harm;
	num_harms_sh  = imbe_param->div_one_by_num_harm_sh;
	num_uv        = imbe_param->l_uv;

	for(i = 0; i < FRAME; i++)
		L_snd[i] = 0;
	
	// Update phases (calculated phase value correspond to bound of frame)
	L_tmp = (((fund_freq_prev + fund_freq) >> 7) * FRAME/2) << 7;  // It is performed integer multiplication by mod 1

	L_ph_acc = 0;
	for(i = 0; i < NUM_HARMS_MAX; i++)
	{
		ph_mem_prev[i] = ph_mem[i];
		L_ph_acc  += L_tmp;
		ph_mem[i] += L_ph_acc;
		dph[i] = 0;
	}

	num_harms_max = (num_harms >= num_harms_prev3)?num_harms:num_harms_prev3;
    num_harms_max_4 = num_harms_max >> 2;

	if(L_abs(L_sub(fund_freq, fund_freq_prev)) >= L_mpy_ls(fund_freq, CNST_0_1_Q1_15))
		freq_flag = 1;
	else
		freq_flag = 0;

	L_ph_step = L_ph_step_prev = 0;
	for(i = 0; i < num_harms_max; i++)
	{
		L_ph_step      += fund_freq;
		L_ph_step_prev += fund_freq_prev;


		if(i > num_harms_max_4)
		{
			if(num_uv == num_harms)
			{
				dph[i] = L_deposit_h(rand_gen());				
			}
			else
			{			
				L_tmp = L_mult(rand_gen(), num_harms_inv); 
				dph[i] = L_shr(L_tmp, 15 - num_harms_sh) * num_uv;
			}
			ph_mem[i] += dph[i];
		}

		if(vu_dsn[i] == 0 && vu_dsn_prev[i] == 0)
			continue;

		if(vu_dsn[i] == 1 && vu_dsn_prev[i] == 0)  // unvoiced => voiced
		{
			s_ptr = (Word16 *)ws;
			L_ph_acc = ph_mem[i] - (((L_ph_step >> 7) * 104) << 7);
			for(j = 56; j <= 104; j++)
			{
				L_tmp = L_mult(*s_ptr++, sa[i]);
				L_tmp = L_mpy_ls(L_tmp, cos_fxp(extract_h(L_ph_acc)));
				L_tmp = L_shr(L_tmp, 1);
				L_snd[j] = L_add(L_snd[j], L_tmp);
				L_ph_acc += L_ph_step;
			}

			for(j = 105; j <= 159; j++)
			{
				L_tmp = L_mult(sa[i], cos_fxp(extract_h(L_ph_acc)));
				L_tmp = L_shr(L_tmp, 1);
				L_snd[j] = L_add(L_snd[j], L_tmp);
				L_ph_acc += L_ph_step;
			}
			continue;
		}

 
		if(vu_dsn[i] == 0 && vu_dsn_prev[i] == 1)  // voiced => unvoiced
		{
			s_ptr = (Word16 *)&ws[48];
			L_ph_acc = ph_mem_prev[i];

			for(j = 0; j <= 55; j++)
			{
				L_tmp = L_mult(sa_prev3[i], cos_fxp(extract_h(L_ph_acc)));
				L_tmp = L_shr(L_tmp, 1);
				L_snd[j] = L_add(L_snd[j], L_tmp);
				L_ph_acc += L_ph_step_prev;
			}

			for(j = 56; j <= 104; j++)
			{
				L_tmp = L_mult(*s_ptr--, sa_prev3[i]);
				L_tmp = L_mpy_ls(L_tmp, cos_fxp(extract_h(L_ph_acc)));
				L_tmp = L_shr(L_tmp, 1);
				L_snd[j] = L_add(L_snd[j], L_tmp);
				L_ph_acc += L_ph_step_prev;
			}
			continue;
		}

		if(i >=7 || freq_flag)
		{
			s_ptr_aux     = (Word16 *)&ws[48];
			L_ph_acc_aux  = ph_mem_prev[i];

			s_ptr    = (Word16 *)ws;
			L_ph_acc = ph_mem[i] - (((L_ph_step >> 7) * 104) << 7);

			for(j = 0; j <= 55; j++)
			{
				L_tmp = L_mult(sa_prev3[i], cos_fxp(extract_h(L_ph_acc_aux)));
				L_tmp = L_shr(L_tmp, 1);
				L_snd[j] = L_add(L_snd[j], L_tmp);
				L_ph_acc_aux += L_ph_step_prev;
			}

			for(j = 56; j <= 104; j++)
			{
				L_tmp = L_mult(*s_ptr_aux--, sa_prev3[i]);
				L_tmp = L_mpy_ls(L_tmp, cos_fxp(extract_h(L_ph_acc_aux)));
				L_tmp = L_shr(L_tmp, 1);
				L_snd[j] = L_add(L_snd[j], L_tmp);

				L_tmp = L_mult(*s_ptr++, sa[i]);
				L_tmp = L_mpy_ls(L_tmp, cos_fxp(extract_h(L_ph_acc)));
				L_tmp = L_shr(L_tmp, 1);
				L_snd[j] = L_add(L_snd[j], L_tmp);

				L_ph_acc_aux += L_ph_step_prev;
				L_ph_acc += L_ph_step;
			}

			for(j = 105; j <= 159; j++)
			{
				L_tmp = L_mult(sa[i], cos_fxp(extract_h(L_ph_acc)));
				L_tmp = L_shr(L_tmp, 1);
				L_snd[j] = L_add(L_snd[j], L_tmp);
				L_ph_acc += L_ph_step;
			}
			continue;
		}
	
		L_amp_step = L_mpy_ls(L_shr(L_deposit_h(sub(sa[i], sa_prev3[i])), 4 + 1), CNST_0_1_Q1_15); // (sa[i] - sa_prev3[i]) / 160, 1/160 = 0.1/16 
		L_amp_acc  = L_shr(L_deposit_h(sa_prev3[i]), 1);

		
		L_ph_step_aux = L_mpy_ls(L_shr(fund_freq - fund_freq_prev, 4 + 1), CNST_0_1_Q1_15);       // (fund_freq - fund_freq_prev)/(2*160)
		L_ph_step_aux = ((L_ph_step_aux >> 7) * (i + 1)) << 7;

		L_ph_acc = ph_mem_prev[i];

		L_tmp1 = L_mpy_ls(L_shr(dph[i], 4), CNST_0_1_Q1_15);  // dph[i] / 160
					
		for(j = 0; j < 160; j++)
		{
			L_ph_acc_aux = ((L_ph_step_aux >> 9) * j) << 9; 
			L_ph_acc_aux = ((L_ph_acc_aux >> 9) * j) << 9; 

			L_tmp = L_mpy_ls(L_amp_acc, cos_fxp(extract_h(L_ph_acc + L_ph_acc_aux)));			
			L_snd[j] = L_add(L_snd[j], L_tmp);

			L_amp_acc = L_add(L_amp_acc, L_amp_step);
			L_ph_acc += L_ph_step_prev;
			L_ph_acc += L_tmp1;
		}
	}

	for(i = 0; i < FRAME; i++)
		*snd++ = extract_h(L_snd[i]);

	v_zap(vu_dsn_prev, NUM_HARMS_MAX);
	v_equ(vu_dsn_prev, imbe_param->v_uv_dsn, num_harms);
	v_equ(sa_prev3, imbe_param->sa, num_harms);

	num_harms_prev3 = num_harms;
	fund_freq_prev = fund_freq;
}
