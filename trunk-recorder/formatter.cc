#include "formatter.h"
#include <boost/lexical_cast.hpp>

int frequencyFormat = 0;
bool statusAsString = true;

boost::format FormatFreq(double f)
{
	if (frequencyFormat == 1)
		return boost::format("%10.6f MHz")	% (f / 1000000.0);
	else if (frequencyFormat == 2)
		return boost::format("%.0f Hz")	% f;
	else
		return boost::format("%e")		% f;
}

boost::format FormatSamplingRate(float f)
{	
	return boost::format("%.0f")  % f;
}

std::string FormatState(State state)
{
	if (statusAsString)
	{
		if (state == monitoring)
			return "monitoring";
		else if (state == recording)
			return "recording";
		else if (state == inactive)
			return "inactive";
		else if (state == active)
			return "active";
		return "Unknown";
	}
	return boost::lexical_cast<std::string>(state);
}
