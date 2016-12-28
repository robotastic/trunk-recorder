#include "source.h"



void Source::set_antenna(std::string ant)
{
	antenna = ant;
	if (driver == "usrp") {
		BOOST_LOG_TRIVIAL(info) << "Setting antenna to [" << antenna << "]";
		cast_to_usrp_sptr(source_block)->set_antenna(antenna,0);
	}
}
std::string Source::get_antenna() {
	return antenna;
}
double Source::get_min_hz() {
	return min_hz;
}
double Source::get_max_hz() {
	return max_hz;
}
double Source::get_center() {
	return center;
}
double Source::get_rate() {
	return rate;
}
std::string Source::get_driver() {
	return driver;
}
std::string Source::get_device() {
	return device;
}
void Source::set_error(double e) {
	error = e;
}
double Source::get_error() {
	return error;
}
void Source::set_bb_gain(int b)
{
	if (driver == "osmosdr") {
		bb_gain = b;
		cast_to_osmo_sptr(source_block)->set_bb_gain(bb_gain);
	}
}
int Source::get_bb_gain() {
	return bb_gain;
}
void Source::set_gain(int r)
{
	if (driver == "osmosdr") {
		gain = r;
		cast_to_osmo_sptr(source_block)->set_gain(gain);
	}
	if (driver == "usrp") {
		gain = r;
		cast_to_usrp_sptr(source_block)->set_gain(gain);
	}
}

int Source::get_gain() {
	return gain;
}
void Source::set_if_gain(int i)
{
	if (driver == "osmosdr") {
		if_gain = i;
		cast_to_osmo_sptr(source_block)->set_if_gain(if_gain);
	}
}



void Source::set_freq_corr(double p)
{
    ppm = p;
    if (driver == "osmosdr") {
        cast_to_osmo_sptr(source_block)->set_freq_corr(ppm);
    }

}
int Source::get_if_gain() {
	return if_gain;
}

void Source::set_squelch_db(double s) {
    squelch_db = s;
}

double Source::get_squelch_db() {
    return squelch_db;
}

void Source::create_analog_recorders(gr::top_block_sptr tb, int r) {
	max_analog_recorders = r;

	for (int i = 0; i < max_analog_recorders; i++) {
		analog_recorder_sptr log = make_analog_recorder( this);
		analog_recorders.push_back(log);
		tb->connect(source_block, 0, log, 0);
	}
}
Recorder * Source::get_analog_recorder(int priority)
{
    if (priority > 99) { 
 		BOOST_LOG_TRIVIAL(info) << "\t\tNot recording because of priority";
 		return NULL;
 	}
	for(std::vector<analog_recorder_sptr>::iterator it = analog_recorders.begin(); it != analog_recorders.end(); it++) {
		analog_recorder_sptr rx = *it;
		if (!rx->is_active())
		{
			return (Recorder *) rx.get();
			break;
		}
	}
	BOOST_LOG_TRIVIAL(info) << "[ " << driver << " ] No Analog Recorders Available";
	return NULL;

}
void Source::create_digital_recorders(gr::top_block_sptr tb, int r, bool qpsk) {
	max_digital_recorders = r;

	for (int i = 0; i < max_digital_recorders; i++) {

		p25_recorder_sptr log = make_p25_recorder( this, qpsk);
		digital_recorders.push_back(log);
		tb->connect(source_block, 0, log, 0);
	}
}
void Source::create_debug_recorders(gr::top_block_sptr tb, int r) {
	max_debug_recorders = r;

	for (int i = 0; i < max_debug_recorders; i++) {

		debug_recorder_sptr log = make_debug_recorder( this);

		debug_recorders.push_back(log);
		tb->connect(source_block, 0, log, 0);
	}
}

Recorder * Source::get_debug_recorder()
{
	for(std::vector<debug_recorder_sptr>::iterator it = debug_recorders.begin(); it != debug_recorders.end(); it++) {
		debug_recorder_sptr rx = *it;
		if (!rx->is_active())
		{
			return (Recorder *) rx.get();
			break;
		}
	}
	//BOOST_LOG_TRIVIAL(info) << "[ " << driver << " ] No Debug Recorders Available";
	return NULL;

}

