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


#ifndef _MATH_SUB
#define _MATH_SUB

#define X05_Q15       16384         // (0.5*(1<<15)) 
#define ONE_Q15       32767         // ((1<<15)-1) 

//-----------------------------------------------------------------------------
//	PURPOSE:
//				Computes the cosine of x whose value is expressed in radians/PI.
//
//  INPUT:
//              x - argument in Q1.15
//
//	OUTPUT:
//		None
//
//	RETURN:
//		        Result in Q1.15
//
//-----------------------------------------------------------------------------
Word16 cos_fxp(Word16 x);

//-----------------------------------------------------------------------------
//	PURPOSE:
//				Computes the sinus of x whose value is expressed in radians/PI.
//
//  INPUT:
//              x - argument in Q1.15
//
//	OUTPUT:
//		None
//
//	RETURN:
//		        Result in Q1.15
//
//-----------------------------------------------------------------------------
Word16 sin_fxp(Word16 x);

//-----------------------------------------------------------------------------
//	PURPOSE:
//				Multiply a 32 bit number (L_var2) and a 16 bit
//              number (var1) returning a 32 bit result. L_var2
//              is truncated to 31 bits prior to executing the
//              multiply.     
//
//  INPUT:
//              L_var2 -   A Word32 input variable
//              var1   -   A Word16 input variable
//
//	OUTPUT:
//		None
//
//	RETURN:
//		        A Word32 value
//
//-----------------------------------------------------------------------------
Word32 L_mpy_ls(Word32 L_var2, Word16 var1);

//-----------------------------------------------------------------------------
//	PURPOSE:
//		Computes  pow(2.0, x)
//
//  INPUT:
//      x -  In signed Q10.22 format            
//
//	OUTPUT:
//		None
//
//	RETURN:
//		Result in signed Q14.2 format
//
//-----------------------------------------------------------------------------
Word16 Pow2 (Word32 x);

//-----------------------------------------------------------------------------
//	PURPOSE:
//				Computes sqrt(L_x), where  L_x is positive.
//
//  INPUT:
//              L_x  - argument in Q1.31
//              *exp - pointer to save denormalization exponent
//	OUTPUT:
//		        Right shift to be applied to result, Q16.0  
//
//	RETURN:
//		        Result in Q1.31
//              Right shift should be applied to it!
//
//-----------------------------------------------------------------------------
Word32 sqrt_l_exp (Word32 L_x,   Word16 *exp);

//-----------------------------------------------------------------------------
//	PURPOSE:
//				Computes log2 of x
//
//  INPUT:
//              x  - argument in Q14.2
//
//	OUTPUT:
//              None
//
//	RETURN:
//		        Result in Q10.22
//
//-----------------------------------------------------------------------------
Word32 Log2(Word16 x);

#endif
