#include "p25_parser.h"
P25Parser::P25Parser() {

}

void P25Parser::add_channel(int chan_id, Channel temp_chan) {
//std::cout << "Add  - Channel id " << std::dec << chan_id << " freq " <<  temp_chan.frequency << " offset " << temp_chan.offset << " step " << temp_chan.step << " slots/carrier " << temp_chan.tdma  << std::endl;

	channels[chan_id] = temp_chan;
}


long P25Parser::get_tdma_slot(int chan_id) {

	long channel = chan_id & 0xfff;
	it = channels.find((chan_id >> 12) & 0xf);

	if (it != channels.end()) {
		Channel temp_chan = it->second;
		if (temp_chan.tdma != 0) {
			return channel & 1;
		}
	}

	return 0;
}

double P25Parser::get_bandwidth(int chan_id) {

	long channel = chan_id & 0xfff;
	it = channels.find((chan_id >> 12) & 0xf);

	if (it != channels.end()) {
		Channel temp_chan = it->second;
		return temp_chan.bandwidth;
	}

	return 0;
}
std::string  P25Parser::channel_id_to_string(int chan_id) {
	double f = channel_id_to_frequency(chan_id);
	if (f == 0) {
		return "ID"; // << std::hex << chan_id;
	} else {
		std::ostringstream strs;
		strs << f / 1000000.0;
		return strs.str();
	}
}
double P25Parser::channel_id_to_frequency(int chan_id) {
	long id = (chan_id >> 12) & 0xf;
	long channel = chan_id & 0xfff;
	it = channels.find((chan_id >> 12) & 0xf);

	if (it != channels.end()) {
		Channel temp_chan = it->second;

		if (temp_chan.tdma == 0) {
			//std::cout << " Freq " << temp_chan.frequency + temp_chan.step * channel << std::endl;
			return temp_chan.frequency + temp_chan.step * channel;
		} else {
			//std::cout << " Freq " << temp_chan.frequency + temp_chan.step * int(channel / temp_chan.tdma) << std::endl;
			return temp_chan.frequency + temp_chan.step * int(channel / temp_chan.tdma);
		}
	}
	BOOST_LOG_TRIVIAL(info) << "\tFind - ChanId " << chan_id << " map id " << id  << "Channel " << channel << " size " << channels.size() << " ! Not Found !";
	return  0;
}

unsigned long P25Parser::bitset_shift_mask(boost::dynamic_bitset<> &tsbk, int shift, unsigned long long mask) {
	boost::dynamic_bitset<> bitmask(tsbk.size(), mask);
	unsigned long result = ((tsbk >> shift) & bitmask).to_ulong();
	//std::cout << "    " << std::dec<< shift << " " << tsbk.size() << " [ " << mask << " ]  = " << result << " - " << ((tsbk >> shift) & bitmask) << std::endl;
	return result;
}

