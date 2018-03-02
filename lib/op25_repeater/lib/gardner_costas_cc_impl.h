/* -*- c++ -*- */
/*
 * Copyright 2005,2006,2007 Free Software Foundation, Inc.
 *
 * Gardner symbol recovery block for GR - Copyright 2010, 2011, 2012, 2013 KA1RBI
 *
 * This file is part of OP25 and part of GNU Radio
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

#ifndef INCLUDED_OP25_REPEATER_GARDNER_COSTAS_CC_IMPL_H
#define INCLUDED_OP25_REPEATER_GARDNER_COSTAS_CC_IMPL_H

#include <op25_repeater/gardner_costas_cc.h>

#include <gnuradio/gr_complex.h>
#include <gnuradio/math.h>

#include <gnuradio/filter/mmse_fir_interpolator_cc.h>
// class gr::filter::mmse_fir_interpolator_cc;

namespace gr {
  namespace op25_repeater {

    class gardner_costas_cc_impl : public gardner_costas_cc
    {
     private:
	uint8_t slicer(float sym);

     public:
      gardner_costas_cc_impl(float samples_per_symbol, float gain_mu, float gain_omega, float alpha, float beta, float max_freq, float min_freq);

/*!
 * \brief Gardner based repeater gardner_costas_cc block with complex input, complex output.
 * \ingroup sync_blk
 *
 * This implements a Gardner discrete-time error-tracking synchronizer.
 *
 * input samples should be within normalized range of -1.0 thru +1.0
 *
 * includes some simplifying approximations KA1RBI
 */
      ~gardner_costas_cc_impl();

      // Where all the action really happens
      void forecast (int noutput_items, gr_vector_int &ninput_items_required);

      int general_work(int noutput_items,
		       gr_vector_int &ninput_items,
		       gr_vector_const_void_star &input_items,
		       gr_vector_void_star &output_items);
  void set_verbose (bool verbose) { d_verbose = verbose; }

  //! Sets value of omega and its min and max values
  void set_omega (float omega);
  float get_freq_error(void);
void reset();
  void update_omega (float samples_per_symbol);
  void update_fmax (float max_freq);

protected:
  bool input_sample0(gr_complex, gr_complex& outp);
  bool input_sample(gr_complex, gr_complex& outp);

 private:

  float				d_mu;
  float d_omega, d_gain_omega, d_omega_rel, d_max_omega, d_min_omega, d_omega_mid;
  float				d_gain_mu;

  gr_complex                    d_last_sample;
  gr::filter::mmse_fir_interpolator_cc 	*d_interp;
  bool			        d_verbose;

  gr_complex			*d_dl;
  int				d_dl_index;

  int				d_twice_sps;

  float				d_timing_error;

  float				d_alpha;
  float				d_beta;
  uint32_t			d_interp_counter;

  float				d_theta;
  float				d_phase;
  float				d_freq;
  float				d_max_freq;

  uint64_t			nid_accum;

  float phase_error_detector_qpsk(gr_complex sample);
  void phase_error_tracking(gr_complex sample);
  };

  } // namespace op25_repeater
} // namespace gr

#endif /* INCLUDED_OP25_REPEATER_GARDNER_COSTAS_CC_IMPL_H */
