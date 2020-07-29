#include "smartnet_parser.h"
#include "../formatter.h"

using namespace std;
SmartnetParser::SmartnetParser() {
  lastaddress = 0;
  lastcmd     = 0;
  numStacked  = 0;
  numConsumed = 0;
}

double SmartnetParser::getfreq(int cmd, System *sys) {
  double freq = 0.0;
  std::string band = sys->get_bandplan();
  if (sys->get_bandfreq() == 800) {
    /*
      BANDPLAN 800Mhz:
      800_standard * Is default base plan
      800_splinter
      800_reband
    */
    if (cmd < 0 || cmd > 0x3FE)
      return freq;
    if (cmd <= 0x2CF) {
      if (band == "800_reband" && cmd >= 0x1B8 &&
          cmd <= 0x22F) { /* Re Banded Site */
        freq = 851.0250 + (0.025 * ((double)(cmd - 0x1B8)));
      } else if (band == "800_splinter" && cmd <= 0x257) { /* Splinter Site */
        freq = 851.0 + (0.025 * ((double)cmd));
      } else {
        freq = 851.0125 + (0.025 * ((double)cmd));
      }
    } else if (cmd <= 0x2f7) {
      freq = 866.0000 + (0.025 * ((double)(cmd - 0x2D0)));
    } else if (cmd >= 0x32F && cmd <= 0x33F) {
      freq = 867.0000 + (0.025 * ((double)(cmd - 0x32F)));
    } else if (cmd == 0x3BE) {
      freq = 868.9750;
    } else if (cmd >= 0x3C1 && cmd <= 0x3FE) {
      freq = 867.4250 + (0.025 * ((double)(cmd - 0x3C1)));
    }
  } else if (sys->get_bandfreq() == 400) {
    /*double test_freq;
    //Step * (channel - low) + Base
    if ((cmd >= 0x17c) && (cmd < 0x2b0)) {
      test_freq = ((cmd - 380) * 25000)  + 489087500;
    } else {
      test_freq = 0;
    }*/

    double high_cmd = sys->get_bandplan_offset() +
                      (sys->get_bandplan_high() - sys->get_bandplan_base()) /
                          sys->get_bandplan_spacing();

    if ((cmd >= sys->get_bandplan_offset()) &&
        (cmd < high_cmd)) { // (cmd <= sys->get_bandplan_base() +
                            // sys->get_bandplan_offset() )) {
      freq = sys->get_bandplan_base() +
             (sys->get_bandplan_spacing() * (cmd - sys->get_bandplan_offset()));
    }
    // cout << "Orig: " <<fixed <<test_freq << " Freq: " << freq << endl;
  }
  return freq * 1000000;
}

bool SmartnetParser::is_chan_outbound(int cmd, System *sys) {
  if (sys->get_bandfreq() == 800) {
    if ((cmd >= OSW_CHAN_BAND_1_MIN && cmd <= OSW_CHAN_BAND_1_MAX) ||
        (cmd >= OSW_CHAN_BAND_2_MIN && cmd <= OSW_CHAN_BAND_2_MAX) ||
        (cmd == OSW_CHAN_BAND_3) ||
        (cmd >= OSW_CHAN_BAND_4_MIN && cmd <= OSW_CHAN_BAND_4_MAX)) {
      return true;
    }
  } else if (sys->get_bandfreq() == 400) {
    //
    if (cmd >= sys->get_bandplan_offset() &&
        cmd <  sys->get_bandplan_offset() + 380) {
      return true;
    } else {
      return false;
    }
  }
  return false;
}

bool SmartnetParser::is_chan_inbound_obt(int cmd, System *sys) {
  return cmd < sys->get_bandplan_offset();
}

