#!/usr/bin/env python2
# -*- coding: utf-8 -*-
##################################################
# GNU Radio Python Flow Graph
# Title: Debug Grc
# Generated: Sun Jan 22 19:12:23 2017
##################################################

if __name__ == '__main__':
    import ctypes
    import sys
    if sys.platform.startswith('linux'):
        try:
            x11 = ctypes.cdll.LoadLibrary('libX11.so')
            x11.XInitThreads()
        except:
            print "Warning: failed to XInitThreads()"

from gnuradio import analog
from gnuradio import blocks
from gnuradio import eng_notation
from gnuradio import filter
from gnuradio import gr
from gnuradio import wxgui
from gnuradio.eng_option import eng_option
from gnuradio.fft import window
from gnuradio.filter import firdes
from gnuradio.wxgui import fftsink2
from gnuradio.wxgui import forms
from gnuradio.wxgui import scopesink2
from gnuradio.wxgui import waterfallsink2
from grc_gnuradio import wxgui as grc_wxgui
from optparse import OptionParser
import math
import wx


class debug_grc(grc_wxgui.top_block_gui):

    def __init__(self):
        grc_wxgui.top_block_gui.__init__(self, title="Debug Grc")

        ##################################################
        # Variables
        ##################################################
        self.samp_per_sym = samp_per_sym = 10
        self.samp_rate = samp_rate = 96000
        self.channel_rate = channel_rate = 4800*samp_per_sym
        self.xlate_trans = xlate_trans = 3000
        self.xlate_offset_fine = xlate_offset_fine = 0
        self.xlate_bandwidth = xlate_bandwidth = 6500
        self.pfb_taps = pfb_taps = firdes.low_pass_2(1, samp_rate, 6250, 4000, 60, firdes.WIN_HANN)
        self.fsk_level = fsk_level = 100
        self.decim = decim = samp_rate/channel_rate
        self.audio_mul = audio_mul = 0

        ##################################################
        # Blocks
        ##################################################
        _xlate_trans_sizer = wx.BoxSizer(wx.VERTICAL)
        self._xlate_trans_text_box = forms.text_box(
        	parent=self.GetWin(),
        	sizer=_xlate_trans_sizer,
        	value=self.xlate_trans,
        	callback=self.set_xlate_trans,
        	label='Xlate Trans',
        	converter=forms.float_converter(),
        	proportion=0,
        )
        self._xlate_trans_slider = forms.slider(
        	parent=self.GetWin(),
        	sizer=_xlate_trans_sizer,
        	value=self.xlate_trans,
        	callback=self.set_xlate_trans,
        	minimum=1000,
        	maximum=10000,
        	num_steps=1000,
        	style=wx.SL_HORIZONTAL,
        	cast=float,
        	proportion=1,
        )
        self.Add(_xlate_trans_sizer)
        _xlate_offset_fine_sizer = wx.BoxSizer(wx.VERTICAL)
        self._xlate_offset_fine_text_box = forms.text_box(
        	parent=self.GetWin(),
        	sizer=_xlate_offset_fine_sizer,
        	value=self.xlate_offset_fine,
        	callback=self.set_xlate_offset_fine,
        	label='Fine Offset',
        	converter=forms.float_converter(),
        	proportion=0,
        )
        self._xlate_offset_fine_slider = forms.slider(
        	parent=self.GetWin(),
        	sizer=_xlate_offset_fine_sizer,
        	value=self.xlate_offset_fine,
        	callback=self.set_xlate_offset_fine,
        	minimum=-10000,
        	maximum=10000,
        	num_steps=1000,
        	style=wx.SL_HORIZONTAL,
        	cast=float,
        	proportion=1,
        )
        self.Add(_xlate_offset_fine_sizer)
        _xlate_bandwidth_sizer = wx.BoxSizer(wx.VERTICAL)
        self._xlate_bandwidth_text_box = forms.text_box(
        	parent=self.GetWin(),
        	sizer=_xlate_bandwidth_sizer,
        	value=self.xlate_bandwidth,
        	callback=self.set_xlate_bandwidth,
        	label='Xlate BW',
        	converter=forms.float_converter(),
        	proportion=0,
        )
        self._xlate_bandwidth_slider = forms.slider(
        	parent=self.GetWin(),
        	sizer=_xlate_bandwidth_sizer,
        	value=self.xlate_bandwidth,
        	callback=self.set_xlate_bandwidth,
        	minimum=3000,
        	maximum=10000,
        	num_steps=1000,
        	style=wx.SL_HORIZONTAL,
        	cast=float,
        	proportion=1,
        )
        self.Add(_xlate_bandwidth_sizer)
        self.nb = self.nb = wx.Notebook(self.GetWin(), style=wx.NB_TOP)
        self.nb.AddPage(grc_wxgui.Panel(self.nb), "BB-1")
        self.nb.AddPage(grc_wxgui.Panel(self.nb), "BB-2")
        self.nb.AddPage(grc_wxgui.Panel(self.nb), "Xlate-1")
        self.nb.AddPage(grc_wxgui.Panel(self.nb), "Xlate-2")
        self.nb.AddPage(grc_wxgui.Panel(self.nb), "4FSK")
        self.Add(self.nb)
        _fsk_level_sizer = wx.BoxSizer(wx.VERTICAL)
        self._fsk_level_text_box = forms.text_box(
        	parent=self.GetWin(),
        	sizer=_fsk_level_sizer,
        	value=self.fsk_level,
        	callback=self.set_fsk_level,
        	label='FSK Level',
        	converter=forms.float_converter(),
        	proportion=0,
        )
        self._fsk_level_slider = forms.slider(
        	parent=self.GetWin(),
        	sizer=_fsk_level_sizer,
        	value=self.fsk_level,
        	callback=self.set_fsk_level,
        	minimum=1,
        	maximum=200,
        	num_steps=100,
        	style=wx.SL_HORIZONTAL,
        	cast=float,
        	proportion=1,
        )
        self.Add(_fsk_level_sizer)
        self.wxgui_waterfallsink2_0_0 = waterfallsink2.waterfall_sink_c(
        	self.nb.GetPage(3).GetWin(),
        	baseband_freq=0,
        	dynamic_range=10,
        	ref_level=10,
        	ref_scale=2.0,
        	sample_rate=channel_rate,
        	fft_size=512,
        	fft_rate=15,
        	average=False,
        	avg_alpha=None,
        	title='Waterfall Plot',
        )
        self.nb.GetPage(3).Add(self.wxgui_waterfallsink2_0_0.win)
        self.wxgui_waterfallsink2_0 = waterfallsink2.waterfall_sink_c(
        	self.nb.GetPage(1).GetWin(),
        	baseband_freq=0,
        	dynamic_range=100,
        	ref_level=50,
        	ref_scale=2.0,
        	sample_rate=samp_rate,
        	fft_size=512,
        	fft_rate=15,
        	average=False,
        	avg_alpha=None,
        	title='Waterfall Plot',
        	win=window.flattop,
        )
        self.nb.GetPage(1).Add(self.wxgui_waterfallsink2_0.win)
        self.wxgui_scopesink2_1 = scopesink2.scope_sink_f(
        	self.nb.GetPage(4).GetWin(),
        	title='Scope Plot',
        	sample_rate=channel_rate,
        	v_scale=1.5,
        	v_offset=0,
        	t_scale=0.05,
        	ac_couple=False,
        	xy_mode=False,
        	num_inputs=1,
        	trig_mode=wxgui.TRIG_MODE_AUTO,
        	y_axis_label='Counts',
        )
        self.nb.GetPage(4).Add(self.wxgui_scopesink2_1.win)
        self.wxgui_fftsink2_0_0 = fftsink2.fft_sink_c(
        	self.nb.GetPage(2).GetWin(),
        	baseband_freq=0,
        	y_per_div=10,
        	y_divs=10,
        	ref_level=0,
        	ref_scale=2.0,
        	sample_rate=channel_rate,
        	fft_size=1024,
        	fft_rate=30,
        	average=True,
        	avg_alpha=None,
        	title='FFT Plot',
        	peak_hold=False,
        	win=window.flattop,
        )
        self.nb.GetPage(2).Add(self.wxgui_fftsink2_0_0.win)
        def wxgui_fftsink2_0_0_callback(x, y):
        	self.set_0(x)
        
        self.wxgui_fftsink2_0_0.set_callback(wxgui_fftsink2_0_0_callback)
        self.wxgui_fftsink2_0 = fftsink2.fft_sink_c(
        	self.nb.GetPage(0).GetWin(),
        	baseband_freq=0,
        	y_per_div=20,
        	y_divs=10,
        	ref_level=0,
        	ref_scale=2.0,
        	sample_rate=samp_rate,
        	fft_size=1024,
        	fft_rate=30,
        	average=True,
        	avg_alpha=None,
        	title='FFT Plot',
        	peak_hold=False,
        )
        self.nb.GetPage(0).Add(self.wxgui_fftsink2_0.win)
        def wxgui_fftsink2_0_callback(x, y):
        	self.set_0(x)
        
        self.wxgui_fftsink2_0.set_callback(wxgui_fftsink2_0_callback)
        self.freq_xlating_fir_filter_xxx_0 = filter.freq_xlating_fir_filter_ccc(decim, (firdes.low_pass_2(1, samp_rate, xlate_bandwidth, xlate_trans, 60, firdes.WIN_HANN)), xlate_offset_fine, samp_rate)
        self.blocks_throttle_0 = blocks.throttle(gr.sizeof_gr_complex*1, samp_rate,True)
        self.blocks_multiply_const_vxx_0 = blocks.multiply_const_vff((fsk_level*.01, ))
        self.blocks_file_source_0 = blocks.file_source(gr.sizeof_gr_complex*1, '/Users/luke/Dropbox/iq/34800-1485122982_4.90888e+08.raw', True)
        _audio_mul_sizer = wx.BoxSizer(wx.VERTICAL)
        self._audio_mul_text_box = forms.text_box(
        	parent=self.GetWin(),
        	sizer=_audio_mul_sizer,
        	value=self.audio_mul,
        	callback=self.set_audio_mul,
        	label='Audio mul',
        	converter=forms.float_converter(),
        	proportion=0,
        )
        self._audio_mul_slider = forms.slider(
        	parent=self.GetWin(),
        	sizer=_audio_mul_sizer,
        	value=self.audio_mul,
        	callback=self.set_audio_mul,
        	minimum=-30,
        	maximum=10,
        	num_steps=40,
        	style=wx.SL_HORIZONTAL,
        	cast=float,
        	proportion=1,
        )
        self.Add(_audio_mul_sizer)
        self.analog_quadrature_demod_cf_0 = analog.quadrature_demod_cf(12.7324)
        self.analog_agc2_xx_0 = analog.agc2_cc(1e-1, 1e-2, 1.0, 1.0)
        self.analog_agc2_xx_0.set_max_gain(65536)

        ##################################################
        # Connections
        ##################################################
        self.connect((self.analog_agc2_xx_0, 0), (self.analog_quadrature_demod_cf_0, 0))    
        self.connect((self.analog_agc2_xx_0, 0), (self.wxgui_fftsink2_0_0, 0))    
        self.connect((self.analog_agc2_xx_0, 0), (self.wxgui_waterfallsink2_0_0, 0))    
        self.connect((self.analog_quadrature_demod_cf_0, 0), (self.blocks_multiply_const_vxx_0, 0))    
        self.connect((self.blocks_file_source_0, 0), (self.blocks_throttle_0, 0))    
        self.connect((self.blocks_multiply_const_vxx_0, 0), (self.wxgui_scopesink2_1, 0))    
        self.connect((self.blocks_throttle_0, 0), (self.freq_xlating_fir_filter_xxx_0, 0))    
        self.connect((self.blocks_throttle_0, 0), (self.wxgui_fftsink2_0, 0))    
        self.connect((self.blocks_throttle_0, 0), (self.wxgui_waterfallsink2_0, 0))    
        self.connect((self.freq_xlating_fir_filter_xxx_0, 0), (self.analog_agc2_xx_0, 0))    

    def get_samp_per_sym(self):
        return self.samp_per_sym

    def set_samp_per_sym(self, samp_per_sym):
        self.samp_per_sym = samp_per_sym
        self.set_channel_rate(4800*self.samp_per_sym)

    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.set_decim(self.samp_rate/self.channel_rate)
        self.wxgui_waterfallsink2_0.set_sample_rate(self.samp_rate)
        self.wxgui_fftsink2_0.set_sample_rate(self.samp_rate)
        self.set_pfb_taps(firdes.low_pass_2(1, self.samp_rate, 6250, 4000, 60, firdes.WIN_HANN))
        self.freq_xlating_fir_filter_xxx_0.set_taps((firdes.low_pass_2(1, self.samp_rate, self.xlate_bandwidth, self.xlate_trans, 60, firdes.WIN_HANN)))
        self.blocks_throttle_0.set_sample_rate(self.samp_rate)

    def get_channel_rate(self):
        return self.channel_rate

    def set_channel_rate(self, channel_rate):
        self.channel_rate = channel_rate
        self.set_decim(self.samp_rate/self.channel_rate)
        self.wxgui_waterfallsink2_0_0.set_sample_rate(self.channel_rate)
        self.wxgui_scopesink2_1.set_sample_rate(self.channel_rate)
        self.wxgui_fftsink2_0_0.set_sample_rate(self.channel_rate)

    def get_xlate_trans(self):
        return self.xlate_trans

    def set_xlate_trans(self, xlate_trans):
        self.xlate_trans = xlate_trans
        self._xlate_trans_slider.set_value(self.xlate_trans)
        self._xlate_trans_text_box.set_value(self.xlate_trans)
        self.freq_xlating_fir_filter_xxx_0.set_taps((firdes.low_pass_2(1, self.samp_rate, self.xlate_bandwidth, self.xlate_trans, 60, firdes.WIN_HANN)))

    def get_xlate_offset_fine(self):
        return self.xlate_offset_fine

    def set_xlate_offset_fine(self, xlate_offset_fine):
        self.xlate_offset_fine = xlate_offset_fine
        self._xlate_offset_fine_slider.set_value(self.xlate_offset_fine)
        self._xlate_offset_fine_text_box.set_value(self.xlate_offset_fine)
        self.freq_xlating_fir_filter_xxx_0.set_center_freq(self.xlate_offset_fine)

    def get_xlate_bandwidth(self):
        return self.xlate_bandwidth

    def set_xlate_bandwidth(self, xlate_bandwidth):
        self.xlate_bandwidth = xlate_bandwidth
        self._xlate_bandwidth_slider.set_value(self.xlate_bandwidth)
        self._xlate_bandwidth_text_box.set_value(self.xlate_bandwidth)
        self.freq_xlating_fir_filter_xxx_0.set_taps((firdes.low_pass_2(1, self.samp_rate, self.xlate_bandwidth, self.xlate_trans, 60, firdes.WIN_HANN)))

    def get_pfb_taps(self):
        return self.pfb_taps

    def set_pfb_taps(self, pfb_taps):
        self.pfb_taps = pfb_taps

    def get_fsk_level(self):
        return self.fsk_level

    def set_fsk_level(self, fsk_level):
        self.fsk_level = fsk_level
        self._fsk_level_slider.set_value(self.fsk_level)
        self._fsk_level_text_box.set_value(self.fsk_level)
        self.blocks_multiply_const_vxx_0.set_k((self.fsk_level*.01, ))

    def get_decim(self):
        return self.decim

    def set_decim(self, decim):
        self.decim = decim

    def get_audio_mul(self):
        return self.audio_mul

    def set_audio_mul(self, audio_mul):
        self.audio_mul = audio_mul
        self._audio_mul_slider.set_value(self.audio_mul)
        self._audio_mul_text_box.set_value(self.audio_mul)


def main(top_block_cls=debug_grc, options=None):

    tb = top_block_cls()
    tb.Start(True)
    tb.Wait()


if __name__ == '__main__':
    main()
