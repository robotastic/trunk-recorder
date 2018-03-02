#include "p25_parser.h"
#include "../formatter.h"

P25Parser::P25Parser() {}

void P25Parser::add_channel(int chan_id, Channel temp_chan) {
   /*std::cout << "Add  - Channel id " << std::dec << chan_id << " freq " <<
    temp_chan.frequency << " offset " << temp_chan.offset << " step " <<
   temp_chan.step << " slots/carrier " << temp_chan.slots_per_carrier  << std::endl;
*/
  channels[chan_id] = temp_chan;
}

long P25Parser::get_tdma_slot(int chan_id) {
  long channel = chan_id & 0xfff;

  it = channels.find((chan_id >> 12) & 0xf);

  if (it != channels.end()) {
    Channel temp_chan = it->second;

    if (temp_chan.phase2_tdma) {
      return channel & 1;
    }
  }

  return -1;
}

double P25Parser::get_bandwidth(int chan_id) {
  it = channels.find((chan_id >> 12) & 0xf);

  if (it != channels.end()) {
    Channel temp_chan = it->second;
    return temp_chan.bandwidth;
  }

  return 0;
}

std::string P25Parser::channel_id_to_string(int chan_id) {
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
  //long id      = (chan_id >> 12) & 0xf;
  long channel = chan_id & 0xfff;

  it = channels.find((chan_id >> 12) & 0xf);

  if (it != channels.end()) {
    Channel temp_chan = it->second;

    if (temp_chan.phase2_tdma) {
      return temp_chan.frequency + temp_chan.step * int(channel / temp_chan.slots_per_carrier);
    }
    else {
      return temp_chan.frequency + temp_chan.step * channel;
    }
  }
  return 0;
}

unsigned long P25Parser::bitset_shift_mask(boost::dynamic_bitset<>& tsbk, int shift, unsigned long long mask) {
  boost::dynamic_bitset<> bitmask(tsbk.size(), mask);
  unsigned long result = ((tsbk >> shift) & bitmask).to_ulong();

  // std::cout << "    " << std::dec<< shift << " " << tsbk.size() << " [ " <<
  // mask << " ]  = " << result << " - " << ((tsbk >> shift) & bitmask) <<
  // std::endl;
  return result;
}


