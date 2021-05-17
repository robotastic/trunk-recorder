/*
 * lfsr.cxx
 *
 * Translation of lfsr.py to c++ - source file
 *
 *  Created on: Feb 19, 2017
 *      Author: S. Lukic
 */
#include "bit_utils.h"
#include "lfsr.h"
#include <iostream>

/*************************************************
 * Implementation of class p25p2_lfsr
 *************************************************/


p25p2_lfsr::p25p2_lfsr(unsigned nac, unsigned sysid, unsigned wacn) :
  xor_chars(new std::string)
{

  // Initialize M
  for (unsigned i=0; i<msize; i++) {
    for (unsigned j=0; j<msize; j++) {
      M(i,j) = 0;
    }
    M(i, i) = 1;
    if ( i<msize-4 ) M(i, i+4) = 1;
    if ( i<msize-9 ) M(i, i+9) = 1;
    if ( i<msize-15 ) M(i, i+15) = 1;
    if ( i<msize-20 ) M(i, i+20) = 1;
    if ( i<msize-34 ) M(i, i+34) = 1;
  }

  Eigen::VectorXi *xorbits = mk_xor_bits(nac,sysid,wacn);


  xorsyms = new Eigen::VectorXi(xorbits->size( )/2);
  for (unsigned i=0; i<xorsyms->size(); i++) {
    (*xorsyms)(i) = ((*xorbits)(i*2) << 1) + (*xorbits)(i*2+1);
    (*xor_chars) += (char)((*xorsyms)(i)%(1<<8));
  }


}


const char * p25p2_lfsr::getXorChars(unsigned &len) const {

  len = xor_chars->size();
  return xor_chars->c_str();
}


Eigen::VectorXi * p25p2_lfsr::mk_xor_bits(unsigned long nac, unsigned long sysid, unsigned long wacn) {

  unsigned long long int n = 16777216ULL*wacn + 4096*sysid + nac;
  Eigen::VectorXi *reg = mk_array(n, 44);

  Eigen::VectorXi product = reg->transpose()*M;

  unsigned long long sreg = mk_int(product);

  const unsigned ssize = 4320;
  Eigen::VectorXi *s = new Eigen::VectorXi(ssize);
  for (unsigned i=0; i<ssize; i++) {
    (*s)(i) = (sreg >> 43) & 1;
    sreg = cyc_reg(sreg);
  }


  return s;

}


unsigned long long p25p2_lfsr::asm_reg(unsigned long long s[6] ) {
  s[0] = s[0] & 0xfULL;
  s[1] = s[1] & 0x1fULL;
  s[2] = s[2] & 0x3fULL;
  s[3] = s[3] & 0x1fULL;
  s[4] = s[4] & 0x3fffULL;
  s[5] = s[5] & 0x3ffULL;
  return (s[0]<<40)+(s[1]<<35)+(s[2]<<29)+(s[3]<<24)+(s[4]<<10)+s[5];
}

unsigned long long * p25p2_lfsr::disasm_reg(unsigned long long r) {

  unsigned long long *s = new unsigned long long[6];
  s[0] = (r>>40) & 0xfULL;
  s[1] = (r>>35) & 0x1fULL;
  s[2] = (r>>29) & 0x3fULL;
  s[3] = (r>>24) & 0x1fULL;
  s[4] = (r>>10) & 0x3fffULL;
  s[5] =  r      & 0x3ffULL;

  return s;
}

unsigned long long p25p2_lfsr::cyc_reg(unsigned long long reg) {

  unsigned long long *s = disasm_reg(reg);
  unsigned long long cy1 = (s[0] >> 3) & 1L;
  unsigned long long cy2 = (s[1] >> 4) & 1L;
  unsigned long long cy3 = (s[2] >> 5) & 1L;
  unsigned long long cy4 = (s[3] >> 4) & 1L;
  unsigned long long cy5 = (s[4] >> 13) & 1L;
  unsigned long long cy6 = (s[5] >> 9) & 1L;
  unsigned long long x1 = cy1 ^ cy2;
  unsigned long long x2 = cy1 ^ cy3;
  unsigned long long x3 = cy1 ^ cy4;
  unsigned long long x4 = cy1 ^ cy5;
  unsigned long long x5 = cy1 ^ cy6;
  s[0] = (s[0] << 1) & 0xfULL;
  s[1] = (s[1] << 1) & 0x1fULL;
  s[2] = (s[2] << 1) & 0x3fULL;
  s[3] = (s[3] << 1) & 0x1fULL;
  s[4] = (s[4] << 1) & 0x3fffULL;
  s[5] = (s[5] << 1) & 0x3ffULL;
  s[0] = s[0] | (x1 & 1ULL);
  s[1] = s[1] | (x2 & 1ULL);
  s[2] = s[2] | (x3 & 1ULL);
  s[3] = s[3] | (x4 & 1ULL);
  s[4] = s[4] | (x5 & 1ULL);
  s[5] = s[5] | (cy1 & 1ULL);

  return asm_reg(s);
}
