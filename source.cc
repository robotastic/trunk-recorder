   #include "source.h" 



    void Source::set_antenna(std::string ant) 
    {
        antenna = ant;
        if (driver == "usrp") {
            std::cout << "Setting antenna to [" << antenna << "]" << std::endl;
            cast_to_usrp_sptr(source_block)->set_antenna(antenna,0);
        }
    }
    std::string Source::get_antenna() {return antenna;}
    double Source::get_min_hz() {return min_hz;}
    double Source::get_max_hz() {return max_hz;}
    double Source::get_center() {return center;}
    double Source::get_rate() {return rate;}
    std::string Source::get_driver() {return driver;} 
    std::string Source::get_device() {return device;} 
    void Source::set_error(double e) {error = e;}
    double Source::get_error() {return error;}
    void Source::set_bb_gain(int b) 
    {
        if (driver == "osmosdr") {
            bb_gain = b;
            cast_to_osmo_sptr(source_block)->set_bb_gain(bb_gain);
        } 
    }
    int Source::get_bb_gain() {return bb_gain;}
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
    int Source::get_gain() {return gain;}
    void Source::set_if_gain(int i) 
    {
        if (driver == "osmosdr") {
            if_gain = i;
            cast_to_osmo_sptr(source_block)->set_if_gain(if_gain);
        } 
    }
    int Source::get_if_gain() {return if_gain;}
    void Source::create_analog_recorders(gr::top_block_sptr tb, int r) {
        max_analog_recorders = r;

        for (int i = 0; i < max_analog_recorders; i++) {
            analog_recorder_sptr log = make_analog_recorder( center, center, rate, 0, i);
            analog_recorders.push_back(log);
            tb->connect(source_block, 0, log, 0);
        }
    }
    Recorder * Source::get_analog_recorder() 
    {
            for(std::vector<analog_recorder_sptr>::iterator it = analog_recorders.begin(); it != analog_recorders.end();it++) {
                analog_recorder_sptr rx = *it;
                if (!rx->is_active())
                {
                    return (Recorder *) rx.get();
                    break;
                }
            }
        std::cout << "[ " << driver << " ] No Analog Recorders Available" << std::endl;
        return NULL;
        
    }
    void Source::create_digital_recorders(gr::top_block_sptr tb, int r) {
        max_digital_recorders = r;

        for (int i = 0; i < max_digital_recorders; i++) {
            dsd_recorder_sptr log = make_dsd_recorder( center, center, rate, 0, i);
            digital_recorders.push_back(log);
            tb->connect(source_block, 0, log, 0);
        }
    }
    Recorder * Source::get_digital_recorder() 
    {
            for(std::vector<dsd_recorder_sptr>::iterator it = digital_recorders.begin(); it != digital_recorders.end();it++) {
                dsd_recorder_sptr rx = *it;
                if (!rx->is_active())
                {
                    return (Recorder *) rx.get();
                    break;
                }
            }
        std::cout << "[ " << driver << " ] No Digital Recorders Available" << std::endl;
        return NULL;
        
    }
    gr::basic_block_sptr Source::get_src_block() {return source_block;}
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
            osmo_src = osmosdr::source::make();

            std::cout << "SOURCE TYPE OSMOSDR (osmosdr)" << std::endl;
            std::cout << "Setting sample rate to: " << rate << std::endl;
            osmo_src->set_sample_rate(rate);
            std::cout << "Tunning to " << center + error << "hz" << std::endl;
            osmo_src->set_center_freq(center + error,0);
            source_block = osmo_src;
        }
        if (driver == "usrp") { 
            gr::uhd::usrp_source::sptr usrp_src;
            usrp_src = gr::uhd::usrp_source::make(device,uhd::stream_args_t("fc32"));

            std::cout << "SOURCE TYPE USRP (UHD)" << std::endl;       

            std::cout << "Setting sample rate to: " << rate << std::endl;
            usrp_src->set_samp_rate(rate);
            double actual_samp_rate = usrp_src->get_samp_rate();
            std::cout << "Actual sample rate: " << actual_samp_rate << std::endl;
            std::cout << "Tunning to " << center + error << "hz" << std::endl;
            usrp_src->set_center_freq(center + error,0);
            

            source_block = usrp_src;
        }
    }