std::vector<TrunkMessage>P25Parser::decode_mbt_data( unsigned long opcode, boost::dynamic_bitset<>& header, boost::dynamic_bitset<>& mbt_data, unsigned long sa,  unsigned long nac, int sys_num) {
  std::vector<TrunkMessage> messages;
  TrunkMessage message;
  std::ostringstream os;


  message.message_type = UNKNOWN;
  message.source       = 0;
  message.wacn         =0;
  message.nac = nac;
  message.sys_id = 0;
  message.sys_num = sys_num;
  message.talkgroup = 0;
  message.emergency = false;
  message.phase2_tdma = false;
  message.tdma_slot = 0;
  message.freq = 0;


      BOOST_LOG_TRIVIAL(trace) <<  "decode_mbt_data: $" <<  opcode;
      if (opcode == 0x0) {  // grp voice channel grant
        //unsigned long mfrid = bitset_shift_mask(header, 72, 0xff);
        unsigned long sa = bitset_shift_mask(header, 48, 0xffffff);
        bool emergency = (bool)bitset_shift_mask(header, 24, 0x80);
        bool encrypted = (bool)bitset_shift_mask(header, 24, 0x40);
        unsigned long ch1 = bitset_shift_mask(mbt_data, 64, 0xffff);
        //unsigned long ch2 = bitset_shift_mask(mbt_data, 48, 0xffff);
        unsigned long ga = bitset_shift_mask(mbt_data, 32, 0xffff);
        unsigned long f1 = channel_id_to_frequency(ch1);

        message.message_type = GRANT;
        message.freq         = f1;
        message.talkgroup    = ga;
        message.source       = sa;
        message.emergency    = emergency;
        message.encrypted    = encrypted;

          if (get_tdma_slot(ch1) >= 0) {
            message.phase2_tdma = true;
            message.tdma_slot = get_tdma_slot(ch1);
          } else {
            message.phase2_tdma = false;
            message.tdma_slot = 0;
          }

          os << "mbt00\tChan Grant\tChannel ID: " << std::setw(5) << ch1 << "\tFreq: " << f1 / 1000000.0 << "\tga " << std::setw(7) << ga  << "\tTDMA " << get_tdma_slot(ch1) <<"\tsa " << sa << "\tEncrypt " << encrypted << "\tBandwidth: " << get_bandwidth(ch1);
          message.meta = os.str();
          BOOST_LOG_TRIVIAL(trace) << os.str();
      } else if (opcode == 0x3a) {  // rfss status
          unsigned long syid = bitset_shift_mask(header, 48, 0xfff);
          unsigned long rfid = bitset_shift_mask(mbt_data, 88, 0xff);
          unsigned long stid = bitset_shift_mask(mbt_data, 80, 0xff);
          unsigned long ch1 = bitset_shift_mask(mbt_data, 64, 0xffff);
          //unsigned long ch2 = bitset_shift_mask(mbt_data, 48, 0xffff);
          //unsigned long f1   = channel_id_to_frequency(ch1);
          //unsigned long f2   = channel_id_to_frequency(ch2);
          message.message_type = SYSID;
          message.sys_id        = syid;
          os << "mbt3a rfss status: syid: " << syid << " rfid " << rfid << " stid " << stid << " ch1 " << ch1 << "(" << channel_id_to_string(ch1) <<  ")" << std::endl;
          message.meta = os.str();
          //BOOST_LOG_TRIVIAL(trace) << os;
      } else if (opcode == 0x3b) {   // network status
          unsigned long wacn = bitset_shift_mask(mbt_data, 76, 0xfffff);
          unsigned long syid = bitset_shift_mask(header, 48, 0xfff);
          unsigned long ch1  = bitset_shift_mask(mbt_data, 56, 0xffff);
          unsigned long ch2  = bitset_shift_mask(mbt_data, 40, 0xffff);
          unsigned long f1   = channel_id_to_frequency(ch1);
          unsigned long f2   = channel_id_to_frequency(ch2);

          if (f1 && f2) {
            message.message_type = STATUS;
            message.wacn         = wacn;
            message.sys_id    = syid;
            message.freq         = f1;
          }
          BOOST_LOG_TRIVIAL(trace) << "mbt3b net stat: wacn " << std::dec << wacn << " syid " << syid << " ch1 " << ch1 << "(" << channel_id_to_string(ch1) << ") ";
      } else if (opcode == 0x3c) {  // adjacent status
        unsigned long syid = bitset_shift_mask(header, 48, 0xfff);
        unsigned long rfid = bitset_shift_mask(header, 24, 0xff);
        unsigned long stid = bitset_shift_mask(header, 16, 0xff);
        unsigned long ch1  = bitset_shift_mask(mbt_data, 80, 0xffff);
        unsigned long ch2  = bitset_shift_mask(mbt_data, 64, 0xffff);
        BOOST_LOG_TRIVIAL(trace) << "mbt3c adjacent status " << "syid " <<syid<<" rfid "<<rfid<<" stid "<<stid<<" ch1 "<<ch1<<" ch2 "<<ch2;
      } else if (opcode == 0x04) { //  Unit to Unit Voice Service Channel Grant -Extended (UU_V_CH_GRANT)
          //unsigned long mfrid = bitset_shift_mask(header, 80, 0xff);
          bool emergency = (bool)bitset_shift_mask(header, 24, 0x80);
          bool encrypted = (bool)bitset_shift_mask(header, 24, 0x40);
          unsigned long ch = bitset_shift_mask(header, 16, 0xffff); /// ????
          unsigned long f  = channel_id_to_frequency(ch);
          unsigned long sa = bitset_shift_mask(header, 48, 0xffffff);
          unsigned long ta = bitset_shift_mask(mbt_data, 24, 0xffffff);

          message.message_type = GRANT;
          message.freq         = f;
          message.talkgroup    = ta;
          message.source       = sa;
          message.emergency    = emergency;
          message.encrypted    = encrypted;
              if (get_tdma_slot(ch) >= 0) {
                message.phase2_tdma = true;
                message.tdma_slot = get_tdma_slot(ch);
              } else {
                message.phase2_tdma = false;
                message.tdma_slot = 0;
              }

          BOOST_LOG_TRIVIAL(trace) << "mbt04\tUnit to Unit Chan Grant\tChannel ID: " << std::setw(5) << ch << "\tFreq: " << f / 1000000.0 << "\tTarget ID: " << std::setw(7) << ta  << "\tTDMA " << get_tdma_slot(ch) <<
          "\tSource ID: " << sa;
    }else {
       BOOST_LOG_TRIVIAL(trace) << "mbt other: " << opcode;
       return messages;
    }
  messages.push_back(message);
  return messages;
}

