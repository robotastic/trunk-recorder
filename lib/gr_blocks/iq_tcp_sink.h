#ifndef _iq_tcp_sink_H_
#define _iq_tcp_sink_H_

#include <string>

#include <boost/log/trivial.hpp>
#include <gnuradio/blocks/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
	namespace blocks {
		class BLOCKS_API ISourceIntf
		{
		public:
			virtual double get_min_hz() { return 0; };
			virtual double get_max_hz() { return 0; };
			virtual double get_center() { return 0; };
			virtual double get_rate() { return 0; };
			virtual std::string get_antenna() { return ""; };
			virtual void set_antenna(std::string ant) {};
			virtual void set_freq_corr(double p) {};
			virtual void set_if_gain(int i) {};
			virtual void set_gain(int r) {};
		};

		class BLOCKS_API iq_tcp_sink : virtual public sync_block
		{
		public:
			typedef boost::shared_ptr<iq_tcp_sink> sptr;
		};
}
}

#endif // _iq_tcp_sink_H_