
// P25 TDMA Decoder (C) Copyright 2013, 2014 Max H. Parke KA1RBI
// 
// This file is part of OP25
// 
// OP25 is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3, or (at your option)
// any later version.
// 
// OP25 is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public
// License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with OP25; see the file COPYING. If not, write to the Free
// Software Foundation, Inc., 51 Franklin Street, Boston, MA
// 02110-1301, USA.

#ifndef INCLUDED_P25P2_DUID_H
#define INCLUDED_P25P2_DUID_H

#include <stdint.h>

static const char* duid_strings[] = {
	"4v",
	"?1",
	"?2",
	"sacch w",
	"?4",
	"?5",
	"2v",
	"?7",
	"?8",
	"facch w",
	"?10",
	"?11"
	"sacch w/o",
	"?13",
	"?14"
	"facch w/o"
};

class p25p2_duid;
class p25p2_duid
{
public:
	p25p2_duid();	// constructor
	int16_t duid_lookup(const uint8_t codeword);
	uint8_t extract_duid(const uint8_t dibits[]);
private:
};
#endif /* INCLUDED_P25P2_DUID_H */
