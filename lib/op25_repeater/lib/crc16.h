/* 
 * This file is part of OP25
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef INCLUDED_CRC16_H
#define INCLUDED_CRC16_H

#include <string.h>

static uint8_t crc7(const uint8_t bits[], unsigned int len) {
	uint8_t crc=0;
	static const unsigned int K = 7;
	static const uint8_t poly[K+1] = {1,0,1,0,0,1,1,1}; // crc7 poly
	uint8_t buf[256];
	if (len+K > sizeof(buf)) {
		fprintf (stderr, "crc8: buffer length %u exceeds maximum %lu\n", len+K, sizeof(buf));
		return 0;
	}
	memset (buf, 0, sizeof(buf));
	for (unsigned int i=0; i<len; i++){
		buf[i] = bits[i];
	}
	for (unsigned int i=0; i<len; i++)
		if (buf[i])
			for (unsigned int j=0; j<K+1; j++)
				buf[i+j] ^= poly[j];
	for (unsigned int i=0; i<K; i++){
		crc = (crc << 1) + buf[len + i];
	}
	return crc;
}


static uint8_t crc8(const uint8_t bits[], unsigned int len) {
	uint8_t crc=0;
	static const unsigned int K = 8;
	static const uint8_t poly[K+1] = {1,0,0,0,0,0,1,1,1}; // crc8 poly
	uint8_t buf[256];
	if (len+K > sizeof(buf)) {
		fprintf (stderr, "crc8: buffer length %u exceeds maximum %lu\n", len+K, sizeof(buf));
		return 0;
	}
	memset (buf, 0, sizeof(buf));
	for (unsigned int i=0; i<len; i++){
		buf[i] = bits[i];
	}
	for (unsigned int i=0; i<len; i++)
		if (buf[i])
			for (unsigned int j=0; j<K+1; j++)
				buf[i+j] ^= poly[j];
	for (unsigned int i=0; i<K; i++){
		crc = (crc << 1) + buf[len + i];
	}
	return crc;
}

static bool crc8_ok(const uint8_t bits[], unsigned int len) {
	uint16_t crc = 0;
	for (unsigned int i=0; i < 8; i++) {
		crc = (crc << 1) + bits[len+i];
	}
	return (crc == crc8(bits,len));
}

static inline uint16_t crc16(const uint8_t buf[], int len) {
        uint32_t poly = (1<<12) + (1<<5) + (1<<0);
        uint32_t crc = 0;
        for(int i=0; i<len; i++) {
                uint8_t bit = buf[i] & 1;
                crc = ((crc << 1) | bit) & 0x1ffff;
                if (crc & 0x10000)
                        crc = (crc & 0xffff) ^ poly;
	}
        crc = crc ^ 0xffff;
        return crc & 0xffff;
}

static inline uint32_t crc32(const uint8_t buf[], size_t len)
{
    uint32_t poly = 0xedb88320;
    uint32_t crc  = 0;
    unsigned int k;

    crc = ~crc;
    while (len--) {
        crc ^= *buf++;
        for (k = 0; k < 8; k++)
            crc = crc & 1 ? (crc >> 1) ^ poly : crc >> 1;
    }
    return ~crc;
}
#endif /* INCLUDED_CRC16_H */
