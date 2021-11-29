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

#include "op25_timer.h"

op25_timer::op25_timer(uint64_t duration) :
	d_duration(duration)
{
	reset();
}

op25_timer::~op25_timer()
{
}

void
op25_timer::reset()
{
	gettimeofday(&d_timer, 0);
}

bool
op25_timer::expired()
{
	struct timeval cur_time;
	gettimeofday(&cur_time, 0);

	int64_t diff_usec = cur_time.tv_usec - d_timer.tv_usec;
	int64_t diff_sec  = cur_time.tv_sec  - d_timer.tv_sec ;

	if (diff_usec < 0) {
		diff_usec += 1000000;
		diff_sec  -= 1;
	}
	diff_usec += diff_sec * 1000000;
	if ((uint64_t)diff_usec >= d_duration)
		return true;

	return false;
}
