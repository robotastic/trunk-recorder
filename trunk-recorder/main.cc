#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/foreach.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/math/constants/constants.hpp>
#include <boost/log/attributes/named_scope.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/tokenizer.hpp>

#include <cassert>
#include <cmath>
#include <exception>
#include <fstream>
#include <iostream>
#include <signal.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <utility>
#include <chrono>

#include "./global_structs.h"
#include "config.h"
#include "recorder_globals.h"
#include "source.h"

#include "recorders/analog_recorder.h"
#include "recorders/p25_recorder.h"
#include "recorders/recorder.h"

#include "call.h"
#include "call_conventional.h"
#include "systems/p25_parser.h"
#include "systems/p25_trunking.h"
#include "systems/parser.h"
#include "systems/smartnet_parser.h"
#include "systems/smartnet_trunking.h"
#include "systems/system.h"
#include "systems/system_impl.h"

#include <osmosdr/source.h>

#include "formatter.h"
#include <gnuradio/blocks/file_sink.h>
#include <gnuradio/gr_complex.h>
#include <gnuradio/message.h>
#include <gnuradio/top_block.h>
#include <gnuradio/uhd/usrp_source.h>

#include "plugin_manager/plugin_manager.h"

#include "cmake.h"
#include "git.h"

using namespace std;
namespace logging = boost::log;
namespace keywords = boost::log::keywords;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;

std::vector<Source *> sources;
std::vector<System *> systems;
std::map<long, long> unit_affiliations;

std::vector<Call *> calls;

gr::top_block_sptr tb;

volatile sig_atomic_t exit_flag = 0;
int exit_code = EXIT_SUCCESS;
SmartnetParser *smartnet_parser;
P25Parser *p25_parser;
Config config;


void exit_interupt(int sig) { // can be called asynchronously
  exit_flag = 1;              // set flag
}

uint64_t time_since_epoch_millisec() {
  using namespace std::chrono;
  return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}


unsigned GCD(unsigned u, unsigned v) {
  while (v != 0) {
    unsigned r = u % v;
    u = v;
    v = r;
  }
  return u;
}

int get_total_recorders() {
  int total_recorders = 0;

  for (vector<Call *>::iterator it = calls.begin(); it != calls.end(); it++) {
    Call *call = *it;

    if (call->get_state() == RECORDING) {
      total_recorders++;
    }
  }
  return total_recorders;
}

// This maybe needed by websocket_pp
bool replace(std::string &str, const std::string &from, const std::string &to) {
  size_t start_pos = str.find(from);

  if (start_pos == std::string::npos)
    return false;

  str.replace(start_pos, from.length(), to);
  return true;
}

void process_signal(long unitId, const char *signaling_type, gr::blocks::SignalType sig_type, Call *call, System *system, Recorder *recorder) {
  plugman_signal(unitId, signaling_type, sig_type, call, system, recorder);
}

bool start_recorder(Call *call, TrunkMessage message, System *sys) {
  Talkgroup *talkgroup = sys->find_talkgroup(call->get_talkgroup());

  bool source_found = false;
  bool recorder_found = false;
  Recorder *recorder;
  Recorder *debug_recorder;
  Recorder *sigmf_recorder;

  if (!talkgroup && (sys->get_record_unknown() == false)) {
    call->set_state(MONITORING);
    call->set_monitoring_state(UNKNOWN_TG);
    if (sys->get_hideUnknown() == false) {
      BOOST_LOG_TRIVIAL(info) << "[" << sys->get_short_name() << "]\t\033[0;34m" << call->get_call_num() << "C\033[0m\tTG: " << call->get_talkgroup_display() << "\tFreq: " << format_freq(call->get_freq()) << "\t\u001b[33mNot Recording: TG not in Talkgroup File\u001b[0m ";
    }
    return false;
  }

  if (talkgroup) {
    call->set_talkgroup_tag(talkgroup->alpha_tag);
  } else {
    call->set_talkgroup_tag("-");
  }

  if (call->get_encrypted() == true || (talkgroup && (talkgroup->mode.compare("E") == 0 || talkgroup->mode.compare("TE") == 0 || talkgroup->mode.compare("DE") == 0))) {
    call->set_state(MONITORING);
    call->set_monitoring_state(ENCRYPTED);
    if (sys->get_hideEncrypted() == false) {
      long unit_id = call->get_current_source_id();
      std::string tag = sys->find_unit_tag(unit_id);
      if (tag != "") {
        tag = " (\033[0;34m" + tag + "\033[0m)";
      }
      BOOST_LOG_TRIVIAL(info) << "[" << sys->get_short_name() << "]\t\033[0;34m" << call->get_call_num() << "C\033[0m\tTG: " << call->get_talkgroup_display() << "\tFreq: " << format_freq(call->get_freq()) << "\t\u001b[31mNot Recording: ENCRYPTED\u001b[0m - src: " << unit_id << tag;
    }
    return false;
  }

  for (vector<Source *>::iterator it = sources.begin(); it != sources.end(); it++) {
    Source *source = *it;

    if ((source->get_min_hz() <= call->get_freq()) &&
        (source->get_max_hz() >= call->get_freq())) {
      source_found = true;

      if (talkgroup) {
        int priority = talkgroup->get_priority();
        BOOST_FOREACH (auto &TGID, sys->get_talkgroup_patch(call->get_talkgroup())) {
          if (sys->find_talkgroup(TGID) != NULL) {
            if (sys->find_talkgroup(TGID)->get_priority() < priority) {
              priority = sys->find_talkgroup(TGID)->get_priority();
              BOOST_LOG_TRIVIAL(info) << "Temporarily increased priority of talkgroup " << call->get_talkgroup() << " to " << sys->find_talkgroup(TGID)->get_priority() << " due to active patch with talkgroup " << TGID;
            }
          }
        }
        if (talkgroup->mode.compare("A") == 0) {
          recorder = source->get_analog_recorder(talkgroup, priority, call);
          call->set_is_analog(true);
        } else {
          recorder = source->get_digital_recorder(talkgroup, priority, call);
        }
      } else {
        BOOST_LOG_TRIVIAL(info) << "[" << sys->get_short_name() << "]\t\033[0;34m" << call->get_call_num() << "C\033[0m\tTG: " << call->get_talkgroup_display() << "\tFreq: " << format_freq(call->get_freq()) << "\tTG not in Talkgroup File ";

        // A talkgroup was not found from the talkgroup file.
        // Use an analog recorder if this is a Type II trunk and defaultMode is analog.
        // All other cases use a digital recorder.
        if ((config.default_mode == "analog") && (sys->get_system_type() == "smartnet")) {
          recorder = source->get_analog_recorder(call);
          call->set_is_analog(true);
        } else {
          recorder = source->get_digital_recorder(call);
        }
      }
      // int total_recorders = get_total_recorders();

      if (recorder) {
        if (message.meta.length()) {
          BOOST_LOG_TRIVIAL(trace) << message.meta;
        }

        if (recorder->start(call)) {
          call->set_recorder(recorder);
          call->set_state(RECORDING);
          plugman_setup_recorder(recorder);
          recorder_found = true;
        } else {
          call->set_state(MONITORING);
          // call->set_monitoring_state(NO_SOURCE);
          recorder_found = false;
          return false;
        }
      } else {
        // not recording call either because the priority was too low or no
        // recorders were available
        return false;
      }

      debug_recorder = source->get_debug_recorder();

      if (debug_recorder) {
        debug_recorder->start(call);
        call->set_debug_recorder(debug_recorder);
        call->set_debug_recording(true);
        plugman_setup_recorder(debug_recorder);
        recorder_found = true;
      } else {
        // BOOST_LOG_TRIVIAL(info) << "\tNot debug recording call";
      }

      sigmf_recorder = source->get_sigmf_recorder();

      if (sigmf_recorder) {
        sigmf_recorder->start(call);
        call->set_sigmf_recorder(sigmf_recorder);
        call->set_sigmf_recording(true);
        plugman_setup_recorder(sigmf_recorder);
        recorder_found = true;
      } else {
        // BOOST_LOG_TRIVIAL(info) << "\tNot SIGMF recording call";
      }

      if (recorder_found) {
        // recording successfully started.
        return true;
      }
    }
  }

  if (!source_found) {
    call->set_state(MONITORING);
    call->set_monitoring_state(NO_SOURCE);
    BOOST_LOG_TRIVIAL(error) << "[" << sys->get_short_name() << "]\t\033[0;34m" << call->get_call_num() << "C\033[0m\tTG: " << call->get_talkgroup_display() << "\tFreq: " << format_freq(call->get_freq()) << "\t\u001b[36mNot Recording: no source covering Freq\u001b[0m";
    return false;
  }
  return false;
}

