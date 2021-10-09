#ifndef FORMATTER_H
#define FORMATTER_H

#include "state.h"
#include <boost/format.hpp>
#include <string>

extern boost::format FormatFreq(double f);
extern boost::format FormatSamplingRate(float f);
extern std::string FormatState(State state);
extern int frequencyFormat;
extern bool statusAsString;

#endif // FORMATTER_H
