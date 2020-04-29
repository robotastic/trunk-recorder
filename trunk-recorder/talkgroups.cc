#include "talkgroups.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/log/trivial.hpp>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>

#include <fstream>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include "csv_helper.h"

Talkgroups::Talkgroups() {}

void Talkgroups::load_talkgroups(std::string filename) {
  if (filename == "") {
    return;
  }

  std::ifstream in(filename.c_str());

  if (!in.is_open()) {
    BOOST_LOG_TRIVIAL(error) << "Error Opening TG File: " << filename
                             << std::endl;
    return;
  }

  boost::char_separator<char> sep(",\t");
  boost::char_separator<char> dash("-");
  typedef boost::tokenizer< boost::char_separator<char> > t_tokenizer;

  std::vector<std::string> vec, r;
  std::string line;

  int lines_read = 0;
  int lines_pushed = 0;
  int priority = 1;
  int low,high,i;

  while (!safeGetline(in, line).eof()) // this works with \r, \n, or \r\n
  {
    if (line.size() && (line[line.size() - 1] == '\r')) {
      line = line.substr(0, line.size() - 1);
    }

    lines_read++;

    if (line == "")
      continue;
    
    t_tokenizer tok(line, sep);

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

    vec.assign(tok.begin(), tok.end());

    if (!((vec.size() == 8) || (vec.size() == 7))) {
      BOOST_LOG_TRIVIAL(error) << "Malformed talkgroup entry at line " << lines_read << ".";
      continue;
    }
    // TODO(nkw): more sanity checking here.
    priority = (vec.size() == 8) ?  atoi(vec[7].c_str()) : 1;

    t_tokenizer tuk(vec[0], dash);
    r.assign(tuk.begin(), tuk.end());
    if (r.size() == 2) {
      low = atoi(r[0].c_str());
      high = atoi(r[1].c_str());
      lines_read--;
      BOOST_LOG_TRIVIAL(info) << "Expanding TG range " << low << "-" << high;
      for(i=low; i<=high ;++i) {
        Talkgroup *tg_exist = NULL;
        if ( tg_exist = find_talkgroup(i) ) {
          tg_exist->number = i;
          tg_exist->mode = vec[2].at(0);
          tg_exist->alpha_tag = vec[3] + std::to_string(i);
          tg_exist->description = vec[4];
          tg_exist->tag = vec[5];
          tg_exist->group = vec[6];
          tg_exist->priority = atoi(vec[7].c_str());
        }
        else {
          Talkgroup *tg = new Talkgroup(i, vec[2].at(0), vec[3].c_str() + std::to_string(i), vec[4].c_str(), vec[5].c_str(), vec[6].c_str(), priority);
          //fprintf(stderr, "TG: %ld\tAlphaTag: %s\tPriority: %u\n", tg->number, tg->alpha_tag.c_str(), tg->priority );  // debugging
          talkgroups.push_back(tg);
          lines_pushed++;
          lines_read++;
        }
      }
    }
    else {
      Talkgroup *tg_exist = NULL;
      if ( tg_exist = find_talkgroup(atoi(vec[0].c_str())) ) {
       BOOST_LOG_TRIVIAL(info) << "TG " << vec[0].c_str() << " exists...overwriting";
       tg_exist->number = atoi(vec[0].c_str());
       tg_exist->mode = vec[2].at(0);
       tg_exist->alpha_tag = vec[3];
       tg_exist->description = vec[4];
       tg_exist->tag = vec[5];
       tg_exist->group = vec[6];
       tg_exist->priority = atoi(vec[7].c_str());
       lines_read--;
      }
      else {
        Talkgroup *tg = new Talkgroup(atoi(vec[0].c_str()), vec[2].at(0), vec[3].c_str(), vec[4].c_str(), vec[5].c_str(), vec[6].c_str(), priority);
        //fprintf(stderr, "TG: %ld\tAlphaTag: %s\tPriority: %u\n", tg->number, tg->alpha_tag.c_str(), tg->priority );  // debugging
        talkgroups.push_back(tg);
        lines_pushed++;
      }
    }

  }

  if (lines_pushed != lines_read) {
    // The parser above is pretty brittle. This will help with debugging it, for
    // now.
    BOOST_LOG_TRIVIAL(error) << "Warning: skipped " << lines_read - lines_pushed
                             << " of " << lines_read
                             << " talkgroup entries! Invalid format?";
    BOOST_LOG_TRIVIAL(error) << "The format is very particular. See "
                                "https://github.com/robotastic/trunk-recorder "
                                "for example input.";
  } else {
    BOOST_LOG_TRIVIAL(info) << "Read " << lines_pushed << " talkgroups.";
  }
/*  // print in-memory talkgroups table
  for (std::vector<Talkgroup *>::iterator it = talkgroups.begin(); it != talkgroups.end(); ++it) {
    Talkgroup *tg = (Talkgroup *)*it;
    fprintf(stderr, "TG: %ld\tAlphaTag: %s\tMode: %c\tPriority: %u\n", tg->number,tg->alpha_tag.c_str(), tg->mode, tg->priority );
  }
*/  //

}

Talkgroup *Talkgroups::find_talkgroup(long tg_number) {
  Talkgroup *tg_match = NULL;

  for (std::vector<Talkgroup *>::iterator it = talkgroups.begin();
       it != talkgroups.end(); ++it) {
    Talkgroup *tg = (Talkgroup *)*it;

    if (tg->number == tg_number) {
      tg_match = tg;
      break;
    }
  }
  return tg_match;
}

void Talkgroups::add(long num, std::string alphaTag)
{
    Talkgroup *tg = new Talkgroup(num, 'X', alphaTag, "", "", "", 0);
    talkgroups.push_back(tg);
}