int Source::get_num_available_recorders() {
	int num_available_recorders = 0;

	for(std::vector<p25_recorder_sptr>::iterator it = digital_recorders.begin(); it != digital_recorders.end(); it++) {
		p25_recorder_sptr rx = *it;
		if (!rx->is_active())
		{
			num_available_recorders++;
		}
	}
	return num_available_recorders;
}

Recorder * Source::get_digital_recorder(int priority)
{
	//int num_available_recorders = get_num_available_recorders();
	//BOOST_LOG_TRIVIAL(info) << "\tTG Priority: "<< priority << " Available Digital Recorders: " <<num_available_recorders;

 	if (priority> 99) { //num_available_recorders) { // a low priority is bad. You need atleast the number of availalbe recorders to your priority
		//BOOST_LOG_TRIVIAL(info) << "Not recording because of priority";
		return NULL;
	}


	for(std::vector<p25_recorder_sptr>::iterator it = digital_recorders.begin(); it != digital_recorders.end(); it++) {
		p25_recorder_sptr rx = *it;
        
		if (!rx->is_active())
		{
			return (Recorder *) rx.get();
			break;
		}
	}
	BOOST_LOG_TRIVIAL(info) << "[ " << driver << " ] No Digital Recorders Available";
	return NULL;

}
gr::basic_block_sptr Source::get_src_block() {
	return source_block;
}
Source::Source(double c, double r, double e, std::string drv, std::string dev)
{
	rate = r;
	center = c;
	error = e;
	min_hz = center - (rate/2);
	max_hz = center + (rate/2);
	driver = drv;
	device = dev;
	if (driver == "osmosdr") {
		osmosdr::source::sptr osmo_src;
		if (dev == "") {
			BOOST_LOG_TRIVIAL(info) << "Source Device not specified";
			osmo_src = osmosdr::source::make();
		} else {
			std::ostringstream msg;
			if(isdigit(dev[0])) {	// Assume this is a serial number and fail back
						// to using rtl as default
				msg << "rtl=" << dev << ",buflen=16384,buffers=8";
				BOOST_LOG_TRIVIAL(info) << "Source device name missing, defaulting to rtl device";
			} else {
				msg << dev << ",buflen=16384,buffers=8";
			}
			BOOST_LOG_TRIVIAL(info) << "Source Device: " << msg.str();
			osmo_src = osmosdr::source::make(msg.str());
		}
		BOOST_LOG_TRIVIAL(info) << "SOURCE TYPE OSMOSDR (osmosdr)";
		BOOST_LOG_TRIVIAL(info) << "Setting sample rate to: " << rate;
		osmo_src->set_sample_rate(rate);
        actual_rate = osmo_src->get_sample_rate();
        BOOST_LOG_TRIVIAL(info) << "Actual sample rate: " << actual_rate;
		BOOST_LOG_TRIVIAL(info) << "Tunning to " << center + error << "hz";
		osmo_src->set_center_freq(center + error,0);
		source_block = osmo_src;
	}
	if (driver == "usrp") {
		gr::uhd::usrp_source::sptr usrp_src;
		usrp_src = gr::uhd::usrp_source::make(device,uhd::stream_args_t("fc32"));

		BOOST_LOG_TRIVIAL(info) << "SOURCE TYPE USRP (UHD)";

		BOOST_LOG_TRIVIAL(info) << "Setting sample rate to: " << rate;
		usrp_src->set_samp_rate(rate);
		actual_rate = usrp_src->get_samp_rate();
		BOOST_LOG_TRIVIAL(info) << "Actual sample rate: " << actual_rate;
		BOOST_LOG_TRIVIAL(info) << "Tunning to " << center + error << "hz";
		usrp_src->set_center_freq(center + error,0);


		source_block = usrp_src;
	}
}