std::vector<TrunkMessage> P25Parser::decode_tsbk(boost::dynamic_bitset<> &tsbk) {
	//self.stats['tsbks'] += 1
	long updated = 0;
	std::vector<TrunkMessage> messages;
	TrunkMessage message;


	message.message_type = UNKNOWN;

	unsigned long opcode = bitset_shift_mask(tsbk,88,0x3f); //x3f


	//std::cout << "TSBK: OpCode: 0x" << std::hex << opcode << " TBSK: 0x " << tsbk << std::endl;
	if (opcode == 0x00 ) { // group voice chan grant
		unsigned long mfrid  = bitset_shift_mask(tsbk,80,0xff);
		unsigned long opts  = bitset_shift_mask(tsbk,72,0xff);
		bool emergency = (bool) bitset_shift_mask(tsbk,72,0x80);
		bool encrypted = (bool) bitset_shift_mask(tsbk,72,0x40);
		bool duplex = (bool) bitset_shift_mask(tsbk,72,0x20);
		bool mode = (bool) bitset_shift_mask(tsbk,72,0x10);
		unsigned long priority = bitset_shift_mask(tsbk,72,0x07);
		unsigned long ch   = bitset_shift_mask(tsbk,56,0xffff);
		unsigned long ga   = bitset_shift_mask(tsbk,40,0xffff);
		unsigned long sa   = bitset_shift_mask(tsbk,16,0xffffff);
		if (mfrid == 0x90) {
			    unsigned long sg  = bitset_shift_mask(tsbk, 64, 0xffff);
                unsigned long ga1   = bitset_shift_mask(tsbk, 48, 0xffff);
                unsigned long ga2   = bitset_shift_mask(tsbk, 32, 0xffff);
                unsigned long ga3   = bitset_shift_mask(tsbk, 16, 0xffff);
                BOOST_LOG_TRIVIAL(trace) << "tsbk00\tMoto Patch Add \tsg: " << sg << "\tga1: " << ga1 << "\tga2: " << ga2 << "\tga3: " << ga3;


		} else {

			unsigned long f1 = channel_id_to_frequency(ch);
			message.message_type = GRANT;
			message.freq = f1;
			message.talkgroup = ga;
			message.source = sa;
			message.emergency = emergency;
			message.encrypted = encrypted;
			if (get_tdma_slot(ch)>0) {
				message.tdma = true;
			} else {
				message.tdma = false;
			}

			BOOST_LOG_TRIVIAL(trace) << "tsbk00\tChan Grant\tChannel ID: " << std::setw(5) << ch << "\tFreq: "<< f1/1000000.0 << "\tga " << std::setw(7) << ga  << "\tTDMA " << get_tdma_slot(ch) << "\tsa " << sa << "\tEncrypt " << encrypted << "\tBandwidth: " << get_bandwidth(ch);
		}

	} else if (opcode == 0x02) {  // group voice chan grant update
		unsigned long mfrid  = bitset_shift_mask(tsbk, 80, 0xff);

		if (mfrid == 0x90) {
			unsigned long ch  = bitset_shift_mask(tsbk, 56, 0xffff);
			unsigned long f = channel_id_to_frequency(ch);
             unsigned long   sg  = bitset_shift_mask(tsbk, 40, 0xffff);
             unsigned long   sa  = bitset_shift_mask(tsbk, 16, 0xffffff);

            message.message_type = GRANT;
			message.freq = f;
			message.talkgroup = sg;
			message.source = sa;
			message.emergency = false;
			message.encrypted = false;
			if (get_tdma_slot(ch)>0) {
				message.tdma = true;
			} else {
				message.tdma = false;
			}




            BOOST_LOG_TRIVIAL(error) << "tbsk02\tMoto Patch Grant\tChannel ID: " << std::setw(5) << ch << "\tFreq: "<< f/1000000.0 << "\tsg " << std::setw(7) << sg  << "\tTDMA " << get_tdma_slot(ch) << "\tsa " << sa;


		}
		else {
			unsigned long ch1  = bitset_shift_mask(tsbk, 64, 0xffff);
			unsigned long ga1  = bitset_shift_mask(tsbk, 48, 0xffff);
			unsigned long ch2  = bitset_shift_mask(tsbk, 32, 0xffff);
			unsigned long ga2  = bitset_shift_mask(tsbk, 16, 0xffff);
			unsigned long f1 = channel_id_to_frequency(ch1);
			unsigned long f2 = channel_id_to_frequency(ch2);



			if (f1) {
				updated += 1;
			}
			if (f2) {
				updated += 1;
			}
			message.message_type = UPDATE;
			message.freq = f1;
			message.talkgroup = ga1;
			message.tdma = get_tdma_slot(ch1);
			if (f1 != f2) {
				messages.push_back(message);
				message.freq = f2;
				message.talkgroup = ga2;
				message.tdma = get_tdma_slot(ch2);
				BOOST_LOG_TRIVIAL(trace) << "tsbk02\tGrant Update\tChannel ID: " << std::setw(5) << ch2 << "\tFreq: " << f2 / 1000000.0 << "\tga " << std::setw(7) << ga2 << "\tTDMA " << get_tdma_slot(ch2);
			}

			BOOST_LOG_TRIVIAL(trace) << "tsbk02\tGrant Update\tChannel ID: " << std::setw(5) << ch1 << "\tFreq: " << f1 / 1000000.0 << "\tga " << std::setw(7) << ga1 << "\tTDMA " << get_tdma_slot(ch1);
		}
	} else if ( opcode == 0x03 ) {

		unsigned long mfrid  = bitset_shift_mask(tsbk, 80, 0xff);
        if (mfrid == 0x90) {   // MOT_GRG_CN_GRANT_UPDT
        	unsigned long ch1  = bitset_shift_mask(tsbk, 64, 0xffff);
			unsigned long sg1  = bitset_shift_mask(tsbk, 48, 0xffff);
			unsigned long ch2  = bitset_shift_mask(tsbk, 32, 0xffff);
			unsigned long sg2  = bitset_shift_mask(tsbk, 16, 0xffff);

			unsigned long f1 = channel_id_to_frequency(ch1);
			unsigned long f2 = channel_id_to_frequency(ch2);

			message.message_type = UPDATE;
			message.freq = f1;
			message.talkgroup = sg1;
			message.tdma = get_tdma_slot(ch1);
			if (f1 != f2) {
				messages.push_back(message);
				message.freq = f2;
				message.talkgroup = sg2;
				message.tdma = get_tdma_slot(ch2);
	         	BOOST_LOG_TRIVIAL(trace) << "MOT_GRG_CN_GRANT_UPDT(0x03): \tChannel ID: "<< std::setw(5) << ch2 << "\tFreq: " << f2 / 1000000.0 << "\tsg " << std::setw(7) << sg2 << "\tTDMA " << get_tdma_slot(ch2);
			}

            BOOST_LOG_TRIVIAL(trace) << "MOT_GRG_CN_GRANT_UPDT(0x03): \tChannel ID: "<< std::setw(5) << ch1 << "\tFreq: " << f1 / 1000000.0 << "\tsg " << std::setw(7) << sg1 << "\tTDMA " << get_tdma_slot(ch1);

        }
	} else if ( opcode == 0x16 ) {   // sndcp data ch
		unsigned long ch1  = bitset_shift_mask(tsbk, 48, 0xffff);
		unsigned long ch2  = bitset_shift_mask(tsbk, 32, 0xffff);
		BOOST_LOG_TRIVIAL(trace) << "tsbk16 sndcp data ch: chan " << ch1 << " "  << ch2;
	} else if (opcode == 0x34 ) {  // iden_up vhf uhf
		unsigned long iden = bitset_shift_mask(tsbk, 76, 0xf);
		unsigned long bwvu = bitset_shift_mask(tsbk, 72, 0xf);
		unsigned long toff0 = bitset_shift_mask(tsbk, 58, 0x3fff);
		unsigned long spac = bitset_shift_mask(tsbk, 48, 0x3ff);
		unsigned long freq = bitset_shift_mask(tsbk, 16, 0xffffffff);
		unsigned long toff_sign = (toff0 >> 13) & 1;
		double bandwidth = 0 ;
		if (bwvu == 4) {
			bandwidth = 6.25;
		} else if (bwvu==5) {
			bandwidth = 12.5;
		}
		long toff = toff0 & 0x1fff;
		if (toff_sign == 0) {
			toff = 0 - toff;
		}
		std::string txt[] = {"mob Tx-", "mob Tx+"};
		Channel temp_chan = {
			iden, //id;
			toff * spac * 125, //offset;
			spac * 125, //step;
			freq * 5, //frequency;
			0, //tdma;
			bandwidth
		};
		add_channel(iden, temp_chan);
		BOOST_LOG_TRIVIAL(trace) << "tsbk34 iden vhf/uhf id " << std::dec << iden << " toff " << toff * spac * 0.125 * 1e-3 << " spac " << spac * 0.125 << " freq " << freq * 0.000005 << " [ " << txt[toff_sign] << "]";
	} else if (opcode == 0x33 ) {  // iden_up_tdma
		unsigned long mfrid  = bitset_shift_mask(tsbk, 80, 0xff);
		if (mfrid == 0) {
			unsigned long iden = bitset_shift_mask(tsbk, 76, 0xf);
			unsigned long channel_type = bitset_shift_mask(tsbk, 72, 0xf);
			unsigned long toff0 = bitset_shift_mask(tsbk, 58, 0x3fff);
			unsigned long spac = bitset_shift_mask(tsbk, 48, 0x3ff);
			unsigned long toff_sign = (toff0 >> 13) & 1;
			long toff = toff0 & 0x1fff;
			if (toff_sign == 0) {
				toff = 0 - toff;
			}
			unsigned long f1   = bitset_shift_mask(tsbk, 16, 0xffffffff);
			int slots_per_carrier[] = {1,1,1,2,4,2};
			Channel temp_chan = {
				iden, //id;
				toff * spac * 125, //offset;
				spac * 125, //step;
				f1 * 5, //frequency;
				slots_per_carrier[channel_type], //tdma;
				6.25
			};
			add_channel(iden,temp_chan);
			BOOST_LOG_TRIVIAL(trace) << "tsbk33 iden up tdma id " << std::dec << iden << " f " <<  temp_chan.frequency << " offset " << temp_chan.offset << " spacing " << temp_chan.step << " slots/carrier " << temp_chan.tdma;
		}
	} else if (opcode == 0x3d)  {  // iden_up
		unsigned long iden = bitset_shift_mask(tsbk, 76, 0xf);
		unsigned long bw   = bitset_shift_mask(tsbk, 67, 0x1ff);
		unsigned long toff0 = bitset_shift_mask(tsbk, 58, 0x1ff);
		unsigned long spac = bitset_shift_mask(tsbk, 48, 0x3ff);
		unsigned long freq = bitset_shift_mask(tsbk, 16, 0xffffffff);
		unsigned long toff_sign = (toff0 >> 8) & 1;
		unsigned long toff = toff0 & 0xff;
		if (toff_sign == 0) {
			toff = 0 - toff;
		}
		std::string txt[] = {"mob xmit < recv", "mob xmit > recv"};
		Channel temp_chan = {
			iden, //id;
			toff * 250000, //offset;
			spac * 125, //step;
			freq * 5, //frequency;
			0, //tdma;
			bw * .125
		};
		add_channel(iden,temp_chan);
		BOOST_LOG_TRIVIAL(trace) << "tsbk3d iden id "<< std::dec << iden <<" toff "<< toff * 0.25 << " spac " << spac * 0.125 << " freq " << freq * 0.000005;
	} else if (opcode == 0x3a)  {  // rfss status
		unsigned long syid = bitset_shift_mask(tsbk, 56, 0xfff);
		unsigned long rfid = bitset_shift_mask(tsbk, 48, 0xff);
		unsigned long stid = bitset_shift_mask(tsbk, 40, 0xff);
		unsigned long chan = bitset_shift_mask(tsbk, 24, 0xffff);
		unsigned long f1 = channel_id_to_frequency(chan);

		BOOST_LOG_TRIVIAL(trace) << "tsbk3a rfss status: syid: " << syid << " rfid " << rfid << " stid " << stid << " ch1 " << chan << "(" << channel_id_to_string(chan) <<  ")"<< std::endl;
	} else if (opcode == 0x39) {  // secondary cc
		unsigned long rfid = bitset_shift_mask(tsbk, 72, 0xff);
		unsigned long stid = bitset_shift_mask(tsbk, 64, 0xff);
		unsigned long ch1  = bitset_shift_mask(tsbk, 48, 0xffff);
		unsigned long ch2  = bitset_shift_mask(tsbk, 24, 0xffff);
		unsigned long f1 = channel_id_to_frequency(ch1);
		unsigned long f2 = channel_id_to_frequency(ch2);
		if (f1 && f2) {
			message.message_type = CONTROL_CHANNEL;
			message.freq = f1;
			message.talkgroup = 0;
			message.tdma = 0;
			messages.push_back(message);
			message.freq = f2;

		}

		BOOST_LOG_TRIVIAL(trace) << "tsbk39 secondary cc: rfid " << std::dec << rfid << " stid " << stid << " ch1 " << ch1 << "(" << channel_id_to_string(ch1) << ") ch2 " << ch2 << "(" << channel_id_to_string(ch2) << ") ";
	} else if (opcode == 0x3b) {  // network status
		unsigned long wacn = bitset_shift_mask(tsbk, 52, 0xfffff);
		unsigned long syid = bitset_shift_mask(tsbk, 40, 0xfff);
		unsigned long ch1  = bitset_shift_mask(tsbk, 24, 0xffff);
		unsigned long f1 = channel_id_to_frequency(ch1);
		if (f1) {
			/*
			self.ns_syid = syid
			self.ns_wacn = wacn
			self.ns_chan = f1*/
		}
		BOOST_LOG_TRIVIAL(trace) << "tsbk3b net stat: wacn " << std::dec << wacn << " syid " << syid << " ch1 " << ch1 << "(" << channel_id_to_string(ch1) << ") ";
	} else if (opcode == 0x3c) {  // adjacent status
		unsigned long rfid = bitset_shift_mask(tsbk, 48, 0xff);
		unsigned long stid = bitset_shift_mask(tsbk, 40, 0xff);
		unsigned long ch1  = bitset_shift_mask(tsbk, 24, 0xffff);
		unsigned long f1 = channel_id_to_frequency(ch1);
		BOOST_LOG_TRIVIAL(trace) << "tsbk3c\tAdjacent Status\t rfid " << std::dec << rfid << " stid " << stid << " ch1 " << ch1 << "(" << channel_id_to_string(ch1) << ") ";

		if (f1 ) {
			it = channels.find((ch1 >> 12) & 0xf);

			if (it != channels.end()) {
				Channel temp_chan = it->second;
				//			self.adjacent[f1] = 'rfid: %d stid:%d uplink:%f tbl:%d' % (rfid, stid, (f1 + self.freq_table[table]['offset']) / 1000000.0, table)
				BOOST_LOG_TRIVIAL(trace) << "\ttsbk3c Chan " << temp_chan.frequency << "  " << temp_chan.step;
			}
		}
	} else if (opcode == 0x20) { //Acknowledge response
		unsigned long mfrid  = bitset_shift_mask(tsbk,80,0xff);
		unsigned long ga   = bitset_shift_mask(tsbk,40,0xffff);
		unsigned long op   = bitset_shift_mask(tsbk,48,0xff);
		unsigned long sa   = bitset_shift_mask(tsbk,16,0xffffff);

		message.talkgroup = ga;
		message.source = sa;

		BOOST_LOG_TRIVIAL(trace) << "tsbk20\tAcknowledge Response\tga " << std::setw(7) << ga  <<"\tsa " << sa << "\tReserved: " << op;

	} else if (opcode == 0x2c) { // Unit Registration Response
		unsigned long mfrid  = bitset_shift_mask(tsbk,80,0xff);
		unsigned long opts  = bitset_shift_mask(tsbk,72,0xff);
		unsigned long sa   = bitset_shift_mask(tsbk,16,0xffffff);
		unsigned long si   = bitset_shift_mask(tsbk,40,0xffffff);


		//message.message_type = ASSIGNMENT;
		//message.message_command = GRANT;
		message.message_type = REGISTRATION;
		message.source = si;



		BOOST_LOG_TRIVIAL(trace) << "tsbk2c\tUnit Registration Response\tsa " << std::setw(7) << sa << " Source ID: " << si;
	} else if (opcode == 0x2f) { // Unit DeRegistration Ack
		unsigned long mfrid  = bitset_shift_mask(tsbk,80,0xff);
		unsigned long opts  = bitset_shift_mask(tsbk,72,0xff);
		unsigned long si   = bitset_shift_mask(tsbk,16,0xffffff);




		message.message_type = DEREGISTRATION;
		message.source = si;



		BOOST_LOG_TRIVIAL(trace) << "tsbk2f\tUnit Deregistration ACK\tSource ID: " << std::setw(7) << si <<std::endl;
	} else if (opcode == 0x28) { // Unit Group Affiliation Response
		unsigned long mfrid  = bitset_shift_mask(tsbk,80,0xff);
		unsigned long opts  = bitset_shift_mask(tsbk,72,0xff);
		unsigned long ta   = bitset_shift_mask(tsbk,16,0xffffff);
		unsigned long ga   = bitset_shift_mask(tsbk,40,0xffff);
		unsigned long aga   = bitset_shift_mask(tsbk,56,0xffff);

		message.message_type = AFFILIATION;
		message.source = ta;
		message.talkgroup = ga;



		BOOST_LOG_TRIVIAL(trace) << "tsbk2f\tUnit Group Affiliation\tSource ID: " << std::setw(7) << ta << "\tGroup Address: " << ga << "\tAnouncement Goup: " << aga;
	} else {
		//std::cout << "tsbk other " << std::hex << opcode << std::endl;
	}
	messages.push_back(message);
	return messages;
}
void P25Parser::print_bitset(boost::dynamic_bitset<> &tsbk) {
	boost::dynamic_bitset<> bitmask(tsbk.size(), 0x3f);
	unsigned long result = (tsbk & bitmask).to_ulong();
	BOOST_LOG_TRIVIAL(trace) << tsbk << " = " << std::hex << result;
}
void printbincharpad(char c)
{
	for (int i = 7; i >= 0; --i)
	{
		std::cout << ( (c & (1 << i)) ? '1' : '0' );
	}
	//std::cout << " | ";
}


