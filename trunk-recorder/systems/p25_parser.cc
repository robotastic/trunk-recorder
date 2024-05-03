#include "p25_parser.h"
#include "../formatter.h"

P25Parser::P25Parser() {}

void P25Parser::add_channel(int chan_id, Channel temp_chan, int sys_num) {
  /*std::cout << "Add  - Channel id " << std::dec << chan_id << " freq " <<
    temp_chan.frequency << " offset " << temp_chan.offset << " step " <<
   temp_chan.step << " slots/carrier " << temp_chan.slots_per_carrier  << std::endl;
*/
  channels[sys_num][chan_id] = temp_chan;
}

long P25Parser::get_tdma_slot(int chan_id, int sys_num) {
  long channel = chan_id & 0xfff;

  it = channels[sys_num].find((chan_id >> 12) & 0xf);

  if (it != channels[sys_num].end()) {
    Channel temp_chan = it->second;

    if (temp_chan.phase2_tdma) {
      return channel & 1;
    }
  }

  return -1;
}

double P25Parser::get_bandwidth(int chan_id, int sys_num) {
  it = channels[sys_num].find((chan_id >> 12) & 0xf);

  if (it != channels[sys_num].end()) {
    Channel temp_chan = it->second;
    return temp_chan.bandwidth;
  }

  return 0;
}

double P25Parser::channel_id_to_frequency(int chan_id, int sys_num) {
  // long id      = (chan_id >> 12) & 0xf;
  long channel = chan_id & 0xfff;

  it = channels[sys_num].find((chan_id >> 12) & 0xf);

  if (it != channels[sys_num].end()) {
    Channel temp_chan = it->second;

    if (temp_chan.phase2_tdma) {
      return temp_chan.frequency + temp_chan.step * int(channel / temp_chan.slots_per_carrier);
    } else {
      return temp_chan.frequency + temp_chan.step * channel;
    }
  }
  return 0;
}

std::string P25Parser::channel_id_to_string(int chan_id, int sys_num) {
  double f = channel_id_to_frequency(chan_id, sys_num);

  if (f == 0) {
    return "ID"; // << std::hex << chan_id;
  } else {
    std::ostringstream strs;
    strs << f / 1000000.0;
    return strs.str();
  }
}

unsigned long P25Parser::bitset_shift_mask(boost::dynamic_bitset<> &tsbk, int shift, unsigned long long mask) {
  boost::dynamic_bitset<> bitmask(tsbk.size(), mask);
  unsigned long result = ((tsbk >> shift) & bitmask).to_ulong();

  // std::cout << "    " << std::dec<< shift << " " << tsbk.size() << " [ " <<
  // mask << " ]  = " << result << " - " << ((tsbk >> shift) & bitmask) <<
  // std::endl;
  return result;
}

unsigned long P25Parser::bitset_shift_left_mask(boost::dynamic_bitset<> &tsbk, int shift, unsigned long long mask) {
  boost::dynamic_bitset<> bitmask(tsbk.size(), mask);
  unsigned long result = ((tsbk << shift) & bitmask).to_ulong();

  // std::cout << "    " << std::dec<< shift << " " << tsbk.size() << " [ " <<
  // mask << " ]  = " << result << " - " << ((tsbk >> shift) & bitmask) <<
  // std::endl;
  return result;
}

