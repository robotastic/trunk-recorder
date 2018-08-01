/* -*- c++ -*- */

/*
 * construct P25 frames out of raw dibits
 * Copyright 2010, KA1RBI
 *
 * usage: after constructing, call rx_sym once per received dibit.
 * frame fields are available for inspection when true is returned
 */

#ifndef INCLUDED_P25_FRAMER_H
#define INCLUDED_P25_FRAMER_H

class p25_framer;

class p25_framer
{
private:
	typedef std::vector<bool> bit_vector;
  // internal functions
	bool nid_codeword(uint64_t acc);
  // internal instance variables and state
	uint8_t reverse_p;
	int nid_syms;
	uint32_t next_bit;
	uint64_t nid_accum;

	uint32_t frame_size_limit;
	int d_debug;

public:
	p25_framer(int debug = 0);
	~p25_framer ();	// destructor
	bool rx_sym(uint8_t dibit) ;

	uint32_t symbols_received;

	// info from received frame
	uint64_t nid_word;	// received NID word
	uint32_t nac;		// extracted NAC
	uint32_t duid;		// extracted DUID
	uint8_t  parity;	// extracted DUID parity
	bit_vector frame_body;	// all bits in frame
	uint32_t frame_size;		// number of bits in frame_body
	uint32_t bch_fails;
	uint32_t bch_errors;		// number of errors detected in bch
};

#endif /* INCLUDED_P25_FRAMER_H */
