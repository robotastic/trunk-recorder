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

#include <stdio.h>
#include <stdint.h>
#include <map>
#include <string.h>
#include <string>
#include <iostream>
#include "p25p2_sync.h"

p25p2_sync::p25p2_sync(void) :	// constructor
	sync_confidence(0),
	_tdma_slotid(0),
	packets(0)
{
}

bool p25p2_sync::in_sync(void)
{
	return (sync_confidence != 0);
}

void p25p2_sync::check_confidence (const uint8_t dibits[])
{
	static const int expected_sync[] = {0, 1, -2, -2, 4, 5, -2, -2, 8, 9, -2, -2};
	_tdma_slotid ++;
	packets++;
	if (_tdma_slotid >= 12)
		_tdma_slotid = 0;
	int rc, cnt, fr, loc, chn, checkval;
	rc = isch.isch_lookup(dibits);
	checkval = cnt = fr = loc = chn = rc;
	if (rc >= 0) {
		cnt = rc & 3;
		rc = rc >> 2;
		fr = rc & 1;
		rc = rc >> 1;
		loc = rc & 3;
		rc = rc >> 2;
		chn = rc & 3;
		checkval = loc*4 + chn;
	}
	if (expected_sync[_tdma_slotid] != checkval && checkval != -1)
		sync_confidence = 0;
	if (chn >= 0) {
		sync_confidence = 1;
		_tdma_slotid = checkval;
	}
}
