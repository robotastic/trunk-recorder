/*-
 * star_types.h
 *  common typedef header for star_encode.h, star_decode.h
 *
 * Author: Matthew Kaufman (matthew@eeph.com)
 *
 * Copyright (c) 2012  Matthew Kaufman  All rights reserved.
 *
 *  This file is part of Matthew Kaufman's STAR Encoder/Decoder Library
 *
 *  The STAR Encoder/Decoder Library is free software; you can
 *  redistribute it and/or modify it under the terms of version 2 of
 *  the GNU General Public License as published by the Free Software
 *  Foundation.
 *
 *  If you cannot comply with the terms of this license, contact
 *  the author for alternative license arrangements or do not use
 *  or redistribute this software.
 *
 *  The STAR Encoder/Decoder Library is distributed in the hope
 *  that it will be useful, but WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 *  USA.
 *
 *  or see http://www.gnu.org/copyleft/gpl.html
 *
-*/

#ifndef _STAR_TYPES_H_
#define _STAR_TYPES_H_

typedef int star_s32_t;
typedef unsigned int star_u32_t;
typedef short star_s16_t;
typedef unsigned short star_u16_t;
typedef char star_s8_t;
typedef unsigned char star_u8_t;
typedef double star_float_t;
typedef int star_int_t;

/* to change the data type, set this typedef: */
typedef float star_sample_t;

/* AND uncomment this to match: */
/* #define STAR_SAMPLE_FORMAT_U8 */
/* #define STAR_SAMPLE_FORMAT_S16 */
/* #define STAR_SAMPLE_FORMAT_U16 */
#define STAR_SAMPLE_FORMAT_FLOAT

#endif
