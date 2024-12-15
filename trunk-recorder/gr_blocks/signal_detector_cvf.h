/* -*- c++ -*- */
/*
 * Copyright 2019 Free Software Foundation Inc..
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

#ifndef INCLUDED_INSPECTOR_SIGNAL_DETECTOR_CVF_H
#define INCLUDED_INSPECTOR_SIGNAL_DETECTOR_CVF_H

#include <gnuradio/sync_decimator.h>
/*
namespace gr {
namespace inspector {*/

/*!
 * \brief Signal detection block using energy detection.
 * \ingroup inspector
 *
 * \details
 * Takes input spectrum as complex float and performs an energy detection
 * to find potential continuous signals and build a RF map (tuple
 * of center frequency and bandwidth). The RF map gets passed as a
 * message with center frequency and bandwidth information for each detected
 * signal.
 *
 * Threshold for energy detection can either be set in dB or an automatic
 * threshold calculation can be performed by setting a sensitivity between 0
 * and 1. The PSD is then sorted and searched for relative power jumps with height
 * (1-sensitivity) and the threshold is set to the sample before that jump
 * (which should be the strongest noise sample).
 *
 * To surpress false detection in noisy scenarios, the minimum signal bandwidth
 * can be set. All detected signals smaller than this value will not be written
 * in the RF map.
 *
 * To average the PSD (and provide a better detection) an single pole IIR
 * filter is implemented in this block. The parameter alpha can be set as
 * block parameter. The IIR equation yields y[n] = alpha*x[n]+(1-alpha)*y[n-1].
 *
 * The bandwidth of the detected signals can be quantized relative to the
 * sampling rate. This leads to less recalculations in the Signal Separator
 * block. There, a filter must be recalculated, when the bandwidth of a signal
 * changed.
 */


struct Detected_Signal {
  int avg_rssi;
  int max_rssi;
  int min_rssi;
  int threshold;
  std::vector<int> rssi;
  double center_freq;
  double bandwidth;
  unsigned int start_bin;
  unsigned int end_bin;
};

class signal_detector_cvf : virtual public gr::sync_decimator
{
public:

#if GNURADIO_VERSION < 0x030900
  typedef boost::shared_ptr<signal_detector_cvf> sptr;
#else
  typedef std::shared_ptr<signal_detector_cvf> sptr;
#endif

    

    /*!
     * \brief Return a signal detector block instance.
     *
     * \param samp_rate Sample rate of the input signal
     * \param fft_len Desired number of FFT points for the PSD. Also sets the input items
     * consumed in evry work cycle. \param window_type Firdes window type to scale the
     * input samples with \param threshold Threshold in dB for energy detection when
     * automatic signal detection is disabled \param sensitivity Sensitivity value between
     * 0 and 1 if automatic signal detection is enabled \param auto_threshold Bool to set
     * automatic threshold calculation \param average Averaging factor in (0,1] (equal to
     * alpha in IIR equation) \param quantization Bandwidth quantization yields
     * quantization*samp_rate [Hz] \param min_bw Minimum signal bandwidth. Don't pass any
     * narrower signals. \param filename Path to a file where the detections are logged.
     * Leave empty for no log.
     */
    static sptr make(double samp_rate,
                     int fft_len = 1024,
                     int window_type = 0,
                     float threshold = 0.7,
                     float sensitivity = 0.2,
                     bool auto_threshold = true,
                     float average = 0.8,
                     float quantization = 0.01,
                     float min_bw = 0.0,
                     float max_bw = 50000,
                     const char* filename = "");

    virtual void set_samp_rate(double d_samp_rate) = 0;
    virtual void set_fft_len(int fft_len) = 0;

    /*!
     *  Takes integers and does internal cast to firdes::win_type
     */
    virtual void set_window_type(int d_window) = 0;
    virtual std::vector<Detected_Signal> get_detected_signals() = 0; 

    virtual void set_threshold(float d_threshold) = 0;
    virtual void set_sensitivity(float d_sensitivity) = 0;
    virtual void set_auto_threshold(bool d_auto_threshold) = 0;
    virtual void set_average(float d_average) = 0;
};

//} // namespace inspector
//} // namespace gr

#endif /* INCLUDED_INSPECTOR_SIGNAL_DETECTOR_CVF_H */