// This is to handle the messages that come off the Analog recorder.
void process_message_queues() {
  for (std::vector<System *>::iterator it = systems.begin(); it != systems.end(); ++it) {
    System_impl *sys = (System_impl *)*it;

    for (std::vector<analog_recorder_sptr>::iterator arit = sys->conventional_recorders.begin(); arit != sys->conventional_recorders.end(); ++arit) {
      analog_recorder_sptr ar = (analog_recorder_sptr)*arit;
      ar->process_message_queues();
    }
  }
}

void manage_conventional_call(Call *call) {

  if (call->get_recorder()) {
    // if any recording has happened

    if (call->get_current_length() > 0) {
      BOOST_LOG_TRIVIAL(trace) << "[" << call->get_short_name() << "]\t\033[0;34m" << call->get_call_num() << "C\033[0m Call Length: " << call->get_current_length() << "s\t Idle: " << call->get_recorder()->is_idle() << "\t Idle Count: " << call->get_idle_count();

      // means that the squelch is on and it has stopped recording
      if (call->get_recorder()->is_squelched()) {
        // increase the number of periods it has not been recording for
        call->increase_idle_count();
      } else if (call->get_idle_count() > 0) {
        // if it starts recording again, then reset the idle count
        call->reset_idle_count();
      }

      // if no additional recording has happened in the past X periods, stop and open new file
      if (call->get_idle_count() > config.call_timeout) {
        Recorder *recorder = call->get_recorder();
        call->conclude_call();
        call->restart_call();
        if (recorder != NULL) {
          plugman_setup_recorder(recorder);
          plugman_call_start(call);
        }
      } else if ((call->get_current_length() > call->get_system()->get_max_duration()) && (call->get_system()->get_max_duration() > 0)) {
        Recorder *recorder = call->get_recorder();
        call->conclude_call();
        call->restart_call();
        if (recorder != NULL) {
          plugman_setup_recorder(recorder);
          plugman_call_start(call);
        }
      }
    } else if (!call->get_recorder()->is_active()) {
      // P25 Conventional and DMR Recorders need a have the graph unlocked before they can start recording.
      Recorder *recorder = call->get_recorder();
      recorder->start(call);
      call->set_state(RECORDING);
      plugman_call_start(call);
      BOOST_LOG_TRIVIAL(trace) << "[" << call->get_short_name() << "]\t\033[0;34m" << call->get_call_num() << "C\033[0m Starting P25 Convetional Recorder ";

      // plugman_setup_recorder((Recorder *)recorder->get());
    }
  }
}

