/*
 * Ezpwd Reed-Solomon -- Reed-Solomon encoder / decoder library
 * 
 * Copyright (c) 2014, Hard Consulting Corporation.
 *
 * Ezpwd Reed-Solomon is free software: you can redistribute it and/or modify it under the terms of
 * the GNU General Public License as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.  See the LICENSE file at the top of the
 * source tree.  Ezpwd Reed-Solomon is also available under Commercial license.  The
 * c++/ezpwd/rs_base file is redistributed under the terms of the LGPL, regardless of the overall
 * licensing terms.
 * 
 * Ezpwd Reed-Solomon is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License for more details.
 * 
 * The core Reed-Solomon codec implementation in c++/ezpwd/rs_base is by Phil Karn, converted to C++
 * by Perry Kundert (perry@hardconsulting.com), and may be used under the terms of the LGPL.  Here
 * is the terms from Phil's README file (see phil-karn/fec-3.0.1/README):
 * 
 *     COPYRIGHT
 * 
 *     This package is copyright 2006 by Phil Karn, KA9Q. It may be used
 *     under the terms of the GNU Lesser General Public License (LGPL). See
 *     the file "lesser.txt" in this package for license details.
 * 
 * The c++/ezpwd/rs_base file is, therefore, redistributed under the terms of the LGPL, while the
 * rest of Ezpwd Reed-Solomon is distributed under either the GPL or Commercial licenses.
 * Therefore, even if you have obtained Ezpwd Reed-Solomon under a Commercial license, you must make
 * available the source code of the c++/ezpwd/rs_base file with your product.  One simple way to
 * accomplish this is to include the following URL in your code or documentation:
 * 
 *     https://github.com/pjkundert/ezpwd-reed-solomon/blob/master/c++/ezpwd/rs_base
 * 
 * 
 * The Linux 3.15.1 version of lib/reed_solomon was also consulted as a cross-reference, which (in
 * turn) is basically verbatim copied from Phil Karn's LGPL implementation, to ensure that no new
 * defects had been found and fixed; there were no meaningful changes made to Phil's implementation.
 * I've personally been using Phil's implementation for years in a heavy industrial use, and it is
 * rock-solid.
 * 
 * However, both Phil's and the Linux kernel's (copy of Phil's) implementation will return a
 * "corrected" decoding with impossible error positions, in some cases where the error load
 * completely overwhelms the R-S encoding.  These cases, when detected, are rejected in this
 * implementation.  This could be considered a defect in Phil's (and hence the Linux kernel's)
 * implementations, which results in them accepting clearly incorrect R-S decoded values as valid
 * (corrected) R-S codewords.  We chose the report failure on these attempts.
 * 
 */
#ifndef _EZPWD_RS_BASE
#define _EZPWD_RS_BASE

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <type_traits>
#include <vector>

// 
// Preprocessor defines available:
// 
// EZPWD_NO_EXCEPTS -- define to use no exceptions; return -1, or abort on catastrophic failures
// EZPWD_NO_MOD_TAB -- define to force no "modnn" Galois modulo table acceleration
// EZPWD_ARRAY_SAFE -- define to force usage of bounds-checked arrays for most tabular data
// EZPWD_ARRAY_TEST -- define to force erroneous sizing of some arrays for non-production testing
// 

#if defined( DEBUG ) && DEBUG >= 2
#  include "output"	// ezpwd::hex... std::ostream shims for outputting containers of uint8_t data
#endif

#if defined( EZPWD_NO_EXCEPTS )
#  include <cstdio>	// No exceptions; don't use C++ ostream
#  define EZPWD_RAISE_OR_ABORT(  typ, str )		do {		\
      std::fputs(( str ), stderr ); std::fputc( '\n', stderr );		\
      abort();								\
  } while ( false )
#  define EZPWD_RAISE_OR_RETURN( typ, str, ret )	return ( ret )
#else
#  define EZPWD_RAISE_OR_ABORT(  typ, str )		throw ( typ )( str )
#  define EZPWD_RAISE_OR_RETURN( typ, str, ret )	throw ( typ )( str )
#endif

namespace ezpwd {

    // ezpwd::log_<N,B> -- compute the log base B of N at compile-time
    template <size_t N, size_t B=2> struct log_{       enum { value = 1 + log_<N/B, B>::value }; };
    template <size_t B>		    struct log_<1, B>{ enum { value = 0 }; };
    template <size_t B>		    struct log_<0, B>{ enum { value = 0 }; };

    //
    // reed_solomon_base - Reed-Solomon codec generic base class
    //
    class reed_solomon_base {
    public:
	virtual size_t		datum()		const = 0;	// a data element's bits
	virtual size_t		symbol()	const = 0;	// a symbol's bits
	virtual int		size()		const = 0;	// R-S block size (maximum total symbols)
	virtual int		nroots()	const = 0;	// R-S roots (parity symbols)
	virtual	int		load()		const = 0;	// R-S net payload (data symbols)

	virtual		       ~reed_solomon_base()
	{
	    ;
	}
				reed_solomon_base()
	{
	    ;
	}

	virtual std::ostream   &output(
					std::ostream       &lhs )
	    const
	{
	    return lhs << "RS(" << this->size() << "," << this->load() << ")";
	}

	// 
	// {en,de}code -- Compute/Correct errors/erasures in a Reed-Solomon encoded container
	// 
	///     The encoded parity symbols may be included in 'data' (len includes nroots() parity
	/// symbols), or may (optionally) supplied separately in (at least nroots()-sized)
	/// 'parity'.  
	/// 
	///     For decode, optionally specify some known erasure positions (up to nroots()).  If
	/// non-empty 'erasures' is provided, it contains the positions of each erasure.  If a
	/// non-zero pointer to a 'position' vector is provided, its capacity will be increased to
	/// be capable of storing up to 'nroots()' ints; the actual deduced error locations will be
	/// returned.
	///  
	/// RETURN VALUE
	/// 
	///     Return -1 on error.  The encode returns the number of parity symbols produced;
	/// decode returns the number of symbols corrected.  Both errors and erasures are included,
	/// so long as they are actually different than the deduced value.  In other words, if a
	/// symbol is marked as an erasure but it actually turns out to be correct, it's index will
	/// NOT be included in the returned count, nor the modified erasure vector!
	/// 

