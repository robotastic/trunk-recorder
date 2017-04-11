/*
 * bit_utils.h
 *
 * Translation of bit_utils.py to c++
 *
 *  Created on: Feb 19, 2017
 *      Author: S. Lukic
 */

#ifndef BIT_UTILS_H_
#define BIT_UTILS_H_

#include <vector>
#include <string>
#include <cassert>
#include <cstdio>
#include <iostream>
#include "Eigen/Dense"


unsigned rev_int(unsigned n, unsigned l) {
  unsigned j=0;
  for (unsigned i=0; i<l; i++) {
    unsigned b = n&1;
    n >>= 1;
    j = (j<<1) | b;
  }
  return j;
}

/* "bits" and "dibits" are coded as unsigned integers.
 * This is an arbitrary decision.
 * Eigen::VectorXi objects are used for python lists
 * containing numbers and for Numpy ndarrays.
 */

Eigen::VectorXi * bits_to_dibits(Eigen::VectorXi &bits) {
  Eigen::VectorXi *d = new Eigen::VectorXi;
  for (unsigned i=0; i<bits.size()/2; i++) {
    (*d)(i) = ((bits(i*2)&1)<<1) + (bits(i*2+1)&1);
  }
  return d;
}

Eigen::VectorXi* dibits_to_bits(Eigen::VectorXi &dibits) {
  Eigen::VectorXi *b = new Eigen::VectorXi;
  for (unsigned i=0; i<dibits.size(); i++) {
    unsigned char d = dibits(i);
    (*b)(i*2) = (d>>1)&1;
    (*b)(i*2+1) = d&1;
  }
  return b;
}

// Tested OK
Eigen::VectorXi *mk_array(unsigned long n, unsigned l) {

  Eigen::VectorXi *a = new Eigen::VectorXi(l);
  for(int i=l-1; i>=0; i--) {
    (*a)(i) = n&1;
    n >>= 1;
  }
  return a;
}

long unsigned mk_int(Eigen::VectorXi a) {
  long unsigned res = 0L;
  for (unsigned i=0; i< a.size(); i++) {
    res *= 2;
    res += (a(i) & 1);
  }
  return res;
}

std::string mk_str(Eigen::VectorXi a) {
  std::string res = "";
  char buffer[10];
  for (unsigned i=0; i<a.size(); i++) {
    sprintf(buffer, "%d", (a(i) & 1));
    res.append(buffer);
  }
  return res;
}

unsigned check_l(Eigen::VectorXi a, Eigen::VectorXi b) {
  unsigned ans = 0;
  assert(a.size()==b.size());
  for (unsigned i=0; i<a.size(); i++) {
    if (a(i) == b(i)) ans++;
  }
  return ans;
}

Eigen::VectorXi *fixup(Eigen::VectorXi a) {
  Eigen::VectorXi *res;
  for (unsigned i=0; i<a.size(); i++) {
    if (a(i) == 3) { (*res)(i) = 1; }
    else           { (*res)(i) = 3; }
  }
  return res;
}


unsigned find_sym(Eigen::VectorXi pattern, Eigen::VectorXi symbols) {

  for (unsigned i=0; i < (pattern.size()-symbols.size()); i++) {
    if (symbols.segment(i, pattern.size()) == pattern) return i;
  }
  return -1;
}

void print_array(const Eigen::VectorXi *a) {
  for (unsigned i=0; i<a->size()-1; i++) {
    std::cout << (*a)(i) << ", ";
  }
  std::cout << (*a)(a->size()-1) << ",";
  std::cout << std::endl;
}


#endif /* BIT_UTILS_H_ */