void print_status() {
  BOOST_LOG_TRIVIAL(info) << "Currently Active Calls: " << calls.size();

  for (vector<Call *>::iterator it = calls.begin(); it != calls.end(); it++) {
    Call *call = *it;
    Recorder *recorder = call->get_recorder();

    if (call->get_state() == MONITORING) {
      BOOST_LOG_TRIVIAL(info) << "[" << call->get_short_name() << "]\t\033[0;34m" << call->get_call_num() << "C\033[0m TG: " << call->get_talkgroup_display() << " Freq: " << format_freq(call->get_freq()) << " Elapsed: " << std::setw(4) << call->elapsed() << " State: " << format_state(call->get_state(), call->get_monitoring_state());
    } else {
      BOOST_LOG_TRIVIAL(info) << "[" << call->get_short_name() << "]\t\033[0;34m" << call->get_call_num() << "C\033[0m TG: " << call->get_talkgroup_display() << " Freq: " << format_freq(call->get_freq()) << " Elapsed: " << std::setw(4) << call->elapsed() << " State: " << format_state(call->get_state());
    }

    if (recorder) {
      if (recorder->is_conventional()) {
        if (recorder->get_rssi() < 0) {
        BOOST_LOG_TRIVIAL(info) << "\t[ " << recorder->get_num() << " ] State: " << format_state(recorder->get_state()) << "\tRSSI: " << recorder->get_rssi() << " dBm";
        } else {
          BOOST_LOG_TRIVIAL(info) << "\t[ " << recorder->get_num() << " ] State: " << format_state(recorder->get_state()) << "\tNot Detected";

        }
      } else {
        BOOST_LOG_TRIVIAL(info) << "\t[ " << recorder->get_num() << " ] State: " << format_state(recorder->get_state());
      }
    }
  }

  BOOST_LOG_TRIVIAL(info) << "Recorders: ";

  for (vector<Source *>::iterator it = sources.begin(); it != sources.end(); it++) {
    Source *source = *it;
    source->print_recorders();
  }

  BOOST_LOG_TRIVIAL(info) << "Control Channel Decode Rates: ";
  for (std::vector<System *>::iterator it = systems.begin(); it != systems.end(); ++it) {
    System_impl *sys = (System_impl *)*it;

    if ((sys->get_system_type() != "conventional") && (sys->get_system_type() != "conventionalP25") && (sys->get_system_type() != "conventionalDMR") && (sys->get_system_type() != "conventionalSIGMF")) {
      BOOST_LOG_TRIVIAL(info) << "[" << sys->get_short_name() << "] " << sys->get_decode_rate() << " msg/sec";
    }
  }
}

void manage_calls() {
  bool ended_call = false;
  for (vector<Call *>::iterator it = calls.begin(); it != calls.end();) {
    Call *call = *it;
    State state = call->get_state();
    // Handle Conventional Calls
    if (call->is_conventional()) {
      manage_conventional_call(call);
      ++it;
      continue;
    }

    // Handle Trunked Calls

    if ((state == MONITORING) && (call->since_last_update() > config.call_timeout)) {
      ended_call = true;
      it = calls.erase(it);
      delete call;
      continue;
    }

    if (state == RECORDING) {
      Recorder *recorder = call->get_recorder();

      // Stop the call if:
      // - there hasn't been an UPDATE for it on the Control Channel in X seconds AND the recorder hasn't written anything in X seconds

      if ((recorder->since_last_write() > config.call_timeout) && (call->since_last_update() > config.call_timeout)) {
        BOOST_LOG_TRIVIAL(trace) << "[" << call->get_short_name() << "]\t\033[0;34m" << call->get_call_num() << "C\033[0m\tTG: " << call->get_talkgroup_display() << "\tFreq: " << format_freq(call->get_freq()) << "\t\u001b[36m Stopping Call because of Recorder \u001b[0m Rec last write: " << recorder->since_last_write() << " State: " << format_state(recorder->get_state());
        call->conclude_call();
        // The State of the Recorders has changed, so lets send an update
        ended_call = true;
        if (recorder != NULL) {
          plugman_setup_recorder(recorder);
        }
        it = calls.erase(it);
        delete call;
        continue;
      }
    } else if (call->since_last_update() > config.call_timeout) {
      Recorder *recorder = call->get_recorder();

      BOOST_LOG_TRIVIAL(trace) << "[" << call->get_short_name() << "]\t\033[0;34m" << call->get_call_num() << "C\033[0m\tTG: " << call->get_talkgroup_display() << "\tFreq: " << format_freq(call->get_freq()) << "\t\u001b[36m  Call UPDATEs has been inactive for more than " << config.call_timeout << " Sec \u001b[0m Rec last write: " << recorder->since_last_write() << " State: " << format_state(recorder->get_state());
    }
    ++it;
  } // foreach loggers

  if (ended_call) {
    plugman_calls_active(calls);
  }
}

void current_system_status(TrunkMessage message, System *sys) {
  if (sys->update_status(message)) {
    plugman_setup_system(sys);
  }
}

void current_system_sysid(TrunkMessage message, System *sys) {
  if ((sys->get_system_type() == "p25") || (sys->get_system_type() == "conventionalP25")) {
    if (sys->update_sysid(message)) {
      plugman_setup_system(sys);
    }
  }
}

void unit_registration(System *sys, long source_id) {
  plugman_unit_registration(sys, source_id);
}

void unit_deregistration(System *sys, long source_id) {
  plugman_unit_deregistration(sys, source_id);
}

void unit_acknowledge_response(System *sys, long source_id) {
  plugman_unit_acknowledge_response(sys, source_id);
}

void unit_group_affiliation(System *sys, long source_id, long talkgroup_num) {
  plugman_unit_group_affiliation(sys, source_id, talkgroup_num);
}

void unit_data_grant(System *sys, long source_id) {
  plugman_unit_data_grant(sys, source_id);
}

void unit_answer_request(System *sys, long source_id, long talkgroup) {
  plugman_unit_answer_request(sys, source_id, talkgroup);
}

void unit_location(System *sys, long source_id, long talkgroup_num) {
  plugman_unit_location(sys, source_id, talkgroup_num);
}

