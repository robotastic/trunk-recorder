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
#include "p25p2_vf.h"
#include "rs.h"

static void load_i(const uint8_t src[], int& dest, int count) {
	dest = 0;
	for (int i=0; i<count; i++)
		dest = (dest << 1) + src[count-1-i];
}

	void p25p2_vf::process_vcw(const uint8_t vf[], int* b) {
		int c0,c1,c2,c3;
		int u0,u1,u2,u3;
		extract_vcw(vf, c0, c1, c2, c3 );

		u0 = gly24128Dec(c0);
		int pr[24];
		int _m1[24];
		pr[0] = 16 * u0;
		for (int n=1; n < 24; n++) {
			pr[n] = (173*pr[n-1] + 13849) - 65536 * int((173*pr[n-1]+13849)/65536);
			_m1[n-1] = (pr[n] / 32768) & 1;
		}
		_m1[23] = 0;
		int m1 = 0;
		for (int i=0; i<23; i++)
			m1 = (m1 << 1) + _m1[i];
	
		u1 = gly23127Dec(c1 ^ m1);
		u2 = c2;
		u3 = c3;
		// int b[9];
		b[0] = ((u0 >> 5) & 0x78) + ((u3 >> 9) & 0x7);
		b[1] = ((u0 >> 3) & 0x1e) + ((u3 >> 13) & 0x1);
		b[2] = ((u0 << 1) & 0x1e) + ((u3 >> 12) & 0x1);
		b[3] = ((u1 >> 3) & 0x1fe) + ((u3 >> 8) & 0x1);
		b[4] = ((u1 << 3) & 0x78) + ((u3 >> 5) & 0x7);
		b[5] = ((u2 >> 6) & 0x1e) + ((u3 >> 4) & 0x1);
		b[6] = ((u2 >> 3) & 0x0e) + ((u3 >> 3) & 0x1);
		b[7] = ( u2       & 0x0e) + ((u3 >> 2) & 0x1);
		b[8] = ((u2 << 2) & 0x04) + ( u3       & 0x3);
	}

	void p25p2_vf::extract_vcw(const uint8_t _vf[], int& _c0, int& _c1, int& _c2, int& _c3){
		uint8_t vf[72];
		uint8_t c0[24];
		uint8_t c1[23];
		uint8_t c2[11];
		uint8_t c3[14];

		for (int i=0; i<sizeof(vf)/2; i++) {
			vf[i*2]   = (_vf[i] >> 1) & 1;
			vf[i*2+1] = _vf[i] & 1;
		}

		c0[23] = vf[0];
		c0[5] = vf[1];
		c1[10] = vf[2];
		c2[3] = vf[3];
		c0[22] = vf[4];
		c0[4] = vf[5];
		c1[9] = vf[6];
		c2[2] = vf[7];
		c0[21] = vf[8];
		c0[3] = vf[9];
		c1[8] = vf[10];
		c2[1] = vf[11];
		c0[20] = vf[12];
		c0[2] = vf[13];
		c1[7] = vf[14];
		c2[0] = vf[15];
		c0[19] = vf[16];
		c0[1] = vf[17];
		c1[6] = vf[18];
		c3[13] = vf[19];
		c0[18] = vf[20];
		c0[0] = vf[21];
		c1[5] = vf[22];
		c3[12] = vf[23];
		c0[17] = vf[24];
		c1[22] = vf[25];
		c1[4] = vf[26];
		c3[11] = vf[27];
		c0[16] = vf[28];
		c1[21] = vf[29];
		c1[3] = vf[30];
		c3[10] = vf[31];
		c0[15] = vf[32];
		c1[20] = vf[33];
		c1[2] = vf[34];
		c3[9] = vf[35];
		c0[14] = vf[36];
		c1[19] = vf[37];
		c1[1] = vf[38];
		c3[8] = vf[39];
		c0[13] = vf[40];
		c1[18] = vf[41];
		c1[0] = vf[42];
		c3[7] = vf[43];
		c0[12] = vf[44];
		c1[17] = vf[45];
		c2[10] = vf[46];
		c3[6] = vf[47];
		c0[11] = vf[48];
		c1[16] = vf[49];
		c2[9] = vf[50];
		c3[5] = vf[51];
		c0[10] = vf[52];
		c1[15] = vf[53];
		c2[8] = vf[54];
		c3[4] = vf[55];
		c0[9] = vf[56];
		c1[14] = vf[57];
		c2[7] = vf[58];
		c3[3] = vf[59];
		c0[8] = vf[60];
		c1[13] = vf[61];
		c2[6] = vf[62];
		c3[2] = vf[63];
		c0[7] = vf[64];
		c1[12] = vf[65];
		c2[5] = vf[66];
		c3[1] = vf[67];
		c0[6] = vf[68];
		c1[11] = vf[69];
		c2[4] = vf[70];
		c3[0] = vf[71];
		load_i(c0, _c0, 24);
		load_i(c1, _c1, 23);
		load_i(c2, _c2, 11);
		load_i(c3, _c3, 14);
	}
