#include "talkgroups.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/log/trivial.hpp>
#include <boost/tokenizer.hpp>

#include "csv_helper.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>

Talkgroups::Talkgroups() {}

void Talkgroups::load_talkgroups(std::string filename) {
  if (filename == "") {
    return;
  }

  std::ifstream in(filename.c_str());

  if (!in.is_open()) {
    BOOST_LOG_TRIVIAL(error) << "Error Opening TG File: " << filename << std::endl;
    return;
  }

  boost::char_separator<char> sep(",\t");
  typedef boost::tokenizer<boost::char_separator<char>> t_tokenizer;

  std::vector<std::string> vec;
  std::string line;

  int lines_read = 0;
  int lines_pushed = 0;
  int priority = 1;
  bool radioreference_format = false;

  while (!safeGetline(in, line).eof()) // this works with \r, \n, or \r\n
  {
    if (line.size() && (line[line.size() - 1] == '\r')) {
      line = line.substr(0, line.size() - 1);
    }

    lines_read++;

    if (line == "")
      continue;

    t_tokenizer tok(line, sep);

    vec.assign(tok.begin(), tok.end());
    if (strcmp(vec[0].c_str(), "Decimal") == 0)
    {
      radioreference_format = true;
      BOOST_LOG_TRIVIAL(info) << "Found radioreference.com header, assuming direct csv download";
      // don't warn later on about the header line not being a valid talk group
      lines_pushed++;
      continue;
    }
    Talkgroup *tg = NULL;
    if (radioreference_format)
    {
      // Talkgroup configuration columns:
      //
      // [0] - talkgroup number
      // [1] - unused (talkgroup number in hex)
      // [2] - alpha_tag
      // [3] - mode
      // [4] - description
      // [5] - tag
      // [6] - group

      if (vec.size() != 7)
      {
        BOOST_LOG_TRIVIAL(error) << "Malformed radioreference talkgroup entry at line " << lines_read << ".";
        continue;
      }

      tg = new Talkgroup(atoi(vec[0].c_str()), vec[3].c_str(), vec[2].c_str(), vec[4].c_str(), vec[5].c_str(), vec[6].c_str(), 1);
    }
    else
    {
      // Talkgroup configuration columns:
      //
      // [0] - talkgroup number
      // [1] - unused
      // [2] - mode
      // [3] - alpha_tag
      // [4] - description
      // [5] - tag
      // [6] - group
      // [7] - priority

      if (!((vec.size() == 8) || (vec.size() == 7))) {
        BOOST_LOG_TRIVIAL(error) << "Malformed talkgroup entry at line " << lines_read << ".";
        continue;
      }
      // TODO(nkw): more sanity checking here.
      priority = (vec.size() == 8) ? atoi(vec[7].c_str()) : 1;

      tg = new Talkgroup(atoi(vec[0].c_str()), vec[2].c_str(), vec[3].c_str(), vec[4].c_str(), vec[5].c_str(), vec[6].c_str(), priority);
    }
    talkgroups.push_back(tg);
    lines_pushed++;
  }

  if (lines_pushed != lines_read) {
    // The parser above is pretty brittle. This will help with debugging it, for
    // now.
    BOOST_LOG_TRIVIAL(error) << "Warning: skipped " << lines_read - lines_pushed << " of " << lines_read << " talkgroup entries! Invalid format?";
    BOOST_LOG_TRIVIAL(error) << "The format is very particular. See https://github.com/robotastic/trunk-recorder for example input.";
  } else {
    BOOST_LOG_TRIVIAL(info) << "Read " << lines_pushed << " talkgroups.";
  }
}

void Talkgroups::load_channels(std::string filename) {
  if (filename == "") {
    return;
  }

  std::ifstream in(filename.c_str());

  if (!in.is_open()) {
    BOOST_LOG_TRIVIAL(error) << "Error Opening Channel File: " << filename << std::endl;
    return;
  }

  boost::char_separator<char> sep(",\t");
  typedef boost::tokenizer<boost::char_separator<char>> t_tokenizer;

  std::vector<std::string> vec;
  std::string line;

  int lines_read = 0;
  int lines_pushed = 0;
  int priority = 1;

  while (!safeGetline(in, line).eof()) // this works with \r, \n, or \r\n
  {
    if (line.size() && (line[line.size() - 1] == '\r')) {
      line = line.substr(0, line.size() - 1);
    }

    lines_read++;

    if (line == "")
      continue;

    t_tokenizer tok(line, sep);

    vec.assign(tok.begin(), tok.end());

    Talkgroup *tg = NULL;

    // Channel File  columns:
    //
    // [0] - talkgroup number
    // [1] - channel freq
    // [2] - tone
    // [3] - alpha_tag
    // [4] - description
    // [5] - tag
    // [6] - group

    if (!((vec.size() == 8) || (vec.size() == 7))) {
      BOOST_LOG_TRIVIAL(error) << "Malformed channel entry at line " << lines_read << ".";
      continue;
    }
    // TODO(nkw): more sanity checking here.

//Talkgroup(long num, double freq, double tone, std::string mode, std::string alpha_tag, std::string description, std::string tag, std::string group) {
    tg = new Talkgroup(atoi(vec[0].c_str()), std::stod(vec[1]), std::stod(vec[2]), vec[3].c_str(), vec[4].c_str(), vec[5].c_str(), vec[6].c_str());
  
    talkgroups.push_back(tg);
    lines_pushed++;
  }

  if (lines_pushed != lines_read) {
    // The parser above is pretty brittle. This will help with debugging it, for
    // now.
    BOOST_LOG_TRIVIAL(error) << "Warning: skipped " << lines_read - lines_pushed << " of " << lines_read << " channel entries! Invalid format?";
    BOOST_LOG_TRIVIAL(error) << "The format is very particular. See https://github.com/robotastic/trunk-recorder for example input.";
  } else {
    BOOST_LOG_TRIVIAL(info) << "Read " << lines_pushed << " channels.";
  }
}

Talkgroup *Talkgroups::find_talkgroup(long tg_number) {
  Talkgroup *tg_match = NULL;

  for (std::vector<Talkgroup *>::iterator it = talkgroups.begin(); it != talkgroups.end(); ++it) {
    Talkgroup *tg = (Talkgroup *)*it;

    if (tg->number == tg_number) {
      tg_match = tg;
      break;
    }
  }
  return tg_match;
}

Talkgroup *Talkgroups::find_talkgroup_by_freq(double freq) {
  Talkgroup *tg_match = NULL;

  for (std::vector<Talkgroup *>::iterator it = talkgroups.begin(); it != talkgroups.end(); ++it) {
    Talkgroup *tg = (Talkgroup *)*it;

    if (tg->freq == freq) {
      tg_match = tg;
      break;
    }
  }
  return tg_match;
}

std::vector<Talkgroup *> Talkgroups::get_talkgroups() {
  return talkgroups;
}

