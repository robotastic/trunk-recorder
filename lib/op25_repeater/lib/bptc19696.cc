/*
 *	 Copyright (C) 2012 by Ian Wraith
 *   Copyright (C) 2015 by Jonathan Naylor G4KLX
 *
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
bool CBPTC19696::decode(const unsigned char* in, unsigned char* out)
{
	assert(in != NULL);
	assert(out != NULL);

	//  Get the raw binary
	decodeExtractBinary(in);

	// Deinterleave
	decodeDeInterleave();

	// Error check
	bool rc = decodeErrorCheck();

	// Extract Data
	decodeExtractData(out);

	return rc;
}

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
bool CBPTC19696::decodeErrorCheck()
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

	return !fixing;
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