void handle_call_grant(TrunkMessage message, System *sys, bool grant_message) {
  bool call_found = false;
  bool duplicate_grant = false;
  bool superseding_grant = false;
  bool recording_started [[maybe_unused]] = false;

  Call *original_call;

  /* Notes: it is possible for 2 Calls to exist for the same talkgroup on different freq. This happens when a Talkgroup starts on a freq
  that current recorder can't retune to. In this case, the current orig Talkgroup reocrder will keep going on the old freq, while a new
  recorder is start on a source that can cover that freq. This makes sure any of the remaining transmission that it is in the buffer
  of the original recorder gets flushed.
  UPDATED: however if we have 2 different talkgroups on the same freq we should do a stop_call on the original call since it is being used by another TG now. This will let the recorder keep
  going until it gets a termination flag.
  */

  // BOOST_LOG_TRIVIAL(info) << "TG: " << message.talkgroup << " sys num: " << message.sys_num << " freq: " << message.freq << " TDMA Slot" << message.tdma_slot << " TDMA: " << message.phase2_tdma;

  unsigned long message_preferredNAC = 0;
  unsigned long call_rfss_site = 0;
  unsigned long sys_rfss_site = 0;

  Talkgroup *message_talkgroup = sys->find_talkgroup(message.talkgroup);
  if (message_talkgroup) {
    message_preferredNAC = message_talkgroup->get_preferredNAC();
  }

  for (vector<Call *>::iterator it = calls.begin(); it != calls.end();) {
    Call *call = *it;

    /* This is for Multi-Site support */
    // Find candidate duplicate calls with the same talkgroup and different multisite-enabled systems
    if (call->get_talkgroup() == message.talkgroup) {
      if (call->get_sys_num() != message.sys_num) {
        if (call->get_system()->get_multiSite() && sys->get_multiSite()) {
          if (call->get_system()->get_wacn() == sys->get_wacn()) {
            // Default mode to match WACN and use RFSS/Site to identify duplicate calls
            sys_rfss_site = sys->get_sys_rfss() * 10000 + sys->get_sys_site_id();
            call_rfss_site = call->get_system()->get_sys_rfss() * 10000 + call->get_system()->get_sys_site_id();
            if ((sys_rfss_site != call_rfss_site) && (call->get_system()->get_multiSiteSystemName() == "")) {
              if (call->get_state() == RECORDING) {

                duplicate_grant = true;
                original_call = call;

                unsigned long call_preferredNAC = 0;
                Talkgroup *call_talkgroup = call->get_system()->find_talkgroup(message.talkgroup);
                if (call_talkgroup) {
                  call_preferredNAC = call_talkgroup->get_preferredNAC();
                }

                // Evaluate superseding grants by comparing call NAC or RFSS-Site against preferred NAC/site in talkgroup .csv
                if ((call_preferredNAC != call->get_system()->get_nac()) && (message_preferredNAC == sys->get_nac())) {
                  superseding_grant = true;
                } else if ((call_preferredNAC != call_rfss_site) && (message_preferredNAC == sys_rfss_site)) {
                  superseding_grant = true;
                }
              }
            }

            // Secondary mode to match multiSiteSystemName and use multiSiteSystemNumber.
            // If a multiSiteSystemName has been manually entered;
            // We already know that Call's system number does not match the message system number.
            // In this case, we check that the multiSiteSystemName is present, and that the Call and System multiSiteSystemNames are the same.
            else if ((call->get_system()->get_multiSiteSystemName() != "") && (call->get_system()->get_multiSiteSystemName() == sys->get_multiSiteSystemName())) {
              if (call->get_state() == RECORDING) {

                duplicate_grant = true;
                original_call = call;

                unsigned long call_preferredNAC = 0;
                Talkgroup *call_talkgroup = call->get_system()->find_talkgroup(message.talkgroup);
                if (call_talkgroup) {
                  call_preferredNAC = call_talkgroup->get_preferredNAC();
                }

                if ((call->get_system()->get_multiSiteSystemNumber() != 0) && (sys->get_multiSiteSystemNumber() != 0)) {
                  if ((call_preferredNAC != call->get_system()->get_multiSiteSystemNumber()) && (message_preferredNAC == sys->get_multiSiteSystemNumber())) {
                    superseding_grant = true;
                  }
                }
              }
            }
          }
        }
      }
    }

    if ((call->get_talkgroup() == message.talkgroup) && (call->get_sys_num() == message.sys_num) && (call->get_freq() == message.freq) && (call->get_tdma_slot() == message.tdma_slot) && (call->get_phase2_tdma() == message.phase2_tdma)) {
      call_found = true;
      bool source_updated = call->update(message);
      if (source_updated) {
        plugman_call_start(call);
      }
    }

    // There is an existing call on freq and slot that the new call will be started on. We should stop the older call. The older recorder will
    // keep writing to the file until it hits a termination flag, so no packets should be dropped.
    if ((call->get_state() == RECORDING) && (call->get_talkgroup() != message.talkgroup) && (call->get_sys_num() == message.sys_num) && (call->get_freq() == message.freq) && (call->get_tdma_slot() == message.tdma_slot) && (call->get_phase2_tdma() == message.phase2_tdma)) {
      Recorder *recorder = call->get_recorder();
      string recorder_state = "UNKNOWN";
      if (recorder != NULL) {
        recorder_state = format_state(recorder->get_state());
      }
      BOOST_LOG_TRIVIAL(trace) << "[" << call->get_short_name() << "]\t\033[0;34m" << call->get_call_num() << "C\033[0m\tTG: " << call->get_talkgroup_display() << "\tFreq: " << format_freq(call->get_freq()) << "\t\u001b[36mShould be Stopping RECORDING call, Recorder State: " << recorder_state << " RX overlapping TG message Freq, TG:" << message.talkgroup << "\u001b[0m";
    }

    it++;
  }

  if (!call_found) {
    Call *call = Call::make(message, sys, config);

    Talkgroup *talkgroup = sys->find_talkgroup(call->get_talkgroup());

    if (talkgroup) {
      call->set_talkgroup_tag(talkgroup->alpha_tag);
    } else {
      call->set_talkgroup_tag("-");
    }

    boost::format original_call_data;
    boost::format grant_call_data;
    
    if ((superseding_grant) || (duplicate_grant)){
      if (original_call->get_system()->get_multiSiteSystemName() == "") {
        original_call_data = boost::format("\u001b[34m%sC\u001b[0m %X/%s-%s ") % original_call->get_call_num() % original_call->get_system()->get_wacn() % original_call->get_system()->get_sys_rfss() % original_call->get_system()->get_sys_site_id();
        grant_call_data = boost::format("\u001b[34m%sC\u001b[0m %X/%s-%s ") % call->get_call_num() % sys->get_wacn() % sys->get_sys_rfss() % + sys->get_sys_site_id();
      } else {
        original_call_data = boost::format("\u001b[34m%sC\u001b[0m %s/%s ") % original_call->get_call_num() % original_call->get_system()->get_multiSiteSystemName() % original_call->get_system()->get_multiSiteSystemNumber();
        grant_call_data = boost::format("\u001b[34m%sC\u001b[0m %s/%s ") % call->get_call_num() % sys->get_multiSiteSystemName() % sys->get_multiSiteSystemNumber();        
      }
    }

    if (superseding_grant) {
      BOOST_LOG_TRIVIAL(info) << "[" << call->get_short_name() << "]\t\033[0;34m" << call->get_call_num() << "C\033[0m\tTG: " << original_call->get_talkgroup_display() << "\tFreq: " << format_freq(call->get_freq()) << "\t\u001b[36mSuperseding Grant\u001b[0m - Stopping original call: " << original_call_data << "- Superseding call: " << grant_call_data;
      // Attempt to start a new call on the preferred NAC.
      recording_started = start_recorder(call, message, sys);

      if (recording_started) {
        // Clean up the original call.
        original_call->set_state(MONITORING);
        original_call->set_monitoring_state(SUPERSEDED);
        original_call->conclude_call();
      } else {
        BOOST_LOG_TRIVIAL(info) << "[" << call->get_short_name() << "]\t\033[0;34m" << call->get_call_num() << "C\033[0m\tTG: " << original_call->get_talkgroup_display() << "\tFreq: " << format_freq(call->get_freq()) << "\t\u001b[36mCould not start Superseding recorder.\u001b[0m Continuing original call: " << original_call->get_call_num() << "C";
      }
    } else if (duplicate_grant) {
      call->set_state(MONITORING);
      call->set_monitoring_state(DUPLICATE);
      BOOST_LOG_TRIVIAL(info) << "[" << call->get_short_name() << "]\t\033[0;34m" << call->get_call_num() << "C\033[0m\tTG: " << original_call->get_talkgroup_display() << "\tFreq: " << format_freq(call->get_freq()) << "\t\u001b[36mDuplicate Grant\u001b[0m - Not recording: " << grant_call_data << "- Original call: " << original_call_data;
    } else {
      recording_started = start_recorder(call, message, sys);
      if (recording_started && !grant_message) {
        BOOST_LOG_TRIVIAL(info) << "[" << call->get_short_name() << "]\t\033[0;34m" << call->get_call_num() << "C\033[0m\tTG: " << call->get_talkgroup_display() << "\tFreq: " << format_freq(call->get_freq()) << "\t\u001b[36mThis was an UPDATE\u001b[0m";
      }
    }
    calls.push_back(call);
    plugman_call_start(call);
    plugman_calls_active(calls);
  }
}

