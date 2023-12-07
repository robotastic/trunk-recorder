import math

from gnuradio import gr, fft, blocks, filter

#from filter import optfir
#from filter import filter_python as filter, fft

// FILEPATH: /Users/luke/Projects/TrunkRecorder/trunk-recorder/trunk-recorder/gr_blocks/pfb_channelizer.h

#ifndef PFB_CHANNELIZER_H
#define PFB_CHANNELIZER_H

#include <gnuradio/hier_block2.h>
#include <gnuradio/blocks.h>
#include <gnuradio/filter/filterbank.h>
#include <gnuradio/fft/fft.h>
#include <vector>
#include <iostream>
#include <cmath>

class pfb_channelizer_hier_ccf : public gr::hier_block2
{
public:
    pfb_channelizer_hier_ccf(int n_chans, int n_filterbanks = 2, std::vector<float> taps = std::vector<float>(), std::vector<int> outchans = std::vector<int>(),
                             int atten = 100, float bw = 1.0, float tb = 0.2, float ripple = 0.1)
        : gr::hier_block2("pfb_channelizer_hier_ccf",
                          gr::io_signature::make(1, 1, sizeof(gr_complex)),
                          gr::io_signature::make(outchans.size(), outchans.size(), sizeof(gr_complex)))
    {
        if (n_filterbanks > n_chans)
            n_filterbanks = n_chans;
        if (outchans.empty())
        {
            for (int i = 0; i < n_chans; i++)
                outchans.push_back(i);
        }

        if (taps.empty())
        {
            taps = create_taps(n_chans, atten, bw, tb, ripple);
        }

        std::cout << "Tap Size: " << taps.size() << std::endl;
        int extra_taps = std::ceil(1.0 * taps.size() / n_chans) * n_chans - taps.size();
        std::cout << "Extra: " << extra_taps << std::endl;
        taps.resize(taps.size() + extra_taps, 0);
        std::cout << "Tap + Extra Size: " << taps.size() << std::endl;

        std::vector<std::vector<float>> chantaps;
        for (int i = 0; i < n_chans; i++)
        {
            std::vector<float> taps_for_channel;
            for (int j = i; j < taps.size(); j += n_chans)
            {
                taps_for_channel.push_back(taps[j]);
            }
            std::reverse(taps_for_channel.begin(), taps_for_channel.end());
            chantaps.push_back(taps_for_channel);
        }
        std::cout << "Chan Tap Size: " << chantaps.size() << std::endl;

        blocks::stream_to_vector::sptr s2v = blocks::stream_to_vector::make(sizeof(gr_complex), n_chans);
        std::vector<std::vector<std::pair<int, int>>> splitter_mapping;
        std::vector<std::vector<std::vector<float>>> filterbanktaps;
        int total = 0;
        for (int i = 0; i < n_filterbanks; i++)
        {
            int cpp = (i < outchans.size()) ? outchans.size() / n_filterbanks + (i < outchans.size() % n_filterbanks) : 0;
            std::cout << "CPP value in loop: " << cpp << std::endl;
            std::vector<std::pair<int, int>> filterbank_splitter_mapping;
            for (int j = total; j < total + cpp; j++)
            {
                filterbank_splitter_mapping.push_back(std::make_pair(0, j));
            }
            splitter_mapping.push_back(filterbank_splitter_mapping);
            std::cout << "New: " << splitter_mapping.size() << std::endl;
            filterbanktaps.push_back(std::vector<std::vector<float>>(chantaps.begin() + total, chantaps.begin() + total + cpp));
            std::cout << filterbanktaps.size() << std::endl;
            total += cpp;
        }
        assert(total == n_chans);
        blocks::vector_map::sptr splitter = blocks::vector_map::make(sizeof(gr_complex), std::vector<int>{n_chans}, splitter_mapping);
        std::vector<gr::filter::filterbank_vcvcf::sptr> fbs;
        for (const auto &taps : filterbanktaps)
        {
            gr::filter::filterbank_vcvcf::sptr filterbank = gr::filter::filterbank_vcvcf::make(taps);
            fbs.push_back(filterbank);
        }
        std::vector<std::vector<std::pair<int, int>>> combiner_mapping(1);
        for (int i = 0; i < n_filterbanks; i++)
        {
            int cpp = (i < outchans.size()) ? outchans.size() / n_filterbanks + (i < outchans.size() % n_filterbanks) : 0;
            std::cout << "i: " << i << " CPP: " << cpp << std::endl;
            for (int j = 0; j < cpp; j++)
            {
                combiner_mapping[0].push_back(std::make_pair(i, j));
            }
        }
        std::cout << "Combiner Map: " << combiner_mapping.size() << std::endl;
        blocks::vector_map::sptr combiner = blocks::vector_map::make(sizeof(gr_complex), cpps, combiner_mapping);
        gr::fft::fft_vcc::sptr fft = gr::fft::fft_vcc::make(n_chans, true, std::vector<float>(n_chans, 1.0));
        blocks::vector_map::sptr selector;
        if (outchans != std::vector<int>(n_chans))
        {
            std::vector<std::vector<std::pair<int, int>>> selector_mapping;
            std::vector<std::pair<int, int>> mapping;
            for (int i : outchans)
            {
                mapping.push_back(std::make_pair(0, i));
            }
            selector_mapping.push_back(mapping);
            selector = blocks::vector_map::make(sizeof(gr_complex), std::vector<int>{n_chans}, selector_mapping);
        }
        blocks::vector_to_streams::sptr v2ss = blocks::vector_to_streams::make(sizeof(gr_complex), outchans.size());

        connect(self(), s2v, splitter);
        for (int i = 0; i < n_filterbanks; i++)
        {
            connect(splitter, i, fbs[i], combiner, i);
        }
        connect(combiner, fft);
        if (outchans != std::vector<int>(n_chans))
        {
            connect(fft, selector, v2ss);
        }
        else
        {
            connect(fft, v2ss);
        }
        for (int i = 0; i < outchans.size(); i++)
        {
            connect(v2ss, i, self(), i);
        }
    }

    static std::vector<float> create_taps(int n_chans, int atten = 100, float bw = 1.0, float tb = 0.2, float ripple = 0.1)
    {
        return gr::filter::optfir::low_pass(1, n_chans, bw, bw + tb, ripple, atten);
    }
};

#endif // PFB_CHANNELIZER_H
