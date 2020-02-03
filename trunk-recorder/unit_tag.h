#ifndef UNIT_TAG_H
#define UNIT_TAG_H

#include <iostream>
#include <string>
#include <stdio.h>
//#include <sstream>

class UnitTag {
public:
	long number;
	std::string tag;
	std::string description;
	
	UnitTag(long num, std::string t, std::string d);

};

#endif // UNIT_TAG_H
