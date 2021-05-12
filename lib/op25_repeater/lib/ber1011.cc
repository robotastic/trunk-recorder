/*
 * (C) 2019, Graham J Norbury
 *           gnorbury@bondcar.com
 *
 * Stand-alone utility to calculate bit error rate of a binary dibit capture file
 * containing APCO 1011Hz test tone sequence.
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <fstream>

#include "frame_sync_magics.h"

static const int          SYNC_SIZE		   = 48; // 48 bits
static const unsigned int SYNC_THRESHOLD	   = 4;  // 4 bit errors in sync allowed

bool test_sync(uint64_t cw, int &sync, int &errs)
{
	int popcnt = 0;
       	popcnt = __builtin_popcountll(cw ^ P25_FRAME_SYNC_MAGIC);
	if (popcnt <= SYNC_THRESHOLD)
	{
		errs = popcnt;
               	return true;
	}

	return false;
}

int main (int argc, char* argv[])
{
	uint64_t cw = 0;
	int sync = 0;
	int s_errs = 0;
	
	if (argc < 2)
	{
		fprintf(stderr, "Usage: ber1011 <filename>\n");
		return 1;
	}

	char dibit;
	size_t fpos = 0;
	std::fstream file(argv[1], std::ios::in | std::ios::binary);
	while (!file.eof())
	{
		file.read((&dibit), 1);
		fpos++;

		cw = ((cw << 1) + ((dibit >>1) & 0x1)) & 0xffffffffffff;
		if (test_sync(cw, sync, s_errs))
			printf("Matched sync with %d errs at %lu bit 1\n", s_errs, fpos);

		cw = ((cw << 1) + (dibit & 0x1)) & 0xffffffffffff;
		if (test_sync(cw, sync, s_errs))
			printf("Matched sync with %d errs at %lu bit 0\n", s_errs, fpos);

	}

    return 0;

}