std::vector<TrunkMessage> P25Parser::parse_message(gr::message::sptr msg) {
	TrunkMessage message;
	std::vector<TrunkMessage> messages;

	message.message_type = UNKNOWN;

	long type = msg->type();
	int updated = 0;
	//curr_time = time.time()

	if (type == -2 ) {	 // # request from gui
		std::string cmd = msg->to_string();

		BOOST_LOG_TRIVIAL(trace) << "process_qmsg: command: " << cmd;
		//self.update_state(cmd, curr_time)
		messages.push_back(message);
		return messages;
	} else if ( type == -1 ) {  //	# timeout
		BOOST_LOG_TRIVIAL(trace) << "process_data_unit timeout";
		//self.update_state('timeout', curr_time)
		messages.push_back(message);
		return messages;
	} else if ( type < 0 ) {
		BOOST_LOG_TRIVIAL(trace) << "unknown message type " << type;
		messages.push_back(message);
		return messages;
	}
	std::string s = msg->to_string();
	//# nac is always 1st two bytes
	long nac = ((int) s[0] << 8) + (int) s[1];
	if (nac == 0xffff) {
		//# TDMA
		//self.update_state('tdma_duid%d' % type, curr_time)
		messages.push_back(message);
		return messages;
	}
	s = s.substr(2);
	//std::cout << std::dec << "nac " << nac << " type " << type <<  " size " << msg->to_string().length() << " mesg len: " << msg->length() << std::endl; //" at %f state %d len %d" %(nac, type, time.time(), self.state, len(s))
	if ((type == 7) || (type == 12)) // and nac not in self.trunked_systems:
	{
		/*
		if not self.configs:
		    # TODO: allow whitelist/blacklist rather than blind automatic-add
		    self.add_trunked_system(nac)
		else:
		    return
		*/
	}
	if (type == 7) { 	 //# trunk: TSBK

		boost::dynamic_bitset<> b((s.length()+2)*8);
		for (int i = 0; i < s.length(); ++i) {
			unsigned char c = (unsigned char) s[i];
			b <<= 8;
			for (int j = 0; j < 8 ; j++) {
				if (c & 0x1) {
					b[j] = 1;
				} else {
					b[j] = 0;
				}
				c >>= 1;
			}
		}
		b <<= 16;	// for missing crc

		return decode_tsbk(b);
	} else if (type == 12) {	//# trunk: MBT
		std::string s1 = s.substr(0,10);
		std::string s2 = s.substr(10);
		long header=0;
		long mbt_data = 0;
		for (int i = 0; i < s1.length(); ++i)
		{
			header = (header << 8) + (int) s1[i];
		}
		for (int i = 0; i < s2.length(); ++i)
		{
			mbt_data = (mbt_data << 8) + (int) s2[i];
		}
		long opcode = (header >> 16) & 0x3f;

		BOOST_LOG_TRIVIAL(trace) << "type " << type << " len " << s1.length() << "/"<< s2.length() << " opcode " << opcode << "[" << header << "/" << mbt_data << "]";
		//self.trunked_systems[nac].decode_mbt_data(opcode, header << 16, mbt_data << 32)
	}
	/*
	if (nac != self.current_nac) {
	    return
	}

	if updated:
	    self.update_state('update', curr_time)
	else:
	    self.update_state('duid%d' % type, curr_time)
	*/
	messages.push_back(message);
	return messages;
}
