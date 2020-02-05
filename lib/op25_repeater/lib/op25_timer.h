//
// Microsecond timer
// (C) Copyright 2019 Graham J. Norbury
// 
// This file is part of OP25
// 
// OP25 is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3, or (at your option)
// any later version.
// 
// OP25 is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public
// License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with OP25; see the file COPYING. If not, write to the Free
// Software Foundation, Inc., 51 Franklin Street, Boston, MA
// 02110-1301, USA.

#ifndef INCLUDED_OP25_TIMER_H
#define INCLUDED_OP25_TIMER_H

#include <sys/time.h>
#include <stdint.h>

class op25_timer {
public:
	op25_timer(const uint64_t duration);
	~op25_timer();
	void reset();
	bool expired();

private:
	struct timeval d_timer;
	uint64_t d_duration;
};

#endif /* INCLUDED_OP25_TIMER_H */
