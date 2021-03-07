#ifndef UNIT_TAG_H
#define UNIT_TAG_H

#include <iostream>
#include <stdio.h>
#include <string>
//#include <sstream>

class UnitTag {
public:
  long number;
  std::string tag;

  UnitTag(long num, std::string t);
};

#endif // UNIT_TAG_H
