/* -*- c++ -*- */
/*
 * Copyright 2004 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * GNU Radio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * GNU Radio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */
#ifndef INCLUDED_DSD_BLOCK_FF_H
#define INCLUDED_DSD_BLOCK_FF_H

#include <dsd/dsd_api.h>
#include <gnuradio/sync_decimator.h>

/*
extern "C"
{
  #include <dsd.h>
}
*/

#define HEURISTICS_SIZE 200
typedef struct
{
	int values[HEURISTICS_SIZE];
	float means[HEURISTICS_SIZE];
	int index;
	int count;
	float sum;
	float var_sum;
} SymbolHeuristics;

typedef struct
{
	unsigned int bit_count;
	unsigned int bit_error_count;
	SymbolHeuristics symbols[4][4];
} P25Heuristics;

typedef struct
{
	int value;
	int dibit;
	int corrected_dibit;
	int sequence_broken;
} AnalogSignal;

struct mbe_parameters
{
	float w0;
	int L;
	int K;
	int Vl[57];
	float Ml[57];
	float log2Ml[57];
	float PHIl[57];
	float PSIl[57];
	float gamma;
	int un;
	int repeat;
};

typedef struct mbe_parameters mbe_parms;

#define NZEROS 60 // because filter is static
#define NXZEROS 134

typedef struct
{
	int onesymbol;
	char mbe_in_file[1024];

	int errorbars;
	int datascope;
	int symboltiming;
	int verbose;
	int p25enc;
	int p25lc;
	int p25status;
	int p25tg;
	int scoperate;
	char audio_in_dev[1024];
	int audio_in_fd;

	int audio_in_type; // 0 for driver, 1 for file
	char audio_out_dev[1024];
	int audio_out_fd;

	int audio_out_type; // 0 for driver, 1 for file
	int split;
	int playoffset;
	char mbe_out_dir[1024];
	char mbe_out_file[1024];

	float audio_gain;
	int audio_out;
	char wav_out_file[1024];

	//int wav_out_fd;
	int serial_baud;
	char serial_dev[1024];
	int serial_fd;
	int resume;
	int frame_dstar;
	int frame_x2tdma;
	int frame_p25p1;
	int frame_nxdn48;
	int frame_nxdn96;
	int frame_dmr;
	int frame_provoice;
	int mod_c4fm;
	int mod_qpsk;
	int mod_gfsk;
	int uvquality;
	int inverted_x2tdma;
	int inverted_dmr;
	int mod_threshold;
	int ssize;
	int msize;
	int playfiles;
	int delay;
	int use_cosine_filter;
	int unmute_encrypted_p25;
} dsd_opts;

typedef struct
{
	int *dibit_buf;
	int *dibit_buf_p;
	int repeat;
	short *audio_out_buf;
	short *audio_out_buf_p;
	float *audio_out_float_buf;
	float *audio_out_float_buf_p;
	float audio_out_temp_buf[160];
	float *audio_out_temp_buf_p;
	int audio_out_idx;
	int audio_out_idx2;
	//int wav_out_bytes;
	int center;
	int jitter;
	int synctype;
	int min;
	int max;
	int lmid;
	int umid;
	int minref;
	int maxref;
	int lastsample;
	int sbuf[128];
	int sidx;
	int maxbuf[1024];
	int minbuf[1024];
	int midx;
	char err_str[64];
	char fsubtype[16];
	char ftype[16];
	int symbolcnt;
	int rf_mod;
	int numflips;
	int lastsynctype;
	int lastp25type;
	int offset;
	int carrier;
	char tg[25][16];
	int tgcount;
	int lasttg;
	long lastsrc;
	long src_list[50];
	int nac;
	int errs;
	int errs2;
	int mbe_file_type;
	int optind;
	int numtdulc;
	int firstframe;
	char slot0light[8];
	char slot1light[8];
	float aout_gain;
	float aout_max_buf[200];
	float *aout_max_buf_p;
	int aout_max_buf_idx;
	int samplesPerSymbol;
	int symbolCenter;
	char algid[9];
	char keyid[17];
	int currentslot;
	mbe_parms *cur_mp;
	mbe_parms *prev_mp;
	mbe_parms *prev_mp_enhanced;
	int p25kid;

	unsigned int debug_audio_errors;
	unsigned int debug_header_errors;
	unsigned int debug_header_critical_errors;

	// Last dibit read
	int last_dibit;

	// Heuristics state data for +P5 signals
	P25Heuristics p25_heuristics;

	// Heuristics state data for -P5 signals
	P25Heuristics inv_p25_heuristics;

#ifdef TRACE_DSD
	char debug_prefix;
	char debug_prefix_2;
	unsigned int debug_sample_index;
	unsigned int debug_sample_left_edge;
	unsigned int debug_sample_right_edge;
	FILE* debug_label_file;
	FILE* debug_label_dibit_file;
	FILE* debug_label_imbe_file;
#endif

	pthread_mutex_t input_mutex;
	pthread_cond_t input_ready;
	const float *input_samples;
	int input_length;
	int input_offset;
	pthread_mutex_t quit_mutex;
	pthread_cond_t quit_now;
	pthread_mutex_t output_mutex;
	pthread_cond_t output_ready;
	short *output_buffer;
	int output_offset;
	float *output_samples;
	int output_num_samples;
	int output_length;
	int output_finished;
	int exitflag;
	float xv[NZEROS+1];
	float nxv[NXZEROS+1];
} dsd_state;




