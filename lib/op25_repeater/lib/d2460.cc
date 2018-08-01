#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <termios.h>
#include <string.h>
#include <fcntl.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/param.h>

#include <netinet/in.h>

#include <stdint.h>

#include <mbelib.h>
#include <ambe.h>
#include <p25p2_vf.h>
#include <imbe_decoder.h>
#include <software_imbe_decoder.h>
#include "imbe_vocoder/imbe_vocoder.h"
#include <ambe_encoder.h>

static const float GAIN_ADJUST=7.0;  /* attenuation (dB) */

typedef uint16_t Uns;
static const Uns RC_OK=0;

static const char prodid[] = "OP25  ";
static const char verstring[] = "1.0";

static ambe_encoder encoder;
static software_imbe_decoder software_decoder;
static p25p2_vf interleaver;
static mbe_parms cur_mp;
static mbe_parms prev_mp;
static mbe_parms enh_mp;

static const Uns DV3K_START_BYTE = 0x61;
enum
{
    DV3K_CONTROL_RATEP     = 0x0A,
    DV3K_CONTROL_CHANFMT   = 0x15,
    DV3K_CONTROL_PRODID    = 0x30,
    DV3K_CONTROL_VERSTRING = 0x31,
    DV3K_CONTROL_RESET     = 0x33,
    DV3K_CONTROL_READY     = 0x39
};
static const Uns DV3K_AMBE_FIELD_CHAND    = 0x01;
static const Uns DV3K_AMBE_FIELD_CMODE    = 0x02;
static const Uns DV3K_AMBE_FIELD_TONE     = 0x08;
static const Uns DV3K_AUDIO_FIELD_SPEECHD = 0x00;
static const Uns DV3K_AUDIO_FIELD_CMODE   = 0x02;

#pragma DATA_ALIGN(dstar_state, 2)
static Uns bitstream[72];

static Uns get_byte(Uns offset, Uns *p)
{
    Uns word = p[offset >> 1];
    return (offset & 1) ? (word >> 8) : (word & 0xff);
}

static void set_byte(Uns offset, Uns *p, Uns byte)
{
    p[offset >> 1] =
        (offset & 1) ? (byte << 8) | (p[offset >> 1] & 0xff)
                     : (p[offset >> 1] & 0xff00) | (byte & 0xff);
}

static Uns get_word(Uns offset, Uns *p)
{
    return get_byte(offset + 1, p) | (get_byte(offset, p) << 8);
}

static void set_word(Uns offset, Uns *p, Uns word)
{
    set_byte(offset, p, word >> 8);
    set_byte(offset + 1, p, word & 0xff);
}

static void set_cstring(Uns offset, Uns *p, const char *str)
{
    do
        set_byte(offset++, p, *str);
    while (*str++ != 0);
}

static Uns pkt_check_ratep(Uns offset, Uns *p)
{
    static const Uns ratep[] = {
        0x01, 0x30, 0x07, 0x63, 0x40, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x48 };
    Uns i;
    for (i = 0; i < sizeof(ratep); ++i)
        if (get_byte(offset + i, p) != ratep[i])
            return 0;
    return 1;
}

static void pack(Uns bits, Uns offset, Uns *p, Uns *bitstream)
{
    Uns i;
    Uns byte = 0;
    for (i = 0; i < bits; ++i)
    {
        byte |= bitstream[i] << (7 - (i & 7));
        if ((i & 7) == 7)
        {
            set_byte(offset++, p, byte);
            byte = 0;
        }
    }
    if (i & 7)
        set_byte(offset, p, byte);
}

static void unpack(Uns bits, Uns offset, Uns *bitstream, Uns *p)
{
    Uns i;
    Uns byte;
    for (i = 0; i < bits; ++i)
    {
        if ((i & 7) == 0)
            byte = get_byte(offset++, p);
        bitstream[i] = (byte >> (7 - (i & 7))) & 1;
    }
}

static int response_len = -1;

static void bksnd(void*task, Uns bid, Uns len)
{
	response_len = len;
}

static void vocoder_setup(void) {
	encoder.set_dstar_mode();
	encoder.set_gain_adjust(GAIN_ADJUST);
	encoder.set_alt_dstar_interleave(true);
	mbe_initMbeParms (&cur_mp, &prev_mp, &enh_mp);
}

static void dump(unsigned char *p, ssize_t n)
{
    int i;
    for (i = 0; i < n; ++i)
        printf("%02x%c", p[i], i % 16 == 15 ? '\n' : ' ');
    if (i % 16)
        printf("\n");
}

