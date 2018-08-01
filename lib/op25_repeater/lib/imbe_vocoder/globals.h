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


#ifndef _GLOBALS
#define _GLOBALS


//#define PI (double)3.1415926535897932384626433832795

#define CNST_0_9254_Q0_16  60647     // 0.9254 in unsigned Q0.16 format
#define CNST_0_33_Q0_16    0x5556    // 0.(3) = 1/3 in unsigned Q0.16 format


#define CNST_ONE_Q8_24     0x01000000
#define CNST_0_7_Q1_15     0x599A
#define CNST_0_4_Q1_15     0x3333
#define CNST_0_03_Q1_15    0x03D7
#define CNST_0_05_Q1_15    0x0666

#define CNST_1_125_Q8_8    0x0120
#define CNST_0_5_Q8_8      0x0080
#define CNST_0_125_Q8_8    0x0020
#define CNST_0_25_Q8_8     0x0040

#define CNST_0_8717_Q1_15  0x6F94
#define CNST_0_0031_Q1_15  0x0064
#define CNST_0_48_Q4_12    0x07AE
#define CNST_1_00_Q4_12    0x1000
#define CNST_0_85_Q4_12    0x0D9B
#define CNST_0_4_Q4_12     0x0666
#define CNST_0_05_Q4_12    0x00CD
#define CNST_0_5882_Q1_15  0x4B4B
#define CNST_0_2857_Q1_15  0x2492


#define HI_BYTE(a) ((a >> 8) & 0xFF)
#define LO_BYTE(a) (a & 0xFF);





#endif
