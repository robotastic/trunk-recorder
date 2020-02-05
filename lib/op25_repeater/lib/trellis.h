/*
 *	Copyright (C) 2016 by Jonathan Naylor, G4KLX
 *
 *	Modifications of original code to work with OP25
 *	Copyright (C) 2019 by Graham J. Norbury
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

#ifndef	DMRTrellis_H
#define	DMRTrellis_H

class CDMRTrellis {
public:
	CDMRTrellis();
	~CDMRTrellis();

	bool decode(const unsigned char* data, unsigned char* payload);

private:
	void deinterleave(const unsigned char* in, signed char* dibits) const;
	void dibitsToPoints(const signed char* dibits, unsigned char* points) const;
	void pointsToDibits(const unsigned char* points, signed char* dibits) const;
	void bitsToTribits(const unsigned char* payload, unsigned char* tribits) const;
	void tribitsToBits(const unsigned char* tribits, unsigned char* payload) const;
	bool fixCode(unsigned char* points, unsigned int failPos, unsigned char* payload) const;
	unsigned int checkCode(const unsigned char* points, unsigned char* tribits) const;
};

#endif
