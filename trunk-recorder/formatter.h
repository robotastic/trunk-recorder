#ifndef FORMATTER_H
#define FORMATTER_H

#include <boost/format.hpp>

extern boost::format FormatFreq(float f);
extern boost::format FormatSamplingRate(float f);
extern int frequencyFormat;

#endif // FORMATTER_H



