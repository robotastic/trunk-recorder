#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#
# SPDX-License-Identifier: GPL-3.0
#
# GNU Radio Python Flow Graph
# Title: Fm Channelize And View
# GNU Radio version: 3.10.5.1

from packaging.version import Version as StrictVersion

if __name__ == '__main__':
    import ctypes
    import sys
    if sys.platform.startswith('linux'):
        try:
            x11 = ctypes.cdll.LoadLibrary('libX11.so')
            x11.XInitThreads()
        except:
            print("Warning: failed to XInitThreads()")

from PyQt5 import Qt
from gnuradio import qtgui
from gnuradio.filter import firdes
import sip
from gnuradio import analog
from gnuradio import blocks
from gnuradio import gr
from gnuradio.fft import window
import sys
import signal
from argparse import ArgumentParser
from gnuradio.eng_arg import eng_float, intx
from gnuradio import eng_notation
from gnuradio import soapy
from gnuradio.filter import pfb
from gnuradio.qtgui import Range, RangeWidget
from PyQt5 import QtCore



from gnuradio import qtgui

class fm_channelize_and_view(gr.top_block, Qt.QWidget):

    def __init__(self):
        gr.top_block.__init__(self, "Fm Channelize And View", catch_exceptions=True)
        Qt.QWidget.__init__(self)
        self.setWindowTitle("Fm Channelize And View")
        qtgui.util.check_set_qss()
        try:
            self.setWindowIcon(Qt.QIcon.fromTheme('gnuradio-grc'))
        except:
            pass
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

        self.settings = Qt.QSettings("GNU Radio", "fm_channelize_and_view")

        try:
            if StrictVersion(Qt.qVersion()) < StrictVersion("5.0.0"):
                self.restoreGeometry(self.settings.value("geometry").toByteArray())
            else:
                self.restoreGeometry(self.settings.value("geometry"))
        except:
            pass

        ##################################################
        # Variables
        ##################################################
        self.channels = channels = 100
        self.ch_rate = ch_rate = 200e3
        self.samp_rate = samp_rate = ch_rate*channels
        self.gain = gain = 65
        self.ch_tb = ch_tb = 100e3
        self.ch_bw = ch_bw = 125e3
        self.atten = atten = 60
        self.volume = volume = 0.1
        self.tun_gain = tun_gain = gain
        self.pfb_taps = pfb_taps = firdes.low_pass_2(1, samp_rate, ch_bw, ch_tb, atten, window.WIN_HANN)
        self.ch2 = ch2 = 22
        self.ch1 = ch1 = 5
        self.center_freq = center_freq = 97900000
        self.address = address = ""

        ##################################################
        # Blocks
        ##################################################

        self._volume_range = Range(0, 10, 0.1, 0.1, 200)
        self._volume_win = RangeWidget(self._volume_range, self.set_volume, "Volume", "counter_slider", float, QtCore.Qt.Horizontal)
        self.top_grid_layout.addWidget(self._volume_win, 2, 0, 1, 1)
        for r in range(2, 3):
            self.top_grid_layout.setRowStretch(r, 1)
        for c in range(0, 1):
            self.top_grid_layout.setColumnStretch(c, 1)
        self._tun_gain_range = Range(0, 73, 0.5, gain, 200)
        self._tun_gain_win = RangeWidget(self._tun_gain_range, self.set_tun_gain, "Gain (dB)", "counter_slider", float, QtCore.Qt.Horizontal)
        self.top_grid_layout.addWidget(self._tun_gain_win, 2, 1, 1, 1)
        for r in range(2, 3):
            self.top_grid_layout.setRowStretch(r, 1)
        for c in range(1, 2):
            self.top_grid_layout.setColumnStretch(c, 1)
        self.soapy_hackrf_source_0 = None
        dev = 'driver=hackrf'
        stream_args = ''
        tune_args = ['']
        settings = ['']

        self.soapy_hackrf_source_0 = soapy.source(dev, "fc32", 1, '',
                                  stream_args, tune_args, settings)
        self.soapy_hackrf_source_0.set_sample_rate(0, samp_rate)
        self.soapy_hackrf_source_0.set_bandwidth(0, 0)
        self.soapy_hackrf_source_0.set_frequency(0, center_freq)
        self.soapy_hackrf_source_0.set_gain(0, 'AMP', True)
        self.soapy_hackrf_source_0.set_gain(0, 'LNA', min(max(20, 0.0), 40.0))
        self.soapy_hackrf_source_0.set_gain(0, 'VGA', min(max(32, 0.0), 62.0))
        self.qtgui_freq_sink_x_0_0_0 = qtgui.freq_sink_c(
            1024, #size
            window.WIN_BLACKMAN_hARRIS, #wintype
            0, #fc
            ch_rate, #bw
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
            ch_rate, #bw
            'QT GUI Plot', #name
            1,
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

        for i in range(1):
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
            4096, #size
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
        self.pfb_channelizer_ccf_0 = pfb.channelizer_ccf(
            channels,
            pfb_taps,
            1.0,
            atten)
        self.pfb_channelizer_ccf_0.set_channel_map([])
        self.pfb_channelizer_ccf_0.declare_sample_delay(0)
        self.blocks_null_sink_0 = blocks.null_sink(gr.sizeof_gr_complex*1)
        self.analog_agc2_xx_0 = analog.agc2_cc((1e-1), (1e-2), 1.0, 1.0)
        self.analog_agc2_xx_0.set_max_gain(65536)


        ##################################################
        # Connections
        ##################################################
        self.connect((self.analog_agc2_xx_0, 0), (self.pfb_channelizer_ccf_0, 0))
        self.connect((self.analog_agc2_xx_0, 0), (self.qtgui_freq_sink_x_0, 0))
        self.connect((self.pfb_channelizer_ccf_0, 99), (self.blocks_null_sink_0, 97))
        self.connect((self.pfb_channelizer_ccf_0, 41), (self.blocks_null_sink_0, 39))
        self.connect((self.pfb_channelizer_ccf_0, 9), (self.blocks_null_sink_0, 8))
        self.connect((self.pfb_channelizer_ccf_0, 98), (self.blocks_null_sink_0, 96))
        self.connect((self.pfb_channelizer_ccf_0, 65), (self.blocks_null_sink_0, 63))
        self.connect((self.pfb_channelizer_ccf_0, 60), (self.blocks_null_sink_0, 58))
        self.connect((self.pfb_channelizer_ccf_0, 93), (self.blocks_null_sink_0, 91))
        self.connect((self.pfb_channelizer_ccf_0, 70), (self.blocks_null_sink_0, 68))
        self.connect((self.pfb_channelizer_ccf_0, 38), (self.blocks_null_sink_0, 36))
        self.connect((self.pfb_channelizer_ccf_0, 10), (self.blocks_null_sink_0, 9))
        self.connect((self.pfb_channelizer_ccf_0, 26), (self.blocks_null_sink_0, 24))
        self.connect((self.pfb_channelizer_ccf_0, 20), (self.blocks_null_sink_0, 19))
        self.connect((self.pfb_channelizer_ccf_0, 3), (self.blocks_null_sink_0, 3))
        self.connect((self.pfb_channelizer_ccf_0, 12), (self.blocks_null_sink_0, 11))
        self.connect((self.pfb_channelizer_ccf_0, 32), (self.blocks_null_sink_0, 30))
        self.connect((self.pfb_channelizer_ccf_0, 28), (self.blocks_null_sink_0, 26))
        self.connect((self.pfb_channelizer_ccf_0, 14), (self.blocks_null_sink_0, 13))
        self.connect((self.pfb_channelizer_ccf_0, 84), (self.blocks_null_sink_0, 82))
        self.connect((self.pfb_channelizer_ccf_0, 18), (self.blocks_null_sink_0, 17))
        self.connect((self.pfb_channelizer_ccf_0, 45), (self.blocks_null_sink_0, 43))
        self.connect((self.pfb_channelizer_ccf_0, 49), (self.blocks_null_sink_0, 47))
        self.connect((self.pfb_channelizer_ccf_0, 69), (self.blocks_null_sink_0, 67))
        self.connect((self.pfb_channelizer_ccf_0, 63), (self.blocks_null_sink_0, 61))
        self.connect((self.pfb_channelizer_ccf_0, 67), (self.blocks_null_sink_0, 65))
        self.connect((self.pfb_channelizer_ccf_0, 21), (self.blocks_null_sink_0, 20))
        self.connect((self.pfb_channelizer_ccf_0, 36), (self.blocks_null_sink_0, 34))
        self.connect((self.pfb_channelizer_ccf_0, 1), (self.blocks_null_sink_0, 1))
        self.connect((self.pfb_channelizer_ccf_0, 81), (self.blocks_null_sink_0, 79))
        self.connect((self.pfb_channelizer_ccf_0, 6), (self.blocks_null_sink_0, 5))
        self.connect((self.pfb_channelizer_ccf_0, 34), (self.blocks_null_sink_0, 32))
        self.connect((self.pfb_channelizer_ccf_0, 73), (self.blocks_null_sink_0, 71))
        self.connect((self.pfb_channelizer_ccf_0, 80), (self.blocks_null_sink_0, 78))
        self.connect((self.pfb_channelizer_ccf_0, 90), (self.blocks_null_sink_0, 88))
        self.connect((self.pfb_channelizer_ccf_0, 57), (self.blocks_null_sink_0, 55))
        self.connect((self.pfb_channelizer_ccf_0, 97), (self.blocks_null_sink_0, 95))
        self.connect((self.pfb_channelizer_ccf_0, 42), (self.blocks_null_sink_0, 40))
        self.connect((self.pfb_channelizer_ccf_0, 96), (self.blocks_null_sink_0, 94))
        self.connect((self.pfb_channelizer_ccf_0, 37), (self.blocks_null_sink_0, 35))
        self.connect((self.pfb_channelizer_ccf_0, 30), (self.blocks_null_sink_0, 28))
        self.connect((self.pfb_channelizer_ccf_0, 75), (self.blocks_null_sink_0, 73))
        self.connect((self.pfb_channelizer_ccf_0, 11), (self.blocks_null_sink_0, 10))
        self.connect((self.pfb_channelizer_ccf_0, 88), (self.blocks_null_sink_0, 86))
        self.connect((self.pfb_channelizer_ccf_0, 16), (self.blocks_null_sink_0, 15))
        self.connect((self.pfb_channelizer_ccf_0, 89), (self.blocks_null_sink_0, 87))
        self.connect((self.pfb_channelizer_ccf_0, 66), (self.blocks_null_sink_0, 64))
        self.connect((self.pfb_channelizer_ccf_0, 83), (self.blocks_null_sink_0, 81))
        self.connect((self.pfb_channelizer_ccf_0, 13), (self.blocks_null_sink_0, 12))
        self.connect((self.pfb_channelizer_ccf_0, 61), (self.blocks_null_sink_0, 59))
        self.connect((self.pfb_channelizer_ccf_0, 76), (self.blocks_null_sink_0, 74))
        self.connect((self.pfb_channelizer_ccf_0, 7), (self.blocks_null_sink_0, 6))
        self.connect((self.pfb_channelizer_ccf_0, 31), (self.blocks_null_sink_0, 29))
        self.connect((self.pfb_channelizer_ccf_0, 64), (self.blocks_null_sink_0, 62))
        self.connect((self.pfb_channelizer_ccf_0, 56), (self.blocks_null_sink_0, 54))
        self.connect((self.pfb_channelizer_ccf_0, 29), (self.blocks_null_sink_0, 27))
        self.connect((self.pfb_channelizer_ccf_0, 74), (self.blocks_null_sink_0, 72))
        self.connect((self.pfb_channelizer_ccf_0, 25), (self.blocks_null_sink_0, 23))
        self.connect((self.pfb_channelizer_ccf_0, 27), (self.blocks_null_sink_0, 25))
        self.connect((self.pfb_channelizer_ccf_0, 2), (self.blocks_null_sink_0, 2))
        self.connect((self.pfb_channelizer_ccf_0, 23), (self.blocks_null_sink_0, 21))
        self.connect((self.pfb_channelizer_ccf_0, 52), (self.blocks_null_sink_0, 50))
        self.connect((self.pfb_channelizer_ccf_0, 53), (self.blocks_null_sink_0, 51))
        self.connect((self.pfb_channelizer_ccf_0, 4), (self.blocks_null_sink_0, 4))
        self.connect((self.pfb_channelizer_ccf_0, 8), (self.blocks_null_sink_0, 7))
        self.connect((self.pfb_channelizer_ccf_0, 72), (self.blocks_null_sink_0, 70))
        self.connect((self.pfb_channelizer_ccf_0, 62), (self.blocks_null_sink_0, 60))
        self.connect((self.pfb_channelizer_ccf_0, 78), (self.blocks_null_sink_0, 76))
        self.connect((self.pfb_channelizer_ccf_0, 87), (self.blocks_null_sink_0, 85))
        self.connect((self.pfb_channelizer_ccf_0, 79), (self.blocks_null_sink_0, 77))
        self.connect((self.pfb_channelizer_ccf_0, 17), (self.blocks_null_sink_0, 16))
        self.connect((self.pfb_channelizer_ccf_0, 68), (self.blocks_null_sink_0, 66))
        self.connect((self.pfb_channelizer_ccf_0, 71), (self.blocks_null_sink_0, 69))
        self.connect((self.pfb_channelizer_ccf_0, 77), (self.blocks_null_sink_0, 75))
        self.connect((self.pfb_channelizer_ccf_0, 55), (self.blocks_null_sink_0, 53))
        self.connect((self.pfb_channelizer_ccf_0, 19), (self.blocks_null_sink_0, 18))
        self.connect((self.pfb_channelizer_ccf_0, 24), (self.blocks_null_sink_0, 22))
        self.connect((self.pfb_channelizer_ccf_0, 35), (self.blocks_null_sink_0, 33))
        self.connect((self.pfb_channelizer_ccf_0, 95), (self.blocks_null_sink_0, 93))
        self.connect((self.pfb_channelizer_ccf_0, 47), (self.blocks_null_sink_0, 45))
        self.connect((self.pfb_channelizer_ccf_0, 58), (self.blocks_null_sink_0, 56))
        self.connect((self.pfb_channelizer_ccf_0, 33), (self.blocks_null_sink_0, 31))
        self.connect((self.pfb_channelizer_ccf_0, 48), (self.blocks_null_sink_0, 46))
        self.connect((self.pfb_channelizer_ccf_0, 54), (self.blocks_null_sink_0, 52))
        self.connect((self.pfb_channelizer_ccf_0, 82), (self.blocks_null_sink_0, 80))
        self.connect((self.pfb_channelizer_ccf_0, 92), (self.blocks_null_sink_0, 90))
        self.connect((self.pfb_channelizer_ccf_0, 91), (self.blocks_null_sink_0, 89))
        self.connect((self.pfb_channelizer_ccf_0, 0), (self.blocks_null_sink_0, 0))
        self.connect((self.pfb_channelizer_ccf_0, 86), (self.blocks_null_sink_0, 84))
        self.connect((self.pfb_channelizer_ccf_0, 46), (self.blocks_null_sink_0, 44))
        self.connect((self.pfb_channelizer_ccf_0, 50), (self.blocks_null_sink_0, 48))
        self.connect((self.pfb_channelizer_ccf_0, 59), (self.blocks_null_sink_0, 57))
        self.connect((self.pfb_channelizer_ccf_0, 15), (self.blocks_null_sink_0, 14))
        self.connect((self.pfb_channelizer_ccf_0, 85), (self.blocks_null_sink_0, 83))
        self.connect((self.pfb_channelizer_ccf_0, 94), (self.blocks_null_sink_0, 92))
        self.connect((self.pfb_channelizer_ccf_0, 51), (self.blocks_null_sink_0, 49))
        self.connect((self.pfb_channelizer_ccf_0, 43), (self.blocks_null_sink_0, 41))
        self.connect((self.pfb_channelizer_ccf_0, 39), (self.blocks_null_sink_0, 37))
        self.connect((self.pfb_channelizer_ccf_0, 40), (self.blocks_null_sink_0, 38))
        self.connect((self.pfb_channelizer_ccf_0, 44), (self.blocks_null_sink_0, 42))
        self.connect((self.pfb_channelizer_ccf_0, 5), (self.qtgui_freq_sink_x_0_0, 0))
        self.connect((self.pfb_channelizer_ccf_0, 22), (self.qtgui_freq_sink_x_0_0_0, 0))
        self.connect((self.soapy_hackrf_source_0, 0), (self.analog_agc2_xx_0, 0))


    def closeEvent(self, event):
        self.settings = Qt.QSettings("GNU Radio", "fm_channelize_and_view")
        self.settings.setValue("geometry", self.saveGeometry())
        self.stop()
        self.wait()

        event.accept()

    def get_channels(self):
        return self.channels

    def set_channels(self, channels):
        self.channels = channels
        self.set_samp_rate(self.ch_rate*self.channels)

    def get_ch_rate(self):
        return self.ch_rate

    def set_ch_rate(self, ch_rate):
        self.ch_rate = ch_rate
        self.set_samp_rate(self.ch_rate*self.channels)
        self.qtgui_freq_sink_x_0_0.set_frequency_range(0, self.ch_rate)
        self.qtgui_freq_sink_x_0_0_0.set_frequency_range(0, self.ch_rate)

    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.set_pfb_taps(firdes.low_pass_2(1, self.samp_rate, self.ch_bw, self.ch_tb, self.atten, window.WIN_HANN))
        self.qtgui_freq_sink_x_0.set_frequency_range(0, self.samp_rate)
        self.soapy_hackrf_source_0.set_sample_rate(0, self.samp_rate)

    def get_gain(self):
        return self.gain

    def set_gain(self, gain):
        self.gain = gain
        self.set_tun_gain(self.gain)

    def get_ch_tb(self):
        return self.ch_tb

    def set_ch_tb(self, ch_tb):
        self.ch_tb = ch_tb
        self.set_pfb_taps(firdes.low_pass_2(1, self.samp_rate, self.ch_bw, self.ch_tb, self.atten, window.WIN_HANN))

    def get_ch_bw(self):
        return self.ch_bw

    def set_ch_bw(self, ch_bw):
        self.ch_bw = ch_bw
        self.set_pfb_taps(firdes.low_pass_2(1, self.samp_rate, self.ch_bw, self.ch_tb, self.atten, window.WIN_HANN))

    def get_atten(self):
        return self.atten

    def set_atten(self, atten):
        self.atten = atten
        self.set_pfb_taps(firdes.low_pass_2(1, self.samp_rate, self.ch_bw, self.ch_tb, self.atten, window.WIN_HANN))

    def get_volume(self):
        return self.volume

    def set_volume(self, volume):
        self.volume = volume

    def get_tun_gain(self):
        return self.tun_gain

    def set_tun_gain(self, tun_gain):
        self.tun_gain = tun_gain

    def get_pfb_taps(self):
        return self.pfb_taps

    def set_pfb_taps(self, pfb_taps):
        self.pfb_taps = pfb_taps
        self.pfb_channelizer_ccf_0.set_taps(self.pfb_taps)

    def get_ch2(self):
        return self.ch2

    def set_ch2(self, ch2):
        self.ch2 = ch2

    def get_ch1(self):
        return self.ch1

    def set_ch1(self, ch1):
        self.ch1 = ch1

    def get_center_freq(self):
        return self.center_freq

    def set_center_freq(self, center_freq):
        self.center_freq = center_freq
        self.soapy_hackrf_source_0.set_frequency(0, self.center_freq)

    def get_address(self):
        return self.address

    def set_address(self, address):
        self.address = address




def main(top_block_cls=fm_channelize_and_view, options=None):

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