std::vector<TrunkMessage> P25Parser::decode_mbt_data(unsigned long opcode, boost::dynamic_bitset<> &header, boost::dynamic_bitset<> &mbt_data, unsigned long sa, unsigned long nac, int sys_num) {
  std::vector<TrunkMessage> messages;
  TrunkMessage message;
  std::ostringstream os;

  message.message_type = UNKNOWN;
  message.source = -1;
  message.wacn = 0;
  message.nac = nac;
  message.sys_id = 0;
  message.sys_num = sys_num;
  message.talkgroup = 0;
  message.emergency = false;
  message.encrypted = false;
  message.duplex = false;
  message.mode = false;
  message.priority = 0;
  message.phase2_tdma = false;
  message.tdma_slot = 0;
  message.freq = 0;
  message.opcode = opcode;

  BOOST_LOG_TRIVIAL(trace) << "decode_mbt_data: $" << opcode;
  if (opcode == 0x0) { // grp voice channel grant
    // unsigned long mfrid = bitset_shift_mask(header, 72, 0xff);
    unsigned long ch1 = bitset_shift_mask(mbt_data, 64, 0xffff);
    unsigned long ch2 = bitset_shift_mask(mbt_data, 48, 0xffff);
    unsigned long ga = bitset_shift_mask(mbt_data, 32, 0xffff);
    unsigned long f1 = channel_id_to_frequency(ch1, sys_num);
    unsigned long f2 = channel_id_to_frequency(ch2, sys_num);
    unsigned long sa = bitset_shift_mask(header, 48, 0xffffff);
    bool emergency = (bool)bitset_shift_mask(header, 24, 0x80);
    bool encrypted = (bool)bitset_shift_mask(header, 24, 0x40);
    bool duplex = (bool)bitset_shift_mask(header, 24, 0x20);
    bool mode = (bool)bitset_shift_mask(header, 24, 0x10);
    int priority = bitset_shift_mask(header, 24, 0x07);


    message.message_type = GRANT;
    message.freq = f1;
    message.talkgroup = ga;
    message.source = sa;
    message.emergency = emergency;
    message.encrypted = encrypted;
    message.duplex = duplex;
    message.mode = mode;
    message.priority = priority;

    if (get_tdma_slot(ch1, sys_num) >= 0) {
      message.phase2_tdma = true;
      message.tdma_slot = get_tdma_slot(ch1, sys_num);
    } else {
      message.phase2_tdma = false;
      message.tdma_slot = 0;
    }

    os << "mbt00\tChan Grant\tChannel 1 ID: " << std::setw(5) << ch1 << "\tFreq: " << format_freq(f1) <<  "\tChannel 2 ID: " << std::setw(5) << ch2 << "\tFreq: " << format_freq(f2) << "\tga " << std::setw(7) << ga << "\tTDMA " << get_tdma_slot(ch1, sys_num) << "\tsa " << sa << "\tEncrypt " << encrypted << "\tBandwidth: " << get_bandwidth(ch1, sys_num);
    message.meta = os.str();
    BOOST_LOG_TRIVIAL(debug) << os.str();
  } else if (opcode == 0x02) { // grp regroup voice channel grant
    unsigned long mfrid = bitset_shift_mask(mbt_data, 168, 0xff);
    if (mfrid == 0x90) {  // MOT_GRG_CN_GRANT_EXP
      unsigned long ch1 = bitset_shift_mask(mbt_data, 80, 0xffff);
      unsigned long ch2 = bitset_shift_mask(mbt_data, 64, 0xffff);
      unsigned long sg = bitset_shift_mask(mbt_data, 48, 0xffff);
      unsigned long f1 = channel_id_to_frequency(ch1, sys_num);
      unsigned long f2 = channel_id_to_frequency(ch2, sys_num);
      message.message_type = GRANT;
      message.freq = f1;
      message.talkgroup = sg;

      if (get_tdma_slot(ch1, sys_num) >= 0) {
        message.phase2_tdma = true;
        message.tdma_slot = get_tdma_slot(ch1, sys_num);
      } else {
        message.phase2_tdma = false;
        message.tdma_slot = 0;
      }

      os << "mbt02\tmfid90_grg_cn_grant_exp\tChannel 1 ID: " << std::setw(5) << ch1 << "\tFreq: " << format_freq(f1) <<  "\tChannel 2 ID: " << std::setw(5) << ch2 << "\tFreq: " << format_freq(f2) << "\tsg " << std::setw(7) << sg << "\tTDMA " << get_tdma_slot(ch1, sys_num) << "\tBandwidth: " << get_bandwidth(ch1, sys_num);
      message.meta = os.str();
      BOOST_LOG_TRIVIAL(debug) << os.str();
    }
  } else if (opcode == 0x028) { // grp_aff_rsp
    unsigned long mfrid = bitset_shift_mask(mbt_data, 56, 0xff);
    unsigned long wacn = (bitset_shift_left_mask(header, 4, 0xffff0) + bitset_shift_mask(mbt_data, 188, 0xf));
    unsigned long syid = bitset_shift_mask(mbt_data, 176, 0xfff);
    unsigned long gid = bitset_shift_mask(mbt_data, 160, 0xffff);
    unsigned long ada = bitset_shift_mask(mbt_data, 144, 0xffff);
    unsigned long ga = bitset_shift_mask(mbt_data, 128, 0xffff);
    unsigned long lg = bitset_shift_mask(mbt_data, 127, 0x1);
    unsigned long gav = bitset_shift_mask(mbt_data, 120, 0x3);

      os << "mbt28\tmbt(0x28) grp_aff_rsp:\tMFRID: " << mfrid <<  "\tWACN: " <<  wacn << "\tSYID: " << syid << "\tLG: " << lg << "\tGAV: " << gav << "\tADA: " << ada << "\tGA: " << ga << "\tLG: " << lg;
      message.meta = os.str();
      BOOST_LOG_TRIVIAL(debug) << os.str();
  } else if (opcode == 0x3a) { // rfss status
    unsigned long syid = bitset_shift_mask(header, 48, 0xfff);
    unsigned long rfid = bitset_shift_mask(mbt_data, 88, 0xff);
    unsigned long stid = bitset_shift_mask(mbt_data, 80, 0xff);
    unsigned long ch1 = bitset_shift_mask(mbt_data, 64, 0xffff);
    // unsigned long ch2 = bitset_shift_mask(mbt_data, 48, 0xffff);
    // unsigned long f1   = channel_id_to_frequency(ch1, sys_num);
    // unsigned long f2   = channel_id_to_frequency(ch2, sys_num);
    message.message_type = SYSID;
    message.sys_id = syid;
    message.sys_rfss = rfid;
    message.sys_site_id = stid;
    os << "mbt3a rfss status: syid: " << syid << " rfid " << rfid << " stid " << stid << " ch1 " << ch1 << "(" << channel_id_to_string(ch1, sys_num) << ")";
    message.meta = os.str();
    BOOST_LOG_TRIVIAL(trace) << os.str();
  } else if (opcode == 0x3b) { // network status
    unsigned long wacn = bitset_shift_mask(mbt_data, 76, 0xfffff);
    unsigned long syid = bitset_shift_mask(header, 48, 0xfff);
    unsigned long ch1 = bitset_shift_mask(mbt_data, 56, 0xffff);
    unsigned long ch2 = bitset_shift_mask(mbt_data, 40, 0xffff);
    unsigned long f1 = channel_id_to_frequency(ch1, sys_num);
    unsigned long f2 = channel_id_to_frequency(ch2, sys_num);

    if (f1 && f2) {
      message.message_type = STATUS;
      message.wacn = wacn;
      message.sys_id = syid;
      message.freq = f1;
    }
    BOOST_LOG_TRIVIAL(trace) << "mbt3b net stat: wacn " << std::dec << wacn << " syid " << syid << " ch1 " << ch1 << "(" << channel_id_to_string(ch1, sys_num) << ") ";
  } else if (opcode == 0x3c) { // adjacent status
    unsigned long syid = bitset_shift_mask(header, 48, 0xfff);
    unsigned long rfid = bitset_shift_mask(header, 24, 0xff);
    unsigned long stid = bitset_shift_mask(header, 16, 0xff);
    unsigned long ch1 = bitset_shift_mask(mbt_data, 80, 0xffff);
    unsigned long ch2 = bitset_shift_mask(mbt_data, 64, 0xffff);
    BOOST_LOG_TRIVIAL(trace) << "mbt3c adjacent status "
                             << "syid " << syid << " rfid " << rfid << " stid " << stid << " ch1 " << ch1 << " ch2 " << ch2;
  } else if (opcode == 0x04) { //  Unit to Unit Voice Service Channel Grant -Extended (UU_V_CH_GRANT)
    // unsigned long mfrid = bitset_shift_mask(header, 80, 0xff);
    bool emergency = (bool)bitset_shift_mask(header, 24, 0x80);
    bool encrypted = (bool)bitset_shift_mask(header, 24, 0x40);
    bool dup = (bool)bitset_shift_mask(header, 24, 0x20);
    bool mod = (bool)bitset_shift_mask(header, 24, 0x10);
    int pri = bitset_shift_mask(header, 24, 0x07);
    unsigned long ch = bitset_shift_mask(header, 16, 0xffff); /// ????
    unsigned long f = channel_id_to_frequency(ch, sys_num);
    unsigned long sa = bitset_shift_mask(header, 48, 0xffffff);
    unsigned long ta = bitset_shift_mask(mbt_data, 24, 0xffffff);

    message.message_type = UU_V_GRANT;
    message.freq = f;
    message.talkgroup = ta;
    message.source = sa;
    message.emergency = emergency;
    message.encrypted = encrypted;
    message.duplex = dup;
    message.mode = mod;
    message.priority = pri;
    if (get_tdma_slot(ch, sys_num) >= 0) {
      message.phase2_tdma = true;
      message.tdma_slot = get_tdma_slot(ch, sys_num);
    } else {
      message.phase2_tdma = false;
      message.tdma_slot = 0;
    }

    BOOST_LOG_TRIVIAL(debug) << "mbt04\tUnit to Unit Chan Grant\tChannel ID: " << std::setw(5) << ch << "\tFreq: " << format_freq(f) << "\tTarget ID: " << std::setw(7) << ta << "\tTDMA " << get_tdma_slot(ch, sys_num) << "\tSource ID: " << sa;
  } else {
    BOOST_LOG_TRIVIAL(debug) << "mbt other: " << opcode;
    return messages;
  }
  messages.push_back(message);
  return messages;
}

