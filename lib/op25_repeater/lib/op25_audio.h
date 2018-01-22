/* -*- c++ -*- */
/* 
 * Copyright 2017 Graham J Norbury, gnorbury@bondcar.com
 * from op25_audio; rewrite Nov 2017 Copyright 2017 Max H. Parke KA1RBI
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

#ifndef INCLUDED_OP25_AUDIO_H
#define INCLUDED_OP25_AUDIO_H

#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

class op25_audio
{
public:
    enum udpFlagEnumType
    {
        DRAIN = 0x0000,  // play queued pcm frames
        DROP  = 0x0001   // discard queued pcm frames
    };

private:
    bool        d_udp_enabled;
    int         d_debug;
    int         d_write_port;
    int         d_audio_port;
    char        d_udp_host[64];
    int         d_write_sock;
    bool        d_file_enabled;
    struct      sockaddr_in d_sock_addr;

    void open_socket();
    void close_socket();
    ssize_t do_send(const void * bufp, size_t len, int port, bool is_ctrl) const;

public:
    op25_audio(const char* udp_host, int port, int debug);
    op25_audio(const char* destination, int debug);
    ~op25_audio();

    inline bool enabled() const { return d_udp_enabled; }

    ssize_t send_to(const void *buf, size_t len) const;

    ssize_t send_audio(const void *buf, size_t len) const;
    ssize_t send_audio_flag(const udpFlagEnumType udp_flag) const;

    ssize_t send_audio_channel(const void *buf, size_t len, ssize_t slot_id) const;
    ssize_t send_audio_flag_channel(const udpFlagEnumType udp_flag, ssize_t slot_id) const;

}; // class op25_audio

#endif /* INCLUDED_OP25_AUDIO_H */