	// 
	// encode(<string>) -- extend string to contain parity, or place in supplied parity string
	// encode(<vector>) -- extend vector to contain parity, or place in supplied parity vector
	// encode(<array>)  -- ignore 'pad' elements of array, puts nroots() parity symbols at end
	// encode(pair<iter,iter>) -- encode all except the last nroots() of the data, put parity at end
	// encode(pair<iter,iter>, pair<iter,iter>) -- encode data between first <iter> pair, put parity in second pair
	// 
	int			encode(
				    std::string	       &data )
	    const
	{
	    typedef uint8_t	uT;
	    typedef std::pair<uT *, uT *>
				uTpair;
	    data.resize( data.size() + nroots() );
	    return encode( uTpair( (uT *)&data.front(), (uT *)&data.front() + data.size() ));
	}

	int			encode(
				    const std::string  &data,
				    std::string	       &parity )
	    const
	{
	    typedef uint8_t	uT;
	    typedef std::pair<const uT *, const uT *>
				cuTpair;
	    typedef std::pair<uT *, uT *>
				uTpair;
	    parity.resize( nroots() );
	    return encode( cuTpair( (const uT *)&data.front(), (const uT *)&data.front() + data.size() ),
			   uTpair( (uT *)&parity.front(), (uT *)&parity.front() + parity.size() ));
	}

	template < typename T >
	int			encode(
				    std::vector<T>     &data )
	    const
	{
	    typedef typename std::make_unsigned<T>::type
				uT;
	    typedef std::pair<uT *, uT *>
				uTpair;
	    data.resize( data.size() + nroots() );
	    return encode( uTpair( (uT *)&data.front(), (uT *)&data.front() + data.size() ));
	}
	template < typename T >
	int			encode(
				    const std::vector<T>&data,
				    std::vector<T>     &parity )
	    const
	{
	    typedef typename std::make_unsigned<T>::type
				uT;
	    typedef std::pair<const uT *, const uT *>
				cuTpair;
	    typedef std::pair<uT *, uT *>
				uTpair;
	    parity.resize( nroots() );
	    return encode( cuTpair( (uT *)&data.front(), (uT *)&data.front() + data.size() ),
			   uTpair( (uT *)&parity.front(), (uT *)&parity.front() + parity.size() ));
	}

	template < typename T, size_t N >
	int			encode(
				    std::array<T,N>    &data,
				    int			pad	= 0 ) // ignore 'pad' symbols at start of array
	    const
	{
	    typedef typename std::make_unsigned<T>::type
				uT;
	    typedef std::pair<uT *, uT *>
				uTpair;
	    return encode( uTpair( (uT *)&data.front() + pad, (uT *)&data.front() + data.size() ));
	}

	virtual int		encode(
				    const std::pair<uint8_t *, uint8_t *>
						       &data )
	    const
	= 0;
	virtual int		encode(
				    const std::pair<const uint8_t *, const uint8_t *>
						       &data,
				    const std::pair<uint8_t *, uint8_t *>
						       &parity )
	    const
	= 0;
	virtual int		encode(
				    const std::pair<uint16_t *, uint16_t *>
						       &data )
	    const
	= 0;
	virtual int		encode(
				    const std::pair<const uint16_t *, const uint16_t *>
						       &data,
				    const std::pair<uint16_t *, uint16_t *>
						       &parity )
	    const
	= 0;
	virtual int		encode(
				    const std::pair<uint32_t *, uint32_t *>
						       &data )
	    const
	= 0;
	virtual int		encode(
				    const std::pair<const uint32_t *, const uint32_t *>
						       &data,
				    const std::pair<uint32_t *, uint32_t *>
						       &parity )
	    const
	= 0;

	int			decode(
				    std::string	       &data,
				    const std::vector<int>
				    		       &erasure	= std::vector<int>(),
				    std::vector<int>   *position= 0 )
	    const
	{
	    typedef uint8_t	uT;
	    typedef std::pair<uT *, uT *>
				uTpair;
	    return decode( uTpair( (uT *)&data.front(), (uT *)&data.front() + data.size() ),
			   erasure, position );
	}

	int			decode(
				    std::string	       &data,
				    std::string	       &parity,
				    const std::vector<int>
				    		       &erasure	= std::vector<int>(),
				    std::vector<int>   *position= 0 )
	    const
	{
	    typedef uint8_t	uT;
	    typedef std::pair<uT *, uT *>
				uTpair;
	    return decode( uTpair( (uT *)&data.front(), (uT *)&data.front() + data.size() ),
			   uTpair( (uT *)&parity.front(), (uT *)&parity.front() + parity.size() ),
			   erasure, position );
	}

	template < typename T >
	int			decode(
				    std::vector<T>     &data,
				    const std::vector<int>
				    		       &erasure	= std::vector<int>(),
				    std::vector<int>   *position= 0 )
	    const
	{
	    typedef typename std::make_unsigned<T>::type
				uT;
	    typedef std::pair<uT *, uT *>
				uTpair;
	    return decode( uTpair( (uT *)&data.front(), (uT *)&data.front() + data.size() ),
			   erasure, position );
	}

	template < typename T >
	int			decode(
				    std::vector<T>     &data,
				    std::vector<T>     &parity,
				    const std::vector<int>
				    		       &erasure	= std::vector<int>(),
				    std::vector<int>   *position= 0 )
	    const
	{
	    typedef typename std::make_unsigned<T>::type
				uT;
	    typedef std::pair<uT *, uT *>
				uTpair;
	    return decode( uTpair( (uT *)&data.front(), (uT *)&data.front() + data.size() ),
			   uTpair( (uT *)&parity.front(), (uT *)&parity.front() + parity.size() ),
			   erasure, position );
	}

	template < typename T, size_t N >
	int			decode(
				    std::array<T,N>    &data,
				    int			pad	= 0, // ignore 'pad' symbols at start of array
				    const std::vector<int>
				    		       &erasure	= std::vector<int>(),
				    std::vector<int>   *position= 0 )
	    const
	{
	    typedef typename std::make_unsigned<T>::type
				uT;
	    typedef std::pair<uT *, uT *>
				uTpair;
	    return decode( uTpair( (uT *)&data.front() + pad, (uT *)&data.front() + data.size() ),
			   erasure, position );
	}

