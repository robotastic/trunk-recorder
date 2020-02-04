/*-
 * fsync_types.h
 *  common typedef header for fsync_decode.h, fsync_encode.h
 *
 * Author: Matthew Kaufman (matthew@eeph.com)
 *
 * Copyright (c) 2012, 2013, 2014  Matthew Kaufman  All rights reserved.
 * 
 *  This file is part of Matthew Kaufman's fsync Encoder/Decoder Library
 *
 *  The fsync Encoder/Decoder Library is free software; you can
 *  redistribute it and/or modify it under the terms of version 2 of
 *  the GNU General Public License as published by the Free Software
 *  Foundation.
 *
 *  If you cannot comply with the terms of this license, contact
 *  the author for alternative license arrangements or do not use
 *  or redistribute this software.
 *
 *  The fsync Encoder/Decoder Library is distributed in the hope
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

#ifndef _FSYNC_TYPES_H_
#define _FSYNC_TYPES_H_

typedef int fsync_s32;
typedef unsigned int fsync_u32_t;
typedef short fsync_s16_t;
typedef unsigned short fsync_u16_t;
typedef char fsync_s8_t;
typedef unsigned char fsync_u8_t;
typedef double fsync_float_t;
typedef int fsync_int_t;

/* to change the data type, set this typedef: */
typedef float fsync_sample_t;

/* AND set this to match: */
/* #define FSYNC_SAMPLE_FORMAT_U8 */
/* #define FSYNC_SAMPLE_FORMAT_S16 */
/* #define FSYNC_SAMPLE_FORMAT_U16 */
#define FSYNC_SAMPLE_FORMAT_FLOAT


#endif
