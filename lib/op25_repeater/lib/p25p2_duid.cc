
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

#include <stdint.h>
#include <vector>
#include <iostream>
#include "p25p2_duid.h"

static const int16_t _duid_lookup[256] = {
	0,0,0,-1,0,-1,-1,1,0,-1,-1,4,-1,8,2,-1,
	0,-1,-1,1,-1,1,1,1,-1,3,9,-1,5,-1,-1,1,
	0,-1,-1,10,-1,6,2,-1,-1,3,2,-1,2,-1,2,2,
	-1,3,7,-1,11,-1,-1,1,3,3,-1,3,-1,3,2,-1,
	0,-1,-1,4,-1,6,12,-1,-1,4,4,4,5,-1,-1,4,
	-1,13,7,-1,5,-1,-1,1,5,-1,-1,4,5,5,5,-1,
	-1,6,7,-1,6,6,-1,6,14,-1,-1,4,-1,6,2,-1,
	7,-1,7,7,-1,6,7,-1,-1,3,7,-1,5,-1,-1,15,
	0,-1,-1,10,-1,8,12,-1,-1,8,9,-1,8,8,-1,8,
	-1,13,9,-1,11,-1,-1,1,9,-1,9,9,-1,8,9,-1,
	-1,10,10,10,11,-1,-1,10,14,-1,-1,10,-1,8,2,-1,
	11,-1,-1,10,11,11,11,-1,-1,3,9,-1,11,-1,-1,15,
	-1,13,12,-1,12,-1,12,12,14,-1,-1,4,-1,8,12,-1,
	13,13,-1,13,-1,13,12,-1,-1,13,9,-1,5,-1,-1,15,
	14,-1,-1,10,-1,6,12,-1,14,14,14,-1,14,-1,-1,15,
	-1,13,7,-1,11,-1,-1,15,14,-1,-1,15,-1,15,15,15,
};

p25p2_duid::p25p2_duid(void)
{
}

int16_t p25p2_duid::duid_lookup(const uint8_t codeword)
{
	return _duid_lookup[codeword];
}

uint8_t p25p2_duid::extract_duid(const uint8_t dibits[])
{
	int duid0 = dibits[10];
	int duid1 = dibits[47];
	int duid2 = dibits[132];
	int duid3 = dibits[169];
	uint8_t v = (duid0 << 6) + (duid1 << 4) + (duid2 << 2) + duid3;
	return (v);
}
