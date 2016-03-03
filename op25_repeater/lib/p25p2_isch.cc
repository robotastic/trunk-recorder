
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
	isch_map["184229d461"] = 0;
	isch_map["18761451f6"] = 1;
	isch_map["181ae27e2f"] = 2;
	isch_map["182edffbb8"] = 3;
	isch_map["18df8a7510"] = 4;
	isch_map["18ebb7f087"] = 5;
	isch_map["188741df5e"] = 6;
	isch_map["18b37c5ac9"] = 7;
	isch_map["1146a44f13"] = 8;
	isch_map["117299ca84"] = 9;
	isch_map["111e6fe55d"] = 10;
	isch_map["112a5260ca"] = 11;
	isch_map["11db07ee62"] = 12;
	isch_map["11ef3a6bf5"] = 13;
	isch_map["1183cc442c"] = 14;
	isch_map["11b7f1c1bb"] = 15;
	isch_map["1a4a2e239e"] = 16;
	isch_map["1a7e13a609"] = 17;
	isch_map["1a12e589d0"] = 18;
	isch_map["1a26d80c47"] = 19;
	isch_map["1ad78d82ef"] = 20;
	isch_map["1ae3b00778"] = 21;
	isch_map["1a8f4628a1"] = 22;
	isch_map["1abb7bad36"] = 23;
	isch_map["134ea3b8ec"] = 24;
	isch_map["137a9e3d7b"] = 25;
	isch_map["13166812a2"] = 26;
	isch_map["1322559735"] = 27;
	isch_map["13d300199d"] = 28;
	isch_map["13e73d9c0a"] = 29;
	isch_map["138bcbb3d3"] = 30;
	isch_map["13bff63644"] = 31;
	isch_map["1442f705ef"] = 32;
	isch_map["1476ca8078"] = 33;
	isch_map["141a3cafa1"] = 34;
	isch_map["142e012a36"] = 35;
	isch_map["14df54a49e"] = 36;
	isch_map["14eb692109"] = 37;
	isch_map["14879f0ed0"] = 38;
	isch_map["14b3a28b47"] = 39;
	isch_map["1d467a9e9d"] = 40;
	isch_map["1d72471b0a"] = 41;
	isch_map["1d1eb134d3"] = 42;
	isch_map["1d2a8cb144"] = 43;
	isch_map["1ddbd93fec"] = 44;
	isch_map["1defe4ba7b"] = 45;
	isch_map["1d831295a2"] = 46;
	isch_map["1db72f1035"] = 47;
	isch_map["164af0f210"] = 48;
	isch_map["167ecd7787"] = 49;
	isch_map["16123b585e"] = 50;
	isch_map["162606ddc9"] = 51;
	isch_map["16d7535361"] = 52;
	isch_map["16e36ed6f6"] = 53;
	isch_map["168f98f92f"] = 54;
	isch_map["16bba57cb8"] = 55;
	isch_map["1f4e7d6962"] = 56;
	isch_map["1f7a40ecf5"] = 57;
	isch_map["1f16b6c32c"] = 58;
	isch_map["1f228b46bb"] = 59;
	isch_map["1fd3dec813"] = 60;
	isch_map["1fe7e34d84"] = 61;
	isch_map["1f8b15625d"] = 62;
	isch_map["1fbf28e7ca"] = 63;
	isch_map["84d62c339"] = 64;
	isch_map["8795f46ae"] = 65;
	isch_map["815a96977"] = 66;
	isch_map["82194ece0"] = 67;
	isch_map["8d0c16248"] = 68;
	isch_map["8e4fce7df"] = 69;
	isch_map["8880ac806"] = 70;
	isch_map["8bc374d91"] = 71;
	isch_map["149ef584b"] = 72;
	isch_map["17dd2dddc"] = 73;
	isch_map["11124f205"] = 74;
	isch_map["125197792"] = 75;
	isch_map["1d44cf93a"] = 76;
	isch_map["1e0717cad"] = 77;
	isch_map["18c875374"] = 78;
	isch_map["1b8bad6e3"] = 79;
	isch_map["a456534c6"] = 80;
	isch_map["a7158b151"] = 81;
	isch_map["a1dae9e88"] = 82;
	isch_map["a29931b1f"] = 83;
	isch_map["ad8c695b7"] = 84;
	isch_map["aecfb1020"] = 85;
	isch_map["a800d3ff9"] = 86;
	isch_map["ab430ba6e"] = 87;
	isch_map["341e8afb4"] = 88;
	isch_map["375d52a23"] = 89;
	isch_map["3192305fa"] = 90;
	isch_map["32d1e806d"] = 91;
	isch_map["3dc4b0ec5"] = 92;
	isch_map["3e8768b52"] = 93;
	isch_map["38480a48b"] = 94;
	isch_map["3b0bd211c"] = 95;
	isch_map["44dbc12b7"] = 96;
	isch_map["479819720"] = 97;
	isch_map["41577b8f9"] = 98;
	isch_map["4214a3d6e"] = 99;
	isch_map["4d01fb3c6"] = 100;
	isch_map["4e4223651"] = 101;
	isch_map["488d41988"] = 102;
	isch_map["4bce99c1f"] = 103;
	isch_map["d493189c5"] = 104;
	isch_map["d7d0c0c52"] = 105;
	isch_map["d11fa238b"] = 106;
	isch_map["d25c7a61c"] = 107;
	isch_map["dd49228b4"] = 108;
	isch_map["de0afad23"] = 109;
	isch_map["d8c5982fa"] = 110;
	isch_map["db864076d"] = 111;
	isch_map["645bbe548"] = 112;
	isch_map["6718660df"] = 113;
	isch_map["61d704f06"] = 114;
	isch_map["6294dca91"] = 115;
	isch_map["6d8184439"] = 116;
	isch_map["6ec25c1ae"] = 117;
	isch_map["680d3ee77"] = 118;
	isch_map["6b4ee6be0"] = 119;
	isch_map["f41367e3a"] = 120;
	isch_map["f750bfbad"] = 121;
	isch_map["f19fdd474"] = 122;
	isch_map["f2dc051e3"] = 123;
	isch_map["fdc95df4b"] = 124;
	isch_map["fe8a85adc"] = 125;
	isch_map["f845e7505"] = 126;
	isch_map["fb063f092"] = 127;
}
int16_t
p25p2_isch::isch_lookup(const uint8_t dibits[])
{
	char s[64];
	uint64_t cw = 0;
	for (int i=0; i<20; i++)
		cw = (cw << 2) + dibits[i];
	sprintf(s, "%llx", (unsigned long long)cw);
	return isch_lookup(std::string(s));
}
int16_t
p25p2_isch::isch_lookup(std::string codeword)
{
	if (codeword.compare("575d57f7ff") == 0)
		return -2;
	else if (isch_map.find(codeword) == isch_map.end())
		return -1;	// FIXME- handle bit error(s) in codeword
	else
		return isch_map[codeword];
}
