/*-
 * mdc_types.h
 *  common typedef header for mdc_decode.h, mdc_encode.h
 *
 * Author: Matthew Kaufman (matthew@eeph.com)
 *
 * Copyright (c) 2010  Matthew Kaufman  All rights reserved.
 *
 *  This file is part of Matthew Kaufman's MDC Encoder/Decoder Library
 *
 *  The MDC Encoder/Decoder Library is free software; you can
 *  redistribute it and/or modify it under the terms of version 2 of
 *  the GNU General Public License as published by the Free Software
 *  Foundation.
 *
 *  If you cannot comply with the terms of this license, contact
 *  the author for alternative license arrangements or do not use
 *  or redistribute this software.
 *
 *  The MDC Encoder/Decoder Library is distributed in the hope
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

#ifndef _MDC_TYPES_H_
#define _MDC_TYPES_H_

typedef int mdc_s32;
typedef unsigned int mdc_u32_t;
typedef short mdc_s16_t;
typedef unsigned short mdc_u16_t;
typedef char mdc_s8_t;
typedef unsigned char mdc_u8_t;
typedef int mdc_int_t;

#ifndef MDC_FIXEDMATH
typedef double mdc_float_t;
#endif // MDC_FIXEDMATH

/* to change the data type, set this typedef: */
typedef float mdc_sample_t;

/* AND set this to match: */
/* #define MDC_SAMPLE_FORMAT_U8 */
/* #define MDC_SAMPLE_FORMAT_S16 */
/* #define MDC_SAMPLE_FORMAT_U16 */
#define MDC_SAMPLE_FORMAT_FLOAT

#endif
