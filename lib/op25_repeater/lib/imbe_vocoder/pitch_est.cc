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
#include "tbls.h"
#include "pitch_est.h"
#include "encode.h"
#include "dsp_sub.h"
#include "imbe_vocoder.h"



static const UWord16 min_max_tbl[203] = 
{
	0x0008, 0x0009, 0x000a, 0x000c, 0x000d, 0x000e, 0x000f, 0x0010, 0x0012, 0x0013, 
	0x0014, 0x0115, 0x0216, 0x0218, 0x0319, 0x041a, 0x051b, 0x061c, 0x061e, 0x071f, 
	0x0820, 0x0921, 0x0a22, 0x0a24, 0x0b25, 0x0c26, 0x0d27, 0x0e28, 0x0e2a, 0x0f2b, 
	0x102c, 0x112d, 0x122e, 0x1230, 0x1331, 0x1432, 0x1533, 0x1634, 0x1636, 0x1737, 
	0x1838, 0x1939, 0x1a3a, 0x1a3c, 0x1b3d, 0x1c3e, 0x1d3f, 0x1e40, 0x1e42, 0x1f43, 
	0x2044, 0x2145, 0x2246, 0x2248, 0x2349, 0x244a, 0x254b, 0x264c, 0x264e, 0x274f, 
	0x2850, 0x2951, 0x2a52, 0x2a54, 0x2b55, 0x2c56, 0x2d57, 0x2e58, 0x2e5a, 0x2f5b, 
	0x305c, 0x315d, 0x325e, 0x3260, 0x3361, 0x3462, 0x3563, 0x3664, 0x3666, 0x3767, 
	0x3868, 0x3969, 0x3a6a, 0x3a6c, 0x3b6d, 0x3c6e, 0x3d6f, 0x3e70, 0x3e72, 0x3f73, 
	0x4074, 0x4175, 0x4276, 0x4278, 0x4379, 0x447a, 0x457b, 0x467c, 0x467e, 0x477f, 
	0x4880, 0x4981, 0x4a82, 0x4a84, 0x4b85, 0x4c86, 0x4d87, 0x4e88, 0x4e8a, 0x4f8b, 
	0x508c, 0x518d, 0x528e, 0x5290, 0x5391, 0x5492, 0x5593, 0x5694, 0x5696, 0x5797, 
	0x5898, 0x5999, 0x5a9a, 0x5a9c, 0x5b9d, 0x5c9e, 0x5d9f, 0x5ea0, 0x5ea2, 0x5fa3, 
	0x60a4, 0x61a5, 0x62a6, 0x62a8, 0x63a9, 0x64aa, 0x65ab, 0x66ac, 0x66ae, 0x67af, 
	0x68b0, 0x69b1, 0x6ab2, 0x6ab4, 0x6bb5, 0x6cb6, 0x6db7, 0x6eb8, 0x6eba, 0x6fbb, 
	0x70bc, 0x71bd, 0x72be, 0x72c0, 0x73c1, 0x74c2, 0x75c3, 0x76c4, 0x76c6, 0x77c7, 
	0x78c8, 0x79c9, 0x7aca, 0x7aca, 0x7bca, 0x7cca, 0x7dca, 0x7eca, 0x7eca, 0x7fca, 
	0x80ca, 0x81ca, 0x82ca, 0x82ca, 0x83ca, 0x84ca, 0x85ca, 0x86ca, 0x86ca, 0x87ca, 
	0x88ca, 0x89ca, 0x8aca, 0x8aca, 0x8bca, 0x8cca, 0x8dca, 0x8eca, 0x8eca, 0x8fca, 
	0x90ca, 0x91ca, 0x92ca, 0x92ca, 0x93ca, 0x94ca, 0x95ca, 0x96ca, 0x96ca, 0x97ca, 
	0x98ca, 0x99ca, 0x9aca
};





void imbe_vocoder::pitch_est_init(void)
{
	prev_pitch = prev_prev_pitch = 158; // 100
	prev_e_p = prev_prev_e_p = 0;
}



Word32 imbe_vocoder::autocorr(Word16 *sigin, Word16 shift, Word16 scale_shift)
{
	Word32 L_sum;
	Word16 i;

	L_sum = 0;
	for(i = 0; i < PITCH_EST_FRAME - shift; i++)
		L_sum = L_add(L_sum, L_shr(L_mult(sigin[i], sigin[i + shift]), scale_shift) );

	return L_sum;
}



