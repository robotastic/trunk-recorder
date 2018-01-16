/*
 *	Copyright (C) 2010,2011,2015 by Jonathan Naylor, G4KLX
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; version 2 of the License.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 */

/* from OpenDV/DStarRepeater */

#ifndef INCLUDED_DSTAR_HEADER_H
#define INCLUDED_DSTAR_HEADER_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "CCITTChecksumReverse.h"
#include "bit_utils.h"

static inline uint8_t rev_byte(const uint8_t b) {
	uint8_t rc = 0;
	for (int i=0; i<8; i++) {
		rc |= ((b >> i) & 1) << (7-i);
	}
	return(rc);
}

static inline void make_dstar_header(
	uint8_t header_data_buf[480],		// result (bits)
	const uint8_t flag1, const uint8_t flag2, const uint8_t flag3,
	const char RptCall2[],
	const char RptCall1[],
	const char YourCall[],
	const char MyCall1[],
	const char MyCall2[])
{
	uint8_t m_headerData[60];
	static const unsigned int  SLOW_DATA_BLOCK_SIZE = 6U;
	static const unsigned int  SLOW_DATA_FULL_BLOCK_SIZE = SLOW_DATA_BLOCK_SIZE * 10U;
	static const unsigned char SLOW_DATA_TYPE_HEADER  = 0x50U;

	::memset(m_headerData, 'f', SLOW_DATA_FULL_BLOCK_SIZE);

	m_headerData[0U]  = SLOW_DATA_TYPE_HEADER | 5U;
	m_headerData[1U]  = flag1;
	m_headerData[2U]  = flag2;
	m_headerData[3U]  = flag3;
	m_headerData[4U]  = RptCall2[0];
	m_headerData[5U]  = RptCall2[1];

	m_headerData[6U]  = SLOW_DATA_TYPE_HEADER | 5U;
	m_headerData[7U]  = RptCall2[2];
	m_headerData[8U]  = RptCall2[3];
	m_headerData[9U]  = RptCall2[4];
	m_headerData[10U] = RptCall2[5];
	m_headerData[11U] = RptCall2[6];

	m_headerData[12U] = SLOW_DATA_TYPE_HEADER | 5U;
	m_headerData[13U] = RptCall2[7];
	m_headerData[14U] = RptCall1[0];
	m_headerData[15U] = RptCall1[1];
	m_headerData[16U] = RptCall1[2];
	m_headerData[17U] = RptCall1[3];

	m_headerData[18U] = SLOW_DATA_TYPE_HEADER | 5U;
	m_headerData[19U] = RptCall1[4];
	m_headerData[20U] = RptCall1[5];
	m_headerData[21U] = RptCall1[6];
	m_headerData[22U] = RptCall1[7];
	m_headerData[23U] = YourCall[0];

	m_headerData[24U] = SLOW_DATA_TYPE_HEADER | 5U;
	m_headerData[25U] = YourCall[1];
	m_headerData[26U] = YourCall[2];
	m_headerData[27U] = YourCall[3];
	m_headerData[28U] = YourCall[4];
	m_headerData[29U] = YourCall[5];

	m_headerData[30U] = SLOW_DATA_TYPE_HEADER | 5U;
	m_headerData[31U] = YourCall[6];
	m_headerData[32U] = YourCall[7];
	m_headerData[33U] = MyCall1[0];
	m_headerData[34U] = MyCall1[1];
	m_headerData[35U] = MyCall1[2];

	m_headerData[36U] = SLOW_DATA_TYPE_HEADER | 5U;
	m_headerData[37U] = MyCall1[3];
	m_headerData[38U] = MyCall1[4];
	m_headerData[39U] = MyCall1[5];
	m_headerData[40U] = MyCall1[6];
	m_headerData[41U] = MyCall1[7];

	m_headerData[42U] = SLOW_DATA_TYPE_HEADER | 5U;
	m_headerData[43U] = MyCall2[0];
	m_headerData[44U] = MyCall2[1];
	m_headerData[45U] = MyCall2[2];
	m_headerData[46U] = MyCall2[3];

	CCCITTChecksumReverse cksum;
	cksum.update(m_headerData + 1U, 5U);
	cksum.update(m_headerData + 7U, 5U);
	cksum.update(m_headerData + 13U, 5U);
	cksum.update(m_headerData + 19U, 5U);
	cksum.update(m_headerData + 25U, 5U);
	cksum.update(m_headerData + 31U, 5U);
	cksum.update(m_headerData + 37U, 5U);
	cksum.update(m_headerData + 43U, 4U);

	unsigned char checkSum[2U];
	cksum.result(checkSum);

	m_headerData[47U] = checkSum[0];

	m_headerData[48U] = SLOW_DATA_TYPE_HEADER | 1U;
	m_headerData[49U] = checkSum[1];

	for (int i=0; i<SLOW_DATA_FULL_BLOCK_SIZE; i+=3) {
		store_i(rev_byte(m_headerData[i] ^ 0x70), header_data_buf+i*8, 8);
		store_i(rev_byte(m_headerData[i+1] ^ 0x4f), header_data_buf+((i+1)*8), 8);
		store_i(rev_byte(m_headerData[i+2] ^ 0x93), header_data_buf+((i+2)*8), 8);
	}
}

#endif /* INCLUDED_DSTAR_HEADER_H */
