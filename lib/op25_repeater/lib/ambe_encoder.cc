/* -*- c++ -*- */
/* 
 * AMBE halfrate encoder - Copyright 2016 Max H. Parke KA1RBI
 * 
 * This file is part of OP25 and part of GNU Radio
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <cmath>

#include "imbe_vocoder/imbe_vocoder.h"
#include "ambe3600x2250_const.h"
#include "ambe3600x2400_const.h"
#include "op25_imbe_frame.h"
#include "mbelib.h"
#include "ambe.h"
#include "p25p2_vf.h"
#include "ambe_encoder.h"

static const short b0_lookup[] = {
	0, 0, 0, 1, 1, 2, 2, 2, 
	3, 3, 4, 4, 4, 5, 5, 5, 
	6, 6, 7, 7, 7, 8, 8, 8, 
	9, 9, 9, 10, 10, 11, 11, 11, 
	12, 12, 12, 13, 13, 13, 14, 14, 
	14, 15, 15, 15, 16, 16, 16, 17, 
	17, 17, 17, 18, 18, 18, 19, 19, 
	19, 20, 20, 20, 21, 21, 21, 21, 
	22, 22, 22, 23, 23, 23, 24, 24, 
	24, 24, 25, 25, 25, 25, 26, 26, 
	26, 27, 27, 27, 27, 28, 28, 28, 
	29, 29, 29, 29, 30, 30, 30, 30, 
	31, 31, 31, 31, 31, 32, 32, 32, 
	32, 33, 33, 33, 33, 34, 34, 34, 
	34, 35, 35, 35, 35, 36, 36, 36, 
	36, 37, 37, 37, 37, 38, 38, 38, 
	38, 38, 39, 39, 39, 39, 40, 40, 
	40, 40, 40, 41, 41, 41, 41, 42, 
	42, 42, 42, 42, 43, 43, 43, 43, 
	43, 44, 44, 44, 44, 45, 45, 45, 
	45, 45, 46, 46, 46, 46, 46, 47, 
	47, 47, 47, 47, 48, 48, 48, 48, 
	48, 49, 49, 49, 49, 49, 49, 50, 
	50, 50, 50, 50, 51, 51, 51, 51, 
	51, 52, 52, 52, 52, 52, 52, 53, 
	53, 53, 53, 53, 54, 54, 54, 54, 
	54, 54, 55, 55, 55, 55, 55, 56, 
	56, 56, 56, 56, 56, 57, 57, 57, 
	57, 57, 57, 58, 58, 58, 58, 58, 
	58, 59, 59, 59, 59, 59, 59, 60, 
	60, 60, 60, 60, 60, 61, 61, 61, 
	61, 61, 61, 62, 62, 62, 62, 62, 
	62, 63, 63, 63, 63, 63, 63, 63, 
	64, 64, 64, 64, 64, 64, 65, 65, 
	65, 65, 65, 65, 65, 66, 66, 66, 
	66, 66, 66, 67, 67, 67, 67, 67, 
	67, 67, 68, 68, 68, 68, 68, 68, 
	68, 69, 69, 69, 69, 69, 69, 69, 
	70, 70, 70, 70, 70, 70, 70, 71, 
	71, 71, 71, 71, 71, 71, 72, 72, 
	72, 72, 72, 72, 72, 73, 73, 73, 
	73, 73, 73, 73, 73, 74, 74, 74, 
	74, 74, 74, 74, 75, 75, 75, 75, 
	75, 75, 75, 75, 76, 76, 76, 76, 
	76, 76, 76, 76, 77, 77, 77, 77, 
	77, 77, 77, 77, 77, 78, 78, 78, 
	78, 78, 78, 78, 78, 79, 79, 79, 
	79, 79, 79, 79, 79, 80, 80, 80, 
	80, 80, 80, 80, 80, 81, 81, 81, 
	81, 81, 81, 81, 81, 81, 82, 82, 
	82, 82, 82, 82, 82, 82, 83, 83, 
	83, 83, 83, 83, 83, 83, 83, 84, 
	84, 84, 84, 84, 84, 84, 84, 84, 
	85, 85, 85, 85, 85, 85, 85, 85, 
	85, 86, 86, 86, 86, 86, 86, 86, 
	86, 86, 87, 87, 87, 87, 87, 87, 
	87, 87, 87, 88, 88, 88, 88, 88, 
	88, 88, 88, 88, 89, 89, 89, 89, 
	89, 89, 89, 89, 89, 89, 90, 90, 
	90, 90, 90, 90, 90, 90, 90, 90, 
	91, 91, 91, 91, 91, 91, 91, 91, 
	91, 92, 92, 92, 92, 92, 92, 92, 
	92, 92, 92, 93, 93, 93, 93, 93, 
	93, 93, 93, 93, 93, 94, 94, 94, 
	94, 94, 94, 94, 94, 94, 94, 94, 
	95, 95, 95, 95, 95, 95, 95, 95, 
	95, 95, 96, 96, 96, 96, 96, 96, 
	96, 96, 96, 96, 96, 97, 97, 97, 
	97, 97, 97, 97, 97, 97, 97, 98, 
	98, 98, 98, 98, 98, 98, 98, 98, 
	98, 98, 99, 99, 99, 99, 99, 99, 
	99, 99, 99, 99, 99, 99, 100, 100, 
	100, 100, 100, 100, 100, 100, 100, 100, 
	100, 101, 101, 101, 101, 101, 101, 101, 
	101, 101, 101, 101, 102, 102, 102, 102, 
	102, 102, 102, 102, 102, 102, 102, 102, 
	103, 103, 103, 103, 103, 103, 103, 103, 
	103, 103, 103, 103, 104, 104, 104, 104, 
	104, 104, 104, 104, 104, 104, 104, 104, 
	105, 105, 105, 105, 105, 105, 105, 105, 
	105, 105, 105, 105, 106, 106, 106, 106, 
	106, 106, 106, 106, 106, 106, 106, 106, 
	107, 107, 107, 107, 107, 107, 107, 107, 
	107, 107, 107, 107, 107, 108, 108, 108, 
	108, 108, 108, 108, 108, 108, 108, 108, 
	108, 109, 109, 109, 109, 109, 109, 109, 
	109, 109, 109, 109, 109, 109, 110, 110, 
	110, 110, 110, 110, 110, 110, 110, 110, 
	110, 110, 110, 111, 111, 111, 111, 111, 
	111, 111, 111, 111, 111, 111, 111, 111, 
	112, 112, 112, 112, 112, 112, 112, 112, 
	112, 112, 112, 112, 112, 112, 113, 113, 
	113, 113, 113, 113, 113, 113, 113, 113, 
	113, 113, 113, 113, 114, 114, 114, 114, 
	114, 114, 114, 114, 114, 114, 114, 114, 
	114, 115, 115, 115, 115, 115, 115, 115, 
	115, 115, 115, 115, 115, 115, 115, 116, 
	116, 116, 116, 116, 116, 116, 116, 116, 
	116, 116, 116, 116, 116, 116, 117, 117, 
	117, 117, 117, 117, 117, 117, 117, 117, 
	117, 117, 117, 117, 118, 118, 118, 118, 
	118, 118, 118, 118, 118, 118, 118, 118, 
	118, 118, 118, 119, 119, 119, 119, 119, 
	119, 119, 119
};

#if 0
// fixme: should not be static
static mbe_parms cur_mp;
static mbe_parms prev_mp;
#endif

static inline float make_f0(int b0) {
	return (powf(2, (-4.311767578125 - (2.1336e-2 * ((float)b0+0.5)))));
}

static void encode_ambe(const IMBE_PARAM *imbe_param, int b[], mbe_parms*cur_mp, mbe_parms*prev_mp, bool dstar, float gain_adjust) {
	static const float SQRT_2 = sqrtf(2.0);
	static const int b0_lmax = sizeof(b0_lookup) / sizeof(b0_lookup[0]);
	// int b[9];

	// ref_pitch is Q8_8 in range 19.875 - 123.125
	int b0_i = (imbe_param->ref_pitch >> 5) - 159;
	if (b0_i < 0 || b0_i > b0_lmax) {
		fprintf(stderr, "encode error b0_i %d\n", b0_i);
		return;
	}
	b[0] = b0_lookup[b0_i];
	int L;
	if (dstar)
		L = (int) AmbePlusLtable[b[0]];
	else
		L = (int) AmbeLtable[b[0]];
#if 1
	// adjust b0 until L agrees
	while (L != imbe_param->num_harms) {
		if (L < imbe_param->num_harms)
			b0_i ++;
		else if (L > imbe_param->num_harms)
			b0_i --;
		if (b0_i < 0 || b0_i > b0_lmax) {
			fprintf(stderr, "encode error2 b0_i %d\n", b0_i);
			return;
		}
		b[0] = b0_lookup[b0_i];
		if (dstar)
			L = (int) AmbePlusLtable[b[0]];
		else
			L = (int) AmbeLtable[b[0]];
	}
#endif
	float m_float2[NUM_HARMS_MAX];
	for (int l=1; l <= L; l++) {
		m_float2[l-1] = (float)imbe_param->sa[l-1] ;
		m_float2[l-1] = m_float2[l-1] * m_float2[l-1];
	}

	float en_min = 0;
	b[1] = 0;
	int vuv_max = (dstar) ? 16 : 17;
	for (int n=0; n < vuv_max; n++) {
		float En = 0;
		for (int l=1; l <= L; l++) {
			int jl;
			if (dstar)
				jl = (int) ((float) l * (float) 16.0 * make_f0(b[0]));
			else
				jl = (int) ((float) l * (float) 16.0 * AmbeW0table[b[0]]);
			int kl = 12;
			if (l <= 36)
				kl = (l + 2) / 3;
			if (dstar) {
				if (imbe_param->v_uv_dsn[(kl-1)*3] != AmbePlusVuv[n][jl])
					En += m_float2[l-1];
			} else {
				if (imbe_param->v_uv_dsn[(kl-1)*3] != AmbeVuv[n][jl])
					En += m_float2[l-1];
			}
		}
		if (n == 0)
			en_min = En;
		else if (En < en_min) {
			b[1] = n;
			en_min = En;
		}
	}

	// log spectral amplitudes
	float num_harms_f = (float) imbe_param->num_harms;
	float log_l_2 =  0.5 * log2f(num_harms_f);	// fixme: table lookup
	float log_l_w0;
	if (dstar)
		log_l_w0 = 0.5 * log2f(num_harms_f * make_f0(b[0]) * 2.0 * M_PI) + 2.289;
	else
		log_l_w0 = 0.5 * log2f(num_harms_f * AmbeW0table[b[0]] * 2.0 * M_PI) + 2.289;
	float lsa[NUM_HARMS_MAX];
	float lsa_sum=0.0;

	for (int i1 = 0; i1 < imbe_param->num_harms; i1++) {
		float sa = (float)imbe_param->sa[i1];
		if (sa < 1) sa = 1.0;
		if (imbe_param->v_uv_dsn[i1])
			lsa[i1] = log_l_2 + log2f(sa);
		else
			lsa[i1] = log_l_w0 + log2f(sa);
		lsa_sum += lsa[i1];
	}
	float gain = lsa_sum / num_harms_f;
	float diff_gain;
	if (dstar)
		diff_gain = gain;
	else
		diff_gain = gain - 0.5 * prev_mp->gamma;

	diff_gain -= gain_adjust;

	float error;
	int error_index;
	int max_dg = (dstar) ? 64 : 32;
	for (int i1 = 0; i1 < max_dg; i1++) {
		float diff;
		if (dstar)
			diff = fabsf(diff_gain - AmbePlusDg[i1]);
		else
			diff = fabsf(diff_gain - AmbeDg[i1]);
		if (i1 == 0 || diff < error) {
			error = diff;
			error_index = i1;
		}
	}
	b[2] = error_index;

	// prediction residuals
	float l_prev_l = (float) (prev_mp->L) / num_harms_f;
	float tmp_s = 0.0;
	prev_mp->log2Ml[0] = prev_mp->log2Ml[1];
	for (int i1 = 0; i1 < imbe_param->num_harms; i1++) {
		float kl = l_prev_l * (float)(i1+1);
		int kl_floor = (int) kl;
		float kl_frac = kl - kl_floor;
		tmp_s += (1.0 - kl_frac) * prev_mp->log2Ml[kl_floor  +0] + kl_frac * prev_mp->log2Ml[kl_floor+1  +0];
	}
	float T[NUM_HARMS_MAX];
	for (int i1 = 0; i1 < imbe_param->num_harms; i1++) {
		float kl = l_prev_l * (float)(i1+1);
		int kl_floor = (int) kl;
		float kl_frac = kl - kl_floor;
		T[i1] = lsa[i1] - 0.65 * (1.0 - kl_frac) * prev_mp->log2Ml[kl_floor  +0]	\
				- 0.65 * kl_frac * prev_mp->log2Ml[kl_floor+1  +0];
	}

	// DCT
	const int * J;
	if (dstar)
		J = AmbePlusLmprbl[imbe_param->num_harms];
	else
		J = AmbeLmprbl[imbe_param->num_harms];
	float * c[4];
	int acc = 0;
	for (int i=0; i<4; i++) {
		c[i] = &T[acc];
		acc += J[i];
	}
	float C[4][17];
	for (int i=1; i<=4; i++) {
		for (int k=1; k<=J[i-1]; k++) {
			float s = 0.0;
			for (int j=1; j<=J[i-1]; j++) {
				//fixme: lut?
				s += (c[i-1][j-1] * cosf((M_PI * (((float)k) - 1.0) * (((float)j) - 0.5)) / (float)J[i-1]));
			}
			C[i-1][k-1] = s / (float)J[i-1];
		}
	}
	float R[8];
	R[0] = C[0][0] + SQRT_2 * C[0][1];
	R[1] = C[0][0] - SQRT_2 * C[0][1];
	R[2] = C[1][0] + SQRT_2 * C[1][1];
	R[3] = C[1][0] - SQRT_2 * C[1][1];
	R[4] = C[2][0] + SQRT_2 * C[2][1];
	R[5] = C[2][0] - SQRT_2 * C[2][1];
	R[6] = C[3][0] + SQRT_2 * C[3][1];
	R[7] = C[3][0] - SQRT_2 * C[3][1];

	// encode PRBA
	float G[8];
	for (int m=1; m<=8; m++) {
		G[m-1] = 0.0;
		for (int i=1; i<=8; i++) {
			//fixme: lut?
			G[m-1] += (R[i-1] * cosf((M_PI * (((float)m) - 1.0) * (((float)i) - 0.5)) / 8.0));
		}
		G[m-1] /= 8.0;
	}
	for (int i=0; i<512; i++) {
		float err=0.0;
		float diff;
		if (dstar) {
			diff = G[1] - AmbePlusPRBA24[i][0];
			err += (diff * diff);
			diff = G[2] - AmbePlusPRBA24[i][1];
			err += (diff * diff);
			diff = G[3] - AmbePlusPRBA24[i][2];
			err += (diff * diff);
		} else {
			diff = G[1] - AmbePRBA24[i][0];
			err += (diff * diff);
			diff = G[2] - AmbePRBA24[i][1];
			err += (diff * diff);
			diff = G[3] - AmbePRBA24[i][2];
			err += (diff * diff);
		}
		if (i == 0 || err < error) {
			error = err;
			error_index = i;
		}
	}
	b[3] = error_index;

	// PRBA58
	for (int i=0; i<128; i++) {
		float err=0.0;
		float diff;
		if (dstar) {
			diff = G[4] - AmbePlusPRBA58[i][0];
			err += (diff * diff);
			diff = G[5] - AmbePlusPRBA58[i][1];
			err += (diff * diff);
			diff = G[6] - AmbePlusPRBA58[i][2];
			err += (diff * diff);
			diff = G[7] - AmbePlusPRBA58[i][3];
			err += (diff * diff);
		} else {
			diff = G[4] - AmbePRBA58[i][0];
			err += (diff * diff);
			diff = G[5] - AmbePRBA58[i][1];
			err += (diff * diff);
			diff = G[6] - AmbePRBA58[i][2];
			err += (diff * diff);
			diff = G[7] - AmbePRBA58[i][3];
			err += (diff * diff);
		}
		if (i == 0 || err < error) {
			error = err;
			error_index = i;
		}
	}
	b[4] = error_index;

	// higher order coeffs b5
	int ii = 1;
	if (J[ii-1] <= 2) {
		b[4+ii] = 0.0;
	} else {
		int max_5 = (dstar) ? 16 : 32;
		for (int n=0; n < max_5; n++) {
			float err=0.0;
			float diff;
			for (int j=1; j <= J[ii-1]-2 && j <= 4; j++) {
				if (dstar)
					diff = AmbePlusHOCb5[n][j-1] - C[ii-1][j+2-1];
				else
					diff = AmbeHOCb5[n][j-1] - C[ii-1][j+2-1];
				err += (diff * diff);
			}
			if (n == 0 || err < error) {
				error = err;
				error_index = n;
			}
		}
		b[4+ii] = error_index;
	}

	// higher order coeffs b6
	ii = 2;
	if (J[ii-1] <= 2) {
		b[4+ii] = 0.0;
	} else {
		for (int n=0; n < 16; n++) {
			float err=0.0;
			float diff;
			for (int j=1; j <= J[ii-1]-2 && j <= 4; j++) {
				if (dstar)
					diff = AmbePlusHOCb6[n][j-1] - C[ii-1][j+2-1];
				else
					diff = AmbeHOCb6[n][j-1] - C[ii-1][j+2-1];
				err += (diff * diff);
			}
			if (n == 0 || err < error) {
				error = err;
				error_index = n;
			}
		}
		b[4+ii] = error_index;
	}

	// higher order coeffs b7
	ii = 3;
	if (J[ii-1] <= 2) {
		b[4+ii] = 0.0;
	} else {
		for (int n=0; n < 16; n++) {
			float err=0.0;
			float diff;
			for (int j=1; j <= J[ii-1]-2 && j <= 4; j++) {
				if (dstar)
					diff = AmbePlusHOCb7[n][j-1] - C[ii-1][j+2-1];
				else
					diff = AmbeHOCb7[n][j-1] - C[ii-1][j+2-1];
				err += (diff * diff);
			}
			if (n == 0 || err < error) {
				error = err;
				error_index = n;
			}
		}
		b[4+ii] = error_index;
	}

	// higher order coeffs b8
	ii = 4;
	if (J[ii-1] <= 2) {
		b[4+ii] = 0.0;
	} else {
		int max_8 = (dstar) ? 16 : 8;
		for (int n=0; n < max_8; n++) {
			float err=0.0;
			float diff;
			for (int j=1; j <= J[ii-1]-2 && j <= 4; j++) {
				if (dstar)
					diff = AmbePlusHOCb8[n][j-1] - C[ii-1][j+2-1];
				else
					diff = AmbeHOCb8[n][j-1] - C[ii-1][j+2-1];
				err += (diff * diff);
			}
			if (n == 0 || err < error) {
				error = err;
				error_index = n;
			}
		}
		b[4+ii] = error_index;
	}
	// fprintf (stderr, "B\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n", b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7], b[8]);
	int rc;
	if (dstar)
		rc = mbe_dequantizeAmbe2400Parms (cur_mp, prev_mp, b);
	else
		rc = mbe_dequantizeAmbe2250Parms (cur_mp, prev_mp, b);
	mbe_moveMbeParms (cur_mp, prev_mp);
}

static void encode_49bit(uint8_t outp[49], const int b[9]) {
	outp[0] = (b[0] >> 6) & 1;
	outp[1] = (b[0] >> 5) & 1;
	outp[2] = (b[0] >> 4) & 1;
	outp[3] = (b[0] >> 3) & 1;
	outp[4] = (b[1] >> 4) & 1;
	outp[5] = (b[1] >> 3) & 1;
	outp[6] = (b[1] >> 2) & 1;
	outp[7] = (b[1] >> 1) & 1;
	outp[8] = (b[2] >> 4) & 1;
	outp[9] = (b[2] >> 3) & 1;
	outp[10] = (b[2] >> 2) & 1;
	outp[11] = (b[2] >> 1) & 1;
	outp[12] = (b[3] >> 8) & 1;
	outp[13] = (b[3] >> 7) & 1;
	outp[14] = (b[3] >> 6) & 1;
	outp[15] = (b[3] >> 5) & 1;
	outp[16] = (b[3] >> 4) & 1;
	outp[17] = (b[3] >> 3) & 1;
	outp[18] = (b[3] >> 2) & 1;
	outp[19] = (b[3] >> 1) & 1;
	outp[20] = (b[4] >> 6) & 1;
	outp[21] = (b[4] >> 5) & 1;
	outp[22] = (b[4] >> 4) & 1;
	outp[23] = (b[4] >> 3) & 1;
	outp[24] = (b[5] >> 4) & 1;
	outp[25] = (b[5] >> 3) & 1;
	outp[26] = (b[5] >> 2) & 1;
	outp[27] = (b[5] >> 1) & 1;
	outp[28] = (b[6] >> 3) & 1;
	outp[29] = (b[6] >> 2) & 1;
	outp[30] = (b[6] >> 1) & 1;
	outp[31] = (b[7] >> 3) & 1;
	outp[32] = (b[7] >> 2) & 1;
	outp[33] = (b[7] >> 1) & 1;
	outp[34] = (b[8] >> 2) & 1;
	outp[35] = b[1] & 1;
	outp[36] = b[2] & 1;
	outp[37] = (b[0] >> 2) & 1;
	outp[38] = (b[0] >> 1) & 1;
	outp[39] = b[0] & 1;
	outp[40] = b[3] & 1;
	outp[41] = (b[4] >> 2) & 1;
	outp[42] = (b[4] >> 1) & 1;
	outp[43] = b[4] & 1;
	outp[44] = b[5] & 1;
	outp[45] = b[6] & 1;
	outp[46] = b[7] & 1;
	outp[47] = (b[8] >> 1) & 1;
	outp[48] = b[8] & 1;
}

ambe_encoder::ambe_encoder(void)
	: d_49bit_mode(false),
	d_dstar_mode(false),
	d_alt_dstar_interleave(false),
	d_gain_adjust(0)
{
	mbe_parms enh_mp;
	mbe_initMbeParms (&cur_mp, &prev_mp, &enh_mp);
	}

void ambe_encoder::set_dstar_mode(void)
{
	d_dstar_mode = true;
}

void ambe_encoder::set_49bit_mode(void)
{
	d_49bit_mode = true;
}
// given a buffer of 160 audio samples (S16_LE),
// generate 72-bit ambe codeword (as 36 dibits in codeword[])
// (as 72 bits in codeword[] if in dstar mode)
// or 49-bit output codeword (if set_49bit_mode() has been called)
void ambe_encoder::encode(int16_t samples[], uint8_t codeword[])
{
	int b[9];
	int16_t frame_vector[8];	// result ignored

	// TODO: should disable fullrate encoding/quantization/interleaving
	// (unneeded in ambe encoder) to save CPU

	// first do speech analysis to generate mbe model parameters
	vocoder.imbe_encode(frame_vector, samples);

	// halfrate audio encoding - output rate is 2450 (49 bits)
	encode_ambe(vocoder.param(), b, &cur_mp, &prev_mp, d_dstar_mode, d_gain_adjust);

	if (d_dstar_mode) {
		interleaver.encode_dstar(codeword, b, d_alt_dstar_interleave);
	} else if (d_49bit_mode) {
		encode_49bit(codeword, b);
	} else {
		// add FEC and interleaving - output rate is 3600 (72 bits)
		interleaver.encode_vcw(codeword, b);
	}
}
