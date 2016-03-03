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
#include "math_sub.h"

//-----------------------------------------------------------------------------
// Table for routine Pow2()     table[] = 2^(-1...0)
//-----------------------------------------------------------------------------
static const Word16 pow2_table[33] =
{
	16384, 16743, 17109, 17484, 17867, 18258, 18658, 19066, 19484, 19911,
	20347, 20792, 21247, 21713, 22188, 22674, 23170, 23678, 24196, 24726,
	25268, 25821, 26386, 26964, 27554, 28158, 28774, 29405, 30048, 30706,
	31379, 32066, 32767
};

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
Word16 Pow2(Word32 x)
{
	Word16 exp, i, a, tmp;
	Word32 L_x;
	Word16 exponent, fraction;

	exponent = extract_h(L_shr(x, 6));
	if(exponent < 0)
		exponent = add(exponent, 1);
	fraction = extract_l(L_shr(L_sub(x, L_shl(L_deposit_l(exponent),6 + 16)), 7));

	if(x < 0)
		fraction = negate(fraction);   			 

	L_x = L_mult(fraction, 32);                             //  L_x = fraction<<6          
	i = extract_h(L_x);                                     //  Extract b10-b16 of fraction 
	L_x = L_shr(L_x, 1);
	a = extract_l(L_x);                                     //  Extract b0-b9   of fraction 
	a = a & (Word16)0x7fff;    

	L_x = L_deposit_h (pow2_table[i]);                      // table[i] << 16        
	tmp = sub(pow2_table[i], pow2_table[i + 1]);            // table[i] - table[i+1] 
	L_x = L_msu(L_x, tmp, a);                               // L_x -= tmp*a*2      

	if(x < 0)
	{
		L_x = L_deposit_h(div_s(0x4000, extract_h(L_x)));   // calculate 1/fraction
		exponent = sub(exponent, 1);
	}

	exp = sub(12, exponent);
	L_x = L_shr_r(L_x, exp);

	return extract_h(L_x);
}

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
Word32 L_mpy_ls(Word32 L_var2, Word16 var1)
{
	Word32 L_varOut;
	Word16 swtemp;

	swtemp = shr(extract_l(L_var2), 1);
	swtemp = (Word16)32767 & (Word16) swtemp;

	L_varOut = L_mult(var1, swtemp);
	L_varOut = L_shr(L_varOut, 15);
	L_varOut = L_mac(L_varOut, var1, extract_h(L_var2));
	return (L_varOut);
}

//-----------------------------------------------------------------------------
// Table for routine cos_fxp()
//-----------------------------------------------------------------------------
static const Word16 cos_table[129] =
{ 
	32767, 32766, 32758, 32746, 32729, 32706, 32679, 32647, 32610,
	32568, 32522, 32470, 32413, 32352, 32286, 32214, 32138, 32058,
	31972, 31881, 31786, 31686, 31581, 31471, 31357, 31238, 31114,
	30986, 30853, 30715, 30572, 30425, 30274, 30118, 29957, 29792,
	29622, 29448, 29269, 29086, 28899, 28707, 28511, 28311, 28106,
	27897, 27684, 27467, 27246, 27020, 26791, 26557, 26320, 26078,
	25833, 25583, 25330, 25073, 24812, 24548, 24279, 24008, 23732,
	23453, 23170, 22884, 22595, 22302, 22006, 21706, 21403, 21097,
	20788, 20475, 20160, 19841, 19520, 19195, 18868, 18538, 18205,
	17869, 17531, 17190, 16846, 16500, 16151, 15800, 15447, 15091,
	14733, 14373, 14010, 13646, 13279, 12910, 12540, 12167, 11793,
	11417, 11039, 10660, 10279,  9896,  9512,  9127,  8740,  8351,
	7962,   7571,  7180,  6787,  6393,  5998,  5602,  5205,  4808,
	4410,   4011,  3612,  3212,  2811,  2411,  2009,  1608,  1206,
	804,     402,     0
};

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
Word16 cos_fxp(Word16 x)
{
	Word16 tx, ty;
	Word16 sign;
	Word16 index1,index2;
	Word16 m;
	Word16 temp;

	sign = 0;
	if(x < 0) 
		tx = negate(x);     
	else
		tx = x;     

	// if angle > pi/2, cos(angle) = -cos(pi-angle) 
	if(tx > X05_Q15)
	{
		tx = sub(ONE_Q15,tx);     
		sign = -1;     
	}
	// convert input to be within range 0-128 
	index1 = shr(tx,7);     
	index2 = add(index1,1);    

	if (index1 == 128)
		return (Word16)0;

	m = sub(tx,shl(index1,7));
	// convert decimal part to Q15 
	m = shl(m,8);    

	temp = sub(cos_table[index2],cos_table[index1]);
	temp = mult(m,temp);
	ty   = add(cos_table[index1],temp);    

	if(sign)
		return negate(ty);
	else
		return ty;
}

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
Word16 sin_fxp(Word16 x)
{
	Word16 tx, ty;
	Word16 sign;

	sign = 0;
	if(x < 0)
	{
		tx = negate(x);     
		sign = 1;
	}
	else
		tx = x;     
	
	if(tx > X05_Q15)
		tx = sub(tx, X05_Q15);     
	else
		tx = sub(X05_Q15,tx); 

	ty = cos_fxp(tx);

	if(sign)
		return negate(ty);
	else
		return ty;
}


