/* -*- c++ -*- */
/* 
 * Copyright 2018 Graham J. Norbury
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef INCLUDED_LOG_TS_H
#define INCLUDED_LOG_TS_H

#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <string>
#include <vector>

// helper for logging support
inline std::string uint8_vector_to_hex_string(const std::vector<uint8_t>& v)
{
    std::string result;
    result.reserve(v.size() * 2);   // two digits per character

    static constexpr char hex[] = "0123456789ABCDEF";

    for (uint8_t c : v)
    {
        result.push_back(hex[c / 16]);
        result.push_back(hex[c % 16]);
    }

    return result;
}

class log_ts
{
private:
	struct timeval curr_time, marker_time;
	struct tm curr_loc_time;
	double tstamp;
	char log_tstring[40];

public:
	inline log_ts()
	{
		if (gettimeofday(&curr_time, 0) == 0)
		{
			memcpy(&marker_time, &curr_time, sizeof(struct timeval));
		}
		else
		{
			memset(&curr_time, 0, sizeof(struct timeval));
			memset(&marker_time, 0, sizeof(struct timeval));
		}
	}

	inline const char* get()
	{
		if (gettimeofday(&curr_time, 0) == 0)
		{
			localtime_r(&curr_time.tv_sec, &curr_loc_time);
			size_t i = strftime(log_tstring, sizeof(log_tstring), "%m/%d/%y %H:%M:%S", &curr_loc_time);
			if (i > 0)
				sprintf((log_tstring + i), ".%06lu", curr_time.tv_usec);
			else
				sprintf(log_tstring, "%010lu.%06lu", curr_time.tv_sec, curr_time.tv_usec);
		}
		else
		{
			log_tstring[0] = 0;
		}

		return log_tstring;
	}	

	inline const char* get(const int id)
	{
		if (gettimeofday(&curr_time, 0) == 0)
		{
			localtime_r(&curr_time.tv_sec, &curr_loc_time);
			size_t i = strftime(log_tstring, sizeof(log_tstring), "%m/%d/%y %H:%M:%S", &curr_loc_time);
			if (i > 0)
				sprintf((log_tstring + i), ".%06lu [%d]", curr_time.tv_usec, id);
			else
				sprintf(log_tstring, "%010lu.%06lu [%d]", curr_time.tv_sec, curr_time.tv_usec, id);
		}
		else
		{
			sprintf(log_tstring, "[%d]", id);
		}

		return log_tstring;
	}

	inline double get_ts()
	{
		if (gettimeofday(&curr_time, 0) == 0)
			tstamp = curr_time.tv_sec + (curr_time.tv_usec / 1e6);
		else
			tstamp = 0;

		return tstamp;
	}

	inline void mark_ts()
	{
		memcpy(&marker_time, &curr_time, sizeof(struct timeval));
	}

	inline double get_tdiff()
	{
        time_t n_sec = curr_time.tv_sec - marker_time.tv_sec;
        long int n_usec = curr_time.tv_usec - marker_time.tv_usec;
		tstamp = n_sec + (n_usec / 1e6);
		return tstamp;
	}
};

#endif // INCLUDED_LOG_TS_H