std::vector<TrunkMessage>P25Parser::decode_tsbk(boost::dynamic_bitset<>& tsbk, unsigned long nac, int sys_num) {
// self.stats['tsbks'] += 1
long updated = 0;
std::vector<TrunkMessage> messages;
TrunkMessage message;
std::ostringstream os;


message.message_type = UNKNOWN;
message.source       = 0;
message.wacn         =0;
message.nac = nac;
message.sys_id = 0;
message.sys_num = sys_num;
message.talkgroup = 0;
message.emergency = false;
message.phase2_tdma = false;
message.tdma_slot = 0;
message.freq = 0;


unsigned long opcode = bitset_shift_mask(tsbk, 88, 0x3f); // x3f


BOOST_LOG_TRIVIAL(trace) << "TSBK: opcode: $" << std::hex << opcode ;
if (opcode == 0x00) { // group voice chan grant
  unsigned long mfrid = bitset_shift_mask(tsbk, 80, 0xff);
  // unsigned long opts  = bitset_shift_mask(tsbk,72,0xff);
  bool emergency = (bool)bitset_shift_mask(tsbk, 72, 0x80);
  bool encrypted = (bool)bitset_shift_mask(tsbk, 72, 0x40);
  // bool duplex = (bool) bitset_shift_mask(tsbk,72,0x20);
  // bool mode = (bool) bitset_shift_mask(tsbk,72,0x10);
  // unsigned long priority = bitset_shift_mask(tsbk,72,0x07);
  unsigned long ch = bitset_shift_mask(tsbk, 56, 0xffff);
  unsigned long ga = bitset_shift_mask(tsbk, 40, 0xffff);
  unsigned long sa = bitset_shift_mask(tsbk, 16, 0xffffff);

  if (mfrid == 0x90) {
    unsigned long sg  = bitset_shift_mask(tsbk, 64, 0xffff);
    unsigned long ga1 = bitset_shift_mask(tsbk, 48, 0xffff);
    unsigned long ga2 = bitset_shift_mask(tsbk, 32, 0xffff);
    unsigned long ga3 = bitset_shift_mask(tsbk, 16, 0xffff);
    BOOST_LOG_TRIVIAL(trace) << "tsbk00\tMoto Patch Add \tsg: " << sg << "\tga1: " << ga1 << "\tga2: " << ga2 << "\tga3: " << ga3;
  } else {
    unsigned long f1 = channel_id_to_frequency(ch);
    message.message_type = GRANT;
    message.freq         = f1;
    message.talkgroup    = ga;
    message.source       = sa;
    message.emergency    = emergency;
    message.encrypted    = encrypted;

    if (get_tdma_slot(ch) >= 0) {
      message.phase2_tdma = true;
      message.tdma_slot = get_tdma_slot(ch);
    } else {
      message.phase2_tdma = false;
      message.tdma_slot = 0;
    }
    os << "tsbk00\tChan Grant\tChannel ID: " << std::setw(5) << ch << "\tFreq: " << f1 / 1000000.0 << "\tga " << std::setw(7) << ga  << "\tTDMA " << get_tdma_slot(ch) <<
    "\tsa " << sa << "\tEncrypt " << encrypted << "\tBandwidth: " << get_bandwidth(ch);
    message.meta = os.str();
    //BOOST_LOG_TRIVIAL(trace) << os;
  }
} else if (opcode == 0x02) { // group voice chan grant update
  unsigned long mfrid = bitset_shift_mask(tsbk, 80, 0xff);
  bool emergency = (bool)bitset_shift_mask(tsbk, 72, 0x80);
  bool encrypted = (bool)bitset_shift_mask(tsbk, 72, 0x40);

  if (mfrid == 0x90) {
    unsigned long ch = bitset_shift_mask(tsbk, 56, 0xffff);
    unsigned long f  = channel_id_to_frequency(ch);
    unsigned long sg = bitset_shift_mask(tsbk, 40, 0xffff);
    unsigned long sa = bitset_shift_mask(tsbk, 16, 0xffffff);

    message.message_type = GRANT;
    message.freq         = f;
    message.talkgroup    = sg;
    message.source       = sa;
    message.emergency    = emergency;
    message.encrypted    = encrypted;

    if (get_tdma_slot(ch) >= 0) {
      message.phase2_tdma = true;
      message.tdma_slot = get_tdma_slot(ch);
    } else {
      message.phase2_tdma = false;
      message.tdma_slot = 0;
    }

    os << "tbsk02\tMoto Patch Grant\tChannel ID: " << std::setw(5) << ch << "\tFreq: " <<  FormatFreq(f) << "\tsg " << std::setw(7) << sg  << "\tTDMA " << get_tdma_slot(ch) <<  "\tsa " << sa;
   message.meta = os.str();
  }
  else {
    unsigned long ch1 = bitset_shift_mask(tsbk, 64, 0xffff);
    unsigned long ga1 = bitset_shift_mask(tsbk, 48, 0xffff);
    unsigned long ch2 = bitset_shift_mask(tsbk, 32, 0xffff);
    unsigned long ga2 = bitset_shift_mask(tsbk, 16, 0xffff);
    unsigned long f1  = channel_id_to_frequency(ch1);
    unsigned long f2  = channel_id_to_frequency(ch2);


    if (f1) {
      updated += 1;

    }

    if (f2) {
      updated += 1;
    }
    message.message_type = UPDATE;
    message.freq         = f1;
    message.talkgroup    = ga1;
    message.emergency    = emergency;
    message.encrypted    = encrypted;

    if (get_tdma_slot(ch1) >= 0) {
      message.phase2_tdma = true;
      message.tdma_slot = get_tdma_slot(ch1);
    } else {
      message.phase2_tdma = false;
      message.tdma_slot = 0;
    }

    if (f1 != f2) {
      messages.push_back(message);
      message.freq      = f2;
      message.talkgroup = ga2;

      if (get_tdma_slot(ch2) >= 0) {
        message.phase2_tdma = true;
        message.tdma_slot = get_tdma_slot(ch2);
      } else {
        message.phase2_tdma = false;
        message.tdma_slot = 0;
      }

      os << "tsbk02\tGrant Update\tChannel ID: " << std::setw(5) << ch2 << "\tFreq: " << FormatFreq(f2) << "\tga " << std::setw(7) << ga2 << "\tTDMA " << get_tdma_slot(ch2);

      message.meta = os.str();
      //BOOST_LOG_TRIVIAL(trace) << os;
    }
    os << "tsbk02\tGrant Update\tChannel ID: " << std::setw(5) << ch1 << "\tFreq: " << f1 / 1000000.0 << "\tga " << std::setw(7) << ga1 << "\tTDMA " << get_tdma_slot(ch1);
    message.meta = os.str();
    //BOOST_LOG_TRIVIAL(trace) << os;
  }
} else if (opcode == 0x03) { //  Group Voice Channel Update-Explicit (GRP_V_CH_GRANT_UPDT_EXP)
  unsigned long mfrid = bitset_shift_mask(tsbk, 80, 0xff);
  bool emergency = (bool)bitset_shift_mask(tsbk, 72, 0x80);
  bool encrypted = (bool)bitset_shift_mask(tsbk, 72, 0x40);

  if (mfrid == 0x90) { // MOT_GRG_CN_GRANT_UPDT
    unsigned long ch1 = bitset_shift_mask(tsbk, 64, 0xffff);
    unsigned long sg1 = bitset_shift_mask(tsbk, 48, 0xffff);
    unsigned long ch2 = bitset_shift_mask(tsbk, 32, 0xffff);
    unsigned long sg2 = bitset_shift_mask(tsbk, 16, 0xffff);

    unsigned long f1 = channel_id_to_frequency(ch1);
    unsigned long f2 = channel_id_to_frequency(ch2);

    message.message_type = UPDATE;
    message.freq         = f1;
    message.talkgroup    = sg1;
    message.emergency    = emergency;
    message.encrypted    = encrypted;
    if (get_tdma_slot(ch1) >= 0) {
      message.phase2_tdma = true;
      message.tdma_slot = get_tdma_slot(ch1);
    } else {
      message.phase2_tdma = false;
      message.tdma_slot = 0;
    }

    if (f1 != f2) {
      messages.push_back(message);
      message.freq      = f2;
      message.talkgroup = sg2;
      message.emergency    = emergency;
      message.encrypted    = encrypted;
      if (get_tdma_slot(ch2) >= 0) {
        message.phase2_tdma = true;
        message.tdma_slot = get_tdma_slot(ch2);
      } else {
        message.phase2_tdma = false;
        message.tdma_slot = 0;
      }
      os << "MOT_GRG_CN_GRANT_UPDT(0x03): \tChannel ID: " << std::setw(5) << ch2 << "\tFreq: " << FormatFreq(f2) << "\tsg " << std::setw(7) << sg2 << "\tTDMA " << get_tdma_slot(
        ch2);
      message.meta = os.str();
      //BOOST_LOG_TRIVIAL(trace) << os;
    }
    os << "MOT_GRG_CN_GRANT_UPDT(0x03): \tChannel ID: " << std::setw(5) << ch1 << "\tFreq: " << FormatFreq(f1) << "\tsg " << std::setw(7) << sg1 << "\tTDMA " <<
    get_tdma_slot(ch1);
    message.meta = os.str();
    //BOOST_LOG_TRIVIAL(trace) << os;
  } else {
    bool emergency = (bool)bitset_shift_mask(tsbk, 72, 0x80);
    bool encrypted = (bool)bitset_shift_mask(tsbk, 72, 0x40);
    unsigned long ch1 = bitset_shift_mask(tsbk, 48, 0xffff);
    unsigned long ga1 = bitset_shift_mask(tsbk, 16, 0xffff);
    unsigned long f1  = channel_id_to_frequency(ch1);

    message.message_type = UPDATE;
    message.freq         = f1;
    message.talkgroup    = ga1;
    message.emergency    = emergency;
    message.encrypted    = encrypted;
    if (get_tdma_slot(ch1) >= 0) {
      message.phase2_tdma = true;
      message.tdma_slot = get_tdma_slot(ch1);
    } else {
      message.phase2_tdma = false;
      message.tdma_slot = 0;
    }

      os << "tsbk03\tExplicit Grant Update\tChannel ID: " << std::setw(5) << ch1 << "\tFreq: " << f1 / 1000000.0 << "\tga " << std::setw(7) << ga1 << "\tTDMA " <<
    get_tdma_slot(ch1);
    message.meta = os.str();
    //BOOST_LOG_TRIVIAL(trace) << os;
  }
} else if (opcode == 0x04) {  //  Unit to Unit Voice Service Channel Grant (UU_V_CH_GRANT)
    //unsigned long mfrid = bitset_shift_mask(tsbk, 80, 0xff);
      // unsigned long opts  = bitset_shift_mask(tsbk,72,0xff);
      bool emergency = (bool)bitset_shift_mask(tsbk, 72, 0x80);
      bool encrypted = (bool)bitset_shift_mask(tsbk, 72, 0x40);
      // bool duplex = (bool) bitset_shift_mask(tsbk,72,0x20);
      // bool mode = (bool) bitset_shift_mask(tsbk,72,0x10);
      // unsigned long priority = bitset_shift_mask(tsbk,72,0x07);
      unsigned long ch = bitset_shift_mask(tsbk, 64, 0xffff);
      unsigned long f  = channel_id_to_frequency(ch);
      unsigned long sa = bitset_shift_mask(tsbk, 16, 0xffffff);
      unsigned long ta = bitset_shift_mask(tsbk, 40, 0xffffff);

      message.message_type = GRANT;
      message.freq         = f;
      message.talkgroup    = ta;
      message.source       = sa;
      message.emergency    = emergency;
      message.encrypted    = encrypted;
      if (get_tdma_slot(ch) >= 0) {
        message.phase2_tdma = true;
        message.tdma_slot = get_tdma_slot(ch);
      } else {
        message.phase2_tdma = false;
        message.tdma_slot = 0;
      }


      BOOST_LOG_TRIVIAL(trace) << "tsbk04\tUnit to Unit Chan Grant\tChannel ID: " << std::setw(5) << ch << "\tFreq: " << FormatFreq(f) << "\tTarget ID: " << std::setw(7) << ta  << "\tTDMA " << get_tdma_slot(ch) <<
      "\tSource ID: " << sa;
    } else if (opcode == 0x05) { // Unit To Unit Answer Request
      BOOST_LOG_TRIVIAL(trace) << "tsbk05";
    } else if (opcode == 0x06) { //  Unit to Unit Voice Channel Grant Update (UU_V_CH_GRANT_UPDT)
      //unsigned long mfrid = bitset_shift_mask(tsbk, 80, 0xff);
      // unsigned long opts  = bitset_shift_mask(tsbk,72,0xff);

      bool emergency = (bool)bitset_shift_mask(tsbk, 72, 0x80);
      bool encrypted = (bool)bitset_shift_mask(tsbk, 72, 0x40);
      // bool duplex = (bool) bitset_shift_mask(tsbk,72,0x20);
      // bool mode = (bool) bitset_shift_mask(tsbk,72,0x10);
      // unsigned long priority = bitset_shift_mask(tsbk,72,0x07);
      unsigned long ch = bitset_shift_mask(tsbk, 64, 0xffff);
      unsigned long f  = channel_id_to_frequency(ch);
      unsigned long sa = bitset_shift_mask(tsbk, 16, 0xffffff);
      unsigned long ta = bitset_shift_mask(tsbk, 40, 0xffffff);

      message.message_type = UPDATE;
      message.freq         = f;
      message.talkgroup    = ta;
      message.source       = sa;
      if (get_tdma_slot(ch) >= 0) {
        message.phase2_tdma = true;
        message.tdma_slot = get_tdma_slot(ch);
        } else {
          message.phase2_tdma = false;
          message.tdma_slot = 0;
          }
        message.emergency    = emergency;
        message.encrypted    = encrypted;
        BOOST_LOG_TRIVIAL(trace) << "tsbk06\tUnit to Unit Chan Update\tChannel ID: " << std::setw(5) << ch << "\tFreq: " << f / 1000000.0 << "\tTarget ID: " << std::setw(7) << ta  << "\tTDMA " << get_tdma_slot(ch) <<
        "\tSource ID: " << sa;
    } else if (opcode == 0x08) { // Telephone Interconnect Voice Channel Grant
      BOOST_LOG_TRIVIAL(trace) << "tsbk08";
    } else if (opcode == 0x09) { // Telephone Interconnect Voice Channel Grant Update
      BOOST_LOG_TRIVIAL(trace) << "tsbk09";
    } else if (opcode == 0x0a) { // Telephone Interconnect Answer Request
      BOOST_LOG_TRIVIAL(trace) << "tsbk0a";
    } else if (opcode == 0x14) { // SNDCP Data Channel Grant
      BOOST_LOG_TRIVIAL(trace) << "tsbk14";
    } else if (opcode == 0x15) { // SNDCP Data Page Request
      BOOST_LOG_TRIVIAL(trace) << "tsbk15";
    } else if (opcode == 0x16) { // SNDCP Data Channel Announcement -Explicit
      BOOST_LOG_TRIVIAL(trace) << "tsbk16";
    } else if (opcode == 0x18) { // Status Update
      BOOST_LOG_TRIVIAL(trace) << "tsbk18";
    } else if (opcode == 0x1a) { // Status Query
      BOOST_LOG_TRIVIAL(trace) << "tsbk1a";
    } else if (opcode == 0x1c) { // Messag Update
      BOOST_LOG_TRIVIAL(trace) << "tsbk1c";
    } else if (opcode == 0x1d) { // Radio Unit Monitor Command
      BOOST_LOG_TRIVIAL(trace) << "tsbk1d";
    } else if (opcode == 0x1f) { // Call Alert
      BOOST_LOG_TRIVIAL(trace) << "tsbk1f";
    } else if (opcode == 0x20) { // Acknowledge response
      // unsigned long mfrid  = bitset_shift_mask(tsbk,80,0xff);
      unsigned long ga = bitset_shift_mask(tsbk, 40, 0xffff);
      unsigned long op = bitset_shift_mask(tsbk, 48, 0xff);
      unsigned long sa = bitset_shift_mask(tsbk, 16, 0xffffff);

      message.talkgroup = ga;
      message.source    = sa;

      BOOST_LOG_TRIVIAL(trace) << "tsbk20\tAcknowledge Response\tga " << std::dec << std::setw(7) << ga  << "\tsa " << sa << "\tReserved: " << op;
    } else if (opcode == 0x21) { // Extended Function Command
      BOOST_LOG_TRIVIAL(trace) << "tsbk21";
    } else if (opcode == 0x24) { // Extended Function Command
      BOOST_LOG_TRIVIAL(trace) << "tsbk24" <<"Extended Function Command";
    } else if (opcode == 0x27) { // Deny Response
      BOOST_LOG_TRIVIAL(trace) << "tsbk27 deny";
    } else if (opcode == 0x28) { // Unit Group Affiliation Response
      // unsigned long mfrid  = bitset_shift_mask(tsbk,80,0xff);
      // unsigned long opts  = bitset_shift_mask(tsbk,72,0xff);
      unsigned long ta  = bitset_shift_mask(tsbk, 16, 0xffffff);
      unsigned long ga  = bitset_shift_mask(tsbk, 40, 0xffff);
      unsigned long aga = bitset_shift_mask(tsbk, 56, 0xffff);

      message.message_type = AFFILIATION;
      message.source       = ta;
      message.talkgroup    = ga;

      BOOST_LOG_TRIVIAL(trace) << "tsbk2f\tUnit Group Affiliation\tSource ID: " << std::setw(7) << ta << "\tGroup Address: " << std::dec << ga << "\tAnouncement Goup: " << aga;
    } else if (opcode == 0x29) { // Secondary Control Channel Broadcast - Explicit
      unsigned long rfid = bitset_shift_mask(tsbk, 72, 0xff);
      unsigned long stid = bitset_shift_mask(tsbk, 64, 0xff);
      unsigned long ch1  = bitset_shift_mask(tsbk, 48, 0xffff);
      unsigned long ch2  = bitset_shift_mask(tsbk, 24, 0xffff);
      unsigned long f1   = channel_id_to_frequency(ch1);
      unsigned long f2   = channel_id_to_frequency(ch2);

      if (f1 && f2) {
        message.message_type = CONTROL_CHANNEL;
        message.freq         = f1;
        message.talkgroup    = 0;
        message.phase2_tdma = false;
        message.tdma_slot    = 0;
        messages.push_back(message);
        message.freq = f2;

        // message.sys_id = syid;
      }
      os << "tsbk29 secondary cc: rfid " << std::dec << rfid << " stid " << stid << " ch1 " << ch1 << "(" << channel_id_to_string(ch1) << ") ch2 " << ch2 << "(" << channel_id_to_string(ch2) << ") ";

      message.meta = os.str();
      //BOOST_LOG_TRIVIAL(trace) << os;


     BOOST_LOG_TRIVIAL(trace) << "tsbk29 secondary cc: rfid " << std::dec << rfid << " stid " << stid << " ch1 " << ch1 << "(" << channel_id_to_string(ch1) << ") ch2 " << ch2 << "(" << channel_id_to_string(ch2) << ") ";
    } else if (opcode == 0x2a) { // Group Affiliation Query
      BOOST_LOG_TRIVIAL(trace) << "tsbk2a";
    } else if (opcode == 0x2b) { // Location Registration Response
      BOOST_LOG_TRIVIAL(trace) << "tsbk2b";
    } else if (opcode == 0x2c) { // Unit Registration Response
      // unsigned long mfrid  = bitset_shift_mask(tsbk,80,0xff);
      // unsigned long opts  = bitset_shift_mask(tsbk,72,0xff);
      unsigned long sa = bitset_shift_mask(tsbk, 16, 0xffffff);
      unsigned long si = bitset_shift_mask(tsbk, 40, 0xffffff);

      message.message_type = REGISTRATION;
      message.source       = si;

      BOOST_LOG_TRIVIAL(trace) << "tsbk2c\tUnit Registration Response\tsa " << std::setw(7) << sa << " Source ID: " << si;
    } else if (opcode == 0x2d) { //
      BOOST_LOG_TRIVIAL(trace) << "tsbk2d";
    } else if (opcode == 0x2e) { //
      BOOST_LOG_TRIVIAL(trace) << "tsbk2e";
    } else if (opcode == 0x2f) { // Unit DeRegistration Ack
      // unsigned long mfrid  = bitset_shift_mask(tsbk,80,0xff);
      // unsigned long opts  = bitset_shift_mask(tsbk,72,0xff);
      unsigned long si = bitset_shift_mask(tsbk, 16, 0xffffff);


      message.message_type = DEREGISTRATION;
      message.source       = si;


      BOOST_LOG_TRIVIAL(trace) << "tsbk2f\tUnit Deregistration ACK\tSource ID: " << std::setw(7) << si << std::endl;
    } else if (opcode == 0x30) { //
      BOOST_LOG_TRIVIAL(trace) << "tsbk30";
    } else if (opcode == 0x31) { //
      BOOST_LOG_TRIVIAL(trace) << "tsbk31";
    } else if (opcode == 0x32) { //
      BOOST_LOG_TRIVIAL(trace) << "tsbk32";
    } else if (opcode == 0x33) { // iden_up_tdma
      unsigned long mfrid = bitset_shift_mask(tsbk, 80, 0xff);

      if (mfrid == 0) {
        unsigned long iden         = bitset_shift_mask(tsbk, 76, 0xf);
        unsigned long channel_type = bitset_shift_mask(tsbk, 72, 0xf);
        unsigned long toff0        = bitset_shift_mask(tsbk, 58, 0x3fff);
        unsigned long spac         = bitset_shift_mask(tsbk, 48, 0x3ff);
        unsigned long toff_sign    = (toff0 >> 13) & 1;
        long toff                  = toff0 & 0x1fff;

        if (toff_sign == 0) {
          toff = 0 - toff;
        }
        unsigned long f1        = bitset_shift_mask(tsbk, 16, 0xffffffff);
        int slots_per_carrier[] = { 1, 1, 1, 2, 4, 2 };
        bool chan_tdma;
        if (slots_per_carrier[channel_type] > 1) {
          chan_tdma = true;
        } else {
          chan_tdma = false;
        }
        Channel temp_chan       = {
          iden,                            // id;
          toff * spac * 125,               // offset;
          spac * 125,                      // step;
          f1 * 5,                          // frequency;
          chan_tdma,
          slots_per_carrier[channel_type], // tdma;
          6.25
        };
        add_channel(iden, temp_chan);
        BOOST_LOG_TRIVIAL(trace) << "tsbk33 iden up tdma id " << std::dec << iden << " f " <<  temp_chan.frequency << " offset " << temp_chan.offset << " spacing " <<
        temp_chan.step << " slots/carrier " << temp_chan.slots_per_carrier;
      }
    } else if (opcode == 0x34) { // iden_up vhf uhf
      unsigned long iden      = bitset_shift_mask(tsbk, 76, 0xf);
      unsigned long bwvu      = bitset_shift_mask(tsbk, 72, 0xf);
      unsigned long toff0     = bitset_shift_mask(tsbk, 58, 0x3fff);
      unsigned long spac      = bitset_shift_mask(tsbk, 48, 0x3ff);
      unsigned long freq      = bitset_shift_mask(tsbk, 16, 0xffffffff);
      unsigned long toff_sign = (toff0 >> 13) & 1;
      double bandwidth        = 0;

      if (bwvu == 4) {
        bandwidth = 6.25;
      } else if (bwvu == 5) {
        bandwidth = 12.5;
      }
      long toff = toff0 & 0x1fff;

      if (toff_sign == 0) {
        toff = 0 - toff;
      }
      std::string txt[]     = { "mob Tx-", "mob Tx+" };
      Channel     temp_chan = {
        iden,              // id;
        toff * spac * 125, // offset;
        spac * 125,        // step;
        freq * 5,          // frequency;
        false,                 // tdma;
        0,                 // slots
        bandwidth
      };
      add_channel(iden, temp_chan);

      BOOST_LOG_TRIVIAL(trace) << "tsbk34 iden vhf/uhf id " << std::dec << iden << " toff " << toff * spac * 0.125 * 1e-3 << " spac " << spac * 0.125 << " freq " << freq *
      0.000005 << " [ " << txt[toff_sign] << "]";
    } else if (opcode == 0x35) { // Time and Date Announcement
      BOOST_LOG_TRIVIAL(trace) << "tsbk35 time date ann";
    } else if (opcode == 0x36) { //
      BOOST_LOG_TRIVIAL(trace) << "tsbk36";
    } else if (opcode == 0x37) { //
      BOOST_LOG_TRIVIAL(trace) << "tsbk37";
    } else if (opcode == 0x38) { //
      BOOST_LOG_TRIVIAL(trace) << "tsbk38";
    } else if (opcode == 0x39) { // secondary cc
      unsigned long rfid = bitset_shift_mask(tsbk, 72, 0xff);
      unsigned long stid = bitset_shift_mask(tsbk, 64, 0xff);
      unsigned long ch1  = bitset_shift_mask(tsbk, 48, 0xffff);
      unsigned long ch2  = bitset_shift_mask(tsbk, 24, 0xffff);
      unsigned long f1   = channel_id_to_frequency(ch1);
      unsigned long f2   = channel_id_to_frequency(ch2);

      if (f1 && f2) {
        message.message_type = CONTROL_CHANNEL;
        message.freq         = f1;
        message.talkgroup    = 0;
        message.phase2_tdma = false;
        message.tdma_slot    = 0;
        messages.push_back(message);
        message.freq = f2;

        // message.sys_id = syid;
      }
      os << "tsbk39 secondary cc: rfid " << std::dec << rfid << " stid " << stid << " ch1 " << ch1 << "(" << channel_id_to_string(ch1) << ") ch2 " << ch2 << "(" <<
      channel_id_to_string(ch2) << ") ";
      message.meta = os.str();
      //BOOST_LOG_TRIVIAL(trace) << os;
    } else if (opcode == 0x3a)  { // rfss status
      unsigned long syid = bitset_shift_mask(tsbk, 56, 0xfff);
      unsigned long rfid = bitset_shift_mask(tsbk, 48, 0xff);
      unsigned long stid = bitset_shift_mask(tsbk, 40, 0xff);
      unsigned long chan = bitset_shift_mask(tsbk, 24, 0xffff);
      message.message_type = SYSID;
      message.sys_id        = syid;
      os << "tsbk3a rfss status: syid: " << syid << " rfid " << rfid << " stid " << stid << " ch1 " << chan << "(" << channel_id_to_string(chan) <<  ")" << std::endl;
      message.meta = os.str();
      //BOOST_LOG_TRIVIAL(trace) << os;
    } else if (opcode == 0x3b) { // network status
      unsigned long wacn = bitset_shift_mask(tsbk, 52, 0xfffff);
      unsigned long syid = bitset_shift_mask(tsbk, 40, 0xfff);
      unsigned long ch1  = bitset_shift_mask(tsbk, 24, 0xffff);
      unsigned long f1   = channel_id_to_frequency(ch1);

      if (f1) {
        message.message_type = STATUS;
        message.wacn         = wacn;
        message.sys_id    = syid;
        message.freq         = f1;
      }
      BOOST_LOG_TRIVIAL(trace) << "tsbk3b net stat: wacn " << std::dec << wacn << " syid " << syid << " ch1 " << ch1 << "(" << channel_id_to_string(ch1) << ") ";
    } else if (opcode == 0x3c) { // adjacent status
      unsigned long rfid = bitset_shift_mask(tsbk, 48, 0xff);
      unsigned long stid = bitset_shift_mask(tsbk, 40, 0xff);
      unsigned long ch1  = bitset_shift_mask(tsbk, 24, 0xffff);
      unsigned long f1   = channel_id_to_frequency(ch1);
      BOOST_LOG_TRIVIAL(trace) << "tsbk3c\tAdjacent Status\t rfid " << std::dec << rfid << " stid " << stid << " ch1 " << ch1 << "(" << channel_id_to_string(ch1) << ") ";

      if (f1) {
        it = channels.find((ch1 >> 12) & 0xf);

        if (it != channels.end()) {
          Channel temp_chan = it->second;

          //			self.adjacent[f1] = 'rfid: %d stid:%d uplink:%f
          // tbl:%d' % (rfid, stid, (f1 + self.freq_table[table]['offset']) /
          // 1000000.0, table)
          BOOST_LOG_TRIVIAL(trace) << "\ttsbk3c Chan " << temp_chan.frequency << "  " << temp_chan.step;
        }
      }
    } else if (opcode == 0x3d)  { // iden_up
      unsigned long iden      = bitset_shift_mask(tsbk, 76, 0xf);
      unsigned long bw        = bitset_shift_mask(tsbk, 67, 0x1ff);
      unsigned long toff0     = bitset_shift_mask(tsbk, 58, 0x1ff);
      unsigned long spac      = bitset_shift_mask(tsbk, 48, 0x3ff);
      unsigned long freq      = bitset_shift_mask(tsbk, 16, 0xffffffff);
      unsigned long toff_sign = (toff0 >> 8) & 1;
      unsigned long toff      = toff0 & 0xff;

      if (toff_sign == 0) {
        toff = 0 - toff;
      }

      Channel temp_chan = {
        iden,          // id;
        toff * 250000, // offset;
        spac * 125,    // step;
        freq * 5,      // frequency;
        false,             // tdma;
        1,            // slots
        bw * .125
      };
      add_channel(iden, temp_chan);
      BOOST_LOG_TRIVIAL(trace) << "tsbk3d iden id " << std::dec << iden << " toff " << toff * 0.25 << " spac " << spac * 0.125 << " freq " << freq * 0.000005;
    } else {
      BOOST_LOG_TRIVIAL(trace) << "tsbk other " << std::hex << opcode ;
      return messages;
    }
    messages.push_back(message);
    return messages;
}