void handle_call_update(TrunkMessage message, System *sys) {
  bool call_found = false;

  /* Notes: it is possible for 2 Calls to exist for the same talkgroup on different freq. This happens when a Talkgroup starts on a freq
  that current recorder can't retune to. In this case, the current orig Talkgroup reocrder will keep going on the old freq, while a new
  recorder is start on a source that can cover that freq. This makes sure any of the remaining transmission that it is in the buffer
  of the original recorder gets flushed.
  UPDATED: however if we have 2 different talkgroups on the same freq we should do a stop_call on the original call since it is being used by another TG now. This will let the recorder keep
  going until it gets a termination flag.
  */

  for (vector<Call *>::iterator it = calls.begin(); it != calls.end(); ++it) {
    Call *call = *it;

    // BOOST_LOG_TRIVIAL(info) << "TG: " << call->get_talkgroup() << " | " << message.talkgroup << " sys num: " << call->get_sys_num() << " | " << message.sys_num << " freq: " << call->get_freq() << " | " << message.freq << " TDMA Slot" << call->get_tdma_slot() << " | " << message.tdma_slot << " TDMA: " << call->get_phase2_tdma() << " | " << message.phase2_tdma;
    if ((call->get_talkgroup() == message.talkgroup) && (call->get_sys_num() == message.sys_num) && (call->get_freq() == message.freq) && (call->get_tdma_slot() == message.tdma_slot) && (call->get_phase2_tdma() == message.phase2_tdma)) {
      call_found = true;

      bool source_updated = call->update(message);
      if (source_updated) {
        plugman_call_start(call);
      }
    }
  }

  if (!call_found) {
    // Note: some calls maybe removed before the UPDATEs stop on the trunking channel if there is some GAP in the updates.
    // BOOST_LOG_TRIVIAL(info) << "Call not found for UPDATE mesg - either we missed GRANT or removed Call too soon\tFreq: " << format_freq(message.freq) << "\tTG:" << message.talkgroup << "\tSource: " << message.source << "\tSys Num: " << message.sys_num << "\tTDMA Slot: " << message.tdma_slot << "\tTDMA: " << message.phase2_tdma;
  }
}

void handle_message(std::vector<TrunkMessage> messages, System *sys) {
  for (std::vector<TrunkMessage>::iterator it = messages.begin(); it != messages.end(); it++) {
    TrunkMessage message = *it;

    switch (message.message_type) {
    case GRANT:
      handle_call_grant(message, sys, true);
      break;

    case UPDATE:
      if (config.new_call_from_update) {
        // Treat UPDATE as a GRANT and start a new call if we don't have one for this TG
        handle_call_grant(message, sys, false);
      } else {
        // Treat UPDATE as an UPDATE and only update existing calls
        handle_call_update(message, sys);
      }
      break;

    case UU_V_GRANT:
      if (config.record_uu_v_calls) {
        handle_call_grant(message, sys, true);
      }
      break;

    case UU_V_UPDATE:
      if (config.record_uu_v_calls) {
        handle_call_update(message, sys);
      }
      break;

    case CONTROL_CHANNEL:
      sys->add_control_channel(message.freq);
      break;

    case REGISTRATION:
      unit_registration(sys, message.source);
      break;

    case DEREGISTRATION:
      unit_deregistration(sys, message.source);
      break;

    case AFFILIATION:
      unit_group_affiliation(sys, message.source, message.talkgroup);
      break;

    case SYSID:
      current_system_sysid(message, sys);
      break;

    case STATUS:
      current_system_status(message, sys);
      break;

    case LOCATION:
      unit_location(sys, message.source, message.talkgroup);
      break;

    case ACKNOWLEDGE:
      unit_acknowledge_response(sys, message.source);
      break;

    case PATCH_ADD:
      sys->update_active_talkgroup_patches(message.patch_data);
      break;
    case PATCH_DELETE:
      sys->delete_talkgroup_patch(message.patch_data);
      break;

    case DATA_GRANT:
      unit_data_grant(sys, message.source);
      break;

    case UU_ANS_REQ:
      unit_answer_request(sys, message.source, message.talkgroup);
      break;

    case UNKNOWN:
      break;
    }
  }
}