std::vector<TrunkMessage> P25Parser::decode_tsbk(boost::dynamic_bitset<> &tsbk, unsigned long nac, int sys_num) {
  // self.stats['tsbks'] += 1
  std::vector<TrunkMessage> messages;
  TrunkMessage message;
  std::ostringstream os;

  // TSBK is shifted 16 prior for the missing CRC prior to this function
  unsigned long opcode = bitset_shift_mask(tsbk, 88, 0x3f); // x3f

  message.message_type = UNKNOWN;
  message.source = -1;
  message.wacn = 0;
  message.nac = nac;
  message.sys_id = 0;
  message.sys_num = sys_num;
  message.talkgroup = 0;
  message.emergency = false;
  message.duplex = false;
  message.mode = false;
  message.priority = 0;
  message.encrypted = false;
  message.phase2_tdma = false;
  message.tdma_slot = 0;
  message.freq = 0;
  message.opcode = opcode;

  BOOST_LOG_TRIVIAL(trace) << "TSBK: opcode: $" << std::hex << opcode;
  if (opcode == 0x00) { // group voice chan grant
    // Group Voice Channel Grant (GRP_V_CH_GRANT)

    unsigned long mfrid = bitset_shift_mask(tsbk, 80, 0xff);

    if (mfrid == 0x90) { // MOT_GRG_ADD_CMD
      unsigned long sg = bitset_shift_mask(tsbk, 64, 0xffff);
      unsigned long ga1 = bitset_shift_mask(tsbk, 48, 0xffff);
      unsigned long ga2 = bitset_shift_mask(tsbk, 32, 0xffff);
      unsigned long ga3 = bitset_shift_mask(tsbk, 16, 0xffff);
      BOOST_LOG_TRIVIAL(trace) << "tsbk00\tMoto Patch Add \tsg: " << sg << "\tga1: " << ga1 << "\tga2: " << ga2 << "\tga3: " << ga3;
      message.message_type = PATCH_ADD;
      PatchData moto_patch_data;
      moto_patch_data.sg = sg;
      moto_patch_data.ga1 = ga1;
      moto_patch_data.ga2 = ga2;
      moto_patch_data.ga3 = ga3;
      message.patch_data = moto_patch_data;
    } else {
      unsigned long opts  = bitset_shift_mask(tsbk, 72, 0xff);
      bool emergency = (bool)bitset_shift_mask(tsbk, 72, 0x80);
      bool encrypted = (bool)bitset_shift_mask(tsbk, 72, 0x40);
      bool duplex = (bool)bitset_shift_mask(tsbk, 72, 0x20);
      bool mode = (bool)bitset_shift_mask(tsbk, 72, 0x10);
      int priority = bitset_shift_mask(tsbk, 72, 0x07);
      unsigned long ch = bitset_shift_mask(tsbk, 56, 0xffff);
      unsigned long ga = bitset_shift_mask(tsbk, 40, 0xffff);
      unsigned long sa = bitset_shift_mask(tsbk, 16, 0xffffff);
      unsigned long f1 = channel_id_to_frequency(ch, sys_num);
      message.message_type = GRANT;
      message.freq = f1;
      message.talkgroup = ga;
      message.source = sa;
      message.emergency = emergency;
      message.encrypted = encrypted;
      message.duplex = duplex;
      message.mode = mode;
      message.priority = priority;

      if (get_tdma_slot(ch, sys_num) >= 0) {
        message.phase2_tdma = true;
        message.tdma_slot = get_tdma_slot(ch, sys_num);
      } else {
        message.phase2_tdma = false;
        message.tdma_slot = 0;
      }
      os << "tsbk00\tChan Grant\tChannel ID: " << std::setw(5) << ch << "\tFreq: " << format_freq(f1) << "\tga " << std::setw(7) << ga << "\tTDMA " << get_tdma_slot(ch, sys_num) << "\tsa " << sa << "\tEncrypt " << encrypted << "\tBandwidth: " << get_bandwidth(ch, sys_num);
      message.meta = os.str();
      BOOST_LOG_TRIVIAL(debug) << os.str();
    }
  } else if (opcode == 0x02) { // group voice chan grant update
    unsigned long mfrid = bitset_shift_mask(tsbk, 80, 0xff);
    // Group Voice Channel Grant Update (GRP_V_CH_GRANT_UPDT) : TIA.102-AABC-B-2005 page 34
    // Options are not present in an UPDATE

    if (mfrid == 0x90) {
        unsigned long opts = bitset_shift_mask(tsbk, 72, 0xff);
        bool emergency = (bool)bitset_shift_mask(tsbk, 72, 0x80);
        bool encrypted = (bool)bitset_shift_mask(tsbk, 72, 0x40);
        bool duplex = (bool)bitset_shift_mask(tsbk, 72, 0x20);
        bool mode = (bool)bitset_shift_mask(tsbk, 72, 0x10);
        int priority = bitset_shift_mask(tsbk, 72, 0x07);
        
        unsigned long ch = bitset_shift_mask(tsbk, 56, 0xffff);
        unsigned long sg = bitset_shift_mask(tsbk, 40, 0xffff);
        unsigned long sa = bitset_shift_mask(tsbk, 16, 0xffffff);
        unsigned long f = channel_id_to_frequency(ch, sys_num);

        message.message_type = GRANT;
        message.freq = f;
        message.talkgroup = sg;
        message.source = sa;
        
        message.encrypted = encrypted;
        message.emergency = emergency;
        message.duplex = duplex;
        message.mode = mode;
        message.priority = priority;

      if (get_tdma_slot(ch, sys_num) >= 0) {
        message.phase2_tdma = true;
        message.tdma_slot = get_tdma_slot(ch, sys_num);
      } else {
        message.phase2_tdma = false;
        message.tdma_slot = 0;
      }

      os << "tsbk02\tMOTOROLA_OSP_PATCH_GROUP_CHANNEL_GRANT\tChannel ID: " << std::setw(5) << ch << "\tFreq: " << format_freq(f) << "\tsg " << std::setw(7) << sg << "\tTDMA " << get_tdma_slot(ch, sys_num) << "\tsa " << sa;
      message.meta = os.str();
      BOOST_LOG_TRIVIAL(debug) << os.str();
    } else {
      unsigned long ch1 = bitset_shift_mask(tsbk, 64, 0xffff);
      unsigned long ga1 = bitset_shift_mask(tsbk, 48, 0xffff);
      unsigned long ch2 = bitset_shift_mask(tsbk, 32, 0xffff);
      unsigned long ga2 = bitset_shift_mask(tsbk, 16, 0xffff);
      unsigned long f1 = channel_id_to_frequency(ch1, sys_num);
      unsigned long f2 = channel_id_to_frequency(ch2, sys_num);

      message.message_type = UPDATE;
      message.freq = f1;
      message.talkgroup = ga1;

      if (get_tdma_slot(ch1, sys_num) >= 0) {
        message.phase2_tdma = true;
        message.tdma_slot = get_tdma_slot(ch1, sys_num);
      } else {
        message.phase2_tdma = false;
        message.tdma_slot = 0;
      }

      if ((f1 != f2) && (ch2 != 65535)) {
        messages.push_back(message);
        message.freq = f2;
        message.talkgroup = ga2;

        if (get_tdma_slot(ch2, sys_num) >= 0) {
          message.phase2_tdma = true;
          message.tdma_slot = get_tdma_slot(ch2, sys_num);
        } else {
          message.phase2_tdma = false;
          message.tdma_slot = 0;
        }

        os << "tsbk02\tGrant Update 2nd\tChannel ID: " << std::setw(5) << ch2 << "\tFreq: " << format_freq(f2) << "\tga " << std::setw(7) << ga2 << "\tTDMA " << get_tdma_slot(ch2, sys_num) << " | ";

        message.meta = os.str();
        
      }
      os << "tsbk02\tGrant Update\tChannel ID: " << std::setw(5) << ch1 << "\tFreq: " << format_freq(f1) << "\tga " << std::setw(7) << ga1 << "\tTDMA " << get_tdma_slot(ch1, sys_num);
      message.meta = os.str();
      BOOST_LOG_TRIVIAL(debug) << os.str();
    }
  } else if (opcode == 0x03) { //  Group Voice Channel Update-Explicit (GRP_V_CH_GRANT_UPDT_EXP)
    // group voice chan grant update exp : TIA.102-AABC-B-2005 page 56
    unsigned long mfrid = bitset_shift_mask(tsbk, 80, 0xff);

    if (mfrid == 0x90) { // MOT_GRG_CN_GRANT_UPDT  // MOTOROLA_OSP_PATCH_GROUP_CHANNEL_GRANT_UPDATE // Service Options are not in the Moto version of the message

      unsigned long ch1 = bitset_shift_mask(tsbk, 64, 0xffff);
      unsigned long sg1 = bitset_shift_mask(tsbk, 48, 0xffff);
      unsigned long ch2 = bitset_shift_mask(tsbk, 32, 0xffff);
      unsigned long sg2 = bitset_shift_mask(tsbk, 16, 0xffff);

      unsigned long f1 = channel_id_to_frequency(ch1, sys_num);
      unsigned long f2 = channel_id_to_frequency(ch2, sys_num);

      message.message_type = UPDATE;
      message.freq = f1;
      message.talkgroup = sg1;

      if (get_tdma_slot(ch1, sys_num) >= 0) {
        message.phase2_tdma = true;
        message.tdma_slot = get_tdma_slot(ch1, sys_num);
      } else {
        message.phase2_tdma = false;
        message.tdma_slot = 0;
      }

      if (f1 != f2) {
        messages.push_back(message);
        message.freq = f2;
        message.talkgroup = sg2;
        if (get_tdma_slot(ch2, sys_num) >= 0) {
          message.phase2_tdma = true;
          message.tdma_slot = get_tdma_slot(ch2, sys_num);
        } else {
          message.phase2_tdma = false;
          message.tdma_slot = 0;
        }
        os << "MOTOROLA_OSP_PATCH_GROUP_CHANNEL_GRANT_UPDATE(0x03): \tChannel ID: " << std::setw(5) << ch2 << "\tFreq: " << format_freq(f2) << "\tsg " << std::setw(7) << sg2 << "\tTDMA " << get_tdma_slot(ch2, sys_num);
        message.meta = os.str();
        BOOST_LOG_TRIVIAL(debug) << os.str();
      }
      os << "MOTOROLA_OSP_PATCH_GROUP_CHANNEL_GRANT_UPDATE(0x03): \tChannel ID: " << std::setw(5) << ch1 << "\tFreq: " << format_freq(f1) << "\tsg " << std::setw(7) << sg1 << "\tTDMA " << get_tdma_slot(ch1, sys_num);
      message.meta = os.str();
      BOOST_LOG_TRIVIAL(debug) << os.str();
    } else {
      bool emergency = (bool)bitset_shift_mask(tsbk, 72, 0x80);
      bool encrypted = (bool)bitset_shift_mask(tsbk, 72, 0x40);
      bool duplex = (bool)bitset_shift_mask(tsbk, 72, 0x20);
      bool mode = (bool)bitset_shift_mask(tsbk, 72, 0x10);
      int priority = bitset_shift_mask(tsbk, 72, 0x07);

      unsigned long ch1 = bitset_shift_mask(tsbk, 48, 0xffff);
      unsigned long ch2 = bitset_shift_mask(tsbk, 32, 0xffff);
      unsigned long ga1 = bitset_shift_mask(tsbk, 16, 0xffff);
      unsigned long f1 = channel_id_to_frequency(ch1, sys_num);
      unsigned long f2 = channel_id_to_frequency(ch2, sys_num);

      message.message_type = UPDATE;
      message.freq = f1;
      message.talkgroup = ga1;
      message.emergency = emergency;
      message.encrypted = encrypted;
      if (get_tdma_slot(ch1, sys_num) >= 0) {
        message.phase2_tdma = true;
        message.tdma_slot = get_tdma_slot(ch2, sys_num);
      } else {
        message.phase2_tdma = false;
        message.tdma_slot = 0;
      }

      os << "tsbk03\tExplicit Grant Update\tTX Channel ID: " << std::setw(5) << ch1 << "\tFreq: " << format_freq(f1) << "\tFNE TX Channel ID: " << std::setw(5) << ch1 << "\tFreq: " << format_freq(f1) << "\tga " << std::setw(7) << ga1 << "\tTDMA " << get_tdma_slot(ch1, sys_num);
      message.meta = os.str();
      BOOST_LOG_TRIVIAL(debug) << os.str();
    }
  } else if (opcode == 0x04) { //  Unit to Unit Voice Service Channel Grant (UU_V_CH_GRANT)
                               // unsigned long mfrid = bitset_shift_mask(tsbk, 80, 0xff);
    // unsigned long opts  = bitset_shift_mask(tsbk,72,0xff);
    bool emergency = (bool)bitset_shift_mask(tsbk, 72, 0x80);
    bool encrypted = (bool)bitset_shift_mask(tsbk, 72, 0x40);
    bool duplex = (bool)bitset_shift_mask(tsbk, 72, 0x20);
    bool mode = (bool)bitset_shift_mask(tsbk, 72, 0x10);
    int priority = bitset_shift_mask(tsbk, 72, 0x07);
    unsigned long ch = bitset_shift_mask(tsbk, 64, 0xffff);
    unsigned long f = channel_id_to_frequency(ch, sys_num);
    unsigned long sa = bitset_shift_mask(tsbk, 16, 0xffffff);
    unsigned long ta = bitset_shift_mask(tsbk, 40, 0xffffff);

    message.message_type = UU_V_GRANT;
    message.freq = f;
    message.talkgroup = ta;
    message.source = sa;
    message.emergency = emergency;
    message.encrypted = encrypted;
    message.duplex = duplex;
    message.mode = mode;
    message.priority = priority;
    if (get_tdma_slot(ch, sys_num) >= 0) {
      message.phase2_tdma = true;
      message.tdma_slot = get_tdma_slot(ch, sys_num);
    } else {
      message.phase2_tdma = false;
      message.tdma_slot = 0;
    }

    BOOST_LOG_TRIVIAL(debug) << "tsbk04\tUnit to Unit Chan Grant\tChannel ID: " << std::setw(5) << ch << "\tFreq: " << format_freq(f) << "\tTarget ID: " << std::setw(7) << ta << "\tTDMA " << get_tdma_slot(ch, sys_num) << "\tSource ID: " << sa;
  } else if (opcode == 0x05) { // Unit To Unit Answer Request
    bool emergency = (bool)bitset_shift_mask(tsbk, 72, 0x80);
    bool encrypted = (bool)bitset_shift_mask(tsbk, 72, 0x40);
    bool duplex = (bool)bitset_shift_mask(tsbk, 72, 0x20);
    bool mode = (bool)bitset_shift_mask(tsbk, 72, 0x10);
    int priority = bitset_shift_mask(tsbk, 72, 0x07);
    unsigned long sa = bitset_shift_mask(tsbk, 16, 0xffffff);
    unsigned long si = bitset_shift_mask(tsbk, 40, 0xffffff);

    message.message_type = UU_ANS_REQ;
    message.emergency = emergency;
    message.encrypted = encrypted;
    message.duplex = duplex;
    message.mode = mode;
    message.priority = priority;
    message.source = sa;
    message.talkgroup = si;

    BOOST_LOG_TRIVIAL(debug) << "tsbk05\tUnit To Unit Answer Request\tsa " << sa << "\tSource ID: " << si;
  } else if (opcode == 0x06) { //  Unit to Unit Voice Channel Grant Update (UU_V_CH_GRANT_UPDT)
    // unsigned long mfrid = bitset_shift_mask(tsbk, 80, 0xff);
    //  unsigned long opts  = bitset_shift_mask(tsbk,72,0xff);


    unsigned long ch = bitset_shift_mask(tsbk, 64, 0xffff);
    unsigned long f = channel_id_to_frequency(ch, sys_num);
    unsigned long sa = bitset_shift_mask(tsbk, 16, 0xffffff);
    unsigned long ta = bitset_shift_mask(tsbk, 40, 0xffffff);

    message.message_type = UU_V_UPDATE;
    message.freq = f;
    message.talkgroup = ta;
    message.source = sa;
    if (get_tdma_slot(ch, sys_num) >= 0) {
      message.phase2_tdma = true;
      message.tdma_slot = get_tdma_slot(ch, sys_num);
    } else {
      message.phase2_tdma = false;
      message.tdma_slot = 0;
    }

    BOOST_LOG_TRIVIAL(debug) << "tsbk06\tUnit to Unit Chan Update\tChannel ID: " << std::setw(5) << ch << "\tFreq: " << format_freq(f) << "\tTarget ID: " << std::setw(7) << ta << "\tTDMA " << get_tdma_slot(ch, sys_num) << "\tSource ID: " << sa;
  } else if (opcode == 0x08) {
    BOOST_LOG_TRIVIAL(debug) << "tsbk08: Telephone Interconnect Voice Channel Grant";
  } else if (opcode == 0x09) {
    BOOST_LOG_TRIVIAL(debug) << "tsbk09: Telephone Interconnect Voice Channel Grant Update";
  } else if (opcode == 0x0a) {
    BOOST_LOG_TRIVIAL(debug) << "tsbk0a: Telephone Interconnect Answer Request";
  } else if (opcode == 0x14) {
    bool emergency = (bool)bitset_shift_mask(tsbk, 72, 0x80);
    bool encrypted = (bool)bitset_shift_mask(tsbk, 72, 0x40);
    bool duplex = (bool)bitset_shift_mask(tsbk, 72, 0x20);
    bool mode = (bool)bitset_shift_mask(tsbk, 72, 0x10);
    unsigned long nsapi = bitset_shift_mask(tsbk, 72, 0xf);
    unsigned long chT = bitset_shift_mask(tsbk, 56, 0xffff);
    unsigned long chR = bitset_shift_mask(tsbk, 40, 0xffff);
    unsigned long sa = bitset_shift_mask(tsbk, 16, 0xffffff);
    unsigned long fT = channel_id_to_frequency(chT, sys_num);
    unsigned long fR = channel_id_to_frequency(chR, sys_num);

    message.message_type = DATA_GRANT;
    message.emergency = emergency;
    message.encrypted = encrypted;
    message.duplex = duplex;
    message.mode = mode;
    message.source = sa;
    message.freq = fT;

    BOOST_LOG_TRIVIAL(debug) << "tsbk14\tSNDCP Data Channel Grant\tsa " << sa << "\tChannels: " << chT << "/" << chR << " Freqs: " << format_freq(fT) << "/" << format_freq(fR) << " NSAPI: " << nsapi;
  } else if (opcode == 0x15) {
    BOOST_LOG_TRIVIAL(debug) << "tsbk15: SNDCP Data Page Request";
  } else if (opcode == 0x16) {
    BOOST_LOG_TRIVIAL(trace) << "tsbk16: SNDCP Data Channel Announcement -Explicit";
  } else if (opcode == 0x18) {
    BOOST_LOG_TRIVIAL(debug) << "tsbk18: Status Update";
  } else if (opcode == 0x1a) {
    BOOST_LOG_TRIVIAL(debug) << "tsbk1a: Status Query";
  } else if (opcode == 0x1c) {
    BOOST_LOG_TRIVIAL(debug) << "tsbk1c: Messag Update";
  } else if (opcode == 0x1d) {
    BOOST_LOG_TRIVIAL(debug) << "tsbk1d: Radio Unit Monitor Command";
  } else if (opcode == 0x1f) {
    BOOST_LOG_TRIVIAL(debug) << "tsbk1f: Call Alert";
  } else if (opcode == 0x20) { // Acknowledge response
    // unsigned long mfrid  = bitset_shift_mask(tsbk,80,0xff);
    unsigned long ga = bitset_shift_mask(tsbk, 40, 0xffff);
    unsigned long op = bitset_shift_mask(tsbk, 48, 0xff);
    unsigned long sa = bitset_shift_mask(tsbk, 16, 0xffffff);

    message.message_type = ACKNOWLEDGE;
    message.talkgroup = ga;
    message.source = sa;

    BOOST_LOG_TRIVIAL(debug) << "tsbk20\tAcknowledge Response\tga " << std::dec << ga << "\tsa " << sa << "\tReserved: " << op;
  } else if (opcode == 0x21) {
    BOOST_LOG_TRIVIAL(debug) << "tsbk21: Extended Function Command";
  } else if (opcode == 0x24) {
    BOOST_LOG_TRIVIAL(debug) << "tsbk24: Extended Function Command";
  } else if (opcode == 0x27) {
    BOOST_LOG_TRIVIAL(debug) << "tsbk27: Deny Response";
  } else if (opcode == 0x28) { // Unit Group Affiliation Response
    // unsigned long mfrid  = bitset_shift_mask(tsbk,80,0xff);
    // unsigned long opts  = bitset_shift_mask(tsbk,72,0xff);
    unsigned long ta = bitset_shift_mask(tsbk, 16, 0xffffff);
    unsigned long ga = bitset_shift_mask(tsbk, 40, 0xffff);
    unsigned long aga = bitset_shift_mask(tsbk, 56, 0xffff);

    message.message_type = AFFILIATION;
    message.source = ta;
    message.talkgroup = ga;

    BOOST_LOG_TRIVIAL(debug) << "tsbk28\tUnit Group Affiliation\tSource ID: " << std::setw(7) << ta << "\tGroup Address: " << std::dec << ga << "\tAnnouncement Group: " << aga;
  } else if (opcode == 0x29) { // Secondary Control Channel Broadcast - Explicit
    unsigned long rfid = bitset_shift_mask(tsbk, 72, 0xff);
    unsigned long stid = bitset_shift_mask(tsbk, 64, 0xff);
    unsigned long ch1 = bitset_shift_mask(tsbk, 48, 0xffff);
    unsigned long ch2 = bitset_shift_mask(tsbk, 24, 0xffff);
    unsigned long f1 = channel_id_to_frequency(ch1, sys_num);
    unsigned long f2 = channel_id_to_frequency(ch2, sys_num);

    if (f1 && f2) {
      message.message_type = CONTROL_CHANNEL;
      message.freq = f1;
      message.talkgroup = 0;
      message.phase2_tdma = false;
      message.tdma_slot = 0;
      messages.push_back(message);
      message.freq = f2;

      // message.sys_id = syid;
    }
    os << "tsbk29 secondary cc: rfid " << std::dec << rfid << " stid " << stid << " ch1 " << ch1 << "(" << channel_id_to_string(ch1, sys_num) << ") ch2 " << ch2 << "(" << channel_id_to_string(ch2, sys_num) << ") ";

    message.meta = os.str();
    BOOST_LOG_TRIVIAL(trace) << os.str();

  } else if (opcode == 0x2a) { // Group Affiliation Query
    BOOST_LOG_TRIVIAL(debug) << "tsbk2a Group Affiliation Query";
  } else if (opcode == 0x2b) { // Location Registration Response
    // unsigned long mfrid  = bitset_shift_mask(tsbk,80,0xff);
    unsigned long ga = bitset_shift_mask(tsbk, 56, 0xffff);
    unsigned long rv = bitset_shift_mask(tsbk, 72, 0x03);
    unsigned long sa = bitset_shift_mask(tsbk, 16, 0xffffff);

    message.message_type = LOCATION;
    message.talkgroup = ga;
    message.source = sa;

    BOOST_LOG_TRIVIAL(debug) << "tsbk2b\tLocation Registration Response\tga " << std::dec << ga << "\tsa " << sa << "\tValue: " << rv;
  } else if (opcode == 0x2c) { // Unit Registration Response
    // unsigned long mfrid  = bitset_shift_mask(tsbk,80,0xff);
    // unsigned long opts  = bitset_shift_mask(tsbk,72,0xff);
    unsigned long sa = bitset_shift_mask(tsbk, 16, 0xffffff);
    unsigned long si = bitset_shift_mask(tsbk, 40, 0xffffff);

    message.message_type = REGISTRATION;
    message.source = si;

    BOOST_LOG_TRIVIAL(debug) << "tsbk2c\tUnit Registration COMMAND\tsa " << std::setw(7) << sa << " Source ID: " << si;
  } else if (opcode == 0x2d) { //
    BOOST_LOG_TRIVIAL(debug) << "tsbk2d AUTHENTICATION COMMAND";
  } else if (opcode == 0x2e) { //
    BOOST_LOG_TRIVIAL(debug) << "tsbk2e DE-REGISTRATION ACKNOWLEDGE";
  } else if (opcode == 0x2f) { // Unit DeRegistration Ack
    // unsigned long mfrid  = bitset_shift_mask(tsbk,80,0xff);
    // unsigned long opts  = bitset_shift_mask(tsbk,72,0xff);
    unsigned long si = bitset_shift_mask(tsbk, 16, 0xffffff);

    message.message_type = DEREGISTRATION;
    message.source = si;

    BOOST_LOG_TRIVIAL(debug) << "tsbk2f\tUnit Deregistration ACK\tSource ID: " << std::setw(7) << si;
  } else if (opcode == 0x30) {
    unsigned long mfrid = bitset_shift_mask(tsbk, 80, 0xff);
    if (mfrid == 0xA4) { // GRG_EXENC_CMD (M/A-COM patch)
      // unsigned long grg_t = bitset_shift_mask(tsbk, 79, 0x1);
      unsigned long grg_g = bitset_shift_mask(tsbk, 28, 0x1);
      unsigned long grg_a = bitset_shift_mask(tsbk, 77, 0x01);
      // unsigned long grg_ssn = bitset_shift_mask(tsbk, 72, 0x1f);  //TODO: SSN should be stored and checked
      unsigned long sg = bitset_shift_mask(tsbk, 56, 0xffff);
      // unsigned long keyid = bitset_shift_mask(tsbk, 40, 0xffff);
      unsigned long rta = bitset_shift_mask(tsbk, 16, 0xffffff);
      // unsigned long algid = (rta >> 16) & 0xff;
      unsigned long ga = rta & 0xffff;
      if (grg_a == 1) {   // Activate
        if (grg_g == 1) { // Group request
          message.message_type = PATCH_ADD;
          PatchData harris_patch_data;
          harris_patch_data.sg = sg;
          harris_patch_data.ga1 = ga;
          harris_patch_data.ga2 = ga;
          harris_patch_data.ga3 = ga;
          message.patch_data = harris_patch_data;
          BOOST_LOG_TRIVIAL(debug) << "tsbk30 M/A-COM GROUP REQUEST PATCH sg TGID is " << sg << " patched with TGID " << ga;
        } else {
          message.message_type = PATCH_ADD;
          PatchData harris_patch_data;
          harris_patch_data.sg = sg;
          harris_patch_data.ga1 = ga;
          harris_patch_data.ga2 = ga;
          harris_patch_data.ga3 = ga;
          message.patch_data = harris_patch_data;
          BOOST_LOG_TRIVIAL(debug) << "tsbk30 M/A-COM UNIT REQUEST PATCH sg TGID is " << sg << " patched with TGID " << ga;
        }
      } else {            // Deactivate
        if (grg_g == 1) { // Group request
          message.message_type = PATCH_DELETE;
          PatchData harris_patch_data;
          harris_patch_data.sg = sg;
          harris_patch_data.ga1 = ga;
          harris_patch_data.ga2 = ga;
          harris_patch_data.ga3 = ga;
          message.patch_data = harris_patch_data;
          BOOST_LOG_TRIVIAL(debug) << "tsbk30 M/A-COM GROUP REQUEST PATCH DELETE for sg " << sg << " with TGID " << ga;
        } else {
          message.message_type = PATCH_DELETE;
          PatchData harris_patch_data;
          harris_patch_data.sg = sg;
          harris_patch_data.ga1 = ga;
          harris_patch_data.ga2 = ga;
          harris_patch_data.ga3 = ga;
          message.patch_data = harris_patch_data;
          BOOST_LOG_TRIVIAL(debug) << "tsbk30 M/A-COM UNIT REQUEST PATCH DELETE for sg " << sg << " with TGID " << ga;
        }
      }
    } else {
      BOOST_LOG_TRIVIAL(trace) << "tsbk30 TDMA SYNCHRONIZATION BROADCAST";
    }
  } else if (opcode == 0x31) { //
    BOOST_LOG_TRIVIAL(debug) << "tsbk31 AUTHENTICATION DEMAND";
  } else if (opcode == 0x32) { //
    BOOST_LOG_TRIVIAL(debug) << "tsbk32 AUTHENTICATION RESPONSE";
  } else if (opcode == 0x33) { // iden_up_tdma
    unsigned long mfrid = bitset_shift_mask(tsbk, 80, 0xff);

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
      unsigned long f1 = bitset_shift_mask(tsbk, 16, 0xffffffff);
      int slots_per_carrier[] = {1, 1, 1, 2, 4, 2};
      bool chan_tdma;
      if (slots_per_carrier[channel_type] > 1) {
        chan_tdma = true;
      } else {
        chan_tdma = false;
      }
      Channel temp_chan = {
          iden,              // id;
          toff * spac * 125, // offset;
          spac * 125,        // step;
          f1 * 5,            // frequency;
          chan_tdma,
          slots_per_carrier[channel_type], // tdma;
          6.25};
      add_channel(iden, temp_chan, sys_num);
      BOOST_LOG_TRIVIAL(trace) << "tsbk33 iden up tdma id " << std::dec << iden << " f " << temp_chan.frequency << " offset " << temp_chan.offset << " spacing " << temp_chan.step << " slots/carrier " << temp_chan.slots_per_carrier;
    }
  } else if (opcode == 0x34) { // iden_up vhf uhf
    unsigned long iden = bitset_shift_mask(tsbk, 76, 0xf);
    unsigned long bwvu = bitset_shift_mask(tsbk, 72, 0xf);
    unsigned long toff0 = bitset_shift_mask(tsbk, 58, 0x3fff);
    unsigned long spac = bitset_shift_mask(tsbk, 48, 0x3ff);
    unsigned long freq = bitset_shift_mask(tsbk, 16, 0xffffffff);
    unsigned long toff_sign = (toff0 >> 13) & 1;
    double bandwidth = 0;

    if (bwvu == 4) {
      bandwidth = 6.25;
    } else if (bwvu == 5) {
      bandwidth = 12.5;
    }
    long toff = toff0 & 0x1fff;

    if (toff_sign == 0) {
      toff = 0 - toff;
    }
    std::string txt[] = {"mob Tx-", "mob Tx+"};
    Channel temp_chan = {
        iden,              // id;
        toff * spac * 125, // offset;
        spac * 125,        // step;
        freq * 5,          // frequency;
        false,             // tdma;
        0,                 // slots
        bandwidth};
    add_channel(iden, temp_chan, sys_num);

    BOOST_LOG_TRIVIAL(trace) << "tsbk34 iden vhf/uhf id " << std::dec << iden << " toff " << toff * spac * 0.125 * 1e-3 << " spac " << spac * 0.125 << " freq " << freq * 0.000005 << " [ " << txt[toff_sign] << "]";
  } else if (opcode == 0x35) { // Time and Date Announcement
    BOOST_LOG_TRIVIAL(debug) << "tsbk35 Time and Date Announcement";
  } else if (opcode == 0x36) { //
    BOOST_LOG_TRIVIAL(debug) << "tsbk36 ROAMING ADDRESS COMMAND";
  } else if (opcode == 0x37) { //
    BOOST_LOG_TRIVIAL(debug) << "tsbk37 ROAMING ADDRESS UPDATE";
  } else if (opcode == 0x38) { //
    BOOST_LOG_TRIVIAL(trace) << "tsbk38 SYSTEM SERVICE BROADCAST";
  } else if (opcode == 0x39) { // secondary cc
    unsigned long rfid = bitset_shift_mask(tsbk, 72, 0xff);
    unsigned long stid = bitset_shift_mask(tsbk, 64, 0xff);
    unsigned long ch1 = bitset_shift_mask(tsbk, 48, 0xffff);
    unsigned long ch2 = bitset_shift_mask(tsbk, 24, 0xffff);
    unsigned long f1 = channel_id_to_frequency(ch1, sys_num);
    unsigned long f2 = channel_id_to_frequency(ch2, sys_num);

    if (f1 && f2) {
      message.message_type = CONTROL_CHANNEL;
      message.freq = f1;
      message.talkgroup = 0;
      message.phase2_tdma = false;
      message.tdma_slot = 0;
      messages.push_back(message);
      message.freq = f2;

      // message.sys_id = syid;
    }
    os << "tsbk39 secondary cc: rfid " << std::dec << rfid << " stid " << stid << " ch1 " << ch1 << "(" << channel_id_to_string(ch1, sys_num) << ") ch2 " << ch2 << "(" << channel_id_to_string(ch2, sys_num) << ") ";
    message.meta = os.str();
    BOOST_LOG_TRIVIAL(trace) << os.str();
  } else if (opcode == 0x3a) { // rfss status
    unsigned long syid = bitset_shift_mask(tsbk, 56, 0xfff);
    unsigned long rfid = bitset_shift_mask(tsbk, 48, 0xff);
    unsigned long stid = bitset_shift_mask(tsbk, 40, 0xff);
    unsigned long chan = bitset_shift_mask(tsbk, 24, 0xffff);
    message.message_type = SYSID;
    message.sys_id = syid;
    message.sys_rfss = rfid;
    message.sys_site_id = stid;
    os << "tsbk3a rfss status: syid: " << syid << " rfid " << rfid << " stid " << stid << " ch1 " << chan << "(" << channel_id_to_string(chan, sys_num) << ")";
    message.meta = os.str();
    BOOST_LOG_TRIVIAL(trace) << os.str();
  } else if (opcode == 0x3b) { // network status
    unsigned long wacn = bitset_shift_mask(tsbk, 52, 0xfffff);
    unsigned long syid = bitset_shift_mask(tsbk, 40, 0xfff);
    unsigned long ch1 = bitset_shift_mask(tsbk, 24, 0xffff);
    unsigned long f1 = channel_id_to_frequency(ch1, sys_num);

    if (f1) {
      message.message_type = STATUS;
      message.wacn = wacn;
      message.sys_id = syid;
      message.freq = f1;
    }
    BOOST_LOG_TRIVIAL(trace) << "tsbk3b net stat: wacn " << std::dec << wacn << " syid " << syid << " ch1 " << ch1 << "(" << channel_id_to_string(ch1, sys_num) << ") ";
  } else if (opcode == 0x3c) { // adjacent status
    unsigned long rfid = bitset_shift_mask(tsbk, 48, 0xff);
    unsigned long stid = bitset_shift_mask(tsbk, 40, 0xff);
    unsigned long ch1 = bitset_shift_mask(tsbk, 24, 0xffff);
    unsigned long f1 = channel_id_to_frequency(ch1, sys_num);
    BOOST_LOG_TRIVIAL(trace) << "tsbk3c\tAdjacent Status\t rfid " << std::dec << rfid << " stid " << stid << " ch1 " << ch1 << "(" << channel_id_to_string(ch1, sys_num) << ") ";

    if (f1) {
      it = channels[stid].find((ch1 >> 12) & 0xf);

      if (it != channels[stid].end()) {
        Channel temp_chan = it->second;

        //			self.adjacent[f1] = 'rfid: %d stid:%d uplink:%f
        // tbl:%d' % (rfid, stid, (f1 + self.freq_table[table]['offset']) /
        // 1000000.0, table)
        BOOST_LOG_TRIVIAL(trace) << "\ttsbk3c Chan " << temp_chan.frequency << "  " << temp_chan.step;
      }
    }
  } else if (opcode == 0x3d) { // iden_up
    unsigned long iden = bitset_shift_mask(tsbk, 76, 0xf);
    unsigned long bw = bitset_shift_mask(tsbk, 67, 0x1ff);
    unsigned long toff0 = bitset_shift_mask(tsbk, 58, 0x1ff);
    unsigned long spac = bitset_shift_mask(tsbk, 48, 0x3ff);
    unsigned long freq = bitset_shift_mask(tsbk, 16, 0xffffffff);
    unsigned long toff_sign = (toff0 >> 8) & 1;
    long toff = toff0 & 0xff;

    if (toff_sign == 0) {
      toff = 0 - toff;
    }

    Channel temp_chan = {
        iden,          // id;
        toff * 250000, // offset;
        spac * 125,    // step;
        freq * 5,      // frequency;
        false,         // tdma;
        1,             // slots
        bw * .125};
    add_channel(iden, temp_chan, sys_num);
    BOOST_LOG_TRIVIAL(trace) << "tsbk3d iden id " << std::dec << iden << " toff " << toff * 0.25 << " spac " << spac * 0.125 << " freq " << freq * 0.000005;
  } else {
    BOOST_LOG_TRIVIAL(trace) << "tsbk other " << std::hex << opcode;
    return messages;
  }
  messages.push_back(message);
  return messages;
}

