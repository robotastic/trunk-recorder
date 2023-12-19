#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#
# SPDX-License-Identifier: GPL-3.0
#
# GNU Radio Python Flow Graph
# Title: Fm Channelizer
# GNU Radio version: 3.10.7.0

from packaging.version import Version as StrictVersion
from PyQt5 import Qt
from gnuradio import qtgui
from gnuradio import analog
from gnuradio import audio
from gnuradio import blocks
from gnuradio import filter
from gnuradio.filter import firdes
from gnuradio import gr
from gnuradio.fft import window
import sys
import signal
from PyQt5 import Qt
from argparse import ArgumentParser
from gnuradio.eng_arg import eng_float, intx
from gnuradio import eng_notation
from gnuradio import soapy
from gnuradio.filter import pfb
from gnuradio.qtgui import Range, RangeWidget
from PyQt5 import QtCore
import sip



class fm_channelizer(gr.top_block, Qt.QWidget):

    def __init__(self):
        gr.top_block.__init__(self, "Fm Channelizer", catch_exceptions=True)
        Qt.QWidget.__init__(self)
        self.setWindowTitle("Fm Channelizer")
        qtgui.util.check_set_qss()
        try:
            self.setWindowIcon(Qt.QIcon.fromTheme('gnuradio-grc'))
        except BaseException as exc:
            print(f"Qt GUI: Could not set Icon: {str(exc)}", file=sys.stderr)
        self.top_scroll_layout = Qt.QVBoxLayout()
        self.setLayout(self.top_scroll_layout)
        self.top_scroll = Qt.QScrollArea()
        self.top_scroll.setFrameStyle(Qt.QFrame.NoFrame)
        self.top_scroll_layout.addWidget(self.top_scroll)
        self.top_scroll.setWidgetResizable(True)
        self.top_widget = Qt.QWidget()
        self.top_scroll.setWidget(self.top_widget)
        self.top_layout = Qt.QVBoxLayout(self.top_widget)
        self.top_grid_layout = Qt.QGridLayout()
        self.top_layout.addLayout(self.top_grid_layout)

        self.settings = Qt.QSettings("GNU Radio", "fm_channelizer")

        try:
            if StrictVersion(Qt.qVersion()) < StrictVersion("5.0.0"):
                self.restoreGeometry(self.settings.value("geometry").toByteArray())
            else:
                self.restoreGeometry(self.settings.value("geometry"))
        except BaseException as exc:
            print(f"Qt GUI: Could not restore geometry: {str(exc)}", file=sys.stderr)

        ##################################################
        # Variables
        ##################################################
        self.synth_channels = synth_channels = 6
        self.channels = channels = 10
        self.ch_rate = ch_rate = 100e3
        self.samp_rate = samp_rate = ch_rate*channels
        self.gain = gain = 20
        self.fm_quad_rate = fm_quad_rate = ch_rate*synth_channels
        self.ch_tb = ch_tb = 20e3
        self.ch_bw = ch_bw = ch_rate/2
        self.audio_rate = audio_rate = 60e3
        self.atten = atten = 80
        self.volume = volume = 0.1
        self.tun_gain = tun_gain = gain
        self.pfb_taps = pfb_taps = firdes.low_pass_2(1, samp_rate, ch_bw, ch_tb, atten, window.WIN_BLACKMAN_HARRIS)
        self.pfb_synth_taps = pfb_synth_taps = firdes.low_pass_2(channels/2, synth_channels*ch_rate, ch_bw, ch_tb, atten, window.WIN_BLACKMAN_HARRIS)
        self.freq_corr = freq_corr = 0
        self.fm_audio_decim = fm_audio_decim = int(fm_quad_rate/audio_rate)*2
        self.channel = channel = 0
        self.center_freq = center_freq = 101.1e6+0e3
        self.address = address = ""

        ##################################################
        # Blocks
        ##################################################

        self._volume_range = Range(0, 10, 0.1, 0.1, 200)
        self._volume_win = RangeWidget(self._volume_range, self.set_volume, "Volume", "counter_slider", float, QtCore.Qt.Horizontal)
        self.top_grid_layout.addWidget(self._volume_win, 3, 0, 1, 1)
        for r in range(3, 4):
            self.top_grid_layout.setRowStretch(r, 1)
        for c in range(0, 1):
            self.top_grid_layout.setColumnStretch(c, 1)
        self._channel_range = Range(0, channels-1, 1, 0, 200)
        self._channel_win = RangeWidget(self._channel_range, self.set_channel, "Output Channel", "counter_slider", float, QtCore.Qt.Horizontal)
        self.top_grid_layout.addWidget(self._channel_win, 2, 0, 1, 1)
        for r in range(2, 3):
            self.top_grid_layout.setRowStretch(r, 1)
        for c in range(0, 1):
            self.top_grid_layout.setColumnStretch(c, 1)
        self._tun_gain_range = Range(0, 70, 1, gain, 200)
        self._tun_gain_win = RangeWidget(self._tun_gain_range, self.set_tun_gain, "Gain (dB)", "counter_slider", float, QtCore.Qt.Horizontal)
        self.top_grid_layout.addWidget(self._tun_gain_win, 2, 1, 1, 1)
        for r in range(2, 3):
            self.top_grid_layout.setRowStretch(r, 1)
        for c in range(1, 2):
            self.top_grid_layout.setColumnStretch(c, 1)
        self.soapy_rtlsdr_source_0 = None
        dev = 'driver=rtlsdr'
        stream_args = ''
        tune_args = ['']
        settings = ['']

        def _set_soapy_rtlsdr_source_0_gain_mode(channel, agc):
            self.soapy_rtlsdr_source_0.set_gain_mode(channel, agc)
            if not agc:
                  self.soapy_rtlsdr_source_0.set_gain(channel, self._soapy_rtlsdr_source_0_gain_value)
        self.set_soapy_rtlsdr_source_0_gain_mode = _set_soapy_rtlsdr_source_0_gain_mode

        def _set_soapy_rtlsdr_source_0_gain(channel, name, gain):
            self._soapy_rtlsdr_source_0_gain_value = gain
            if not self.soapy_rtlsdr_source_0.get_gain_mode(channel):
                self.soapy_rtlsdr_source_0.set_gain(channel, gain)
        self.set_soapy_rtlsdr_source_0_gain = _set_soapy_rtlsdr_source_0_gain

        def _set_soapy_rtlsdr_source_0_bias(bias):
            if 'biastee' in self._soapy_rtlsdr_source_0_setting_keys:
                self.soapy_rtlsdr_source_0.write_setting('biastee', bias)
        self.set_soapy_rtlsdr_source_0_bias = _set_soapy_rtlsdr_source_0_bias

        self.soapy_rtlsdr_source_0 = soapy.source(dev, "fc32", 1, '',
                                  stream_args, tune_args, settings)

        self._soapy_rtlsdr_source_0_setting_keys = [a.key for a in self.soapy_rtlsdr_source_0.get_setting_info()]

        self.soapy_rtlsdr_source_0.set_sample_rate(0, samp_rate)
        self.soapy_rtlsdr_source_0.set_frequency(0, center_freq)
        self.soapy_rtlsdr_source_0.set_frequency_correction(0, 0)
        self.set_soapy_rtlsdr_source_0_bias(bool(False))
        self._soapy_rtlsdr_source_0_gain_value = 20
        self.set_soapy_rtlsdr_source_0_gain_mode(0, bool(False))
        self.set_soapy_rtlsdr_source_0_gain(0, 'TUNER', 20)
        self.qtgui_freq_sink_x_0_0_0 = qtgui.freq_sink_c(
            1024, #size
            window.WIN_BLACKMAN_hARRIS, #wintype
            0, #fc
            (ch_rate*synth_channels), #bw
            'QT GUI Plot', #name
            1,
            None # parent
        )
        self.qtgui_freq_sink_x_0_0_0.set_update_time(0.10)
        self.qtgui_freq_sink_x_0_0_0.set_y_axis((-140), 10)
        self.qtgui_freq_sink_x_0_0_0.set_y_label('Relative Gain', 'dB')
        self.qtgui_freq_sink_x_0_0_0.set_trigger_mode(qtgui.TRIG_MODE_FREE, 0.0, 0, "")
        self.qtgui_freq_sink_x_0_0_0.enable_autoscale(False)
        self.qtgui_freq_sink_x_0_0_0.enable_grid(False)
        self.qtgui_freq_sink_x_0_0_0.set_fft_average(1.0)
        self.qtgui_freq_sink_x_0_0_0.enable_axis_labels(True)
        self.qtgui_freq_sink_x_0_0_0.enable_control_panel(False)
        self.qtgui_freq_sink_x_0_0_0.set_fft_window_normalized(False)



        labels = ['', '', '', '', '',
            '', '', '', '', '']
        widths = [1, 1, 1, 1, 1,
            1, 1, 1, 1, 1]
        colors = ["blue", "red", "green", "black", "cyan",
            "magenta", "yellow", "dark red", "dark green", "dark blue"]
        alphas = [1.0, 1.0, 1.0, 1.0, 1.0,
            1.0, 1.0, 1.0, 1.0, 1.0]

        for i in range(1):
            if len(labels[i]) == 0:
                self.qtgui_freq_sink_x_0_0_0.set_line_label(i, "Data {0}".format(i))
            else:
                self.qtgui_freq_sink_x_0_0_0.set_line_label(i, labels[i])
            self.qtgui_freq_sink_x_0_0_0.set_line_width(i, widths[i])
            self.qtgui_freq_sink_x_0_0_0.set_line_color(i, colors[i])
            self.qtgui_freq_sink_x_0_0_0.set_line_alpha(i, alphas[i])

        self._qtgui_freq_sink_x_0_0_0_win = sip.wrapinstance(self.qtgui_freq_sink_x_0_0_0.qwidget(), Qt.QWidget)
        self.top_grid_layout.addWidget(self._qtgui_freq_sink_x_0_0_0_win, 1, 1, 1, 1)
        for r in range(1, 2):
            self.top_grid_layout.setRowStretch(r, 1)
        for c in range(1, 2):
            self.top_grid_layout.setColumnStretch(c, 1)
        self.qtgui_freq_sink_x_0_0 = qtgui.freq_sink_c(
            1024, #size
            window.WIN_BLACKMAN_hARRIS, #wintype
            0, #fc
            (ch_rate*2), #bw
            'QT GUI Plot', #name
            synth_channels,
            None # parent
        )
        self.qtgui_freq_sink_x_0_0.set_update_time(0.10)
        self.qtgui_freq_sink_x_0_0.set_y_axis((-140), 10)
        self.qtgui_freq_sink_x_0_0.set_y_label('Relative Gain', 'dB')
        self.qtgui_freq_sink_x_0_0.set_trigger_mode(qtgui.TRIG_MODE_FREE, 0.0, 0, "")
        self.qtgui_freq_sink_x_0_0.enable_autoscale(False)
        self.qtgui_freq_sink_x_0_0.enable_grid(False)
        self.qtgui_freq_sink_x_0_0.set_fft_average(1.0)
        self.qtgui_freq_sink_x_0_0.enable_axis_labels(True)
        self.qtgui_freq_sink_x_0_0.enable_control_panel(False)
        self.qtgui_freq_sink_x_0_0.set_fft_window_normalized(False)



        labels = ['', '', '', '', '',
            '', '', '', '', '']
        widths = [1, 1, 1, 1, 1,
            1, 1, 1, 1, 1]
        colors = ["blue", "red", "green", "black", "cyan",
            "magenta", "yellow", "dark red", "dark green", "dark blue"]
        alphas = [1.0, 1.0, 1.0, 1.0, 1.0,
            1.0, 1.0, 1.0, 1.0, 1.0]

        for i in range(synth_channels):
            if len(labels[i]) == 0:
                self.qtgui_freq_sink_x_0_0.set_line_label(i, "Data {0}".format(i))
            else:
                self.qtgui_freq_sink_x_0_0.set_line_label(i, labels[i])
            self.qtgui_freq_sink_x_0_0.set_line_width(i, widths[i])
            self.qtgui_freq_sink_x_0_0.set_line_color(i, colors[i])
            self.qtgui_freq_sink_x_0_0.set_line_alpha(i, alphas[i])

        self._qtgui_freq_sink_x_0_0_win = sip.wrapinstance(self.qtgui_freq_sink_x_0_0.qwidget(), Qt.QWidget)
        self.top_grid_layout.addWidget(self._qtgui_freq_sink_x_0_0_win, 1, 0, 1, 1)
        for r in range(1, 2):
            self.top_grid_layout.setRowStretch(r, 1)
        for c in range(0, 1):
            self.top_grid_layout.setColumnStretch(c, 1)
        self.qtgui_freq_sink_x_0 = qtgui.freq_sink_c(
            1024, #size
            window.WIN_BLACKMAN_hARRIS, #wintype
            0, #fc
            samp_rate, #bw
            'QT GUI Plot', #name
            1,
            None # parent
        )
        self.qtgui_freq_sink_x_0.set_update_time(0.10)
        self.qtgui_freq_sink_x_0.set_y_axis((-140), 10)
        self.qtgui_freq_sink_x_0.set_y_label('Relative Gain', 'dB')
        self.qtgui_freq_sink_x_0.set_trigger_mode(qtgui.TRIG_MODE_FREE, 0.0, 0, "")
        self.qtgui_freq_sink_x_0.enable_autoscale(False)
        self.qtgui_freq_sink_x_0.enable_grid(False)
        self.qtgui_freq_sink_x_0.set_fft_average(1.0)
        self.qtgui_freq_sink_x_0.enable_axis_labels(True)
        self.qtgui_freq_sink_x_0.enable_control_panel(False)
        self.qtgui_freq_sink_x_0.set_fft_window_normalized(False)



        labels = ['', '', '', '', '',
            '', '', '', '', '']
        widths = [1, 1, 1, 1, 1,
            1, 1, 1, 1, 1]
        colors = ["blue", "red", "green", "black", "cyan",
            "magenta", "yellow", "dark red", "dark green", "dark blue"]
        alphas = [1.0, 1.0, 1.0, 1.0, 1.0,
            1.0, 1.0, 1.0, 1.0, 1.0]

        for i in range(1):
            if len(labels[i]) == 0:
                self.qtgui_freq_sink_x_0.set_line_label(i, "Data {0}".format(i))
            else:
                self.qtgui_freq_sink_x_0.set_line_label(i, labels[i])
            self.qtgui_freq_sink_x_0.set_line_width(i, widths[i])
            self.qtgui_freq_sink_x_0.set_line_color(i, colors[i])
            self.qtgui_freq_sink_x_0.set_line_alpha(i, alphas[i])

        self._qtgui_freq_sink_x_0_win = sip.wrapinstance(self.qtgui_freq_sink_x_0.qwidget(), Qt.QWidget)
        self.top_grid_layout.addWidget(self._qtgui_freq_sink_x_0_win, 0, 0, 1, 2)
        for r in range(0, 1):
            self.top_grid_layout.setRowStretch(r, 1)
        for c in range(0, 2):
            self.top_grid_layout.setColumnStretch(c, 1)
        self.pfb_synthesizer_ccf_0 = filter.pfb_synthesizer_ccf(
            synth_channels,
            pfb_synth_taps,
            True)
        self.pfb_synthesizer_ccf_0.set_channel_map([10, 11, 0, 1, 2, 3])
        self.pfb_synthesizer_ccf_0.declare_sample_delay(0)
        self.pfb_channelizer_ccf_0 = pfb.channelizer_ccf(
            channels,
            pfb_taps,
            2.0,
            atten)
        self.pfb_channelizer_ccf_0.set_channel_map()
        self.pfb_channelizer_ccf_0.declare_sample_delay(0)
        self.pfb_arb_resampler_xxx_0 = pfb.arb_resampler_fff(
            (44.1e3/(fm_quad_rate/fm_audio_decim)),
            taps=None,
            flt_size=32,
            atten=100)
        self.pfb_arb_resampler_xxx_0.declare_sample_delay(0)
        self._freq_corr_range = Range(-40e3, 40e3, 100, 0, 200)
        self._freq_corr_win = RangeWidget(self._freq_corr_range, self.set_freq_corr, "Frequency Correction", "counter_slider", float, QtCore.Qt.Horizontal)
        self.top_grid_layout.addWidget(self._freq_corr_win, 3, 1, 1, 1)
        for r in range(3, 4):
            self.top_grid_layout.setRowStretch(r, 1)
        for c in range(1, 2):
            self.top_grid_layout.setColumnStretch(c, 1)
        self.fir_filter_xxx_0 = filter.fir_filter_ccc(2, firdes.low_pass_2(1, ch_rate*synth_channels, 250e3, 300e3, 40, window.WIN_BLACKMAN_HARRIS) )
        self.fir_filter_xxx_0.declare_sample_delay(0)
        self.blocks_null_sink_0 = blocks.null_sink(gr.sizeof_gr_complex*1)
        self.blocks_multiply_const_vxx = blocks.multiply_const_ff(volume)
        self.audio_sink = audio.sink(44100, '', True)
        self.analog_wfm_rcv = analog.wfm_rcv(
        	quad_rate=fm_quad_rate,
        	audio_decimation=fm_audio_decim,
        )
        self.analog_agc2_xx_0 = analog.agc2_cc((1e-1), (1e-2), 1.0, 1.0, 65536)


        ##################################################
        # Connections
        ##################################################
        self.connect((self.analog_agc2_xx_0, 0), (self.pfb_channelizer_ccf_0, 0))
        self.connect((self.analog_agc2_xx_0, 0), (self.qtgui_freq_sink_x_0, 0))
        self.connect((self.analog_wfm_rcv, 0), (self.pfb_arb_resampler_xxx_0, 0))
        self.connect((self.blocks_multiply_const_vxx, 0), (self.audio_sink, 0))
        self.connect((self.fir_filter_xxx_0, 0), (self.analog_wfm_rcv, 0))
        self.connect((self.fir_filter_xxx_0, 0), (self.qtgui_freq_sink_x_0_0_0, 0))
        self.connect((self.pfb_arb_resampler_xxx_0, 0), (self.blocks_multiply_const_vxx, 0))
        self.connect((self.pfb_channelizer_ccf_0, 9), (self.blocks_null_sink_0, 3))
        self.connect((self.pfb_channelizer_ccf_0, 7), (self.blocks_null_sink_0, 1))
        self.connect((self.pfb_channelizer_ccf_0, 8), (self.blocks_null_sink_0, 2))
        self.connect((self.pfb_channelizer_ccf_0, 6), (self.blocks_null_sink_0, 0))
        self.connect((self.pfb_channelizer_ccf_0, 0), (self.pfb_synthesizer_ccf_0, 0))
        self.connect((self.pfb_channelizer_ccf_0, 1), (self.pfb_synthesizer_ccf_0, 1))
        self.connect((self.pfb_channelizer_ccf_0, 4), (self.pfb_synthesizer_ccf_0, 4))
        self.connect((self.pfb_channelizer_ccf_0, 3), (self.pfb_synthesizer_ccf_0, 3))
        self.connect((self.pfb_channelizer_ccf_0, 5), (self.pfb_synthesizer_ccf_0, 5))
        self.connect((self.pfb_channelizer_ccf_0, 2), (self.pfb_synthesizer_ccf_0, 2))
        self.connect((self.pfb_channelizer_ccf_0, 5), (self.qtgui_freq_sink_x_0_0, 5))
        self.connect((self.pfb_channelizer_ccf_0, 1), (self.qtgui_freq_sink_x_0_0, 1))
        self.connect((self.pfb_channelizer_ccf_0, 4), (self.qtgui_freq_sink_x_0_0, 4))
        self.connect((self.pfb_channelizer_ccf_0, 3), (self.qtgui_freq_sink_x_0_0, 3))
        self.connect((self.pfb_channelizer_ccf_0, 2), (self.qtgui_freq_sink_x_0_0, 2))
        self.connect((self.pfb_channelizer_ccf_0, 0), (self.qtgui_freq_sink_x_0_0, 0))
        self.connect((self.pfb_synthesizer_ccf_0, 0), (self.fir_filter_xxx_0, 0))
        self.connect((self.soapy_rtlsdr_source_0, 0), (self.analog_agc2_xx_0, 0))


    def closeEvent(self, event):
        self.settings = Qt.QSettings("GNU Radio", "fm_channelizer")
        self.settings.setValue("geometry", self.saveGeometry())
        self.stop()
        self.wait()

        event.accept()

    def get_synth_channels(self):
        return self.synth_channels

    def set_synth_channels(self, synth_channels):
        self.synth_channels = synth_channels
        self.set_fm_quad_rate(self.ch_rate*self.synth_channels)
        self.set_pfb_synth_taps(firdes.low_pass_2(self.channels/2, self.synth_channels*self.ch_rate, self.ch_bw, self.ch_tb, self.atten, window.WIN_BLACKMAN_HARRIS))
        self.fir_filter_xxx_0.set_taps(firdes.low_pass_2(1, self.ch_rate*self.synth_channels, 250e3, 300e3, 40, window.WIN_BLACKMAN_HARRIS) )
        self.qtgui_freq_sink_x_0_0_0.set_frequency_range(0, (self.ch_rate*self.synth_channels))

    def get_channels(self):
        return self.channels

    def set_channels(self, channels):
        self.channels = channels
        self.set_pfb_synth_taps(firdes.low_pass_2(self.channels/2, self.synth_channels*self.ch_rate, self.ch_bw, self.ch_tb, self.atten, window.WIN_BLACKMAN_HARRIS))
        self.set_samp_rate(self.ch_rate*self.channels)

    def get_ch_rate(self):
        return self.ch_rate

    def set_ch_rate(self, ch_rate):
        self.ch_rate = ch_rate
        self.set_ch_bw(self.ch_rate/2)
        self.set_fm_quad_rate(self.ch_rate*self.synth_channels)
        self.set_pfb_synth_taps(firdes.low_pass_2(self.channels/2, self.synth_channels*self.ch_rate, self.ch_bw, self.ch_tb, self.atten, window.WIN_BLACKMAN_HARRIS))
        self.set_samp_rate(self.ch_rate*self.channels)
        self.fir_filter_xxx_0.set_taps(firdes.low_pass_2(1, self.ch_rate*self.synth_channels, 250e3, 300e3, 40, window.WIN_BLACKMAN_HARRIS) )
        self.qtgui_freq_sink_x_0_0.set_frequency_range(0, (self.ch_rate*2))
        self.qtgui_freq_sink_x_0_0_0.set_frequency_range(0, (self.ch_rate*self.synth_channels))

    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.set_pfb_taps(firdes.low_pass_2(1, self.samp_rate, self.ch_bw, self.ch_tb, self.atten, window.WIN_BLACKMAN_HARRIS))
        self.qtgui_freq_sink_x_0.set_frequency_range(0, self.samp_rate)
        self.soapy_rtlsdr_source_0.set_sample_rate(0, self.samp_rate)

    def get_gain(self):
        return self.gain

    def set_gain(self, gain):
        self.gain = gain
        self.set_tun_gain(self.gain)

    def get_fm_quad_rate(self):
        return self.fm_quad_rate

    def set_fm_quad_rate(self, fm_quad_rate):
        self.fm_quad_rate = fm_quad_rate
        self.set_fm_audio_decim(int(self.fm_quad_rate/self.audio_rate)*2)
        self.pfb_arb_resampler_xxx_0.set_rate((44.1e3/(self.fm_quad_rate/self.fm_audio_decim)))

    def get_ch_tb(self):
        return self.ch_tb

    def set_ch_tb(self, ch_tb):
        self.ch_tb = ch_tb
        self.set_pfb_synth_taps(firdes.low_pass_2(self.channels/2, self.synth_channels*self.ch_rate, self.ch_bw, self.ch_tb, self.atten, window.WIN_BLACKMAN_HARRIS))
        self.set_pfb_taps(firdes.low_pass_2(1, self.samp_rate, self.ch_bw, self.ch_tb, self.atten, window.WIN_BLACKMAN_HARRIS))

    def get_ch_bw(self):
        return self.ch_bw

    def set_ch_bw(self, ch_bw):
        self.ch_bw = ch_bw
        self.set_pfb_synth_taps(firdes.low_pass_2(self.channels/2, self.synth_channels*self.ch_rate, self.ch_bw, self.ch_tb, self.atten, window.WIN_BLACKMAN_HARRIS))
        self.set_pfb_taps(firdes.low_pass_2(1, self.samp_rate, self.ch_bw, self.ch_tb, self.atten, window.WIN_BLACKMAN_HARRIS))

    def get_audio_rate(self):
        return self.audio_rate

    def set_audio_rate(self, audio_rate):
        self.audio_rate = audio_rate
        self.set_fm_audio_decim(int(self.fm_quad_rate/self.audio_rate)*2)

    def get_atten(self):
        return self.atten

    def set_atten(self, atten):
        self.atten = atten
        self.set_pfb_synth_taps(firdes.low_pass_2(self.channels/2, self.synth_channels*self.ch_rate, self.ch_bw, self.ch_tb, self.atten, window.WIN_BLACKMAN_HARRIS))
        self.set_pfb_taps(firdes.low_pass_2(1, self.samp_rate, self.ch_bw, self.ch_tb, self.atten, window.WIN_BLACKMAN_HARRIS))

    def get_volume(self):
        return self.volume

    def set_volume(self, volume):
        self.volume = volume
        self.blocks_multiply_const_vxx.set_k(self.volume)

    def get_tun_gain(self):
        return self.tun_gain

    def set_tun_gain(self, tun_gain):
        self.tun_gain = tun_gain

    def get_pfb_taps(self):
        return self.pfb_taps

    def set_pfb_taps(self, pfb_taps):
        self.pfb_taps = pfb_taps
        self.pfb_channelizer_ccf_0.set_taps(self.pfb_taps)

    def get_pfb_synth_taps(self):
        return self.pfb_synth_taps

    def set_pfb_synth_taps(self, pfb_synth_taps):
        self.pfb_synth_taps = pfb_synth_taps
        self.pfb_synthesizer_ccf_0.set_taps(self.pfb_synth_taps)

    def get_freq_corr(self):
        return self.freq_corr

    def set_freq_corr(self, freq_corr):
        self.freq_corr = freq_corr

    def get_fm_audio_decim(self):
        return self.fm_audio_decim

    def set_fm_audio_decim(self, fm_audio_decim):
        self.fm_audio_decim = fm_audio_decim
        self.pfb_arb_resampler_xxx_0.set_rate((44.1e3/(self.fm_quad_rate/self.fm_audio_decim)))

    def get_channel(self):
        return self.channel

    def set_channel(self, channel):
        self.channel = channel

    def get_center_freq(self):
        return self.center_freq

    def set_center_freq(self, center_freq):
        self.center_freq = center_freq
        self.soapy_rtlsdr_source_0.set_frequency(0, self.center_freq)

    def get_address(self):
        return self.address

    def set_address(self, address):
        self.address = address




def main(top_block_cls=fm_channelizer, options=None):

    if StrictVersion("4.5.0") <= StrictVersion(Qt.qVersion()) < StrictVersion("5.0.0"):
        style = gr.prefs().get_string('qtgui', 'style', 'raster')
        Qt.QApplication.setGraphicsSystem(style)
    qapp = Qt.QApplication(sys.argv)

    tb = top_block_cls()

    tb.start()

    tb.show()

    def sig_handler(sig=None, frame=None):
        tb.stop()
        tb.wait()

        Qt.QApplication.quit()

    signal.signal(signal.SIGINT, sig_handler)
    signal.signal(signal.SIGTERM, sig_handler)

    timer = Qt.QTimer()
    timer.start(500)
    timer.timeout.connect(lambda: None)

    qapp.exec_()

if __name__ == '__main__':
    main()