enum dsd_frame_mode {
	dsd_FRAME_AUTO_DETECT,
	dsd_FRAME_P25_PHASE_1,
	dsd_FRAME_DSTAR,
	dsd_FRAME_NXDN48_IDAS,
	dsd_FRAME_NXDN96,
	dsd_FRAME_PROVOICE,
	dsd_FRAME_DMR_MOTOTRBO,
	dsd_FRAME_X2_TDMA
};

enum dsd_modulation_optimizations {
	dsd_MOD_AUTO_SELECT,
	dsd_MOD_C4FM,
	dsd_MOD_GFSK,
	dsd_MOD_QPSK
};

typedef struct
{
	int num;
	dsd_opts opts;
	dsd_state state;
} dsd_params;

class dsd_block_ff;

/*
 * We use boost::shared_ptr's instead of raw pointers for all access
 * to gr_blocks (and many other data structures).  The shared_ptr gets
 * us transparent reference counting, which greatly simplifies storage
 * management issues.  This is especially helpful in our hybrid
 * C++ / Python system.
 *
 * See http://www.boost.org/libs/smart_ptr/smart_ptr.htm
 *
 * As a convention, the _sptr suffix indicates a boost::shared_ptr
 */
typedef boost::shared_ptr<dsd_block_ff> dsd_block_ff_sptr;

/*!
 * \brief Return a shared_ptr to a new instance of dsd_block_ff.
 *
 * To avoid accidental use of raw pointers, dsd_block_ff's
 * constructor is private.  dsd_make_block_ff is the public
 * interface for creating new instances.
 */
DSD_API dsd_block_ff_sptr dsd_make_block_ff (dsd_frame_mode frame = dsd_FRAME_AUTO_DETECT,
        dsd_modulation_optimizations mod = dsd_MOD_AUTO_SELECT,
        int uvquality = 3, bool errorbars = true, int verbosity = 2, bool empty = false, int num=-1);

/*!
 * \brief pass discriminator output through Digital Speech Decoder
 * \ingroup block
 */
//class DSD_API dsd_block_ff : public gr_sync_decimator
class DSD_API dsd_block_ff : public gr::block
{
private:
	// The friend declaration allows dsd_make_block_ff to
	// access the private constructor.

	friend DSD_API dsd_block_ff_sptr dsd_make_block_ff (dsd_frame_mode frame, dsd_modulation_optimizations mod, int uvquality, bool errorbars, int verbosity, bool empty, int num);

	dsd_params params;

	/*!
	 * \brief pass discriminator output thread Digital Speech Decoder
	 */
	dsd_block_ff (dsd_frame_mode frame, dsd_modulation_optimizations mod, int uvquality, bool errorbars, int verbosity, bool empty, int num); // private constructor
	bool empty_frames;

public:
	~dsd_block_ff ();	// public destructor
	void reset_state();
	dsd_state *get_state();
	// Where all the action really happens

	int close();
	int general_work (int noutput_items,
	                  gr_vector_int &ninput_items,
	                  gr_vector_const_void_star &input_items,
	                  gr_vector_void_star &output_items);
	/*
	  int work (int noutput_items,
		    gr_vector_const_void_star &input_items,
		    gr_vector_void_star &output_items);*/
};

#endif /* INCLUDED_DSD_BLOCK_FF_H */
