#ifndef FORMATTER_H
#define FORMATTER_H

#include <boost/format.hpp>
#include <string>
#include "state.h"

extern boost::format FormatFreq(float f);
extern boost::format FormatSamplingRate(float f);
extern std::string FormatState(State state);
extern int frequencyFormat;
extern bool statusAsString;

#endif // FORMATTER_H



