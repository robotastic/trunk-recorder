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
#include "qnt_sub.h"
#include "aux_sub.h"
#include "math_sub.h"
#include "sa_enh.h"




//-----------------------------------------------------------------------------
//	PURPOSE:
//		Perform Spectral Amplitude Enhancement
//     
//
//  INPUT:
//		IMBE_PARAM *imbe_param - pointer to IMBE_PARAM structure with
//                               valid num_harms, sa and fund_freq items
//
//	OUTPUT:
//		None
//
//	RETURN:
//		Enhanced Spectral Amplitudes 
//
//-----------------------------------------------------------------------------
void sa_enh(IMBE_PARAM *imbe_param)
{
	Word16 *sa, num_harm, sa_tmp[NUM_HARMS_MAX], nm;
	Word16 cos_w[NUM_HARMS_MAX], i, tmp;
	Word32 L_tmp, Rm0, Rm1;
	Word32 w0, cos_acc;
	Word32 L_den, L_num, L_Rm0_2, L_Rm1_2, L_sum_Rm02_Rm12, L_sum_mod;
	Word16 Rm0Rm1, nm1, nm2, tot_nm;
	Word16 Rm0_s, Rm1_s;

	
	sa       = imbe_param->sa;
	num_harm = imbe_param->num_harms;

	v_equ(sa_tmp, sa, num_harm);

	Rm0 = L_v_magsq(sa, num_harm);

	if(Rm0 == 0)
		return;

	nm = norm_l (Rm0);
	if(Rm0 == MAX_32)
	{
		nm = 1;
		v_equ_shr(sa_tmp, sa, nm, num_harm);
		Rm0 = L_v_magsq(sa_tmp, num_harm);
	}
	else
	{
		if(nm > 2)
		{
			nm = -(nm >> 1);
			v_equ_shr(sa_tmp, sa, nm, num_harm);
			Rm0 = L_v_magsq(sa_tmp, num_harm);
		}
	}
	
	w0 = imbe_param->fund_freq; 
	
	cos_acc = 0; Rm1 = 0;
	for(i = 0; i < num_harm; i++)
	{
		cos_acc = L_add(cos_acc, w0);

		cos_w[i] = cos_fxp(extract_h(cos_acc));
		Rm1 = L_add(Rm1, L_mpy_ls(L_mult(sa_tmp[i], sa_tmp[i]), cos_w[i]));		
	}
	
	Rm0_s = extract_h(Rm0);
	Rm1_s = extract_h(Rm1);

	L_Rm0_2 = L_mult(Rm0_s, Rm0_s);
	L_Rm1_2 = L_mult(Rm1_s, Rm1_s);
	L_den   = L_sub(L_Rm0_2, L_Rm1_2);	
	L_den   = L_mult(extract_h(L_den), Rm0_s);
	nm1     = norm_l(L_den);
	L_den   = L_shl(L_den, nm1);

	nm2     = norm_l(w0);
	L_den   = L_mpy_ls(L_den, extract_h(L_shl(w0, nm2)));     // Calculate w0 * Rm0 * (Rm0^2 - Rm1^2)
	nm1    += nm2;                                            // total denominator shift

	if (L_den < 1) return;  // fix bug infinite loop due to invalid input
	
	L_sum_Rm02_Rm12 = L_add(L_shr(L_Rm0_2, 2), L_shr(L_Rm1_2, 2));
	Rm0Rm1 = shr(mult_r(Rm0_s, Rm1_s), 1); 

	for(i = 0; i < num_harm; i++)
	{	
		if((((i + 1) << 3) > num_harm) && (sa_tmp[i] != 0x0000))
		{	
			L_num  = L_sub(L_sum_Rm02_Rm12, L_mult(Rm0Rm1, cos_w[i]));
			tot_nm = norm_l(L_num);
			L_num  = L_shl(L_num, tot_nm);
			while(L_num >= L_den)
			{
				L_num   = L_shr(L_num, 1);
				tot_nm -= 1;
			}

			tmp = div_s(extract_h(L_num), extract_h(L_den));
			tot_nm -= nm1; 

			L_tmp   = L_mult(sa_tmp[i], sa_tmp[i]);
			nm2     = norm_l(L_tmp);
			L_tmp   = L_shl(L_tmp, nm2);
			L_tmp   = L_mult(extract_h(L_tmp), tmp);
			tot_nm += nm2;
			tot_nm -= 2;

			if(tot_nm <= 0)
			{
				L_tmp = L_shr(L_tmp, add(8, tot_nm));
				L_tmp = sqrt_l_exp(L_tmp, &tot_nm);
				L_tmp = L_shr(L_tmp, tot_nm);
				L_tmp = sqrt_l_exp(L_tmp, &tot_nm);
				L_tmp = L_shr(L_tmp, tot_nm);
				L_tmp = L_mult(extract_h(L_tmp), CNST_0_9898_Q1_15);
				tmp   = extract_h(L_shl(L_tmp, 1));
			}
			else
			{
				if(tot_nm <= 8)
				{
					L_tmp = L_shr(L_tmp, tot_nm);
					L_tmp = sqrt_l_exp(L_tmp, &tot_nm);
					L_tmp = L_shr(L_tmp, tot_nm);
					L_tmp = sqrt_l_exp(L_tmp, &tot_nm);
					L_tmp = L_shr(L_tmp, tot_nm + 1);
					tmp   = mult(extract_h(L_tmp), CNST_0_9898_Q1_15);
				}
				else
				{
					nm1 = tot_nm & 0xFFFE;
					L_tmp = L_shr(L_tmp, tot_nm - nm1);
					L_tmp = sqrt_l_exp(L_tmp, &tot_nm);
					L_tmp = L_shr(L_tmp, tot_nm);
                    tot_nm = nm1 >> 1;

					nm1 = tot_nm & 0xFFFE;
					L_tmp = L_shr(L_tmp, tot_nm - nm1);
					L_tmp = sqrt_l_exp(L_tmp, &tot_nm);
					L_tmp = L_shr(L_tmp, tot_nm);
					L_tmp = L_mult(extract_h(L_tmp), CNST_0_9898_Q1_15);
					tot_nm = nm1 >> 1;
					tmp   = extract_h(L_shr(L_tmp, tot_nm + 1));
				}
			}

			if(tmp > CNST_1_2_Q2_14)
				sa[i] = extract_h(L_shl(L_mult(sa[i], CNST_1_2_Q2_14), 1)); 
			else if(tmp < CNST_0_5_Q2_14)
				sa[i] = shr(sa[i], 1); 
			else
				sa[i] = extract_h(L_shl(L_mult(sa[i], tmp), 1)); 
		}		
	}

    // Compute the correct scale factor
	v_equ_shr(sa_tmp, sa, nm, num_harm);
	L_sum_mod = L_v_magsq(sa_tmp, num_harm);
	if(L_sum_mod > Rm0)
	{
		tmp = div_s(extract_h(Rm0), extract_h(L_sum_mod));
		L_tmp = sqrt_l_exp(L_deposit_h(tmp), &tot_nm);
		tmp = shr(extract_h(L_tmp), tot_nm);
		for(i = 0; i < num_harm; i++)
			sa[i] = mult_r(sa[i], tmp); 
	}	
}