bool SmartnetParser::is_first_normal(int cmd, SYstem *sys) {
  if (sys->get_bandfreq == 800) {
    // anything "800" should be replaced with 8/9 compatible switching
    return ((cmd == OSW_FIRST_NORMAL) || \
            (cmd == OSW_FIRST_ASTRO));
  } else {
    // if we're looking at an OBT trunk, inbound channel commands are first normals too
    // anything "400" should be replaced as "OBT" in the future =/
    return (is_chan_obt_inbound(cmd, sys) || \
            (cmd == OSW_FIRST_NORMAL) || \
            (cmd == OSW_FIRST_ASTRO));
  }
}


std::vector<TrunkMessage> SmartnetParser::parse_message(std::string s,
                                                        System *system) {
  std::vector<TrunkMessage> messages;
  TrunkMessage message;

  // char tempArea[512];
  // unsigned short blockNum;
  // char banktype;
  // unsigned short tt1, tt2;
  // static unsigned int ott1, ott2;

  // print_osw(s);
  message.message_type = UNKNOWN;
  message.encrypted = false;
  message.phase2_tdma = false;
  message.tdma_slot = 0;
  message.source = 0;
  message.sys_id = 0;
  message.sys_num = system->get_sys_num();
  message.emergency = false;

  std::vector<std::string> x;
  boost::split(x, s, boost::is_any_of(","), boost::token_compress_on);

  if (x.size() < 3) {
    BOOST_LOG_TRIVIAL(error)
        << "SmartNet Parser recieved invalid message." << x.size();
    return messages;
  }

  int full_address = atoi(x[0].c_str());
  int status = full_address & 0x000F;
  long address = full_address & 0xFFF0;
  int groupflag = atoi(x[1].c_str());
  int command = atoi(x[2].c_str());

  struct osw_stru bosw;
  bosw.id = address;
  bosw.full_address = full_address;
  bosw.address = address;
  bosw.status = status;
  bosw.grp = groupflag;
  bosw.cmd = command;

  // struct osw_stru* Inposw = &bosw;
  cout.precision(0);

  // maintain a sliding stack of 5 OSWs. If previous iteration used more than
  // one,
  // don't utilize stack until all used ones have slid past.

  switch (numStacked) // note: drop-thru is intentional!
  {
  case 5:
  case 4:
    stack[4] = stack[3];

  case 3:
    stack[3] = stack[2];

  case 2:
    stack[2] = stack[1];

  case 1:
    stack[1] = stack[0];

  case 0:
    stack[0] = bosw;
    break;

  default:
    BOOST_LOG_TRIVIAL(info) << "corrupt value for nstacked" << endl;
    break;
  }

  if (numStacked < 5) {
    ++numStacked;
  }

  /*
     if (numConsumed > 0)
     {
      if (--numConsumed > 0)
      {
        return messages;
      }
     }

     if (numStacked < 3)
     {
      return messages; // at least need a window of 3 and 5 is better.
     }
   */
  x.clear();
  vector<string>().swap(x);



  // If we get here, we don't know about this OCW (and/or a combination of it with others beside it).
  // Error accordingly and log the buffer.
  // If we just got started, then we also could just be looking at the tail of a known message.
  // Adding the logic to test for this and stay quiet might be desirable, but for the time being,
  // we are willing to accept an error (or up to 3) on a fresh start or CC retune.
  BOOST_LOG_TRIVIAL(warning)
      << "[" << system->get_short_name()
      << "] [Unknown OSW!] [ "
      << std::hex << stack[0].cmd << " " << std::hex << stack[0].grp << " " << std::hex << stack[0].full_address << "  |  "
      << std::hex << stack[1].cmd << " " << std::hex << stack[1].grp << " " << std::hex << stack[1].full_address << "  |  "
      << std::hex << stack[2].cmd << " " << std::hex << stack[2].grp << " " << std::hex << stack[2].full_address << "  | >"
      << std::hex << stack[3].cmd << " " << std::hex << stack[3].grp << " " << std::hex << stack[3].full_address << "< |  "
      << std::hex << stack[4].cmd << " " << std::hex << stack[4].grp << " " << std::hex << stack[4].full_address << " ]";
  message.message_type = UNKNOWN;
  messages.push_back(message);
  return messages;
}
