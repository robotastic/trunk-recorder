
#ifndef INCLUDED_CHECK_FRAME_SYNC_H
#define INCLUDED_CHECK_FRAME_SYNC_H

#include <stdint.h>

static inline bool check_frame_sync(uint64_t x, int err_threshold, int len) {
        int errs=0;
	const uint64_t mask = (1LL<<len)-1LL;
	x = x & mask;
	// source: https://en.wikipedia.org/wiki/Hamming_weight
	static const uint64_t m1  = 0x5555555555555555; //binary: 0101...
	static const uint64_t m2  = 0x3333333333333333; //binary: 00110011..
	static const uint64_t m4  = 0x0f0f0f0f0f0f0f0f; //binary:  4 zeros,  4 ones ...
	static const uint64_t h01 = 0x0101010101010101; //the sum of 256 to the power of 0,1,2,3...
	x -= (x >> 1) & m1;             //put count of each 2 bits into those 2 bits
	x = (x & m2) + ((x >> 2) & m2); //put count of each 4 bits into those 4 bits 
	x = (x + (x >> 4)) & m4;        //put count of each 8 bits into those 8 bits 
	errs = (x * h01) >> 56;  //returns left 8 bits of x + (x<<8) + (x<<16) + (x<<24) + ... 
        if (errs <= err_threshold) return true;
        return false;
}

#endif /* INCLUDED_CHECK_FRAME_SYNC_H */
