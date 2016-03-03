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

static const unsigned int P25P2_BURST_SIZE=360; /* in bits */
static const uint64_t P25P2_FRAME_SYNC_MAGIC = 0x575D57F7FFLL;
static const uint64_t P25P2_FRAME_SYNC_REV_P = 0x575D57F7FFLL ^ 0xAAAAAAAAAALL;
static const uint64_t P25P2_FRAME_SYNC_MASK  = 0xFFFFFFFFFFLL;

class p25p2_framer;

class p25p2_framer
{
private:
	typedef std::vector<bool> bit_vector;
  // internal instance variables and state
	uint8_t d_reverse_p;
	uint32_t d_next_bit;
	uint32_t d_in_sync;
	uint64_t nid_accum;

public:
	p25p2_framer();  	// constructor
	~p25p2_framer ();	// destructor
	bool rx_sym(uint8_t dibit) ;

	uint32_t symbols_received;

	bit_vector d_frame_body;	// all bits in frame
};

#endif /* INCLUDED_P25P2_FRAMER_H */
