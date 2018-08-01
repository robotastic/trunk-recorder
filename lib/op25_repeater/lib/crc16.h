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
#endif /* INCLUDED_CRC16_H */
