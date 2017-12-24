
#include "formatter.h"
#include <boost/algorithm/string.hpp>
#include "call.h"
#include "config.h"
#include "call_conventional.h"


Call_conventional::Call_conventional(long t, double f, System *s, Config c) : Call(t,f,s,c) {
}

Call_conventional::~Call_conventional() {
  //  BOOST_LOG_TRIVIAL(info) << " This call is over!!";
}
