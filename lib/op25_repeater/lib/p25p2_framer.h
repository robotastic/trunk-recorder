/* -*- c++ -*- */

/*
 * construct P25 P2 TDMA frames out of raw dibits
 * Copyright 2014, Max H. Parke KA1RBI
 *
 * usage: after constructing, call rx_sym once per received dibit.
 * frame available when true is returned
 */

#ifndef INCLUDED_P25P2_FRAMER_H
#define INCLUDED_P25P2_FRAMER_H

#include "frame_sync_magics.h"

static const unsigned int P25P2_BURST_SIZE=360; /* in bits */

class p25p2_framer;

class p25p2_framer
{
private:
	typedef std::vector<bool> bit_vector;
  // internal instance variables and state
	uint32_t d_next_bit;
	uint32_t d_in_sync;
	uint64_t d_fs;
	uint64_t nid_accum;

public:
	p25p2_framer();  	// constructor
	~p25p2_framer ();	// destructor
	bool rx_sym(uint8_t dibit) ;
    uint64_t get_fs() { return d_fs; }

	uint32_t symbols_received;

	bit_vector d_frame_body;	// all bits in frame
};

#endif /* INCLUDED_P25P2_FRAMER_H */
