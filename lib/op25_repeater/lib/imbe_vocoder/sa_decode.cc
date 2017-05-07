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
#include "basic_op.h"
#include "imbe.h"
#include "tbls.h"
#include "qnt_sub.h"
#include "sa_decode.h"
#include "aux_sub.h"
#include "dsp_sub.h"
#include "math_sub.h"
#include "encode.h"
#include "imbe_vocoder.h"



//-----------------------------------------------------------------------------
//	PURPOSE:
//		Initialization of Spectral Amplitude Decoder
//     
//
//  INPUT:
//      None
//
//	OUTPUT:
//		None
//
//	RETURN:
//      None
//
//-----------------------------------------------------------------------------
void imbe_vocoder::sa_decode_init(void)
{
	num_harms_prev1 = 30;
	v_zap((Word16 *)sa_prev1, 2 * (NUM_HARMS_MAX + 2));
}


//-----------------------------------------------------------------------------
//	PURPOSE:
//		Perform Spectral Amplitude Decoding
//     
//
//  INPUT:
//		IMBE_PARAM *imbe_param - pointer to IMBE_PARAM structure with
//                               valid num_harms and b_vec items
//
//	OUTPUT:
//		None
//
//	RETURN:
//      Decoded Spectral Amplitudes
//
//-----------------------------------------------------------------------------
void imbe_vocoder::sa_decode(IMBE_PARAM *imbe_param)
{
	Word16 gain_vec[6], gain_r[6];
	UWord16 index, index_1, num_harms;
	Word16 *gss_ptr, *ba_ptr, i, j, *t_vec_ptr, *b_vec_ptr;
	Word16 t_vec[NUM_HARMS_MAX], c_vec[MAX_BLOCK_LEN];
	Word32 lmprbl_item;
	Word16 bl_len, step_size, num_bits, tmp, ro_coef, si_coef, tmp1;
	UWord32 k_coef, k_acc;
	Word32 sum, tmp_word32, sa_tmp[NUM_HARMS_MAX];
	Word16 *sa;

	num_harms = imbe_param->num_harms;
	index     = num_harms - NUM_HARMS_MIN;
	ba_ptr    = imbe_param->bit_alloc;
	b_vec_ptr = &imbe_param->b_vec[2];
	sa        = imbe_param->sa;
    
	// Decoding the Gain Vector. gain_vec has signed Q5.11 format
	gss_ptr = (Word16 *)&gain_step_size_tbl[index * 5];
	gain_vec[0] = gain_qnt_tbl[*b_vec_ptr++]; 

	for(i = 1; i < 6; i++)
		gain_vec[i] = extract_l(L_shr(deqnt_by_step(*b_vec_ptr++, *gss_ptr++, *ba_ptr++), 5));
	idct(gain_vec, NUM_PRED_RES_BLKS, NUM_PRED_RES_BLKS, gain_r);

	lmprbl_item = lmprbl_tbl[index];
	v_zap(t_vec, NUM_HARMS_MAX);

	// Decoding the Higher Order DCT Coefficients
	t_vec_ptr = t_vec;
	for(i = 0;  i < NUM_PRED_RES_BLKS; i++)
	{
		bl_len = (lmprbl_item >> 28) & 0xF; lmprbl_item <<= 4;
		v_zap(c_vec, MAX_BLOCK_LEN);
		c_vec[0] = gain_r[i];
		for(j = 1; j < bl_len; j++)
		{
			num_bits = *ba_ptr++;
			if(num_bits)
			{
				step_size = extract_h(((Word32)hi_ord_std_tbl[j - 1] * hi_ord_step_size_tbl[num_bits - 1]) << 1);
				c_vec[j]  = extract_l(L_shr(deqnt_by_step(*b_vec_ptr, step_size, num_bits), 5));
			}
			else			
				c_vec[j] = 0;		

			b_vec_ptr++;
		}
		idct(c_vec, bl_len, bl_len, t_vec_ptr);
		t_vec_ptr += bl_len; 
	}
	
	// Calculate num_harms_prev/num_harms. Result save in unsigned format Q8.24
	if(num_harms == num_harms_prev1)
		k_coef = (Word32)CNST_ONE_Q8_24;  
	else if(num_harms > num_harms_prev1)
		k_coef = (Word32)div_s(num_harms_prev1 << 9, num_harms << 9) << 9;
	else
	{   
		// imbe_param->num_harms < num_harms_prev1
		k_coef = 0;
		tmp = num_harms_prev1; 
		while(tmp > num_harms)
		{
			tmp -= num_harms;
			k_coef += (Word32)CNST_ONE_Q8_24;
		}
		k_coef += (Word32)div_s(tmp << 9, num_harms << 9) << 9;
	}

	if(num_harms <= 15)
		ro_coef = CNST_0_4_Q1_15;
	else if(num_harms <= 24)
		ro_coef = num_harms * CNST_0_03_Q1_15 - CNST_0_05_Q1_15;
	else
		ro_coef = CNST_0_7_Q1_15;

	k_acc = k_coef;
	sum   = 0;

	for(i = num_harms_prev1 + 1; i < NUM_HARMS_MAX + 2; i++)
		sa_prev1[i] = sa_prev1[num_harms_prev1];

	for(i = 0; i < num_harms; i++)
	{
		index   = (UWord16)(k_acc >> 24);                    // Get integer part
		si_coef = (Word16)((k_acc - ((UWord32)index << 24)) >> 9); // Get fractional part 

		if(si_coef == 0)
		{
			tmp_word32 = L_mpy_ls(sa_prev1[index], ro_coef);                        // sa_prev1 here is in Q10.22 format 
			sa_tmp[i]  = L_add(L_shr(L_deposit_h(t_vec[i]), 5), tmp_word32);       // Convert t_vec to Q10.22 and add ...
			sum = L_add(sum, sa_prev1[index]);                                      // sum in Q10.22 format
		}
		else
		{
			index_1 = index + 1;
			tmp_word32 = L_mpy_ls(sa_prev1[index], sub(0x7FFF, si_coef)); 
			sum = L_add(sum, tmp_word32);    
			sa_tmp[i]  = L_add(L_shr(L_deposit_h(t_vec[i]), 5), L_mpy_ls(tmp_word32, ro_coef));

			tmp_word32 = L_mpy_ls(sa_prev1[index_1], si_coef); 
			sum = L_add(sum, tmp_word32); 
			sa_tmp[i] = L_add(sa_tmp[i], L_mpy_ls(tmp_word32, ro_coef));
		}

		k_acc += k_coef;
	}

	imbe_param->div_one_by_num_harm_sh = tmp = norm_s(num_harms);	
	imbe_param->div_one_by_num_harm = tmp1 = div_s(0x4000, num_harms << tmp); // calculate 1/num_harms with scaling for better pricision
	                                                                          // save result to use late  

	sum = L_shr(L_mpy_ls(L_mpy_ls(sum, ro_coef), tmp1), (14 - tmp));

	for(i = 1; i <= num_harms; i++)
	{
		sa_prev1[i] = L_sub(sa_tmp[i - 1], sum);
		sa[i - 1] = Pow2(sa_prev1[i]);
	}

	num_harms_prev1 = num_harms;
}
