#ifndef FORMATTER_H
#define FORMATTER_H

#include "state.h"
#include <boost/format.hpp>
#include <string>

extern boost::format format_freq(double f);
extern boost::format FormatSamplingRate(float f);
extern boost::format format_time(float f);
extern std::string format_state(State state);
extern std::string format_state(State state, MonitoringState monitoringState);
std::string get_frequency_format();
extern int frequency_format;
extern bool statusAsString;

#endif // FORMATTER_H