void P25Parser::print_bitset(boost::dynamic_bitset<> &tsbk) {
  /*boost::dynamic_bitset<> bitmask(tsbk.size(), 0x3f);
     unsigned long result = (tsbk & bitmask).to_ulong();
     BOOST_LOG_TRIVIAL(debug) << tsbk << " = " << std::hex << result;*/
}

void printbincharpad(char c) {
  for (int i = 7; i >= 0; --i) {
    std::cout << ((c & (1 << i)) ? '1' : '0');
  }

  // std::cout << " | ";
}

std::vector<TrunkMessage> P25Parser::parse_message(gr::message::sptr msg, System *system) {
  std::vector<TrunkMessage> messages;

  long type = msg->type();
  int sys_num = system->get_sys_num();
  TrunkMessage message;
  message.message_type = UNKNOWN;
  message.opcode = 255;
  message.source = -1;
  message.sys_num = sys_num;
  if (type == -2) { // # request from gui
    std::string cmd = msg->to_string();

    BOOST_LOG_TRIVIAL(debug) << "process_qmsg: command: " << cmd;

    // self.update_state(cmd, curr_time)
    messages.push_back(message);
    return messages;
  } else if (type == -1) { //	# timeout

    // self.update_state('timeout', curr_time)
    messages.push_back(message);
    return messages;
  } else if (type < 0) {
    BOOST_LOG_TRIVIAL(debug) << "unknown message type " << type;
    messages.push_back(message);
    return messages;
  }

  std::string s = msg->to_string();

  // # nac is always 1st two bytes
  // ac = (ord(s[0]) << 8) + ord(s[1])
  uint8_t s0 = (int)s[0];
  uint8_t s1 = (int)s[1];
  int shift = s0 << 8;
  long nac = shift + s1;

  if (s.length() < 2) {
    BOOST_LOG_TRIVIAL(error) << "P25 Parse error, s: " << s << " s0: " << static_cast<int>(s0) << " s1: " << static_cast<int>(s1) << " shift: " << shift << " nac: " << nac << " type: " << type << " Len: " << s.length();
    messages.push_back(message);
    return messages;
  }

  if (nac == 0xffff) {
    // # TDMA
    // self.update_state('tdma_duid%d' % type, curr_time)
    messages.push_back(message);
    return messages;
  }
  s = s.substr(2);

  BOOST_LOG_TRIVIAL(trace) << std::hex << "nac " << nac << std::dec << " type " << type << " size " << msg->to_string().length() << " mesg len: " << msg->length();
  // //" at %f state %d len %d" %(nac, type, time.time(), self.state, len(s))
  if ((type != 7) && (type != 12)) // and nac not in self.trunked_systems:
  {
    BOOST_LOG_TRIVIAL(debug) << std::hex << "NON TBSK: nac " << nac << std::dec << " type " << type << " size " << msg->to_string().length() << " mesg len: " << msg->length();
  
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
    b <<= 16; // for missing crc

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
    mbt_data <<= 32; // for missing crc
    unsigned long opcode = bitset_shift_mask(header, 32, 0x3f);
    unsigned long link_id = bitset_shift_mask(header, 48, 0xffffff);
    /*BOOST_LOG_TRIVIAL(debug) << "RAW  Data    " <<b;
    BOOST_LOG_TRIVIAL(debug) << "RAW  Data Length " <<s.length();*/
    BOOST_LOG_TRIVIAL(debug) << "MBT:  opcode: $" << std::hex << opcode;
    /* BOOST_LOG_TRIVIAL(debug) << "MBT  type :$" << std::hex << type << " len $" << std::hex << s1.length() << "/" << s2.length();
    BOOST_LOG_TRIVIAL(debug) <<  "MBT Header: " <<  header;
    BOOST_LOG_TRIVIAL(debug) <<  "MBT  Data   " <<  mbt_data; */
    return decode_mbt_data(opcode, header, mbt_data, link_id, nac, sys_num);
    // self.trunked_systems[nac].decode_mbt_data(opcode, header << 16, mbt_data
    // << 32)
  }
  messages.push_back(message);
  return messages;
}

