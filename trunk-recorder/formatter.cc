#include "formatter.h"

int frequencyFormat = 0;

boost::format FormatFreq(float f)
{
	if (frequencyFormat == 1)
		return boost::format("%3.6f")  % (f / 1000000);
	else if (frequencyFormat == 2)
		return boost::format("%.0f") %f;
	else
		return boost::format("%e")  % f;
}

boost::format FormatSamplingRate(float f)
{	
	return boost::format("%f")  % f;
}

