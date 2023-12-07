import math

from gnuradio import gr, fft, blocks, filter

#from filter import optfir
#from filter import filter_python as filter, fft

class channelizer_hier_ccf(gr.hier_block2):
    """
    Make a Polyphase Filter channelizer (complex in, complex out, floating-point taps)

    Args:
    n_chans: The number of channels to split into.
    n_filterbanks: The number of filterbank blocks to use (default=2).
    taps: The taps to use.  If this is `None` then taps are generated using optfir.low_pass.
    outchans: Which channels to output streams for (a list of integers) (default is all channels).
    atten: Stop band attenuation.
    bw: The fraction of the channel you want to keep.
    tb: Transition band with as fraction of channel width.
    ripple: Pass band ripple in dB.
    """

    def __init__(self, n_chans, n_filterbanks=2, taps=None, outchans=None,
                 atten=100, bw=1.0, tb=0.2, ripple=0.1):
        if n_filterbanks > n_chans:
            n_filterbanks = n_chans
        if outchans is None:
            outchans = list(range(n_chans))
        gr.hier_block2.__init__(
            self, "pfb_channelizer_hier_ccf",
            gr.io_signature(1, 1, gr.sizeof_gr_complex),
            gr.io_signature(len(outchans), len(outchans), gr.sizeof_gr_complex))
        if taps is None:
            taps = self.create_taps(n_chans, atten, bw, tb, ripple)
        taps = list(taps)
        print(f'Tap Size: {len(taps)}')
        extra_taps = int(math.ceil(1.0 * len(taps) / n_chans) *
                         n_chans - len(taps))
        print(f'Extra: {extra_taps}')
        taps = taps + [0] * extra_taps
        print(f'Tap + Extra Size: {len(taps)}')
        # Make taps for each channel
        chantaps = []
        for i in range(0, n_chans):
            taps_for_channel = []
            for j in range(i, len(taps), n_chans):
                taps_for_channel.append(taps[j])
            chantaps.append(list(reversed(taps_for_channel)))
        print(f'Chan Tap Size: {len(chantaps)}')
        # Convert the input stream into a stream of vectors.
        self.s2v = blocks.stream_to_vector(gr.sizeof_gr_complex, n_chans)
        # Create a mapping to separate out each filterbank (a group of channels to be processed together)
        # And a list of sets of taps for each filterbank.
        low_cpp = int(n_chans / n_filterbanks)
        print(f'low_cpp: {low_cpp}')
        extra = n_chans - low_cpp * n_filterbanks
        print(f'extra: {extra}')
        cpps = []
        for i in range(n_filterbanks):
            if i < extra:
                cpps.append(low_cpp + 1)
            else:
                cpps.append(low_cpp)
        #CPPs = channels per filterbank
        print(f'CPPs = channels per filterbank: {cpps}')
        splitter_mapping = []
        filterbanktaps = []
        total = 0
        for cpp in cpps:
            print(f'CPP value in loop: {cpp}')
            print(f'Splitter mapping range: {range(total, total + cpp)}')
            filterbank_splitter_mapping = []
            for i in range(total, total + cpp):
                filterbank_splitter_mapping.append((0, i))
            splitter_mapping.append(filterbank_splitter_mapping)
            print(f'New: {splitter_mapping}')
            filterbanktaps.append(chantaps[total: total + cpp])
            print(len(filterbanktaps))
            total += cpp
        assert(total == n_chans)
        # Split the stream of vectors in n_filterbanks streams of vectors.
        self.splitter = blocks.vector_map(
            gr.sizeof_gr_complex, [n_chans], splitter_mapping)
        # Create the filterbanks
        self.fbs = []
        for taps in filterbanktaps:
            filterbank = filter.filterbank_vcvcf(taps)
            self.fbs.append(filterbank)
        # Combine the streams of vectors back into a single stream of vectors.
        combiner_mapping = [[]]
        
        for i, cpp in enumerate(cpps):
            print(f'i: {i} CPP: {cpp}')
            for j in range(cpp):
                combiner_mapping[0].append((i, j))
        print(f'Combiner Map: {combiner_mapping}')
        self.combiner = blocks.vector_map(
            gr.sizeof_gr_complex, cpps, combiner_mapping)
        # Add the final FFT to the channelizer.
        self.fft = fft.fft_vcc(n_chans, forward=True, window=[1.0] * n_chans)
        # Select the desired channels
        if outchans != list(range(n_chans)):
            mapping = []
            selector_mapping = []
            for i in outchans:
                mapping.append((0, i))
            selector_mapping.append(mapping)
            self.selector = blocks.vector_map(
                gr.sizeof_gr_complex, [n_chans], selector_mapping)
        # Convert stream of vectors to a normal stream.
        self.v2ss = blocks.vector_to_streams(
            gr.sizeof_gr_complex, len(outchans))
        self.connect(self, self.s2v, self.splitter)
        for i in range(0, n_filterbanks):
            self.connect((self.splitter, i), self.fbs[i], (self.combiner, i))
        self.connect(self.combiner, self.fft)
        if outchans != list(range(n_chans)):
            self.connect(self.fft, self.selector, self.v2ss)
        else:
            self.connect(self.fft, self.v2ss)
        for i in range(0, len(outchans)):
            self.connect((self.v2ss, i), (self, i))

    @staticmethod
    def create_taps(n_chans, atten=100, bw=1.0, tb=0.2, ripple=0.1):
        return filter.optfir.low_pass(1, n_chans, bw, bw + tb, ripple, atten)