/*
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

#if !defined(BPTC19696_H)
#define	BPTC19696_H

class CBPTC19696
{
public:
	CBPTC19696();
	~CBPTC19696();

	void decode(const unsigned char* in, unsigned char* out);

private:
	bool* m_rawData;
	bool* m_deInterData;

	void decodeExtractBinary(const unsigned char* in);
	void decodeErrorCheck();
	void decodeDeInterleave();
	void decodeExtractData(unsigned char* data) const;

#if 0
public:
	void encode(const unsigned char* in, unsigned char* out);

private:
	void encodeExtractData(const unsigned char* in) const;
	void encodeInterleave();
	void encodeErrorCheck();
	void encodeExtractBinary(unsigned char* data);
#endif
};

#endif