	virtual int		decode(
				    const std::pair<uint8_t *, uint8_t *>
						       &data,
				    const std::vector<int>
				    		       &erasure	= std::vector<int>(),
				    std::vector<int>   *position= 0 )
	    const
	= 0;
	virtual int		decode(
				    const std::pair<uint8_t *, uint8_t *>
						       &data,
				    const std::pair<uint8_t *, uint8_t *>
						       &parity,
				    const std::vector<int>
				    		       &erasure	= std::vector<int>(),
				    std::vector<int>   *position= 0 )
	    const
	= 0;
	virtual int		decode(
				    const std::pair<uint16_t *, uint16_t *>
						       &data,
				    const std::vector<int>
				    		       &erasure	= std::vector<int>(),
				    std::vector<int>   *position= 0 )
	    const
	= 0;
	virtual int		decode(
				    const std::pair<uint16_t *, uint16_t *>
						       &data,
				    const std::pair<uint16_t *, uint16_t *>
						       &parity,
				    const std::vector<int>
				    		       &erasure	= std::vector<int>(),
				    std::vector<int>   *position= 0 )
	    const
	= 0;
	virtual int		decode(
				    const std::pair<uint32_t *, uint32_t *>
						       &data,
				    const std::vector<int>
				    		       &erasure	= std::vector<int>(),
				    std::vector<int>   *position= 0 )
	    const
	= 0;
	virtual int		decode(
				    const std::pair<uint32_t *, uint32_t *>
						       &data,
				    const std::pair<uint32_t *, uint32_t *>
						       &parity,
				    const std::vector<int>
				    		       &erasure	= std::vector<int>(),
				    std::vector<int>   *position= 0 )
	    const
	= 0;
    }; // class reed_solomon_base

    // 
    // std::ostream << ezpwd::reed_solomon<...>
    // 
    //     Output a R-S codec description in standard form eg. RS(255,253)
    // 
    inline
    std::ostream	       &operator<<(
				    std::ostream       &lhs,
				    const ezpwd::reed_solomon_base
						       &rhs )
    {
	return rhs.output( lhs );
    }

    //
    // gfpoly - default field polynomial generator functor.
    //
    template < int SYM, int PLY >
    struct gfpoly {
	int 			operator() ( int sr )
	    const
	{
	    if ( sr == 0 )
		sr			= 1;
	    else {
		sr	      	      <<= 1;
		if ( sr & ( 1 << SYM ))
		    sr	       	       ^= PLY;
		sr		       &= (( 1 << SYM ) - 1);
	    }
	    return sr;
	}
    };
    
    // 
    // class reed_solomon_tabs -- R-S tables common to all RS(NN,*) with same SYM, PRM and PLY
    // 
    template < typename TYP, int SYM, int PRM, class PLY >
    class reed_solomon_tabs
	: public reed_solomon_base {

    public:
	typedef TYP		symbol_t;
	static const size_t	DATUM	= 8 * sizeof TYP();	// bits / TYP
	static const size_t	SYMBOL	= SYM;			// bits / symbol
	static const int	MM	= SYM;
	static const int	SIZE	= ( 1 << SYM ) - 1;	// maximum symbols in field
	static const int	NN	= SIZE;
	static const int	A0	= SIZE;
	static const int	MODS				// modulo table: 1/2 the symbol size squared, up to 4k
#if defined( EZPWD_NO_MOD_TAB )
					= 0;
#else
					= SYM > 8 ? ( 1 << 12 ) : ( 1 << SYM << SYM/2 );
#endif

	static int 		iprim;				// initialized to -1, below

    protected:
	static std::array<TYP,
#if not defined( EZPWD_ARRAY_TEST )
                              NN + 1>
#else
#  warning "EZPWD_ARRAY_TEST: Erroneously declaring alpha_to size!"
                              NN    >
#endif
				alpha_to;
	static std::array<TYP,NN + 1>
				index_of;
	static std::array<TYP,MODS>
				mod_of;
	virtual		       ~reed_solomon_tabs()
	{
	    ;
	}
				reed_solomon_tabs()
				    : reed_solomon_base()
	{
	    // Do init if not already done.  We check one value which is initialized to -1; this is
	    // safe, 'cause the value will not be set 'til the initializing thread has completely
	    // initialized the structure.  Worst case scenario: multiple threads will initialize
	    // identically.  No mutex necessary.
	    if ( iprim >= 0 )
		return;

#if defined( DEBUG ) && DEBUG >= 1
	    std::cout << "RS(" << SIZE << ",*): Initialize for " << NN << " symbols size, " << MODS << " modulo table." << std::endl;
#endif
	    // Generate Galois field lookup tables
	    index_of[0]			= A0;	// log(zero) = -inf
	    alpha_to[A0]		= 0;	// alpha**-inf = 0
	    PLY			poly;
	    int			sr	= poly( 0 );
	    for ( int i = 0; i < NN; i++ ) {
		index_of[sr]		= i;
		alpha_to[i]		= sr;
		sr			= poly( sr );
	    }
	    // If it's not primitive, raise exception or abort
	    if ( sr != alpha_to[0] ) {
	        EZPWD_RAISE_OR_ABORT( std::runtime_error, "reed-solomon: Galois field polynomial not primitive" );
	    }

	    // Generate modulo table for some commonly used (non-trivial) values
	    for ( int x = NN; x < NN + MODS; ++x )
		mod_of[x-NN]		= _modnn( x );
	    // Find prim-th root of 1, index form, used in decoding.
	    int			iptmp	= 1;
	    while ( iptmp % PRM != 0 )
		iptmp		       += NN;
	    iprim			= iptmp / PRM;
	}

	// 
	// modnn -- modulo replacement for galois field arithmetics, optionally w/ table acceleration
	//
	//  @x:		the value to reduce (will never be -'ve)
	//
	//  where
	//  MM = number of bits per symbol
	//  NN = (2^MM) - 1
	//
	//  Simple arithmetic modulo would return a wrong result for values >= 3 * NN
	//
	TYP	 		_modnn(
				    int			x )
	    const
	{
	    while ( x >= NN ) {
		x		       -= NN;
		x			= ( x >> MM ) + ( x & NN );
	    }
	    return x;
	}
	TYP	 		modnn(
				    int			x )
	    const
	{
	    while ( x >= NN + MODS ) {
		x		       -= NN;
		x			= ( x >> MM ) + ( x & NN );
	    }
	    if ( MODS && x >= NN )
		x			= mod_of[x-NN];
	    return x;
	}
    };

