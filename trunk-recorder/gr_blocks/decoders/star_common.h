/*-
 * star_common.h
 *  common header for star_encode.h, star_decode.h
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

#ifndef __STAR_COMMON_H__
#define __STAR_COMMON_H__

#ifndef TWOPI
#define TWOPI (2.0 * 3.1415926535)
#endif

typedef enum _star_format {
  star_format_1,       // t1, t2, and s1 do not contribute to unit ID - original format, IDs to 2047
  star_format_1_4095,  // t1 = 2048, t2 used for mobile/portable, s1 ignored
  star_format_1_16383, // t1 = 8192, t2 = 4096, s1 = 2048 used to allow unit IDs to 16383 -- most typical
  star_format_sys,     // t1,t2 used for system ID, s1 = 2048
  star_format_2,       // t1 = 4096, t2 = mobile/portable, s1 = 2048
  star_format_3,       // t1 = 4096, t2 = 8192, s1 = 2048
  star_format_4        // t1 = 4096, t2 = 2048, s1 = 8192
} star_format;

#endif
