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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netdb.h>

#include "op25_audio.h"

// convert hostname to ip address
static int hostname_to_ip(const char *hostname , char *ip)
{
    int sockfd;  
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_in *h;
    int rv;
 
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // use AF_INET6 to force IPv6
    hints.ai_socktype = SOCK_DGRAM;
 
    if ( (rv = getaddrinfo( hostname , NULL , &hints , &servinfo)) != 0) 
    {
        fprintf(stderr, "op25_audio::hostname_to_ip() getaddrinfo: %s\n", gai_strerror(rv));
        return -1;
    }
 
    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) 
    {
        h = (struct sockaddr_in *) p->ai_addr;
        if (h->sin_addr.s_addr != 0)
        {
            strcpy(ip , inet_ntoa( h->sin_addr ) );
            break;
        }
    }
     
    freeaddrinfo(servinfo); // all done with this structure
    return 0;

}

// constructor
op25_audio::op25_audio(const char* udp_host, int port, int debug) :
    d_udp_enabled(false),
    d_debug(debug),
    d_write_port(port),
    d_audio_port(port),
    d_write_sock(0),
    d_file_enabled(false)
{
    char ip[20];
    if (hostname_to_ip(udp_host, ip) == 0)
    {
        strncpy(d_udp_host, ip, sizeof(d_udp_host));
        d_udp_host[sizeof(d_udp_host)-1] = 0;
        if ( port )
            open_socket();
    }
}

// destructor
op25_audio::~op25_audio()
{
    if (d_file_enabled)
        close(d_write_sock);
    close_socket();
}

// constructor
op25_audio::op25_audio(const char* destination, int debug) :
    d_udp_enabled(false),
    d_debug(debug),
    d_write_port(0),
    d_audio_port(0),
    d_write_sock(0),
    d_file_enabled(false)
{
    static const int DEFAULT_UDP_PORT = 23456;
    static const char P_UDP[] = "udp://";
    static const char P_FILE[] = "file://";
    int port = DEFAULT_UDP_PORT;

    if (memcmp(destination, P_UDP, strlen(P_UDP)) == 0) {
        char ip[20];
        char host[64];
        const char * p1 = destination+strlen(P_UDP);
        strncpy(host, p1, sizeof(host));
        char * pc = index(host, ':');
        if (pc) {
            sscanf(pc+1, "%d", &port);
            *pc = 0;
        }
        if (hostname_to_ip(host, ip) == 0) {
            strncpy(d_udp_host, ip, sizeof(d_udp_host));
            d_udp_host[sizeof(d_udp_host)-1] = 0;
        d_write_port = d_audio_port = port;
        open_socket();
        }
    } else if (memcmp(destination, P_FILE, strlen(P_FILE)) == 0) {
        const char * filename = destination+strlen(P_FILE);
        size_t l = strlen(filename);
        if (l > 4 && (strcmp(&filename[l-4], ".wav") == 0 || strcmp(&filename[l-4], ".WAV") == 0)) {
		fprintf (stderr, "Warning! Output file %s will be written, but in raw form ***without*** a WAV file header!\n", filename);
	}
        d_write_sock = open(filename, O_WRONLY | O_CREAT, 0644);
        if (d_write_sock < 0) {
            fprintf(stderr, "op25_audio::open file %s: error: %d (%s)\n", filename, errno, strerror(errno));
            d_write_sock = 0;
            return;
        }
        d_file_enabled = true;
    }
}
// open socket and set up data structures
void op25_audio::open_socket()
{
    memset (&d_sock_addr, 0, sizeof(d_sock_addr));

    // open handle to socket
    d_write_sock = socket(PF_INET, SOCK_DGRAM, 17);   // UDP socket
    if ( d_write_sock < 0 )
    {
        fprintf(stderr, "op25_audio::open_socket(): error: %d\n", errno);
        d_write_sock = 0;
        return;
    }

    // set up data structure for generic udp host/port
    if ( !inet_aton(d_udp_host, &d_sock_addr.sin_addr) )
    {
        fprintf(stderr, "op25_audio::open_socket(): inet_aton: bad IP address\n");
        close(d_write_sock);
	d_write_sock = 0;
	return;
    }
    d_sock_addr.sin_family = AF_INET;

    fprintf(stderr, "op25_audio::open_socket(): enabled udp host(%s), wireshark(%d), audio(%d)\n", d_udp_host, d_write_port, d_audio_port);
    d_udp_enabled = true;
}

// close socket
void op25_audio::close_socket()
{
        if (!d_udp_enabled)
            return;
        close(d_write_sock);
        d_write_sock = 0;
        d_udp_enabled = false;
}

ssize_t op25_audio::do_send(const void * buf, size_t len, int port, bool is_ctrl ) const {
        ssize_t rc = 0;
        struct sockaddr_in tmp_sockaddr;
        if (len <= 0)
            return 0;
        if (d_udp_enabled) {
            memcpy(&tmp_sockaddr, &d_sock_addr, sizeof(struct sockaddr));
            tmp_sockaddr.sin_port = htons(port);
            rc = sendto(d_write_sock, buf, len, 0, (struct sockaddr *)&tmp_sockaddr, sizeof(struct sockaddr_in));
            if (rc == -1)
            {
                fprintf(stderr, "op25_audio::do_send(length %lu): error(%d): %s\n", len, errno, strerror(errno));
                rc = 0;
            }
        } else if (d_file_enabled && !is_ctrl) {
            size_t amt_written = 0;
            for (;;) {
                rc = write(d_write_sock, amt_written + (char*)buf, len - amt_written);
                if (rc < 0) {
                    fprintf(stderr, "op25_audio::write(length %lu): error(%d): %s\n", len, errno, strerror(errno));
                    rc = 0;
                } else if (rc == 0) {
                    fprintf(stderr, "op25_audio::write(length %lu): error, write rc zero\n", len);
                } else {
                    amt_written += rc;
                }
                if (rc <= 0 || amt_written >= len)
                    break;
            } /* end of for() */
            rc = amt_written;
        }
        return rc;
}

// send generic data to destination
ssize_t op25_audio::send_to(const void *buf, size_t len) const
{
    return do_send(buf, len, d_write_port, false);
}

// send audio data to destination
ssize_t op25_audio::send_audio(const void *buf, size_t len) const
{
    return do_send(buf, len, d_audio_port, false);
}

// send audio data on specifed channel to destination
ssize_t op25_audio::send_audio_channel(const void *buf, size_t len, ssize_t slot_id) const
{
    return do_send(buf, len, d_audio_port + slot_id*2, false);
}

// send flag to audio destination 
ssize_t op25_audio::send_audio_flag_channel(const udpFlagEnumType udp_flag, ssize_t slot_id) const
{
        char audio_flag[2];
        // 16 bit little endian encoding
        audio_flag[0] = (udp_flag & 0x00ff);
        audio_flag[1] = ((udp_flag & 0xff00) >> 8);
        return do_send(audio_flag, 2, d_audio_port+slot_id, true);
}

ssize_t op25_audio::send_audio_flag(const op25_audio::udpFlagEnumType udp_flag) const
{
        return send_audio_flag_channel(udp_flag, 0);
}
