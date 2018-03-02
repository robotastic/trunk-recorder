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


#ifndef _SA_ENH
#define _SA_ENH

#define CNST_0_9898_Q1_15  0x7EB3
#define CNST_0_5_Q2_14     0x2000
#define CNST_1_2_Q2_14     0x4CCC


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
void sa_enh(IMBE_PARAM *imbe_param);


#endif