void imbe_vocoder::e_p(Word16 *sigin, Word16 *res_buf)
{
	Word16 i, j, den_part_acc, tmp;
	Word32 L_sum, L_num, L_den, L_e0, L_tmp;
	Word16 sig_wndwed[PITCH_EST_FRAME];
	Word32 corr[259];
	Word16 index_beg, index_step;
	Word16 scale_shift;


	// Windowing input signal s * wi^2
	for(i = 0 ; i < PITCH_EST_FRAME; i++)
		sig_wndwed[i] = mult_r(sigin[i], wi[i]);                                

	L_sum = 0;
	for(i = 0 ; i < PITCH_EST_FRAME; i++)
		L_sum = L_add(L_sum, L_mpy_ls( L_mult(sigin[i], sigin[i]), wi[i]) );     // sum(s^2 * wi^2)

	// Check for the overflow
	if(L_sum == MAX_32)
	{
		// Recalculate with scaling
		L_sum = 0;
		for(i = 0 ; i < PITCH_EST_FRAME; i++)
			L_sum = L_add(L_sum, L_mpy_ls( L_shr(L_mult(sigin[i], sigin[i]), 5), wi[i]));    
		scale_shift = 5;
	}
	else
		scale_shift = 0;
	
	L_e0 = 0;
	for(i = 0 ; i < PITCH_EST_FRAME; i++)
		L_e0 = L_add(L_e0, L_shr( L_mult(sig_wndwed[i], sig_wndwed[i]), scale_shift));              // sum(s^2 * wi^4) 

    // Calculate correlation for time shift in range 21...150 with step 0.5
	// For integer shifts
	for(tmp = 21, i = 0; tmp <= 150; tmp++, i += 2)
		corr[i] = autocorr(sig_wndwed, tmp, scale_shift);
	// For intermediate shifts
	for(i = 1; i < 258; i += 2)
		corr[i] = L_shr( L_add(corr[i - 1], corr[i + 1]), 1);


	// variable to calculate 1 - P * sum(wi ^4) in denominator
	den_part_acc = CNST_0_8717_Q1_15;

	index_step = 42;               // Note: 42 = 21 in Q15.1 format, so index_step will be used also as p in Q15.1 format
	index_beg  = 0;
	L_e0 = L_shr(L_e0, 7);         // divide by 64 to compensate wi scaling
	// p = 21...122 by step 0.5
	for(i = 0; i < 203; i++)
	{

		// Calculate sum( corr ( n * p) )
		L_tmp = 0;
		j = index_beg;
		while(j <= 258)
		{
			L_tmp = L_add(L_tmp, corr[j]);
			j += index_step;
		}
		
		L_tmp = L_shr(L_tmp, 6);       // compensate wi scaling 
		L_tmp = L_add(L_tmp, L_e0);    // For n = 0 
		L_tmp = L_tmp * index_step;
		L_num = L_sub(L_sum, L_tmp);

		index_beg++;
		index_step++;
		
		L_den = L_mpy_ls(L_sum, den_part_acc);

		if(L_num < L_den && L_den != 0)
		{
			//res_buf[i] = (Word16)((double)L_num/(double)L_den * 4096.);  // Q4.12

			if(L_num <= 0)
				res_buf[i] = 0;
			else
			{
				tmp = norm_l(L_den);
				tmp = div_s(extract_h(L_shl(L_num, tmp)), extract_h(L_shl(L_den, tmp)));
				res_buf[i] = shr(tmp, 3);  // convert to Q4.12
			}
		}
		else if(L_num >= L_den)
		{
			res_buf[i] = CNST_1_00_Q4_12;
			//res_buf[i] = (Word16)((double)L_num/(double)L_den * 4096.);  // Q4.12
		}
		else 
			res_buf[i] = CNST_1_00_Q4_12;

		den_part_acc = sub(den_part_acc, CNST_0_0031_Q1_15);
	}
}




