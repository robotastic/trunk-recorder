/*
 *	 Copyright (C) 2012 by Ian Wraith
 *   Copyright (C) 2015 by Jonathan Naylor G4KLX

 *   Modifications of original code to work with OP25
 *   Copyright (C) 2019 by Graham J. Norbury
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "bptc19696.h"

#include "hamming.h"

#include <cstdio>
#include <cassert>
#include <cstring>

CBPTC19696::CBPTC19696() :
m_rawData(NULL),
m_deInterData(NULL)
{
        m_rawData     = new bool[196];
        m_deInterData = new bool[196];
}

CBPTC19696::~CBPTC19696()
{
        delete[] m_rawData;
        delete[] m_deInterData;
}

// The main decode function
void CBPTC19696::decode(const unsigned char* in, unsigned char* out)
{
	assert(in != NULL);
	assert(out != NULL);

	//  Get the raw binary
	decodeExtractBinary(in);

	// Deinterleave
	decodeDeInterleave();

	// Error check
	decodeErrorCheck();

	// Extract Data
	decodeExtractData(out);
}

#if 0
// The main encode function
void CBPTC19696::encode(const unsigned char* in, unsigned char* out)
{
	assert(in != NULL);
	assert(out != NULL);

	// Extract Data
	encodeExtractData(in);

	// Error check
	encodeErrorCheck();

	// Deinterleave
	encodeInterleave();

	//  Get the raw binary
	encodeExtractBinary(out);
}
#endif

// Extract two blocks of payload from received binary coded frame array
void CBPTC19696::decodeExtractBinary(const unsigned char* in)
{
	// First block
	for (int i = 0; i < 98; i++)
		m_rawData[i] = in[i];

	// Second block
	for (int i = 98; i < 196; i++)
		m_rawData[i] = in[i + 68];
}

// Deinterleave the raw data
void CBPTC19696::decodeDeInterleave()
{
	for (unsigned int i = 0U; i < 196U; i++)
		m_deInterData[i] = false;

	// The first bit is R(3) which is not used so can be ignored
	for (unsigned int a = 0U; a < 196U; a++)	{
		// Calculate the interleave sequence
		unsigned int interleaveSequence = (a * 181U) % 196U;
		// Shuffle the data
		m_deInterData[a] = m_rawData[interleaveSequence];
	}
}
	
// Check each row with a Hamming (15,11,3) code and each column with a Hamming (13,9,3) code
void CBPTC19696::decodeErrorCheck()
{
	bool fixing;
	unsigned int count = 0U;
	do {
		fixing = false;

		// Run through each of the 15 columns
		bool col[13U];
		for (unsigned int c = 0U; c < 15U; c++) {
			unsigned int pos = c + 1U;
			for (unsigned int a = 0U; a < 13U; a++) {
				col[a] = m_deInterData[pos];
				pos = pos + 15U;
			}

			if (CHamming::decode1393(col)) {
				unsigned int pos = c + 1U;
				for (unsigned int a = 0U; a < 13U; a++) {
					m_deInterData[pos] = col[a];
					pos = pos + 15U;
				}

				fixing = true;
			}
		}
		
		// Run through each of the 9 rows containing data
		for (unsigned int r = 0U; r < 9U; r++) {
			unsigned int pos = (r * 15U) + 1U;
			if (CHamming::decode15113_2(m_deInterData + pos))
				fixing = true;
		}

		count++;
	} while (fixing && count < 5U);
}

// Extract the 96 bits of payload
void CBPTC19696::decodeExtractData(unsigned char* data) const
{
	unsigned int pos = 0U;
	for (unsigned int a = 4U; a <= 11U; a++, pos++)
		data[pos] = m_deInterData[a];

	for (unsigned int a = 16U; a <= 26U; a++, pos++)
		data[pos] = m_deInterData[a];

	for (unsigned int a = 31U; a <= 41U; a++, pos++)
		data[pos] = m_deInterData[a];

	for (unsigned int a = 46U; a <= 56U; a++, pos++)
		data[pos] = m_deInterData[a];

	for (unsigned int a = 61U; a <= 71U; a++, pos++)
		data[pos] = m_deInterData[a];

	for (unsigned int a = 76U; a <= 86U; a++, pos++)
		data[pos] = m_deInterData[a];

	for (unsigned int a = 91U; a <= 101U; a++, pos++)
		data[pos] = m_deInterData[a];

	for (unsigned int a = 106U; a <= 116U; a++, pos++)
		data[pos] = m_deInterData[a];

	for (unsigned int a = 121U; a <= 131U; a++, pos++)
		data[pos] = m_deInterData[a];
}

#if 0
// Encode routines would need to be modified to make compatible with op25 

// Extract the 96 bits of payload
void CBPTC19696::encodeExtractData(const unsigned char* in) const
{
	bool bData[96U];
	CUtils::byteToBitsBE(in[0U],  bData + 0U);
	CUtils::byteToBitsBE(in[1U],  bData + 8U);
	CUtils::byteToBitsBE(in[2U],  bData + 16U);
	CUtils::byteToBitsBE(in[3U],  bData + 24U);
	CUtils::byteToBitsBE(in[4U],  bData + 32U);
	CUtils::byteToBitsBE(in[5U],  bData + 40U);
	CUtils::byteToBitsBE(in[6U],  bData + 48U);
	CUtils::byteToBitsBE(in[7U],  bData + 56U);
	CUtils::byteToBitsBE(in[8U],  bData + 64U);
	CUtils::byteToBitsBE(in[9U],  bData + 72U);
	CUtils::byteToBitsBE(in[10U], bData + 80U);
	CUtils::byteToBitsBE(in[11U], bData + 88U);

	for (unsigned int i = 0U; i < 196U; i++)
		m_deInterData[i] = false;

	unsigned int pos = 0U;
	for (unsigned int a = 4U; a <= 11U; a++, pos++)
		m_deInterData[a] = bData[pos];

	for (unsigned int a = 16U; a <= 26U; a++, pos++)
		m_deInterData[a] = bData[pos];

	for (unsigned int a = 31U; a <= 41U; a++, pos++)
		m_deInterData[a] = bData[pos];

	for (unsigned int a = 46U; a <= 56U; a++, pos++)
		m_deInterData[a] = bData[pos];

	for (unsigned int a = 61U; a <= 71U; a++, pos++)
		m_deInterData[a] = bData[pos];

	for (unsigned int a = 76U; a <= 86U; a++, pos++)
		m_deInterData[a] = bData[pos];

	for (unsigned int a = 91U; a <= 101U; a++, pos++)
		m_deInterData[a] = bData[pos];

	for (unsigned int a = 106U; a <= 116U; a++, pos++)
		m_deInterData[a] = bData[pos];

	for (unsigned int a = 121U; a <= 131U; a++, pos++)
		m_deInterData[a] = bData[pos];
}

// Check each row with a Hamming (15,11,3) code and each column with a Hamming (13,9,3) code
void CBPTC19696::encodeErrorCheck()
{
	// Run through each of the 9 rows containing data
	for (unsigned int r = 0U; r < 9U; r++) {
		unsigned int pos = (r * 15U) + 1U;
		CHamming::encode15113_2(m_deInterData + pos);
	}
	
	// Run through each of the 15 columns
	bool col[13U];
	for (unsigned int c = 0U; c < 15U; c++) {
		unsigned int pos = c + 1U;
		for (unsigned int a = 0U; a < 13U; a++) {
			col[a] = m_deInterData[pos];
			pos = pos + 15U;
		}

		CHamming::encode1393(col);

		pos = c + 1U;
		for (unsigned int a = 0U; a < 13U; a++) {
			m_deInterData[pos] = col[a];
			pos = pos + 15U;
		}
	}
}

// Interleave the raw data
void CBPTC19696::encodeInterleave()
{
	for (unsigned int i = 0U; i < 196U; i++)
		m_rawData[i] = false;

	// The first bit is R(3) which is not used so can be ignored
	for (unsigned int a = 0U; a < 196U; a++)	{
		// Calculate the interleave sequence
		unsigned int interleaveSequence = (a * 181U) % 196U;
		// Unshuffle the data
		m_rawData[interleaveSequence] = m_deInterData[a];
	}
}

void CBPTC19696::encodeExtractBinary(unsigned char* data)
{
	// First block
	CUtils::bitsToByteBE(m_rawData + 0U,  data[0U]);
	CUtils::bitsToByteBE(m_rawData + 8U,  data[1U]);
	CUtils::bitsToByteBE(m_rawData + 16U, data[2U]);
	CUtils::bitsToByteBE(m_rawData + 24U, data[3U]);
	CUtils::bitsToByteBE(m_rawData + 32U, data[4U]);
	CUtils::bitsToByteBE(m_rawData + 40U, data[5U]);
	CUtils::bitsToByteBE(m_rawData + 48U, data[6U]);
	CUtils::bitsToByteBE(m_rawData + 56U, data[7U]);
	CUtils::bitsToByteBE(m_rawData + 64U, data[8U]);
	CUtils::bitsToByteBE(m_rawData + 72U, data[9U]);
	CUtils::bitsToByteBE(m_rawData + 80U, data[10U]);
	CUtils::bitsToByteBE(m_rawData + 88U, data[11U]);

	// Handle the two bits
	unsigned char byte;
	CUtils::bitsToByteBE(m_rawData + 96U, byte);
	data[12U] = (data[12U] & 0x3FU) | ((byte >> 0) & 0xC0U);
	data[20U] = (data[20U] & 0xFCU) | ((byte >> 4) & 0x03U);

	// Second block
	CUtils::bitsToByteBE(m_rawData + 100U,  data[21U]);
	CUtils::bitsToByteBE(m_rawData + 108U,  data[22U]);
	CUtils::bitsToByteBE(m_rawData + 116U,  data[23U]);
	CUtils::bitsToByteBE(m_rawData + 124U,  data[24U]);
	CUtils::bitsToByteBE(m_rawData + 132U,  data[25U]);
	CUtils::bitsToByteBE(m_rawData + 140U,  data[26U]);
	CUtils::bitsToByteBE(m_rawData + 148U,  data[27U]);
	CUtils::bitsToByteBE(m_rawData + 156U,  data[28U]);
	CUtils::bitsToByteBE(m_rawData + 164U,  data[29U]);
	CUtils::bitsToByteBE(m_rawData + 172U,  data[30U]);
	CUtils::bitsToByteBE(m_rawData + 180U,  data[31U]);
	CUtils::bitsToByteBE(m_rawData + 188U,  data[32U]);
}
#endif