System *find_system(int sys_num) {
  System *sys_match = NULL;

  for (std::vector<System *>::iterator it = systems.begin(); it != systems.end(); ++it) {
    System *sys = (System *)*it;

    if (sys->get_sys_num() == sys_num) {
      sys_match = sys;
      break;
    }
  }

  if (sys_match == NULL) {
    BOOST_LOG_TRIVIAL(info) << "Sys is null";
  }
  return sys_match;
}

void retune_system(System *sys) {
  System_impl *system = (System_impl *)sys;
  bool source_found = false;
  Source *current_source = system->get_source();
  double control_channel_freq = system->get_next_control_channel();

  BOOST_LOG_TRIVIAL(error) << "[" << system->get_short_name() << "] Retuning to Control Channel: " << format_freq(control_channel_freq);

  if ((current_source->get_min_hz() <= control_channel_freq) &&
      (current_source->get_max_hz() >= control_channel_freq)) {
    source_found = true;
    BOOST_LOG_TRIVIAL(info) << "\t - System Source " << current_source->get_num() << " - Min Freq: " << format_freq(current_source->get_min_hz()) << " Max Freq: " << format_freq(current_source->get_max_hz());
    // The source can cover the System's control channel, break out of the
    // For Loop
    if (system->get_system_type() == "smartnet") {
      system->smartnet_trunking->tune_freq(control_channel_freq);
      system->smartnet_trunking->reset();
    } else if (system->get_system_type() == "p25") {
      system->p25_trunking->tune_freq(control_channel_freq);
    } else {
      BOOST_LOG_TRIVIAL(error) << "\t - Unknown system type for Retune";
    }
  } else {
    for (vector<Source *>::iterator src_it = sources.begin(); src_it != sources.end(); src_it++) {
      Source *source = *src_it;

      if ((source->get_min_hz() <= control_channel_freq) &&
          (source->get_max_hz() >= control_channel_freq)) {
        source_found = true;
        BOOST_LOG_TRIVIAL(info) << "\t - System Source " << source->get_num() << " - Min Freq: " << format_freq(source->get_min_hz()) << " Max Freq: " << format_freq(source->get_max_hz());

        if (system->get_system_type() == "smartnet") {
          system->set_source(source);
          // We must lock the flow graph in order to disconnect and reconnect blocks
          tb->lock();
          tb->disconnect(current_source->get_src_block(), 0, system->smartnet_trunking, 0);
          system->smartnet_trunking = make_smartnet_trunking(control_channel_freq, source->get_center(), source->get_rate(), system->get_msg_queue(), system->get_sys_num());
          tb->connect(source->get_src_block(), 0, system->smartnet_trunking, 0);
          tb->unlock();
          system->smartnet_trunking->reset();
        } else if (system->get_system_type() == "p25") {
          system->set_source(source);
          // We must lock the flow graph in order to disconnect and reconnect blocks
          tb->lock();
          tb->disconnect(current_source->get_src_block(), 0, system->p25_trunking, 0);
          system->p25_trunking = make_p25_trunking(control_channel_freq, source->get_center(), source->get_rate(), system->get_msg_queue(), system->get_qpsk_mod(), system->get_sys_num());
          tb->connect(source->get_src_block(), 0, system->p25_trunking, 0);
          tb->unlock();
        } else {
          BOOST_LOG_TRIVIAL(error) << "\t - Unkown system type for Retune";
        }

        // break out of the For Loop
        break;
      }
    }
  }
  if (!source_found) {
    BOOST_LOG_TRIVIAL(error) << "\t - Unable to retune System control channel, freq not covered by any source.";
  }
}

void check_message_count(float timeDiff) {
  plugman_setup_config(sources, systems);
  plugman_system_rates(systems, timeDiff);

  for (std::vector<System *>::iterator it = systems.begin(); it != systems.end(); ++it) {
    System_impl *sys = (System_impl *)*it;

    if ((sys->get_system_type() != "conventional") && (sys->get_system_type() != "conventionalP25") && (sys->get_system_type() != "conventionalDMR") && (sys->get_system_type() != "conventionalSIGMF")) {
      int msgs_decoded_per_second = std::floor(sys->message_count / timeDiff);
      sys->set_decode_rate(msgs_decoded_per_second);

      if (msgs_decoded_per_second < 2) {

        // if it loses track of the control channel, quit after a while
        if (config.control_retune_limit > 0) {
          sys->retune_attempts++;
          if (sys->retune_attempts > config.control_retune_limit) {
            BOOST_LOG_TRIVIAL(error) << "[" << sys->get_short_name() << "]\t"
                                     << "Control channel retune limit exceeded after " << sys->retune_attempts << " tries - Terminating trunk recorder";
            exit_flag = 1;
            exit_code = EXIT_FAILURE;
            return;
          }
        }
        if (sys->control_channel_count() > 1) {
          retune_system(sys);
        } else {
          BOOST_LOG_TRIVIAL(error) << "[" << sys->get_short_name() << "]\tThere is only one control channel defined";
        }

      } else {
        sys->retune_attempts = 0;
      }

      if (msgs_decoded_per_second < config.control_message_warn_rate || config.control_message_warn_rate == -1) {
        BOOST_LOG_TRIVIAL(error) << "[" << sys->get_short_name() << "]\t Control Channel Message Decode Rate: " << msgs_decoded_per_second << "/sec, count:  " << sys->message_count;
      }
    }
    sys->message_count = 0;
  }
}


void check_conventional_channel_detection() {
  Source *source = NULL;
  for (vector<Source *>::iterator src_it = sources.begin(); src_it != sources.end(); src_it++) {
    source = *src_it;
    std::vector<Recorder *> recorders = source->get_detected_recorders();
    for (std::vector<Recorder *>::iterator it = recorders.begin(); it != recorders.end(); it++) {
      Recorder *recorder = *it;
      
      if (!recorder->is_enabled()) {
        recorder->set_enabled(true);
        BOOST_LOG_TRIVIAL(info) << "Enabled Freq: " << format_freq(recorder->get_freq());
      }
    }
  }
}


