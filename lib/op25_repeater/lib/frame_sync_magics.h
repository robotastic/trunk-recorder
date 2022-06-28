//
// Definitions for Frame Sync Magics collected from various headers
// and brought together in one place for completeness 
// (C) 2019, Max Parke, Graham Norbury et-al
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

#ifndef INCLUDED_FRAME_SYNC_MAGICS
#define INCLUDED_FRAME_SYNC_MAGICS

// DMR
static const uint64_t DMR_BS_VOICE_SYNC_MAGIC  = 0x755fd7df75f7LL;
static const uint64_t DMR_BS_DATA_SYNC_MAGIC   = 0xdff57d75df5dLL;
static const uint64_t DMR_MS_VOICE_SYNC_MAGIC  = 0x7f7d5dd57dfdLL;
static const uint64_t DMR_MS_DATA_SYNC_MAGIC   = 0xd5d7f77fd757LL;
static const uint64_t DMR_MS_RC_SYNC_MAGIC     = 0x77d55f7dfd77LL;
static const uint64_t DMR_T1_VOICE_SYNC_MAGIC  = 0x5d577f7757ffLL;
static const uint64_t DMR_T1_DATA_SYNC_MAGIC   = 0xf7fdd5ddfd55LL;
static const uint64_t DMR_T2_VOICE_SYNC_MAGIC  = 0x7dffd5f55d5fLL;
static const uint64_t DMR_T2_DATA_SYNC_MAGIC   = 0xd7557f5ff7f5LL;
static const uint64_t DMR_RESERVED_SYNC_MAGIC  = 0xdd7ff5d757ddLL;

// P25
static const uint64_t P25_FRAME_SYNC_MAGIC     = 0x5575F5FF77FFLL;
static const uint64_t P25_FRAME_SYNC_REV_P     = 0x5575F5FF77FFLL ^ 0xAAAAAAAAAAAALL;
static const uint64_t P25_FRAME_SYNC_MASK      = 0xFFFFFFFFFFFFLL;

static const uint64_t P25_FRAME_SYNC_N1200     = 0x001050551155LL;
static const uint64_t P25_FRAME_SYNC_X2400     = 0xAA8A0A008800LL;
static const uint64_t P25_FRAME_SYNC_P1200     = 0xFFEFAFAAEEAALL;

static const uint64_t P25P2_FRAME_SYNC_MAGIC   = 0x575D57F7FFLL;   // 40 bits
static const uint64_t P25P2_FRAME_SYNC_REV_P   = 0x575D57F7FFLL ^ 0xAAAAAAAAAALL;
static const uint64_t P25P2_FRAME_SYNC_MASK    = 0xFFFFFFFFFFLL;

static const uint64_t P25P2_FRAME_SYNC_N1200   = 0x0104015155LL;
static const uint64_t P25P2_FRAME_SYNC_X2400   = 0xa8a2a8d800LL;
static const uint64_t P25P2_FRAME_SYNC_P1200   = 0xfefbfeaeaaLL;

// DSTAR
static const uint64_t DSTAR_FRAME_SYNC_MAGIC   = 0x444445101440LL; 

// YSF
static const uint64_t YSF_FRAME_SYNC_MAGIC     = 0xd471c9634dLL;   // 40 bits

// MOTO SMARTNET
static const uint64_t SMARTNET_SYNC_MAGIC      = 0xACLL; // 8 bits
static const uint64_t SUBCHANNEL_SYNC_MAGIC    = 0x08LL; // 5 bits


#endif /* INCLUDED_FRAME_SYNC_MAGICS */
