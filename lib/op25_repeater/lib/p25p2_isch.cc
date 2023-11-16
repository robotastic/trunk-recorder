
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
#include <string.h>
#include <stdio.h>
#include <map>
#include <string>
#include <iostream>
#include "p25p2_isch.h"

#if 0
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
#endif 


p25p2_isch::p25p2_isch(void)
{
	isch_map[0x184229d461ULL] = 0;
	isch_map[0x18761451f6ULL] = 1;
	isch_map[0x181ae27e2fULL] = 2;
	isch_map[0x182edffbb8ULL] = 3;
	isch_map[0x18df8a7510ULL] = 4;
	isch_map[0x18ebb7f087ULL] = 5;
	isch_map[0x188741df5eULL] = 6;
	isch_map[0x18b37c5ac9ULL] = 7;
	isch_map[0x1146a44f13ULL] = 8;
	isch_map[0x117299ca84ULL] = 9;
	isch_map[0x111e6fe55dULL] = 10;
	isch_map[0x112a5260caULL] = 11;
	isch_map[0x11db07ee62ULL] = 12;
	isch_map[0x11ef3a6bf5ULL] = 13;
	isch_map[0x1183cc442cULL] = 14;
	isch_map[0x11b7f1c1bbULL] = 15;
	isch_map[0x1a4a2e239eULL] = 16;
	isch_map[0x1a7e13a609ULL] = 17;
	isch_map[0x1a12e589d0ULL] = 18;
	isch_map[0x1a26d80c47ULL] = 19;
	isch_map[0x1ad78d82efULL] = 20;
	isch_map[0x1ae3b00778ULL] = 21;
	isch_map[0x1a8f4628a1ULL] = 22;
	isch_map[0x1abb7bad36ULL] = 23;
	isch_map[0x134ea3b8ecULL] = 24;
	isch_map[0x137a9e3d7bULL] = 25;
	isch_map[0x13166812a2ULL] = 26;
	isch_map[0x1322559735ULL] = 27;
	isch_map[0x13d300199dULL] = 28;
	isch_map[0x13e73d9c0aULL] = 29;
	isch_map[0x138bcbb3d3ULL] = 30;
	isch_map[0x13bff63644ULL] = 31;
	isch_map[0x1442f705efULL] = 32;
	isch_map[0x1476ca8078ULL] = 33;
	isch_map[0x141a3cafa1ULL] = 34;
	isch_map[0x142e012a36ULL] = 35;
	isch_map[0x14df54a49eULL] = 36;
	isch_map[0x14eb692109ULL] = 37;
	isch_map[0x14879f0ed0ULL] = 38;
	isch_map[0x14b3a28b47ULL] = 39;
	isch_map[0x1d467a9e9dULL] = 40;
	isch_map[0x1d72471b0aULL] = 41;
	isch_map[0x1d1eb134d3ULL] = 42;
	isch_map[0x1d2a8cb144ULL] = 43;
	isch_map[0x1ddbd93fecULL] = 44;
	isch_map[0x1defe4ba7bULL] = 45;
	isch_map[0x1d831295a2ULL] = 46;
	isch_map[0x1db72f1035ULL] = 47;
	isch_map[0x164af0f210ULL] = 48;
	isch_map[0x167ecd7787ULL] = 49;
	isch_map[0x16123b585eULL] = 50;
	isch_map[0x162606ddc9ULL] = 51;
	isch_map[0x16d7535361ULL] = 52;
	isch_map[0x16e36ed6f6ULL] = 53;
	isch_map[0x168f98f92fULL] = 54;
	isch_map[0x16bba57cb8ULL] = 55;
	isch_map[0x1f4e7d6962ULL] = 56;
	isch_map[0x1f7a40ecf5ULL] = 57;
	isch_map[0x1f16b6c32cULL] = 58;
	isch_map[0x1f228b46bbULL] = 59;
	isch_map[0x1fd3dec813ULL] = 60;
	isch_map[0x1fe7e34d84ULL] = 61;
	isch_map[0x1f8b15625dULL] = 62;
	isch_map[0x1fbf28e7caULL] = 63;
	isch_map[0x084d62c339ULL] = 64;
	isch_map[0x08795f46aeULL] = 65;
	isch_map[0x0815a96977ULL] = 66;
	isch_map[0x082194ece0ULL] = 67;
	isch_map[0x08d0c16248ULL] = 68;
	isch_map[0x08e4fce7dfULL] = 69;
	isch_map[0x08880ac806ULL] = 70;
	isch_map[0x08bc374d91ULL] = 71;
	isch_map[0x0149ef584bULL] = 72;
	isch_map[0x017dd2dddcULL] = 73;
	isch_map[0x011124f205ULL] = 74;
	isch_map[0x0125197792ULL] = 75;
	isch_map[0x01d44cf93aULL] = 76;
	isch_map[0x01e0717cadULL] = 77;
	isch_map[0x018c875374ULL] = 78;
	isch_map[0x01b8bad6e3ULL] = 79;
	isch_map[0x0a456534c6ULL] = 80;
	isch_map[0x0a7158b151ULL] = 81;
	isch_map[0x0a1dae9e88ULL] = 82;
	isch_map[0x0a29931b1fULL] = 83;
	isch_map[0x0ad8c695b7ULL] = 84;
	isch_map[0x0aecfb1020ULL] = 85;
	isch_map[0x0a800d3ff9ULL] = 86;
	isch_map[0x0ab430ba6eULL] = 87;
	isch_map[0x0341e8afb4ULL] = 88;
	isch_map[0x0375d52a23ULL] = 89;
	isch_map[0x03192305faULL] = 90;
	isch_map[0x032d1e806dULL] = 91;
	isch_map[0x03dc4b0ec5ULL] = 92;
	isch_map[0x03e8768b52ULL] = 93;
	isch_map[0x038480a48bULL] = 94;
	isch_map[0x03b0bd211cULL] = 95;
	isch_map[0x044dbc12b7ULL] = 96;
	isch_map[0x0479819720ULL] = 97;
	isch_map[0x041577b8f9ULL] = 98;
	isch_map[0x04214a3d6eULL] = 99;
	isch_map[0x04d01fb3c6ULL] = 100;
	isch_map[0x04e4223651ULL] = 101;
	isch_map[0x0488d41988ULL] = 102;
	isch_map[0x04bce99c1fULL] = 103;
	isch_map[0x0d493189c5ULL] = 104;
	isch_map[0x0d7d0c0c52ULL] = 105;
	isch_map[0x0d11fa238bULL] = 106;
	isch_map[0x0d25c7a61cULL] = 107;
	isch_map[0x0dd49228b4ULL] = 108;
	isch_map[0x0de0afad23ULL] = 109;
	isch_map[0x0d8c5982faULL] = 110;
	isch_map[0x0db864076dULL] = 111;
	isch_map[0x0645bbe548ULL] = 112;
	isch_map[0x06718660dfULL] = 113;
	isch_map[0x061d704f06ULL] = 114;
	isch_map[0x06294dca91ULL] = 115;
	isch_map[0x06d8184439ULL] = 116;
	isch_map[0x06ec25c1aeULL] = 117;
	isch_map[0x0680d3ee77ULL] = 118;
	isch_map[0x06b4ee6be0ULL] = 119;
	isch_map[0x0f41367e3aULL] = 120;
	isch_map[0x0f750bfbadULL] = 121;
	isch_map[0x0f19fdd474ULL] = 122;
	isch_map[0x0f2dc051e3ULL] = 123;
	isch_map[0x0fdc95df4bULL] = 124;
	isch_map[0x0fe8a85adcULL] = 125;
	isch_map[0x0f845e7505ULL] = 126;
	isch_map[0x0fb063f092ULL] = 127;
    isch_map[0x575d57f7ffULL] =  -2; // S-ISCH
}
int16_t
p25p2_isch::isch_lookup(const uint8_t dibits[])
{
	char s[64];
	uint64_t cw = 0;
	for (int i=0; i<20; i++)
		cw = (cw << 2) + dibits[i];
	return isch_lookup(cw);
}
int16_t
p25p2_isch::isch_lookup(uint64_t cw)
{
	if (isch_map.find(cw) == isch_map.end())                // Must contain bit errors if no exact match, so
    {                                                       // look for codeword with smallest hamming distance
        int popct, popmin = 40;
        int16_t msg = -1;
        for (auto it = isch_map.begin(); it != isch_map.end(); ++it)
        {
            popct = __builtin_popcountll(cw ^ it->first);
            if ((popct <= 7) && (popct < popmin))           // Encoding is (40, 9, 16) which can correct at most 7 bit-errors
            {
                msg = it->second;
                popmin = popct;
            }
        }
		return msg;
    }
	else                                                    // No errors, so simply return the matching value
		return isch_map[cw];
}