void monitor_messages() {
  gr::message::sptr msg;

  time_t last_status_time = time(NULL);
  time_t last_decode_rate_check = time(NULL);
  time_t management_timestamp = time(NULL);
  uint64_t last_conventional_channel_detection_check = time_since_epoch_millisec();
  time_t current_time = time(NULL);
  uint64_t current_time_ms = time_since_epoch_millisec();
  std::vector<TrunkMessage> trunk_messages;

  while (1) {

    if (exit_flag) { // my action when signal set it 1
      BOOST_LOG_TRIVIAL(info) << "Caught an Exit Signal...";
      for (vector<Call *>::iterator it = calls.begin(); it != calls.end();) {
        Call *call = *it;

        if (call->get_state() != MONITORING) {
          call->conclude_call();
        }

        it = calls.erase(it);
        delete call;
      }

      BOOST_LOG_TRIVIAL(info) << "Cleaning up & Exiting...";

      // Sleep for 5 seconds to allow for all of the Call Concluder threads to finish.
      boost::this_thread::sleep(boost::posix_time::milliseconds(5000));
      return;
    }



    process_message_queues();

    plugman_poll_one();

    for (vector<System *>::iterator sys_it = systems.begin(); sys_it != systems.end(); sys_it++) {
      System_impl *system = (System_impl *)*sys_it;

      if ((system->get_system_type() == "p25") || (system->get_system_type() == "smartnet")) {
        msg.reset();
        msg = system->get_msg_queue()->delete_head_nowait();
        while (msg != 0) {
          system->set_message_count(system->get_message_count() + 1);

          if (system->get_system_type() == "smartnet") {
            trunk_messages = smartnet_parser->parse_message(msg->to_string(), system);
            handle_message(trunk_messages, system);
            plugman_trunk_message(trunk_messages, system);
          }

          if (system->get_system_type() == "p25") {
            trunk_messages = p25_parser->parse_message(msg, system);
            handle_message(trunk_messages, system);
            plugman_trunk_message(trunk_messages, system);
          }

          if (msg->type() == -1) {
            BOOST_LOG_TRIVIAL(error) << "[" << system->get_short_name() << "]\t process_data_unit timeout";
          }

          msg.reset();
          msg = system->get_msg_queue()->delete_head_nowait();
        }
      }
    }
    current_time = time(NULL);
    current_time_ms = time_since_epoch_millisec();
    if ((current_time_ms - last_conventional_channel_detection_check) >= 0.1) {
      check_conventional_channel_detection();
      last_conventional_channel_detection_check = current_time_ms;
    }


    if ((current_time - management_timestamp) >= 1.0) {
      manage_calls();
      Call_Concluder::manage_call_data_workers();
      management_timestamp = current_time;
    }

    boost::this_thread::sleep(boost::posix_time::milliseconds(10));

    float decode_rate_check_time_diff = current_time - last_decode_rate_check;

    if (decode_rate_check_time_diff >= 3.0) {
      check_message_count(decode_rate_check_time_diff);
      last_decode_rate_check = current_time;
      for (vector<System *>::iterator sys_it = systems.begin(); sys_it != systems.end(); sys_it++) {
        System *system = *sys_it;
        if (system->get_system_type() == "p25") {
          system->clear_stale_talkgroup_patches();
        }
      }
    }

    float print_status_time_diff = current_time - last_status_time;

    if (print_status_time_diff > 200) {
      last_status_time = current_time;
      print_status();
    }
  }
}

bool setup_conventional_channel(System *system, double frequency, long channel_index) {
  bool channel_added = false;
  Source *source = NULL;
  float tone_freq = 0.0;
  for (vector<Source *>::iterator src_it = sources.begin(); src_it != sources.end(); src_it++) {
    source = *src_it;

    if ((source->get_min_hz() <= frequency) && (source->get_max_hz() >= frequency)) {
      channel_added = true;
      if (system->get_squelch_db() == -160) {
        BOOST_LOG_TRIVIAL(error) << "[" << system->get_short_name() << "]\tSquelch needs to be specified for the Source for Conventional Systems";
        return false;
      } else {
        source->set_signal_detector_threshold(system->get_squelch_db());
        channel_added = true;
      }

      Call_conventional *call = NULL;
      if (system->has_channel_file()) {
        Talkgroup *tg = system->find_talkgroup_by_freq(frequency);
        tone_freq = tg->tone;
        call = new Call_conventional(tg->number, tg->freq, system, config);
        call->set_talkgroup_tag(tg->alpha_tag);
      } else {
        call = new Call_conventional(channel_index, frequency, system, config);
      }
      BOOST_LOG_TRIVIAL(info) << "[" << system->get_short_name() << "]\tMonitoring " << system->get_system_type() << " channel: " << format_freq(frequency) << " Talkgroup: " << channel_index;
      if (system->get_system_type() == "conventional") {
        analog_recorder_sptr rec;
        if (tone_freq > 0.0) {
          rec = source->create_conventional_recorder(tb, tone_freq);
        } else {
          rec = source->create_conventional_recorder(tb);
        }
        rec = source->create_conventional_recorder(tb);
        rec->start(call);
        call->set_is_analog(true);
        call->set_recorder((Recorder *)rec.get());
        call->set_state(RECORDING);
        system->add_conventional_recorder(rec);
        calls.push_back(call);
        plugman_setup_recorder((Recorder *)rec.get());
        plugman_call_start(call);
      } else if (system->get_system_type() == "conventionalDMR") {
        // Because of dynamic mod assignment we can not start the recorder until the graph has been unlocked.
        // This has something to do with the way the Selector block works.
        // the manage_conventional_calls() function handles adding and starting the P25 Recorder
        dmr_recorder_sptr rec;
        rec = source->create_dmr_conventional_recorder(tb);
        call->set_recorder((Recorder *)rec.get());
        system->add_conventionalDMR_recorder(rec);
        calls.push_back(call);
      } else if (system->get_system_type() == "conventionalP25") { // has to be "conventional P25"
        // Because of dynamic mod assignment we can not start the recorder until the graph has been unlocked.
        // This has something to do with the way the Selector block works.
        // the manage_conventional_calls() function handles adding and starting the P25 Recorder
        p25_recorder_sptr rec;
        rec = source->create_digital_conventional_recorder(tb);
        call->set_recorder((Recorder *)rec.get());
        system->add_conventionalP25_recorder(rec);
        calls.push_back(call);
      } else if (system->get_system_type() == "conventionalSIGMF") { 
        sigmf_recorder_sptr rec;
        rec = source->create_sigmf_conventional_recorder(tb);
        call->set_recorder((Recorder *)rec.get());
        system->add_conventionalSIGMF_recorder(rec);
        calls.push_back(call);
      } else {
        BOOST_LOG_TRIVIAL(error) << "Error - Unknown system type: " << system->get_system_type();
      }

      // break out of the for loop
      break;
    }
  }
  return channel_added;
}

