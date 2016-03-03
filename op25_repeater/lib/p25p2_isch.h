
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

#ifndef INCLUDED_P25P2_ISCH_H
#define INCLUDED_P25P2_ISCH_H

#include <stdint.h>
#include <map>
#include <string>

class p25p2_isch;
class p25p2_isch
{
public:
	p25p2_isch();	// constructor
	int16_t isch_lookup(std::string codeword);
	int16_t isch_lookup(const uint8_t dibits[]);
private:
	std::map <std::string, int> isch_map;
};
#endif /* INCLUDED_P25P2_ISCH_H */
