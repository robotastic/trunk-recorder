#include "recorder.h"
#include "../source.h"
#include <boost/algorithm/string.hpp>
#include "../../lib/gr_blocks/nonstop_wavfile_sink_impl.h"
#include "../../lib/gr_blocks/nonstop_wavfile_delayopen_sink_impl.h"
#include "../../lib/gr_blocks/mp3_file_sink_impl.h"
#include "../../lib/gr_blocks/mp3_file_delayopen_sink_impl.h"

Recorder::Recorder(std::string type)
{
	this->type = type;
}

boost::property_tree::ptree Recorder::get_stats()
{
  	boost::property_tree::ptree node;
  	node.put("id",           	boost::lexical_cast<std::string>(get_source()->get_num()) + "_" + boost::lexical_cast<std::string>(get_num()));
	node.put("type",       		type);
  	node.put("srcNum",       	get_source()->get_num());
  	node.put("recNum",			get_num());
  	node.put("count", 	   		recording_count);
  	node.put("duration",    	recording_duration);
  	node.put("state",    		get_state());

	Rx_Status status = get_rx_status();
  	node.put("status_len",    	status.total_len);
	node.put("status_error", 	status.error_count);
	node.put("status_spike", 	status.spike_count);

  	return node;
}

boost::shared_ptr<gr::blocks::recording_file_sink> Recorder::make_audio_recorder(std::string recording_format, bool delayopen)
{
    if (recording_format == "mp3")
    {
        if (delayopen)
        {
            boost::shared_ptr<gr::blocks::mp3_file_delayopen_sink_impl> w = gr::blocks::mp3_file_delayopen_sink_impl::make(1, 8000, 16);
            w->set_recorder(this);

            return w;
        }
        else
        {
            return gr::blocks::mp3_file_sink_impl::make(1, 8000, 16);
        }
    }
    else
    {
        if (delayopen)
        {
            boost::shared_ptr<gr::blocks::nonstop_wavfile_delayopen_sink_impl> w = gr::blocks::nonstop_wavfile_delayopen_sink_impl::make(1, 8000, 16, true);
            w->set_recorder(this);

            return w;
        }
        else
        {
            return gr::blocks::nonstop_wavfile_sink_impl::make(1, 8000, 16, true);
        }
    }
    return NULL;
}