bool setup_conventional_system(System *system) {
  bool system_added = false;

  if (system->has_channel_file()) {
    std::vector<Talkgroup *> talkgroups = system->get_talkgroups();
    for (vector<Talkgroup *>::iterator tg_it = talkgroups.begin(); tg_it != talkgroups.end(); tg_it++) {
      Talkgroup *tg = *tg_it;

      bool channel_added = setup_conventional_channel(system, tg->freq, tg->number);

      if (!channel_added) {
        BOOST_LOG_TRIVIAL(error) << "[" << system->get_short_name() << "]\t Unable to find a source for this conventional channel! Channel not added: " << format_freq(tg->freq) << " Talkgroup: " << tg->number;
        // return false;
      } else {
        system_added = true;
      }
    }
  } else {
    std::vector<double> channels = system->get_channels();
    int channel_index = 0;
    for (vector<double>::iterator chan_it = channels.begin(); chan_it != channels.end(); chan_it++) {
      double channel = *chan_it;
      ++channel_index;
      bool channel_added = setup_conventional_channel(system, channel, channel_index);

      if (!channel_added) {
        BOOST_LOG_TRIVIAL(error) << "[" << system->get_short_name() << "]\t Unable to find a source for this conventional channel! Channel not added: " << format_freq(channel) << " Talkgroup: " << channel_index;
        // return false;
      } else {
        system_added = true;
      }
    }
  }
  return system_added;
}

bool setup_systems() {

  Source *source = NULL;

  for (vector<System *>::iterator sys_it = systems.begin(); sys_it != systems.end(); sys_it++) {
    System_impl *system = (System_impl *)*sys_it;
    // bool    source_found = false;
    bool system_added = false;
    if ((system->get_system_type() == "conventional") || (system->get_system_type() == "conventionalP25") || (system->get_system_type() == "conventionalDMR")) {
      system_added = setup_conventional_system(system);
    } else {
      // If it's not a conventional system, then it's a trunking system
      double control_channel_freq = system->get_current_control_channel();
      BOOST_LOG_TRIVIAL(info) << "[" << system->get_short_name() << "]\tStarted with Control Channel: " << format_freq(control_channel_freq);

      for (vector<Source *>::iterator src_it = sources.begin(); src_it != sources.end(); src_it++) {
        source = *src_it;

        if ((source->get_min_hz() <= control_channel_freq) &&
            (source->get_max_hz() >= control_channel_freq)) {
          // The source can cover the System's control channel
          system_added = true;
          system->set_source(source);

          if (system->get_system_type() == "smartnet") {
            // what you really need to do is go through all of the sources to find
            // the one with the right frequencies
            system->smartnet_trunking = make_smartnet_trunking(control_channel_freq,
                                                               source->get_center(),
                                                               source->get_rate(),
                                                               system->get_msg_queue(),
                                                               system->get_sys_num());
            tb->connect(source->get_src_block(), 0, system->smartnet_trunking, 0);
          }

          if (system->get_system_type() == "p25") {
            // what you really need to do is go through all of the sources to find
            // the one with the right frequencies
            system->p25_trunking = make_p25_trunking(control_channel_freq,
                                                     source->get_center(),
                                                     source->get_rate(),
                                                     system->get_msg_queue(),
                                                     system->get_qpsk_mod(),
                                                     system->get_sys_num());
            tb->connect(source->get_src_block(), 0, system->p25_trunking, 0);
          }

          // break out of the For Loop
          break;
        }
      }
      if (!system_added) {
        BOOST_LOG_TRIVIAL(error) << "[" << system->get_short_name() << "]\t Unable to find a source for this System! Control Channel Freq: " << format_freq(control_channel_freq);
        return false;
      }
    }
  }
  return true;
}

int main(int argc, char **argv) {
  // BOOST_STATIC_ASSERT(true) __attribute__((unused));

  logging::core::get()->set_filter(logging::trivial::severity >= logging::trivial::info);

  boost::log::register_simple_formatter_factory<boost::log::trivial::severity_level, char>("Severity");

  boost::log::add_common_attributes();
  boost::log::core::get()->add_global_attribute("Scope", boost::log::attributes::named_scope());
  boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::info);

  boost::program_options::options_description desc("Options");
  desc.add_options()("help,h", "Help screen")("config,c", boost::program_options::value<string>()->default_value("./config.json"), "Config File")("version,v", "Version Information");

  boost::program_options::variables_map vm;
  boost::program_options::store(parse_command_line(argc, argv, desc), vm);
  boost::program_options::notify(vm);

  if (vm.count("version")) {
    GitMetadata::VersionInfo();
    exit(0);
  }

  if (vm.count("help")) {
    std::cout << "Usage: trunk-recorder [options]\n";
    std::cout << desc;
    exit(0);
  }
  string config_file = vm["config"].as<string>();

  tb = gr::make_top_block("Trunking");

  smartnet_parser = new SmartnetParser(); // this has to eventually be generic;
  p25_parser = new P25Parser();

  std::string uri = "ws://localhost:3005";

  if (!load_config(config_file, config, tb, sources, systems)) {
    exit(1);
  }

  start_plugins(sources, systems);

  if (setup_systems()) {
    signal(SIGINT, exit_interupt);
    tb->start();

    monitor_messages();

    // ------------------------------------------------------------------
    // -- stop flow graph execution
    // ------------------------------------------------------------------
    BOOST_LOG_TRIVIAL(info) << "stopping flow graph" << std::endl;
    tb->stop();
    tb->wait();

    BOOST_LOG_TRIVIAL(info) << "stopping plugins" << std::endl;
    stop_plugins();
  } else {
    BOOST_LOG_TRIVIAL(error) << "Unable to setup a System to record, exiting..." << std::endl;
  }

  return exit_code;
}