//-----------------------------------------------------------------------------
// Table for routine sqrt_l_exp()     
// table[] = sqrt((i+16)*2^-6) * 2^15, i.e. sqrt(x) scaled Q15 
//-----------------------------------------------------------------------------
static const Word16 sqrt_table[49] =
{
	16384, 16888, 17378, 17854, 18318, 18770, 19212, 19644, 20066, 20480,
	20886, 21283, 21674, 22058, 22435, 22806, 23170, 23530, 23884, 24232,
	24576, 24915, 25249, 25580, 25905, 26227, 26545, 26859, 27170, 27477,
	27780, 28081, 28378, 28672, 28963, 29251, 29537, 29819, 30099, 30377,
	30652, 30924, 31194, 31462, 31727, 31991, 32252, 32511, 32767
};

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
Word32 sqrt_l_exp(Word32 L_x,  Word16 *exp)
{
	Word16 e, i, a, tmp;
	Word32 L_y;
	
	if(L_x <= (Word32)0)
	{
		*exp = 0;              
		return (Word32)0;
	}

	e    = norm_l(L_x) & 0xFFFE;     // get next lower EVEN norm. exp  
	L_x  = L_shl(L_x, e);            // L_x is normalized to [0.25..1) 
	*exp = e >> 1;                   // return 2*exponent (or Q1)      

	L_x  = L_shr(L_x, 9);
	i    = extract_h(L_x);           // Extract b25-b31, 16 <= i <= 63 because of normalization                  
	L_x  = L_shr(L_x, 1);   
	a    = extract_l(L_x);           // Extract b10-b24                        
	a    = a & (Word16)0x7fff;   

	i    = sub(i, 16);               // 0 <= i <= 47                           

	L_y  = L_deposit_h(sqrt_table[i]);                // table[i] << 16                 
	tmp  = sub(sqrt_table[i], sqrt_table[i + 1]);     // table[i] - table[i+1])         
	L_y  = L_msu(L_y, tmp, a);                        // L_y -= tmp*a*2                 

	return L_y;
}

//-----------------------------------------------------------------------------
// Table for routine Log2()     
//-----------------------------------------------------------------------------
static const Word16 log_table[33] =
{
	0,      1455,  2866,  4236,  5568,  6863,  8124,  9352, 10549, 11716,
	12855, 13967, 15054, 16117, 17156, 18172, 19167, 20142, 21097, 22033,
	22951, 23852, 24735, 25603, 26455, 27291, 28113, 28922, 29716, 30497,
	31266, 32023, 32767
};

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
Word32 Log2(Word16 x)
{
	Word16 exp, i, a, tmp;
	Word32 L_y;

	if(x <= (Word16)0)
		return 0;
	
	exp = norm_s(x);
	x   = shl(x, exp);

	i = shr(x, 9);                              // Extract b15-b9 
	a = shl(x & 0x1FF, 6);                      // Extract b8-b0 
	i = sub (i, 32);

	L_y = L_deposit_h(log_table[i]);            // table[i] << 16        
	tmp = sub(log_table[i], log_table[i + 1]);  // table[i] - table[i+1] 
	L_y = L_msu(L_y, tmp, a);                   // L_y -= tmp*a*2  

	L_y = L_shr(L_y, 9);

	exp = sub(12, exp);
	L_y = L_add(L_y, L_deposit_h(shl(exp, 6)));		
	
	return L_y;
}









