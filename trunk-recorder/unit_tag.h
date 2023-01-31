#ifndef UNIT_TAG_H
#define UNIT_TAG_H

#include <iostream>
#include <stdio.h>
#include <string>
#include <boost/regex.hpp>

class UnitTag {
public:
  boost::regex pattern;
  std::string tag;

  UnitTag(std::string p, std::string t);
};

#endif // UNIT_TAG_H
