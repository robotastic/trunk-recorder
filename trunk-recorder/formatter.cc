#include "formatter.h"
#include <boost/lexical_cast.hpp>

int frequencyFormat = 0;
bool statusAsString = true;

boost::format format_freq(double f) {
  if (frequencyFormat == 1)
    return boost::format("%10.6f MHz") % (f / 1000000.0);
  else if (frequencyFormat == 2)
    return boost::format("%.0f Hz") % f;
  else
    return boost::format("%e") % f;
}

std::string get_frequency_format() {
  if (frequencyFormat == 1)
    return "mhz";
  else if (frequencyFormat == 2)
    return "hz";
  else
    return "exp";
}

boost::format FormatSamplingRate(float f) {
  return boost::format("%.0f") % f;
}

std::string format_state(State state) {
  if (statusAsString) {
    if (state == MONITORING)
      return "monitoring";
    else if (state == RECORDING)
      return "recording";
    else if (state == INACTIVE)
      return "inactive";
    else if (state == ACTIVE)
      return "active";
    else if (state == IDLE)
      return "idle";
    else if (state == COMPLETED)
      return "completed";
    else if (state == STOPPED)
      return "stopped";
    else if (state == AVAILABLE)
      return "available";
    return "Unknown";
  }
  return boost::lexical_cast<std::string>(state);
}
