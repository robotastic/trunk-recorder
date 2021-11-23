/* 
 * Copyright 2019 Graham J Norbury, gnorbury@bondcar.com
 * op25 msg_types used to communicate between protocols and trunking
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

#ifndef INCLUDED_OP25_MSG_TYPES_H
#define INCLUDED_OP25_MSG_TYPES_H

// Protocols
static const int16_t PROTOCOL_P25       =  0;
static const int16_t PROTOCOL_DMR       =  1;
static const int16_t PROTOCOL_SMARTNET  =  2;

// Msg Types (protocol specific)

// P25 Messages
//   message types < 0 are special
//   message types >= 0 are based on DUID (0 thru 15)
static const int16_t M_P25_TIMEOUT      = -1;
static const int16_t M_P25_UI_REQ       = -2;
static const int16_t M_P25_JSON_DATA    = -3;
static const int16_t M_P25_SYNC_ESTAB   = -4;
static const int16_t M_P25_RESERVED     = -5; // used in Osmocom' NXDN trunking
static const int16_t M_P25_DUID_HDU     =  0;
static const int16_t M_P25_DUID_TDU     =  3;
static const int16_t M_P25_DUID_LDU1    =  5;
static const int16_t M_P25_DUID_TSBK    =  7;
static const int16_t M_P25_DUID_LDU2    = 10;
static const int16_t M_P25_DUID_PDU     = 12;
static const int16_t M_P25_DUID_TDULC   = 15;
static const int16_t M_P25_MAC_PTT      = 16;
static const int16_t M_P25_MAC_END_PTT  = 17;
static const int16_t M_P25_MAC_PDU      = 18;
static const int16_t M_P25_FDMA_LCW     = 19;

// DMR Messages
static const int16_t M_DMR_TIMEOUT      = -1;

static const int16_t M_DMR_CACH_SLC     =  0;
static const int16_t M_DMR_CACH_CSBK    =  1;
static const int16_t M_DMR_SLOT_PI      =  2;
static const int16_t M_DMR_SLOT_VLC     =  3;
static const int16_t M_DMR_SLOT_TLC     =  4;
static const int16_t M_DMR_SLOT_CSBK    =  5;
static const int16_t M_DMR_SLOT_MBC     =  6;
static const int16_t M_DMR_SLOT_ELC     =  7;
static const int16_t M_DMR_SLOT_ERC     =  8;
static const int16_t M_DMR_SLOT_ESB     =  9;

// SMARTNET Messages
static const int16_t M_SMARTNET_TIMEOUT = -1;
static const int16_t M_SMARTNET_OSW     =  0;
static const int16_t M_SMARTNET_END_PTT = 15;

inline long get_msg_type(const int16_t proto, const int16_t msg) { return ((uint32_t)proto << 16) | ((uint32_t)msg & 0xffff); }

#endif /* INCLUDED_OP25_MSG_TYPES_H */

