#include "unit_tag.h"
#include <boost/regex.hpp>

UnitTag::UnitTag(std::string p, std::string t) {
  pattern = p;
  tag = t;
}
