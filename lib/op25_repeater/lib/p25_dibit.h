/* -*- c++ -*- */
/* 
 * Copyright 2022 Graham J. Norbury
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

#ifndef INCLUDED_P25_DIBIT_H
#define INCLUDED_P25_DIBIT_H

#include <stdint.h>
#include "log_ts.h"
#include "frame_sync_magics.h"

class p25_dibit
{
    // internal instance variables and state
    private:
        int d_debug;
        int d_msgq_id;
        uint8_t fs_map_idx;
        log_ts logts;
        static const uint8_t  fs_table_len           = 10;
        const uint64_t fs_table[fs_table_len]        = {P25_FRAME_SYNC_MAGIC,
                                                        P25P2_FRAME_SYNC_MAGIC,
                                                        P25_FRAME_SYNC_REV_P,
                                                        P25P2_FRAME_SYNC_REV_P,
                                                        P25_FRAME_SYNC_X2400,
                                                        P25P2_FRAME_SYNC_X2400,
                                                        P25_FRAME_SYNC_N1200,
                                                        P25P2_FRAME_SYNC_N1200,
                                                        P25_FRAME_SYNC_P1200,
                                                        P25P2_FRAME_SYNC_P1200};

        const uint8_t  fs_dibit_map[fs_table_len][4] = {{0,1,2,3},
                                                        {0,1,2,3},
                                                        {2,3,0,1},
                                                        {2,3,0,1},
                                                        {3,2,1,0},
                                                        {3,2,1,0},
                                                        {1,3,0,2},
                                                        {1,3,0,2},
                                                        {2,0,3,1},
                                                        {2,0,3,1}};
    // public
    public:
        p25_dibit(int debug = 0, int msgq_id = 0) : d_debug(debug), d_msgq_id(msgq_id), fs_map_idx(0) { }
        ~p25_dibit() { }

        inline void set_debug(int debug) { d_debug = debug; }

        inline void set_fs_index(const uint64_t fs) {
            fs_map_idx = 0;
            for (uint8_t i = 0; i < fs_table_len; i++) {
                if (fs == fs_table[i]) {
                    fs_map_idx = i;
                    break;
                }
            }
            if ((d_debug >= 10) && (fs_map_idx != 0) && (fs_map_idx != 1)) {
                fprintf(stderr, "%s p25_dibit::set_fs_index(): fs_type=%d, fs=%012lx\n", logts.get(d_msgq_id), fs_map_idx, fs);
            }
        }

        inline void correct_dibits(uint8_t* syms, int nsyms) {
        for (int i = 0; i < nsyms; i++) {
            syms[i] = fs_dibit_map[fs_map_idx][syms[i] & 0x3];
            }
        }

        inline const uint8_t* dibit_map() { return fs_dibit_map[fs_map_idx]; }
};

#endif /* INCLUDED_P25_DIBIT_H  */
