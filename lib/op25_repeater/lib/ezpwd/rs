/*
 * Ezpwd Reed-Solomon -- Reed-Solomon encoder / decoder library
 * 
 * Copyright (c) 2014, Hard Consulting Corporation.
 *
 * Ezpwd Reed-Solomon is free software: you can redistribute it and/or modify it under the terms of
 * the GNU General Public License as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.  See the LICENSE file at the top of the
 * source tree.  Ezpwd Reed-Solomon is also available under Commercial license.  c++/ezpwd/rs_base
 * is redistributed under the terms of the LGPL, regardless of the overall licensing terms.
 * 
 * Ezpwd Reed-Solomon is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License for more details.
 */
#ifndef _EZPWD_RS
#define _EZPWD_RS

#include "rs_base"

// 
// ezpwd::RS<SYMBOLS,PAYLOAD>	-- Implements an RS(SYMBOLS,PAYLOAD) codec
// ezpwd::RS_CCSDS<...>		-- CCSDS standard 8-bit R-S codec
// 
//     Support for Reed-Solomon codecs for symbols of 2 to 16 bits is supported.  The R-S "codeword"
// for an N-bit symbol is defined to be 2^N-1 symbols in size.  For example, for 5-bit symbols,
// 2^5-1 == 31, so the notation for defining an Reed-Solomon codec for 5-bit symbols is always:
// RS(31,PAYLOAD), where PAYLOAD is always some value less than 31.  The difference is the number of
// "parity" symbols.
// 
//     For example, to define an RS codeword of 31 symbols w/ 4 symbols of parity and up to 27
// symbols of data, you would say: RS(31,27).  Of course, you can supply smaller amounts of data;
// the balance is assumed to be NUL (zero) symbols.
// 
namespace ezpwd {

    // 
    // __RS( ... ) -- Define a reed-solomon codec 
    // 
    // @SYMBOLS:	Total number of symbols; must be a power of 2 minus 1, eg 2^8-1 == 255
    // @PAYLOAD:	The maximum number of non-parity symbols, eg 253 ==> 2 parity symbols
    // @POLY:		A primitive polynomial appropriate to the SYMBOLS size
    // @FCR:		The first consecutive root of the Reed-Solomon generator polynomial
    // @PRIM:		The primitive root of the generator polynomial
    // 
#   define __RS_TYP( TYPE, SYMBOLS, PAYLOAD, POLY, FCR, PRIM )		\
	ezpwd::reed_solomon<						\
	    TYPE,							\
	    ezpwd::log_< (SYMBOLS) + 1 >::value,			\
	    (SYMBOLS) - (PAYLOAD), FCR, PRIM,				\
	    ezpwd::gfpoly<						\
		ezpwd::log_< (SYMBOLS) + 1 >::value,			\
		POLY >>

#   define __RS( NAME, TYPE, SYMBOLS, PAYLOAD, POLY, FCR, PRIM )	\
	__RS_TYP( TYPE, SYMBOLS, PAYLOAD, POLY, FCR, PRIM ) {		\
	    NAME()							\
		: __RS_TYP( TYPE, SYMBOLS, PAYLOAD, POLY, FCR, PRIM )()	\
	    {;}								\
	}

    //
    // RS<SYMBOLS, PAYLOAD> -- Standard partial specializations for Reed-Solomon codec type access
    //
    //     Normally, Reed-Solomon codecs are described with terms like RS(255,252).  Obtain various
    // standard Reed-Solomon codecs using macros of a similar form, eg. RS<255, 252>. Standard PLY,
    // FCR and PRM values are provided for various SYMBOL sizes, along with appropriate basic types
    // capable of holding all internal Reed-Solomon tabular data.
    // 
    //     In order to provide "default initialization" of const RS<...> types, a user-provided
    // default constructor must be provided.
    // 
    template < size_t SYMBOLS, size_t PAYLOAD > struct RS;
    template < size_t PAYLOAD > struct RS<    3, PAYLOAD> : public __RS( RS, uint8_t,      3, PAYLOAD,     0x7,	 1,  1 );
    template < size_t PAYLOAD > struct RS<    7, PAYLOAD> : public __RS( RS, uint8_t,      7, PAYLOAD,     0xb,	 1,  1 );
    template < size_t PAYLOAD > struct RS<   15, PAYLOAD> : public __RS( RS, uint8_t,     15, PAYLOAD,    0x13,	 1,  1 );
    template < size_t PAYLOAD > struct RS<   31, PAYLOAD> : public __RS( RS, uint8_t,     31, PAYLOAD,    0x25,	 1,  1 );
    template < size_t PAYLOAD > struct RS<   63, PAYLOAD> : public __RS( RS, uint8_t,     63, PAYLOAD,    0x43,	 1,  1 );
    template < size_t PAYLOAD > struct RS<  127, PAYLOAD> : public __RS( RS, uint8_t,    127, PAYLOAD,    0x89,	 1,  1 );
    template < size_t PAYLOAD > struct RS<  255, PAYLOAD> : public __RS( RS, uint8_t,    255, PAYLOAD,   0x11d,	 1,  1 );
    template < size_t PAYLOAD > struct RS<  511, PAYLOAD> : public __RS( RS, uint16_t,   511, PAYLOAD,   0x211,	 1,  1 );
    template < size_t PAYLOAD > struct RS< 1023, PAYLOAD> : public __RS( RS, uint16_t,  1023, PAYLOAD,   0x409,	 1,  1 );
    template < size_t PAYLOAD > struct RS< 2047, PAYLOAD> : public __RS( RS, uint16_t,  2047, PAYLOAD,   0x805,	 1,  1 );
    template < size_t PAYLOAD > struct RS< 4095, PAYLOAD> : public __RS( RS, uint16_t,  4095, PAYLOAD,  0x1053,	 1,  1 );
    template < size_t PAYLOAD > struct RS< 8191, PAYLOAD> : public __RS( RS, uint16_t,  8191, PAYLOAD,  0x201b,	 1,  1 );
    template < size_t PAYLOAD > struct RS<16383, PAYLOAD> : public __RS( RS, uint16_t, 16383, PAYLOAD,  0x4443,	 1,  1 );
    template < size_t PAYLOAD > struct RS<32767, PAYLOAD> : public __RS( RS, uint16_t, 32767, PAYLOAD,  0x8003,	 1,  1 );
    template < size_t PAYLOAD > struct RS<65535, PAYLOAD> : public __RS( RS, uint16_t, 65535, PAYLOAD, 0x1100b,	 1,  1 );
    
