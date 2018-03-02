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

static UWord32 seed = 1;


//-----------------------------------------------------------------------------
//	PURPOSE:
//				Generate pseudo-random numbers in range -1...1
//
//
//  INPUT:
//		None
//
//	OUTPUT:
//		None
//
//	RETURN:
//		        Pseudo-random number in signed Q1.16 format
//
//-----------------------------------------------------------------------------
Word16 rand_gen(void)
{
	UWord32 hi, lo;

	lo = 16807 * (seed & 0xFFFF);
	hi = 16807 * (seed >> 16);

	lo += (Word32)(hi & 0x7FFF) << 16;
	lo += (hi >> 15);

	if(lo > 0x7FFFFFFF)
		lo -= 0x7FFFFFFF;

	seed = lo;

	return (Word16)lo;
}