    //
    // class reed_solomon - Reed-Solomon codec
    //
    // @TYP:		A symbol datum; {en,de}code operates on arrays of these
    // @DATUM:		Bits per datum (a TYP())
    // @SYM{BOL}, MM:	Bits per symbol
    // @NN:		Symbols per block (== (1<<MM)-1)
    // @alpha_to:	log lookup table
    // @index_of:	Antilog lookup table
    // @genpoly:	Generator polynomial
    // @NROOTS:		Number of generator roots = number of parity symbols
    // @FCR:		First consecutive root, index form
    // @PRM:		Primitive element, index form
    // @iprim:		prim-th root of 1, index form
    // @PLY:		The primitive generator polynominal functor
    //
    //     All reed_solomon<T, ...> instances with the same template type parameters share a common
    // (static) set of alpha_to, index_of and genpoly tables.  The first instance to be constructed
    // initializes the tables.
    // 
    //     Each specialized type of reed_solomon implements a specific encode/decode method
    // appropriate to its datum 'TYP'.  When accessed via a generic reed_solomon_base pointer, only
    // access via "safe" (size specifying) containers or iterators is available.
    //
    template < typename TYP, int SYM, int RTS, int FCR, int PRM, class PLY >
    class reed_solomon
	: public reed_solomon_tabs<TYP, SYM, PRM, PLY> {

    public:
	typedef reed_solomon_tabs<TYP, SYM, PRM, PLY>
				tabs_t;
	using tabs_t::DATUM;
	using tabs_t::SYMBOL;
	using tabs_t::MM;
	using tabs_t::SIZE;
	using tabs_t::NN;
	using tabs_t::A0;

	using tabs_t::iprim;

	using tabs_t::alpha_to;
	using tabs_t::index_of;

	using tabs_t::modnn;

	static const int	NROOTS	= RTS;
	static const int	LOAD	= SIZE - NROOTS;	// maximum non-parity symbol payload

    protected:
	static std::array<TYP, NROOTS + 1>
				genpoly;

    public:
	virtual size_t		datum() const
	{
	    return DATUM;
	}

	virtual size_t		symbol() const
	{
	    return SYMBOL;
	}

	virtual int		size() const
	{
	    return SIZE;
	}

	virtual int		nroots() const
	{
	    return NROOTS;
	}

	virtual int		load() const
	{
	    return LOAD;
	}

	using reed_solomon_base::encode;
	virtual int		encode(
				    const std::pair<uint8_t *, uint8_t *>
						       &data )
	    const
	{
	    return encode_mask( data.first, data.second - data.first - NROOTS, data.second - NROOTS );
	}

	virtual int		encode(
				    const std::pair<const uint8_t *, const uint8_t *>
						       &data,
				    const std::pair<uint8_t *, uint8_t *>
						       &parity )
	    const
	{
	    if ( parity.second - parity.first != NROOTS ) {
	        EZPWD_RAISE_OR_RETURN( std::runtime_error, "reed-solomon: parity length incompatible with number of roots", -1 );
	    }
	    return encode_mask( data.first, data.second - data.first, parity.first );
	}

	virtual int		encode(
				    const std::pair<uint16_t *, uint16_t *>
						       &data )
	    const
	{
	    return encode_mask( data.first, data.second - data.first - NROOTS, data.second - NROOTS );
	}

	virtual int		encode(
				    const std::pair<const uint16_t *, const uint16_t *>
						       &data,
				    const std::pair<uint16_t *, uint16_t *>
						       &parity )
	    const
	{
	    if ( parity.second - parity.first != NROOTS ) {
	        EZPWD_RAISE_OR_RETURN( std::runtime_error, "reed-solomon: parity length incompatible with number of roots", -1 );
	    }
	    return encode_mask( data.first, data.second - data.first, parity.first );
	}

	virtual int		encode(
				    const std::pair<uint32_t *, uint32_t *>
						       &data )
	    const
	{
	    return encode_mask( data.first, data.second - data.first - NROOTS, data.second - NROOTS );
	}

	virtual int		encode(
				    const std::pair<const uint32_t *, const uint32_t *>
						       &data,
				    const std::pair<uint32_t *, uint32_t *>
						       &parity )
	    const
	{
	    if ( parity.second - parity.first != NROOTS ) {
	        EZPWD_RAISE_OR_RETURN( std::runtime_error, "reed-solomon: parity length incompatible with number of roots", -1 );
	    }
	    return encode_mask( data.first, data.second - data.first, parity.first );
	}

	template < typename INP >
	int			encode_mask(
				    const INP	       *data,
				    int			len,
				    INP		       *parity )	// pointer to all NROOTS parity symbols

	    const
	{
	    if ( len < 1 ) {
	        EZPWD_RAISE_OR_RETURN( std::runtime_error, "reed-solomon: must provide space for all parity and at least one non-parity symbol", -1 );
	    }

	    const TYP       	       *dataptr;
	    TYP			       *pariptr;
	    const size_t		INPUT	= 8 * sizeof ( INP );

	    if ( DATUM != SYMBOL || DATUM != INPUT ) {
		// Our DATUM (TYP) size (eg. uint8_t ==> 8, uint16_t ==> 16, uint32_t ==> 32)
		// doesn't exactly match our R-S SYMBOL size (eg. 6), or our INP size; Must mask and
		// copy.  The INP data must fit at least the SYMBOL size!
		if ( SYMBOL > INPUT ) {
		    EZPWD_RAISE_OR_RETURN( std::runtime_error, "reed-solomon: output data type too small to contain symbols", -1 );
		}
		std::array<TYP,SIZE>	tmp;
		TYP			msk	= static_cast<TYP>( ~0UL << SYMBOL );
		for ( int i = 0; i < len; ++i )
		    tmp[LOAD - len + i]		= data[i] & ~msk;
		dataptr				= &tmp[LOAD - len];
		pariptr				= &tmp[LOAD];

		encode( dataptr, len, pariptr );

		// we copied/masked data; copy the parity symbols back (may be different sizes)
		for ( int i = 0; i < NROOTS; ++i )
		    parity[i]			= pariptr[i];
	    } else {
		// Our R-S SYMBOL size, DATUM size and INP type size exactly matches; use in-place.
		dataptr				= reinterpret_cast<const TYP *>( data );
		pariptr				= reinterpret_cast<TYP *>( parity );

		encode( dataptr, len, pariptr );
	    }
	    return NROOTS;
	}
	    
	using reed_solomon_base::decode;
	virtual int		decode(
				    const std::pair<uint8_t *, uint8_t *>
						       &data,
				    const std::vector<int>
				    		       &erasure	= std::vector<int>(),
				    std::vector<int>   *position= 0 )
	    const
	{
	    return decode_mask( data.first, data.second - data.first, (uint8_t *)0,
				erasure, position );
	}

	virtual int		decode(
				    const std::pair<uint8_t *, uint8_t *>
						       &data,
				    const std::pair<uint8_t *, uint8_t *>
						       &parity,
				    const std::vector<int>
				    		       &erasure	= std::vector<int>(),
				    std::vector<int>   *position= 0 )
	    const
	{
	    if ( parity.second - parity.first != NROOTS ) {
	        EZPWD_RAISE_OR_RETURN( std::runtime_error, "reed-solomon: parity length incompatible with number of roots", -1 );
	    }
	    return decode_mask( data.first, data.second - data.first, parity.first,
				erasure, position );
	}

	virtual int		decode(
				    const std::pair<uint16_t *, uint16_t *>
						       &data,
				    const std::vector<int>
				    		       &erasure	= std::vector<int>(),
				    std::vector<int>   *position= 0 )
	    const
	{
	    return decode_mask( data.first, data.second - data.first, (uint16_t *)0,
				erasure, position );
	}

	virtual int		decode(
				    const std::pair<uint16_t *, uint16_t *>
						       &data,
				    const std::pair<uint16_t *, uint16_t *>
						       &parity,
				    const std::vector<int>
				    		       &erasure	= std::vector<int>(),
				    std::vector<int>   *position= 0 )
	    const
	{
	    if ( parity.second - parity.first != NROOTS ) {
		EZPWD_RAISE_OR_RETURN( std::runtime_error, "reed-solomon: parity length incompatible with number of roots", -1 );
	    }
	    return decode_mask( data.first, data.second - data.first, parity.first,
				erasure, position );
	}

	virtual int		decode(
				    const std::pair<uint32_t *, uint32_t *>
						       &data,
				    const std::vector<int>
				    		       &erasure	= std::vector<int>(),
				    std::vector<int>   *position= 0 )
	    const
	{
	    return decode_mask( data.first, data.second - data.first, (uint32_t *)0,
				erasure, position );
	}

	virtual int		decode(
				    const std::pair<uint32_t *, uint32_t *>
						       &data,
				    const std::pair<uint32_t *, uint32_t *>
						       &parity,
				    const std::vector<int>
				    		       &erasure	= std::vector<int>(),
				    std::vector<int>   *position= 0 )
	    const
	{
	    if ( parity.second - parity.first != NROOTS ) {
	        EZPWD_RAISE_OR_RETURN( std::runtime_error, "reed-solomon: parity length incompatible with number of roots", -1 );
	    }
	    return decode_mask( data.first, data.second - data.first, parity.first,
				erasure, position );
	}

	// 
	// decode_mask	-- mask INP data into valid SYMBOL data
	// 
	///     Incoming data may be in a variety of sizes, and may contain information beyond the
	/// R-S symbol capacity.  For example, we might use a 6-bit R-S symbol to correct the lower
	/// 6 bits of an 8-bit data character.  This would allow us to correct common substitution
	/// errors (such as '2' for '3', 'R' for 'T', 'n' for 'm').
	/// 
	template < typename INP >
	int			decode_mask(
				    INP		       *data,
				    int			len,
				    INP		       *parity	= 0,	// either 0, or pointer to all parity symbols
				    const std::vector<int>
				    		       &erasure	= std::vector<int>(),
				    std::vector<int>   *position= 0 )
	    const
	{
	    if ( len < ( parity ? 0 : NROOTS ) + 1 ) {
	        EZPWD_RAISE_OR_RETURN( std::runtime_error, "reed-solomon: must provide all parity and at least one non-parity symbol", -1 );
	    }
	    if ( ! parity ) {
		len			       -= NROOTS;
		parity				= data + len;
	    }

	    TYP		       	       *dataptr;
	    TYP			       *pariptr;
	    const size_t		INPUT	= 8 * sizeof ( INP );

	    std::array<TYP,SIZE>	tmp;
	    TYP				msk	= static_cast<TYP>( ~0UL << SYMBOL );
	    const bool			cpy	= DATUM != SYMBOL || DATUM != INPUT;
	    if ( cpy ) {
		// Our DATUM (TYP) size (eg. uint8_t ==> 8, uint16_t ==> 16, uint32_t ==> 32)
		// doesn't exactly match our R-S SYMBOL size (eg. 6), or our INP size; Must copy.
		// The INP data must fit at least the SYMBOL size!
		if ( SYMBOL > INPUT ) {
		    EZPWD_RAISE_OR_RETURN( std::runtime_error, "reed-solomon: input data type too small to contain symbols", -1 );
		}
		for ( int i = 0; i < len; ++i ) {
		    tmp[LOAD - len + i]		= data[i] & ~msk;
		}
		dataptr				= &tmp[LOAD - len];
		for ( int i = 0; i < NROOTS; ++i ) {
		    if ( TYP( parity[i] ) & msk ) {
		        EZPWD_RAISE_OR_RETURN( std::runtime_error, "reed-solomon: parity data contains information beyond R-S symbol size", -1 );
		    }
		    tmp[LOAD + i]		= parity[i];
		}
		pariptr				= &tmp[LOAD];
	    } else {
		// Our R-S SYMBOL size, DATUM size and INPUT type sizes exactly matches
		dataptr				= reinterpret_cast<TYP *>( data );
		pariptr				= reinterpret_cast<TYP *>( parity );
	    }

	    int			corrects;
	    if ( ! erasure.size() && ! position ) {
		// No erasures, and error position info not wanted.
		corrects			= decode( dataptr, len, pariptr );
	    } else {
		// Either erasure location info specified, or resultant error position info wanted;
		// Prepare pos (a temporary, if no position vector provided), and copy any provided
		// erasure positions.  After number of corrections is known, resize the position
		// vector.  Thus, we use any supplied erasure info, and optionally return any
		// correction position info separately.
		std::vector<int>       _pos;
		std::vector<int>       &pos	= position ? *position : _pos;
		pos.resize( std::max( size_t( NROOTS ), erasure.size() ));
		std::copy( erasure.begin(), erasure.end(), pos.begin() );
		corrects			= decode( dataptr, len, pariptr,
							  &pos.front(), erasure.size() );
		if ( corrects > int( pos.size() )) {
		    EZPWD_RAISE_OR_RETURN( std::runtime_error, "reed-solomon: FATAL: produced too many corrections; possible corruption!", -1 );
		}
		pos.resize( std::max( 0, corrects ));
	    }

	    if ( cpy && corrects > 0 ) {
		for ( int i = 0; i < len; ++i ) {
		    data[i]		       &= msk;
		    data[i]		       |= tmp[LOAD - len + i];
		}
		for ( int i = 0; i < NROOTS; ++i ) {
		    parity[i]			= tmp[LOAD + i];
		}
	    }
	    return corrects;
	}

	virtual		       ~reed_solomon()
	{
	    ;
	}
				reed_solomon()
				    : reed_solomon_tabs<TYP, SYM, PRM, PLY>()
	{
	    // We check one element of the array; this is safe, 'cause the value will not be
	    // initialized 'til the initializing thread has completely initialized the array.  Worst
	    // case scenario: multiple threads will initialize identically.  No mutex necessary.
	    if ( genpoly[0] )
		return;

#if defined( DEBUG ) && DEBUG >= 2
	    std::cout << "RS(" << SIZE << "," << LOAD << "): Initialize for " << NROOTS << " roots." << std::endl;
#endif
	    std::array<TYP, NROOTS + 1>
				tmppoly; // uninitialized
	    // Form RS code generator polynomial from its roots.  Only lower-index entries are
	    // consulted, when computing subsequent entries; only index 0 needs initialization.
	    tmppoly[0]			= 1;
	    for ( int i = 0, root = FCR * PRM; i < NROOTS; i++, root += PRM ) {
		tmppoly[i + 1]		= 1;
		// Multiply tmppoly[] by  @**(root + x)
		for ( int j = i; j > 0; j-- ) {
		    if ( tmppoly[j] != 0 )
			tmppoly[j]	= tmppoly[j - 1]
			    ^ alpha_to[modnn(index_of[tmppoly[j]] + root)];
		    else
			tmppoly[j]	= tmppoly[j - 1];
		}
		// tmppoly[0] can never be zero
		tmppoly[0]		= alpha_to[modnn(index_of[tmppoly[0]] + root)];
	    }
	    // convert NROOTS entries of tmppoly[] to genpoly[] in index form for quicker encoding,
	    // in reverse order so genpoly[0] is last element initialized.
	    for ( int i = NROOTS; i >= 0; --i )
		genpoly[i]		= index_of[tmppoly[i]];
	}

	int			encode(
				    const TYP	       *data,
				    int			len,
				    TYP		       *parity ) // at least nroots
	    const
	{
	    // Check length parameter for validity
	    int			pad	= NN - NROOTS - len;
	    if ( pad < 0 || pad >= NN ) {
	        EZPWD_RAISE_OR_RETURN( std::runtime_error, "reed-solomon: data length incompatible with block size and error correction symbols", -1 );
	    }
	    for ( int i = 0; i < NROOTS; i++ )
		parity[i]		= 0;
	    for ( int i = 0; i < len; i++ ) {
		TYP		feedback= index_of[data[i] ^ parity[0]];
		if ( feedback != A0 )
		    for ( int j = 1; j < NROOTS; j++ )
			parity[j]      ^= alpha_to[modnn(feedback + genpoly[NROOTS - j])];

		std::rotate( parity, parity + 1, parity + NROOTS );
		if ( feedback != A0 )
		    parity[NROOTS - 1]	= alpha_to[modnn(feedback + genpoly[0])];
		else
		    parity[NROOTS - 1]	= 0;
	    }
#if defined( DEBUG ) && DEBUG >= 2
	    std::cout << *this << " encode " << std::vector<TYP>( data, data + len )
		      << " --> " << std::vector<TYP>( parity, parity + NROOTS ) << std::endl;
#endif
	    return NROOTS;
	}

	int			decode(
				    TYP		       *data,
				    int			len,
				    TYP		       *parity,		// Requires: at least NROOTS
				    int		       *eras_pos= 0,	// Capacity: at least NROOTS
				    int			no_eras	= 0,	// Maximum:  at most  NROOTS
				    TYP		       *corr	= 0 )	// Capacity: at least NROOTS
	    const
	{
	    typedef std::array< TYP, NROOTS >
				typ_nroots;
	    typedef std::array< TYP, NROOTS+1 >
				typ_nroots_1;
	    typedef std::array< int, NROOTS >
				int_nroots;

	    typ_nroots_1	lambda  { { 0 } };
	    typ_nroots		syn;
	    typ_nroots_1	b;
	    typ_nroots_1	t;
	    typ_nroots_1	omega;
	    int_nroots		root;
	    typ_nroots_1	reg;
	    int_nroots		loc;
	    int			count	= 0;

	    // Check length parameter and erasures for validity
	    int			pad	= NN - NROOTS - len;
	    if ( pad < 0 || pad >= NN ) {
	        EZPWD_RAISE_OR_RETURN( std::runtime_error, "reed-solomon: data length incompatible with block size and error correction symbols", -1 );
	    }
	    if ( no_eras ) {
		if ( no_eras > NROOTS ) {
		    EZPWD_RAISE_OR_RETURN( std::runtime_error, "reed-solomon: number of erasures exceeds capacity (number of roots)", -1 );
		}
		for ( int i = 0; i < no_eras; ++i ) {
		    if ( eras_pos[i] < 0 || eras_pos[i] >= len + NROOTS ) {
		        EZPWD_RAISE_OR_RETURN( std::runtime_error, "reed-solomon: erasure positions outside data+parity", -1 );
		    }
		}
	    }

	    // form the syndromes; i.e., evaluate data(x) at roots of g(x)
	    for ( int i = 0; i < NROOTS; i++ )
		syn[i]			= data[0];

	    for ( int j = 1; j < len; j++ ) {
		for ( int i = 0; i < NROOTS; i++ ) {
		    if ( syn[i] == 0 ) {
			syn[i]		= data[j];
		    } else {
			syn[i]		= data[j]
			    ^ alpha_to[modnn(index_of[syn[i]] + ( FCR + i ) * PRM)];
		    }
		}
	    }

	    for ( int j = 0; j < NROOTS; j++ ) {
		for ( int i = 0; i < NROOTS; i++ ) {
		    if ( syn[i] == 0 ) {
			syn[i]		= parity[j];
		    } else {
			syn[i] 		= parity[j]
			    ^ alpha_to[modnn(index_of[syn[i]] + ( FCR + i ) * PRM)];
		    }
		}
	    }

	    // Convert syndromes to index form, checking for nonzero condition
	    TYP 		syn_error = 0;
	    for ( int i = 0; i < NROOTS; i++ ) {
		syn_error	       |= syn[i];
		syn[i]			= index_of[syn[i]];
	    }

	    int			deg_lambda = 0;
	    int			deg_omega = 0;
	    int			r	= no_eras;
	    int			el	= no_eras;
	    if ( ! syn_error ) {
		// if syndrome is zero, data[] is a codeword and there are no errors to correct.
		count			= 0;
		goto finish;
	    }

	    lambda[0] 			= 1;
	    if ( no_eras > 0 ) {
		// Init lambda to be the erasure locator polynomial.  Convert erasure positions
		// from index into data, to index into Reed-Solomon block.
		lambda[1]		= alpha_to[modnn(PRM * (NN - 1 - ( eras_pos[0] + pad )))];
		for ( int i = 1; i < no_eras; i++ ) {
		    TYP		u	= modnn(PRM * (NN - 1 - ( eras_pos[i] + pad )));
		    for ( int j = i + 1; j > 0; j-- ) {
			TYP	tmp	= index_of[lambda[j - 1]];
			if ( tmp != A0 ) {
			    lambda[j]  ^= alpha_to[modnn(u + tmp)];
			}
		    }
		}
	    }

#if DEBUG >= 1
	    // Test code that verifies the erasure locator polynomial just constructed
	    // Needed only for decoder debugging.
    
	    // find roots of the erasure location polynomial
	    for( int i = 1; i<= no_eras; i++ )
		reg[i]			= index_of[lambda[i]];

	    count			= 0;
	    for ( int i = 1, k = iprim - 1; i <= NN; i++, k = modnn( k + iprim )) {
		TYP		q	= 1;
		for ( int j = 1; j <= no_eras; j++ ) {
		    if ( reg[j] != A0 ) {
			reg[j]		= modnn( reg[j] + j );
			q	       ^= alpha_to[reg[j]];
		    }
		}
		if ( q != 0 )
		    continue;
		// store root and error location number indices
		root[count]		= i;
		loc[count]		= k;
		count++;
	    }
	    if ( count != no_eras ) {
		std::cout << "ERROR: count = " << count << ", no_eras = " << no_eras 
			  << "lambda(x) is WRONG"
			  << std::endl;
		count			= -1;
		goto finish;
	    }
#if DEBUG >= 2
	    if ( count ) {
	        std::cout
		    << "Erasure positions as determined by roots of Eras Loc Poly: ";
		for ( int i = 0; i < count; i++ )
		    std::cout << loc[i] << ' ';
		std::cout << std::endl;
	        std::cout
		    << "Erasure positions as determined by roots of eras_pos array: ";
		for ( int i = 0; i < no_eras; i++ )
		    std::cout << eras_pos[i] << ' ';
		std::cout << std::endl;
	    }
#endif
#endif

	    for ( int i = 0; i < NROOTS + 1; i++ )
		b[i]			= index_of[lambda[i]];

	    //
	    // Begin Berlekamp-Massey algorithm to determine error+erasure locator polynomial
	    //
	    while ( ++r <= NROOTS ) { // r is the step number
		// Compute discrepancy at the r-th step in poly-form
		TYP		discr_r	= 0;
		for ( int i = 0; i < r; i++ ) {
		    if (( lambda[i] != 0 ) && ( syn[r - i - 1] != A0 )) {
			discr_r	       ^= alpha_to[modnn(index_of[lambda[i]] + syn[r - i - 1])];
		    }
		}
		discr_r			= index_of[discr_r];	// Index form
		if ( discr_r == A0 ) {
		    // 2 lines below: B(x) <-- x*B(x)
		    // Rotate the last element of b[NROOTS+1] to b[0]
		    std::rotate( b.begin(), b.begin()+NROOTS, b.end() );
		    b[0]		= A0;
		} else {
		    // 7 lines below: T(x) <-- lambda(x)-discr_r*x*b(x)
		    t[0]		= lambda[0];
		    for ( int i = 0; i < NROOTS; i++ ) {
			if ( b[i] != A0 ) {
			    t[i + 1]	= lambda[i + 1]
				^ alpha_to[modnn(discr_r + b[i])];
			} else
			    t[i + 1]	 = lambda[i + 1];
		    }
		    if ( 2 * el <= r + no_eras - 1 ) {
			el		= r + no_eras - el;
			// 2 lines below: B(x) <-- inv(discr_r) * lambda(x)
			for ( int i = 0; i <= NROOTS; i++ ) {
			    b[i]	= ((lambda[i] == 0)
					   ? A0
					   : modnn(index_of[lambda[i]] - discr_r + NN));
			}
		    } else {
			// 2 lines below: B(x) <-- x*B(x)
			std::rotate( b.begin(), b.begin()+NROOTS, b.end() );
			b[0]		= A0;
		    }
		    lambda		= t;
		}
	    }

	    // Convert lambda to index form and compute deg(lambda(x))
	    for ( int i = 0; i < NROOTS + 1; i++ ) {
		lambda[i]		= index_of[lambda[i]];
		if ( lambda[i] != NN )
		    deg_lambda		= i;
	    }
	    // Find roots of error+erasure locator polynomial by Chien search
	    reg				= lambda;
	    count			= 0; // Number of roots of lambda(x)
	    for ( int i = 1, k = iprim - 1; i <= NN; i++, k = modnn( k + iprim )) {
		TYP		q	= 1; // lambda[0] is always 0
		for ( int j = deg_lambda; j > 0; j-- ) {
		    if ( reg[j] != A0 ) {
			reg[j]		= modnn( reg[j] + j );
			q	       ^= alpha_to[reg[j]];
		    }
		}
		if ( q != 0 )
		    continue; // Not a root
		// store root (index-form) and error location number
#if DEBUG >= 2
		std::cout << "count " << count << " root " << i << " loc " << k << std::endl;
#endif
		root[count]		= i;
		loc[count]		= k;
		// If we've already found max possible roots, abort the search to save time
		if ( ++count == deg_lambda )
		    break;
	    }
	    if ( deg_lambda != count ) {
		// deg(lambda) unequal to number of roots => uncorrectable error detected
		count			= -1;
		goto finish;
	    }
	    //
	    // Compute err+eras evaluator poly omega(x) = s(x)*lambda(x) (modulo x**NROOTS). in
	    // index form. Also find deg(omega).
	    //
	    deg_omega 			= deg_lambda - 1;
	    for ( int i = 0; i <= deg_omega; i++ ) {
		TYP		tmp	= 0;
		for ( int j = i; j >= 0; j-- ) {
		    if (( syn[i - j] != A0 ) && ( lambda[j] != A0 ))
			tmp	       ^= alpha_to[modnn(syn[i - j] + lambda[j])];
		}
		omega[i]		= index_of[tmp];
	    }

	    //
	    // Compute error values in poly-form. num1 = omega(inv(X(l))), num2 = inv(X(l))**(fcr-1)
	    // and den = lambda_pr(inv(X(l))) all in poly-form
	    //
	    for ( int j = count - 1; j >= 0; j-- ) {
		TYP		num1	= 0;
		for ( int i = deg_omega; i >= 0; i-- ) {
		    if ( omega[i] != A0 )
			num1	       ^= alpha_to[modnn(omega[i] + i * root[j])];
		}
		TYP		num2	= alpha_to[modnn(root[j] * ( FCR - 1 ) + NN)];
		TYP		den	= 0;

		// lambda[i+1] for i even is the formal derivative lambda_pr of lambda[i]
		for ( int i = std::min(deg_lambda, NROOTS - 1) & ~1; i >= 0; i -= 2 ) {
		    if ( lambda[i + 1] != A0 ) {
			den	       ^= alpha_to[modnn(lambda[i + 1] + i * root[j])];
		    }
		}
#if defined( DEBUG ) && DEBUG >= 1
		if ( den == 0 ) {
		    std::cout << "ERROR: denominator = 0" << std::endl;
		    count		= -1;
		    goto finish;
		}
#endif
		// Apply error to data.  Padding ('pad' unused symbols) begin at index 0.
		if ( num1 != 0 ) {
		    if ( loc[j] < pad ) {
			// If the computed error position is in the 'pad' (the unused portion of the
			// R-S data capacity), then our solution has failed -- we've computed a
			// correction location outside of the data and parity we've been provided!
#if DEBUG >= 2
			std::cout
			    << "ERROR: RS(" << SIZE <<"," << LOAD
			    << ") computed error location: " << loc[j]
			    << " within " << pad << " pad symbols, not within "
			    << LOAD - pad << " data or " << NROOTS << " parity"
			    << std::endl;
#endif
			count 		= -1;
			goto finish;
		    }

		    TYP		cor	= alpha_to[modnn(index_of[num1]
							 + index_of[num2]
							 + NN - index_of[den])];
		    // Store the error correction pattern, if a correction buffer is available
		    if ( corr )
			corr[j] 	= cor;
		    // If a data/parity buffer is given and the error is inside the message or
		    // parity data, correct it
		    if ( loc[j] < ( NN - NROOTS )) {
			if ( data ) {
			    data[loc[j] - pad] ^= cor;
			}
		    } else if ( loc[j] < NN ) {
		        if ( parity )
		            parity[loc[j] - ( NN - NROOTS )] ^= cor;
		    }
		}
	    }

	finish:
#if defined( DEBUG ) && DEBUG > 0
	    if ( count > NROOTS )
		std::cout << "ERROR: Number of corrections: " << count << " exceeds NROOTS: " << NROOTS << std::endl;
#endif
#if defined( DEBUG ) && DEBUG > 1
	    std::cout << "data      x" << std::setw( 3 ) << len    << ": " << std::vector<uint8_t>( data, data + len ) << std::endl;
	    std::cout << "parity    x" << std::setw( 3 ) << NROOTS << ": " << std::string(  len * 2, ' ' ) << std::vector<uint8_t>( parity, parity + NROOTS ) << std::endl;
	    if ( count > 0 ) {
		std::string errors( 2 * ( len + NROOTS ), ' ' );
		for ( int i = 0; i < count; ++i ) {
		    errors[2*(loc[i]-pad)+0] = 'E';
		    errors[2*(loc[i]-pad)+1] = 'E';
		}
		for ( int i = 0; i < no_eras; ++i ) {
		    errors[2*(eras_pos[i])+0] = 'e';
		    errors[2*(eras_pos[i])+1] = 'e';
		}
		std::cout << "e)ra,E)rr x" << std::setw( 3 ) << count  << ": " << errors << std::endl;
	    }
#endif
	    if ( eras_pos != NULL ) {
		for ( int i = 0; i < count; i++)
		    eras_pos[i]		= loc[i] - pad;
	    }
	    return count;
	}
    }; // class reed_solomon