    template < size_t SYMBOLS, size_t PAYLOAD > struct RS_CCSDS;
    template < size_t PAYLOAD > struct RS_CCSDS<255, PAYLOAD> : public __RS( RS_CCSDS, uint8_t, 255, PAYLOAD, 0x187, 112, 11 );


    // 
    // strength<PARITY>	-- compute strength (given N parity symbols) of R-S correction
    // 
    //     Returns a confidence strength rating, which is the ratio:
    // 
    //         100 - ( errors * 2 + erasures ) * 100 / parity
    // 
    // which is proportional to the number of parity symbols unused by the reported number of
    // corrected symbols.  If 0, then all parity resources were consumed to recover the R-S
    // codeword, and we can have no confidence in the result.  If -'ve, indicates more parity
    // resources were consumed than available, indicating that the result is likely incorrect.
    // 
    //     Accounts for the fact that a signalled erasure may not be reported in the corrected
    // position vector, if the symbol happens to match the computed value.  Note that even if the
    // error or erasure occurs within the "parity" portion of the codeword, this doesn't reduce the
    // effective strength -- all symbols in the R-S complete codeword are equally effective in
    // recovering any other symbol in error/erasure.
    // 
    template < size_t PARITY >
    int				strength(
				    int			corrected,
				    const std::vector<int>&erasures,	// original erasures positions
				    const std::vector<int>&positions )	// all reported correction positions
    {
	// -'ve indicates R-S failure; all parity consumed, but insufficient to correct the R-S
	// codeword.  Missing an unknown number of additional required parity symbols, so just
	// return -1 as the strength.
	if ( corrected < 0 ) {
#if defined( DEBUG ) && DEBUG >= 2
	    std::cout
		<< corrected << " corrections (R-S decode failure) == -1 confidence"
		<< std::endl;
#endif
	    return -1;
	}
	if ( corrected != int( positions.size() ))
	    EZPWD_RAISE_OR_RETURN( std::runtime_error, "inconsistent R-S decode results", -1 );

	// Any erasures that don't turn out to contain errors are not returned as fixed positions.
	// However, they have consumed parity resources.  Search for each erasure location in
	// positions, and if not reflected, add to the corrected/missed counters.
	int			missed	= 0;
	for ( auto e : erasures ) {
	    if ( std::find( positions.begin(), positions.end(), e ) == positions.end() ) {
		++corrected;
		++missed;
#if defined( DEBUG ) && DEBUG >= 2
		std::cout
		    << corrected << " corrections (R-S erasure missed): " << e
		    << std::endl;
#endif
	    }
	}
	int			errors	= corrected - erasures.size();
	int			consumed= errors * 2 + erasures.size();
	int			confidence= 100 - consumed * 100 / PARITY;
#if defined( DEBUG ) && DEBUG >= 2
	std::cout
	    << corrected << " corrections (R-S decode success)"
	    << " at: "			<< positions
	    << ", "			<< erasures.size() + missed
	    << " erasures ("		<< missed
	    << " unreported) at: "	<< erasures
	    << ") ==> "			<< errors
	    <<  " errors, and " 	<< consumed << " / " << PARITY
	    << " parity used == "	<< confidence
	    << "% confidence"
	    << std::endl;
#endif
	return confidence;
    }
    
} // namespace ezpwd
    
#endif // _EZPWD_RS