static Uns pkt_process(Uns*pkt, Uns cnt)
{
    Uns bid=0;
    Uns len = cnt << 1;
    Uns payload_length;
    Uns i;
    Uns cmode = 0;
    Uns tone = 0;
    uint8_t codeword[72];
    int b[9];
    int K;
    int rc = -1;

    if (len < 4 || cnt > 256)
        goto fail;

    if (get_byte(0, pkt) != DV3K_START_BYTE)
        goto fail;

    payload_length = get_word(1, pkt);
    if (payload_length == 0)
        goto fail;
    if (4 + payload_length > len)
        goto fail;

    switch (get_byte(3, pkt))
    {
    case 0:
        switch (get_byte(4, pkt))
        {
        case DV3K_CONTROL_RATEP:
            if (payload_length != 13)
                goto fail;
            if (!pkt_check_ratep(5, pkt))
                goto fail;
            set_word(1, pkt, 1);
            bksnd(NULL, bid, 3);
            return RC_OK;
        case DV3K_CONTROL_CHANFMT:
            if (payload_length != 3)
                goto fail;
            if (get_word(5, pkt) != 0x0001)
                goto fail;
            set_word(1, pkt, 2);
            set_byte(5, pkt, 0);
            bksnd(NULL, bid, 3);
            return RC_OK;
        case DV3K_CONTROL_PRODID:
            set_word(1, pkt, 8);
            set_cstring(5, pkt, prodid);
            bksnd(NULL, bid, 6);
            return RC_OK;
        case DV3K_CONTROL_VERSTRING:
            set_word(1, pkt, 5);
            set_cstring(5, pkt, verstring);
            bksnd(NULL, bid, 5);
            return RC_OK;
        case DV3K_CONTROL_RESET:
            if (payload_length != 1)
                goto fail;
            vocoder_setup();
            set_byte(4, pkt, DV3K_CONTROL_READY);
            bksnd(NULL, bid, 3);
            return RC_OK;
        default:
            goto fail;
        }
    case 1:
        switch (payload_length)
        {
        case 17:
            if (get_byte(18, pkt) != DV3K_AMBE_FIELD_TONE)
                goto fail;
            tone = get_word(19, pkt);
            /* FALLTHROUGH */
        case 14:
            if (get_byte(15, pkt) != DV3K_AMBE_FIELD_CMODE)
                goto fail;
            cmode = get_word(16, pkt);
            /* FALLTHROUGH */
        case 11:
            if (get_byte(4, pkt) != DV3K_AMBE_FIELD_CHAND)
                goto fail;
            if (get_byte(5, pkt) != 72)
                goto fail;
            unpack(72, 6, bitstream, pkt);
            break;
        default:
            goto fail;
        }

        for (i = 0; i < 72; i++) {
            codeword[i] = bitstream[i];
        }
        interleaver.decode_dstar(codeword, b, true);
        if (b[0] >= 120) {
            memset(6+(char*)pkt, 0, 320);   // silence
            // FIXME: add handling for tone case (b0=126)
        } else {
            rc = mbe_dequantizeAmbe2400Parms(&cur_mp, &prev_mp, b);
            printf("B\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n", b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7], b[8]);
            K = 12;
            if (cur_mp.L <= 36)
                K = int(float(cur_mp.L + 2.0) / 3.0);
            software_decoder.decode_tap(cur_mp.L, K, cur_mp.w0, &cur_mp.Vl[1], &cur_mp.Ml[1]);
            audio_samples *samples = software_decoder.audio();
            int16_t snd;
            for (i=0; i < 160; i++) {
                    if (samples->size() > 0) {
                            snd = (int16_t)(samples->front());
                            samples->pop_front();
                    } else {
                            snd = 0;
                    }
                    set_word(6 + (i << 1), pkt, snd);
            }
            mbe_moveMbeParms (&cur_mp, &prev_mp);
        }

        set_word(1, pkt, 322);
        set_byte(3, pkt, 2);
        set_byte(4, pkt, DV3K_AUDIO_FIELD_SPEECHD);
        set_byte(5, pkt, 160);
        bksnd(NULL, bid, 165);
        return RC_OK;
    case 2:
        if (payload_length != 322 && payload_length != 325)
            goto fail;
        if (get_byte(4, pkt) != DV3K_AUDIO_FIELD_SPEECHD)
            goto fail;
        if (get_byte(5, pkt) != 160)
            goto fail;
        if (payload_length == 325)
        {
            if (get_byte(326, pkt) != DV3K_AUDIO_FIELD_CMODE)
                goto fail;
            cmode = get_word(323, pkt);
        }
        int16_t samples[160];
        for (i=0; i < 160; i++) {
                samples[i] = (int16_t) get_word(6 + (i << 1), pkt);
        }
        encoder.encode(samples, codeword);
        for (i = 0; i < 72; i++) {
            bitstream[i] = codeword[i];
        }
        set_word(1, pkt, 11);
        set_byte(3, pkt, 1);
        set_byte(4, pkt, DV3K_AMBE_FIELD_CHAND);
        set_byte(5, pkt, 72);
        pack(72, 6, pkt, bitstream);
        bksnd(NULL, bid, 8);
        return RC_OK;
    default:
        goto fail;
    }

fail:
    bksnd(NULL, bid, 0);
    return RC_OK;
}

int main()
{
    int sockfd;
    const ssize_t size_max = 1024;
    ssize_t size_in, size_out;
    char buf_in[size_max], buf_out[size_max];
    socklen_t length = sizeof(struct sockaddr_in);
    struct sockaddr_in sa = { 0 };
    Uns rc;

    vocoder_setup();

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        exit(2);

    sa.sin_family = AF_INET;
    sa.sin_port = htons(2460);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd, (struct sockaddr *)&sa, sizeof(sa)) < 0)
        exit(3);

    while (1)
    {
        if ((size_in = recvfrom(sockfd, buf_in, size_max,
            0, (struct sockaddr *)&sa, &length)) < 0)
            exit(4);

        if (size_in & 1)
            buf_in[size_in++] = 0;

        rc = pkt_process((Uns*)buf_in, size_in >> 1);
        if (response_len <= 0)
            exit(9);

        size_out = 4 + ntohs(*(short *)&buf_in[1]);
        if (sendto(sockfd, buf_in, size_out, 0, (struct sockaddr *)&sa,
            sizeof(struct sockaddr_in)) != size_out)
            exit(7);
    }

    return 0;
}
