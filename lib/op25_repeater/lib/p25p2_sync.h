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

#ifndef INCLUDED_P25P2_SYNC_H
#define INCLUDED_P25P2_SYNC_H

#include <stdint.h>
#include "p25p2_isch.h"

class p25p2_sync;
class p25p2_sync
{
public:
	p25p2_sync();	// constructor
	p25p2_isch isch;
	void check_confidence (const uint8_t dibits[]);
	bool in_sync(void);
	uint32_t tdma_slotid(void) { return _tdma_slotid; }
private:
	int32_t	sync_confidence;
	uint32_t _tdma_slotid;
	uint32_t packets;
};
#endif /* INCLUDED_P25P2_SYNC_H */
