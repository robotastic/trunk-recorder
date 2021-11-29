/*
 * (C) 2019, Graham J Norbury
 *           gnorbury@bondcar.com
 *
 * Stand-alone utility to calculate bit error ratio of a binary dibit capture file
 * compared to an APCO P25 pattern file
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <fstream>
#include <vector>

#include "frame_sync_magics.h"

typedef std::vector<uint8_t> byte_vector;

static const int          FRAME_SIZE		   = 1728;
static const int          SYNC_SIZE		   = 48;
static const int          SYNC_THRESHOLD	   = 4;

bool test_sync(uint64_t cw, int &errs) {
	int popcnt = 0;
       	popcnt = __builtin_popcountll(cw ^ P25_FRAME_SYNC_MAGIC);
	if (popcnt <= SYNC_THRESHOLD) {
		errs = popcnt;
               	return true;
	}

	return false;
}

int main (int argc, char* argv[]) {
	char dibit;
	uint64_t cw = 0;
	int s_errs = 0;
	byte_vector pattern;
	byte_vector rx_syms;

	if (argc != 3) {
		fprintf(stderr, "Usage: BER <pattern> <filename>\n");
		return 1;
	}

	// read in pattern file
	pattern.clear();
	std::fstream p_file(argv[1], std::ios::in | std::ios::binary);
	if ( (p_file.rdstate() & std::ifstream::failbit ) != 0 ) {
		printf("Error opening pattern file: %s\n", argv[1]);
		return 1;
	}
	while (!p_file.eof()) {
		p_file.read((&dibit), 1);
		pattern.push_back((dibit >> 1) & 0x1);
		pattern.push_back(dibit & 0x1);
	}

	// read in received symbols file
	rx_syms.clear();
	std::fstream r_file(argv[2], std::ios::in | std::ios::binary);
	if ( (r_file.rdstate() & std::ifstream::failbit ) != 0 ) {
		printf("Error opening symbol file: %s\n", argv[2]);
		return 1;
	}
	while (!r_file.eof()) {
		r_file.read((&dibit), 1);
		rx_syms.push_back((dibit >> 1) & 0x1);
		rx_syms.push_back(dibit & 0x1);
	}

	// find starting sync in pattern file
	size_t p_pos = 0;
	size_t p_start = 0;
	int cw_bits = 0;
	while (p_pos < pattern.size()) {
		cw_bits++;
		cw = ((cw << 1) + pattern[p_pos]) & 0xffffffffffff;
		if ((cw_bits >= SYNC_SIZE) && test_sync(cw, s_errs)) {
			p_start = p_pos - SYNC_SIZE + 1;
			printf("Pattern sync at %lu, errs %d\n", p_start, s_errs);
			break;
		}
		p_pos++;
	}
	if (p_pos >= pattern.size()) {
		printf("Error: no sync found in pattern file\n");
		return 1;
	}

	// pattern ichunk must be an even multiple of frame size
	size_t p_len = pattern.size() - p_start;
	if (p_len % FRAME_SIZE)
		p_len -= (p_len % FRAME_SIZE);
	printf("Pattern length: %lu bits\n", p_len);
	printf("Symbols file: %lu bits\n", rx_syms.size());

	size_t r_pos = 0;
	size_t r_start = 0;
	size_t r_len = rx_syms.size();
	bool calculating = true;

	// stats
	size_t total_bit_errs = 0;
	size_t total_bit_count = 0;
	double total_ber = 0;
	size_t max_bit_errs = 0;
	double max_ber = 0;

	// run through received symbols chunk by chunk
	do {
		size_t chunk_bit_errs = 0;
		size_t chunk_bit_count = 0;
		double chunk_ber = 0;
		
		// find next sync in symbols file
		cw = 0;
		cw_bits = 0;
		while (r_pos < r_len) {
			cw_bits++;
			cw = ((cw << 1) + rx_syms[r_pos]) & 0xffffffffffff;
			if ((cw_bits >= SYNC_SIZE) && test_sync(cw, s_errs)) {
				r_start = r_pos - SYNC_SIZE + 1;
				break;
			}
			r_pos++;
		}
		if ((r_pos >= r_len) || (p_len > (r_len - r_start))) {
			calculating = false;
			break;
		}

		r_pos = r_start;
		p_pos = p_start;
		while(p_pos < p_len) {
			if (rx_syms[r_pos] ^ pattern[p_pos])
				chunk_bit_errs++;
			r_pos++;
			p_pos++;
			chunk_bit_count++;
		}
		chunk_ber = 100 * (double)chunk_bit_errs / (double)chunk_bit_count;
		printf("Pattern match (%lu) bit errs=%lu, BER=%lf%%\n", r_start, chunk_bit_errs, chunk_ber);
		// Check for excessive BER and attempt to resync
		// usually this means pattern and capture file got out of step with each other
		if (chunk_ber > 5) {
			printf("High BER >5.0 detected. Discarding current frame and attempting resync.\n");
			r_pos = r_start + 2;
			continue;
		}

		// Update totals
		total_bit_count += chunk_bit_count;
		total_bit_errs += chunk_bit_errs;
		if (chunk_ber > max_ber) {
			max_bit_errs = chunk_bit_errs;
			max_ber = chunk_ber;
		}

	} while (calculating);

	total_ber = 100 * (double)total_bit_errs / (double)total_bit_count;
	printf("Max bit errs=%lu, Max BER=%lf%%\n", max_bit_errs, max_ber);
	printf("Total bit errs=%lu, Total BER=%lf%%\n", total_bit_errs, total_ber);

    return 0;
}

