/*
 * (C) 2019, Graham J Norbury
 *           gnorbury@bondcar.com
 *
 * Stand-alone utility to scan a binary dibit file and look for
 * various sync sequences.  Can match with up to 6 bit errors
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <fstream>

#include "frame_sync_magics.h"

static const int          SYNC_SIZE            = 48; // 48 bits
static const int          SYNC_THRESHOLD       = 6;  // up to 6 bit errorss
static const int          SYNC_MAGICS_COUNT    = 12;
static const uint64_t     SYNC_MAGICS[]        = {DMR_BS_VOICE_SYNC_MAGIC,
                                                  DMR_BS_DATA_SYNC_MAGIC,
                                                  DMR_MS_VOICE_SYNC_MAGIC,
                                                  DMR_MS_DATA_SYNC_MAGIC,
                                                  DMR_MS_RC_SYNC_MAGIC,
                                                  DMR_T1_VOICE_SYNC_MAGIC,
                                                  DMR_T1_DATA_SYNC_MAGIC,
                                                  DMR_T2_VOICE_SYNC_MAGIC,
                                                  DMR_T2_DATA_SYNC_MAGIC,
                                                  DMR_RESERVED_SYNC_MAGIC,
                                                  DSTAR_FRAME_SYNC_MAGIC,
						 P25_FRAME_SYNC_MAGIC};
static const char         SYNCS[][25]          = {"DMR_BS_VOICE_SYNC",
                                                  "DMR_BS_DATA_SYNC",
                                                  "DMR_MS_VOICE_SYNC",
                                                  "DMR_MS_DATA_SYNC",
                                                  "DMR_MS_RC_SYNC",
                                                  "DMR_T1_VOICE_SYNC",
                                                  "DMR_T1_DATA_SYNC",
                                                  "DMR_T2_VOICE_SYNC",
                                                  "DMR_T2_DATA_SYNC",
                                                  "DMR_RESERVED_SYNC",
						  "DSTAR_FRAME_SYNC",
						  "P25_FRAME_SYNC"};

bool test_sync(uint64_t cw, int &sync, int &errs)
{
	int popcnt = 0;
	for (int i = 0; i < SYNC_MAGICS_COUNT; i ++)
	{
        	popcnt = __builtin_popcountll(cw ^ SYNC_MAGICS[i]);
		if (popcnt <= SYNC_THRESHOLD)
		{
			sync = i;
			errs = popcnt;
                	return true;
                }
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
		fprintf(stderr, "Usage: scan4sync <filename>\n");
		return 1;
	}

	char dibit;
	size_t fpos = 0;
	size_t last_fpos = 0;
	std::fstream file(argv[1], std::ios::in | std::ios::binary);
	while (!file.eof())
	{
		file.read((&dibit), 1);
		fpos++;

		cw = ((cw << 1) + ((dibit >>1) & 0x1)) & 0xffffffffffff;
		cw = ((cw << 1) + (dibit & 0x1)) & 0xffffffffffff;
		if (test_sync(cw, sync, s_errs)) {
			printf("%s [%06lx] matched [%06lx] with %d errs at sym %lu (dist=%lu)\n", SYNCS[sync], SYNC_MAGICS[sync], cw, s_errs, fpos, fpos-last_fpos);
			last_fpos = fpos;
		}

	}

    return 0;

}