    // 
    // Define the static reed_solomon...<...> members; allowed in header for template types.
    // 
    //     The reed_solomon_tags<...>::iprim < 0 is used to indicate to the first instance that the
    // static tables require initialization.
    // 
    template < typename TYP, int SYM, int PRM, class PLY >
        int				reed_solomon_tabs< TYP, SYM, PRM, PLY >::iprim = -1;

    template < typename TYP, int SYM, int PRM, class PLY >
        std::array< TYP, reed_solomon_tabs< TYP, SYM, PRM, PLY >
#if not defined( EZPWD_ARRAY_TEST )
                                                                          ::NN + 1 >
#else
#  warning "EZPWD_ARRAY_TEST: Erroneously defining alpha_to size!"
                                                                          ::NN     >
#endif
					reed_solomon_tabs< TYP, SYM, PRM, PLY >::alpha_to;

    template < typename TYP, int SYM, int PRM, class PLY >
        std::array< TYP, reed_solomon_tabs< TYP, SYM, PRM, PLY >::NN + 1 >
					reed_solomon_tabs< TYP, SYM, PRM, PLY >::index_of;
    template < typename TYP, int SYM, int PRM, class PLY >
        std::array< TYP, reed_solomon_tabs< TYP, SYM, PRM, PLY >::MODS >
					reed_solomon_tabs< TYP, SYM, PRM, PLY >::mod_of;

    template < typename TYP, int SYM, int RTS, int FCR, int PRM, class PLY >
        std::array< TYP, reed_solomon< TYP, SYM, RTS, FCR, PRM, PLY >::NROOTS + 1 >
					reed_solomon< TYP, SYM, RTS, FCR, PRM, PLY >::genpoly;

} // namespace ezpwd
    
#endif // _EZPWD_RS_BASE