void imbe_vocoder::pitch_est(IMBE_PARAM *imbe_param, Word16 *frames_buf)
{
	Word16 e_p_arr0[203], e_p_arr1[203], e_p_arr2[203], e1p1_e2p2_est_save[203];
	Word16 min_index, max_index, p, i, p_index;
	UWord16 tmp=0, p_fp;
	UWord32 UL_tmp;
	Word16 e_p_cur, pb, pf, ceb, s_tmp;
	Word16 cef_est, cef, p0_est, p0, p1, p2, p1_max_index, p2_max_index, e1p1_e2p2_est, e1p1_e2p2; 
        Word16 e_p_arr2_min[203];

	// Calculate E(p) function for current and two future frames
	e_p(&frames_buf[0], e_p_arr0);

	// Look-Back Pitch Tracking
	min_index = HI_BYTE(min_max_tbl[prev_pitch]);
	max_index = LO_BYTE(min_max_tbl[prev_pitch]);

	p = pb = min_index;
	e_p_cur = e_p_arr0[min_index];	
	while(++p <= max_index)
		if(e_p_arr0[p] < e_p_cur)
		{
			e_p_cur = e_p_arr0[p];
			pb = p;
		}
	ceb = add(e_p_cur, add(prev_e_p, prev_prev_e_p));


	if(ceb <= CNST_0_48_Q4_12)
	{
		prev_prev_pitch = prev_pitch;
		prev_pitch      = pb;
		prev_prev_e_p   = prev_e_p;
		prev_e_p        = e_p_arr0[pb];

		imbe_param->pitch = pb + 42;  // Result in Q15.1 format
		imbe_param->e_p = prev_e_p;
		return;
	}


	// Look-Ahead Pitch Tracking
	e_p(&frames_buf[FRAME],     e_p_arr1);
	e_p(&frames_buf[2 * FRAME], e_p_arr2);

	p0_est = p0 = 0;
	cef_est = e_p_arr0[p0] + e_p_arr1[p0] + e_p_arr2[p0];
	e1p1_e2p2 = 1;

            p1 = 0;
            while(p1 < 203)
            {
                        p2 = HI_BYTE(min_max_tbl[p1]);
                        p2_max_index = LO_BYTE(min_max_tbl[p1]);
                        s_tmp = e_p_arr2[p1];
                        while(p2 <= p2_max_index)
                        {
                                   if(e_p_arr2[p2] < s_tmp)
                                               s_tmp = e_p_arr2[p2];                                                          
                                   p2++;
                        }
                        e_p_arr2_min[p1] = s_tmp;
                        p1++;
            }
            while(p0 < 203)
            {
                        e1p1_e2p2_est = e_p_arr1[p0] + e_p_arr2_min[p0];
                        p1 = HI_BYTE(min_max_tbl[p0]);
                        p1_max_index = LO_BYTE(min_max_tbl[p0]);
                        while(p1 <= p1_max_index)
                        {                     
                                   if(add(e_p_arr1[p1], e_p_arr2_min[p1]) < e1p1_e2p2_est)                          
                                               e1p1_e2p2_est = add(e_p_arr1[p1], e_p_arr2_min[p1]);                                                                 
                                   p1++;
                        }
                        e1p1_e2p2_est_save[p0] = e1p1_e2p2_est;
                        cef = add(e_p_arr0[p0], e1p1_e2p2_est);
                        if(cef < cef_est)
                        {
                                   cef_est = cef;
                                   p0_est  = p0;
                        }    
                        p0++;
            }

	pf = p0_est;
	// Sub-multiples analysis
	if(pf >= 42)  // Check if Sub-multiples are possible
	{	
		if(pf < 84)
			i = 1;
		else if(pf < 126)
			i = 2;
		else if(pf < 168)
			i = 3;
		else
			i = 4;

		p_fp = (pf + 42) << 8; // Convert pitch estimation from array index to unsigned Q7.19 format

		while(i--)
		{
			switch(i)
			{
				case 0:
					tmp = p_fp >> 1;                     // P_est/2
				break;

				case 1:
					UL_tmp = (UWord32)p_fp * 0x5555;     // P_est/3
					tmp = UL_tmp >> 16;
				break;

				case 2:
					tmp = p_fp >> 2;                     // P_est/4
				break;

				case 3:
					UL_tmp = (UWord32)p_fp * 0x3333;     // P_est/5
					tmp = UL_tmp >> 16;
				break;
			}

			p_index = ((tmp + 0x0080) >> 8) - 42;       // Convert fixed-point pitch value to integer array index with rounding

			cef = add(e_p_arr0[p_index], e1p1_e2p2_est_save[p_index]);

			if(cef <= CNST_0_85_Q4_12 && mult_r(cef, CNST_0_5882_Q1_15) <= cef_est)  // 1/1.7 = 0.5882
			{
				pf = p_index;
				break;
			}

			if(cef <= CNST_0_4_Q4_12 && mult_r(cef, CNST_0_2857_Q1_15) <= cef_est)   // 1/3.5 = 0.2857
			{
				pf = p_index;
				break;
			}

			if(cef <= CNST_0_05_Q4_12)
			{
				pf = p_index;
				break;
			}		
		}
	}

	cef = add(e_p_arr0[pf], e1p1_e2p2_est_save[pf]);

	if(ceb <= cef)
		p = pb;
	else    
		p = pf; 

	prev_prev_pitch = prev_pitch;
	prev_pitch      = p;
	prev_prev_e_p   = prev_e_p;
	prev_e_p        = e_p_arr0[p];


	imbe_param->pitch = p + 42;  // Result in Q15.1 format
	imbe_param->e_p = prev_e_p; 
}
