
#ifndef INCLUDED_OP25_RS_H
#define INCLUDED_OP25_RS_H

#include <stdlib.h>
#include <stdint.h>
#include <vector>
#include <op25_imbe_frame.h>

void ProcHDU(const_bit_vector A);
void ProcTDU(const_bit_vector A);
void ProcLDU1(const_bit_vector A);
void ProcLDU2(const_bit_vector A);
uint32_t gly24128Dec (uint32_t n) ;
uint32_t gly23127Dec (uint32_t n) ;

#endif
