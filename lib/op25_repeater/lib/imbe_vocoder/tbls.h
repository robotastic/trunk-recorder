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

#ifndef _TBLS
#define _TBLS


//-----------------------------------------------------------------------------
//
// Bit allocation and ancillary tables
//
//-----------------------------------------------------------------------------
extern const UWord16 bit_allocation_tbl[]; 
extern const Word16 bit_allocation_offset_tbl[]; 

//-----------------------------------------------------------------------------
//
// Log Magnitude Prediction Residual Block Length
//
//-----------------------------------------------------------------------------
extern const UWord32 lmprbl_tbl[];

//-----------------------------------------------------------------------------
//
// Gain Quantizer Levels in Q5.11 format
//
//-----------------------------------------------------------------------------
extern const Word16 gain_qnt_tbl[];
#define GAIN_QNT_TBL_SIZE 64

//-----------------------------------------------------------------------------
//
// Gain Ste Size in unsigned Q0.16 format
//
//-----------------------------------------------------------------------------
extern const UWord16 gain_step_size_tbl[];

//-----------------------------------------------------------------------------
//
// Standard Deviation of Higher Order DCT Coefficients in unsigned Q0.16 format
//
//-----------------------------------------------------------------------------
extern const UWord16 hi_ord_std_tbl[];

//-----------------------------------------------------------------------------
//
// Quantizer Step for Higher Order DCT Coefficients in unsigned Q1.15 format
//
//-----------------------------------------------------------------------------
extern const UWord16 hi_ord_step_size_tbl[];

//-----------------------------------------------------------------------------
//
// Speech Synthesis Window 
//
//-----------------------------------------------------------------------------
extern const Word16 ws[];

//-----------------------------------------------------------------------------
//
// Squared Pitch Estimation Window 64*wi^2
//
//-----------------------------------------------------------------------------
extern const Word16 wi[];

//-----------------------------------------------------------------------------
//
// Pitch Refinement Window 
//
//-----------------------------------------------------------------------------
extern const Word16 wr[];

//-----------------------------------------------------------------------------
//
// Real Part Spectrum of Pitch Refinement Window * 256 
//
//-----------------------------------------------------------------------------
extern const Word16 wr_sp[];

#endif