void P25Parser::print_bitset(boost::dynamic_bitset<>& tsbk) {
  /*boost::dynamic_bitset<> bitmask(tsbk.size(), 0x3f);
     unsigned long result = (tsbk & bitmask).to_ulong();
     BOOST_LOG_TRIVIAL(trace) << tsbk << " = " << std::hex << result;*/
}

void printbincharpad(char c)
{
  for (int i = 7; i >= 0; --i)
  {
    std::cout << ((c & (1 << i)) ? '1' : '0');
  }

  // std::cout << " | ";
}



std::vector<TrunkMessage>P25Parser::parse_message(gr::message::sptr msg) {
  std::vector<TrunkMessage> messages;



  long type = msg->type();
  int sys_num = msg->arg1();
  TrunkMessage message;
  message.message_type = UNKNOWN;
  message.source       = 0;
  message.sys_num     = sys_num;
  if (type == -2) { // # request from gui
    std::string cmd = msg->to_string();

    BOOST_LOG_TRIVIAL(trace) << "process_qmsg: command: " << cmd;

    // self.update_state(cmd, curr_time)
    messages.push_back(message);
    return messages;
  } else if (type == -1) { //	# timeout
    BOOST_LOG_TRIVIAL(trace) << "process_data_unit timeout";

    // self.update_state('timeout', curr_time)
    messages.push_back(message);
    return messages;
  } else if (type < 0) {
    BOOST_LOG_TRIVIAL(trace) << "unknown message type " << type;
    messages.push_back(message);
    return messages;
  }
  std::string s = msg->to_string();

  // # nac is always 1st two bytes
  //ac = (ord(s[0]) << 8) + ord(s[1])
  uint8_t s0 = (int) s[0];
  uint8_t s1 = (int) s[1];
  int shift = s0 << 8;
  long nac = shift + s1;

  if (nac == 0xffff) {
    // # TDMA
    // self.update_state('tdma_duid%d' % type, curr_time)
    messages.push_back(message);
    return messages;
  }
  s = s.substr(2);

  BOOST_LOG_TRIVIAL(trace)  << std::dec << "nac " << nac << " type " << type <<  " size " <<
  msg->to_string().length() << " mesg len: " << msg->length() << std::endl;
  // //" at %f state %d len %d" %(nac, type, time.time(), self.state, len(s))
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

  if (type == 7) { // # trunk: TSBK
    boost::dynamic_bitset<> b((s.length() + 2) * 8);

    for (unsigned int i = 0; i < s.length(); ++i) {
      unsigned char c = (unsigned char)s[i];
      b <<= 8;

      for (int j = 0; j < 8; j++) {
        if (c & 0x1) {
          b[j] = 1;
        } else {
          b[j] = 0;
        }
        c >>= 1;
      }
    }
    b <<= 16;              // for missing crc

    return decode_tsbk(b, nac, sys_num);
  } else if (type == 12) { // # trunk: MBT
    std::string s1 = s.substr(0, 10);
    std::string s2 = s.substr(10);
    boost::dynamic_bitset<> header((s1.length() + 2) * 8);

    for (unsigned int i = 0; i < s1.length(); ++i) {
      unsigned char c = (unsigned char)s1[i];
      header <<= 8;

      for (int j = 0; j < 8; j++) {
        if (c & 0x1) {
          header[j] = 1;
        } else {
          header[j] = 0;
        }
        c >>= 1;
      }
    }
   header <<= 16; // for missing crc

    boost::dynamic_bitset<> mbt_data((s2.length() + 4) * 8);
    for (unsigned int i = 0; i < s2.length(); ++i) {
      unsigned char c = (unsigned char)s2[i];
      mbt_data <<= 8;

      for (int j = 0; j < 8; j++) {
        if (c & 0x1) {
          mbt_data[j] = 1;
        } else {
          mbt_data[j] = 0;
        }
        c >>= 1;
      }
    }
    mbt_data <<= 32;  // for missing crc
    unsigned long opcode = bitset_shift_mask(header, 32, 0x3f);
    unsigned long link_id = bitset_shift_mask(header, 48, 0xffffff);
    /*BOOST_LOG_TRIVIAL(trace) << "RAW  Data    " <<b;
    BOOST_LOG_TRIVIAL(trace) << "RAW  Data Length " <<s.length();*/
    BOOST_LOG_TRIVIAL(trace) << "MBT:  opcode: $" << std::hex << opcode ;
   /* BOOST_LOG_TRIVIAL(trace) << "MBT  type :$" << std::hex << type << " len $" << std::hex << s1.length() << "/" << s2.length();
    BOOST_LOG_TRIVIAL(trace) <<  "MBT Header: " <<  header;
    BOOST_LOG_TRIVIAL(trace) <<  "MBT  Data   " <<  mbt_data; */
    return decode_mbt_data(opcode, header, mbt_data, link_id, nac, sys_num);
    // self.trunked_systems[nac].decode_mbt_data(opcode, header << 16, mbt_data
    // << 32)
  }
  messages.push_back(message);
  return messages;
}
