#ifndef _CSV_HELPER_H_
#define _CSV_HELPER_H_

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/log/trivial.hpp>
#include <boost/tokenizer.hpp>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

std::istream &safeGetline(std::istream &is, std::string &t);

#endif // _CSV_HELPER_H_