#include "talkgroups.h"
#include <boost/log/trivial.hpp>


// Retrieved from
// https://stackoverflow.com/questions/6089231/getting-std-ifstream-to-handle-lf-cr-and-crlf
std::istream& safeGetline(std::istream& is, std::string& t)
{
  t.clear();

  // The characters in the stream are read one-by-one using a std::streambuf.
  // That is faster than reading them one-by-one using the std::istream.
  // Code that uses streambuf this way must be guarded by a sentry object.
  // The sentry object performs various tasks,
  // such as thread synchronization and updating the stream state.

  std::istream::sentry se(is, true);
  std::streambuf *sb = is.rdbuf();

  for (;;) {
    int c = sb->sbumpc();

    switch (c) {
    case '\n':
      return is;

    case '\r':

      if (sb->sgetc() == '\n') sb->sbumpc();
      return is;

    case EOF:

      // Also handle the case when the last line has no line ending
      if (t.empty()) is.setstate(std::ios::eofbit);
      return is;

    default:
      t += (char)c;
    }
  }
}

Talkgroups::Talkgroups() {}

void Talkgroups::load_talkgroups(std::string filename) {
  std::ifstream in(filename.c_str());

  if (!in.is_open()) {
    BOOST_LOG_TRIVIAL(error) << "Error Opening TG File: " << filename <<
      std::endl;
    return;
  }
  
  boost::char_separator<char> sep(",");
  typedef boost::tokenizer<boost::char_separator<char> >t_tokenizer;

  std::vector<std::string> vec;
  std::string line;

  int lines_read   = 0;
  int lines_pushed = 0;

  while (safeGetline(in, line)) // this works with \r, \n, or \r\n
  {
    if (line.size() && (line[line.size() - 1] == '\r')) {
      line = line.substr(0, line.size() - 1);
    }

    if (line == "") {
      continue;
    }

    lines_read++;
    t_tokenizer tok(line, sep);

    vec.assign(tok.begin(), tok.end());

    if (vec.size() < 8) continue;  // yuck

    Talkgroup *tg = new Talkgroup(atoi(vec[0].c_str()), vec[2].at(
                                    0), vec[3].c_str(),
                                  vec[4].c_str(), vec[5].c_str(), vec[6].c_str(),
                                  atoi(vec[7].c_str()));

    talkgroups.push_back(tg);
    lines_pushed++;
  }

  if (lines_pushed != lines_read) {
    // The parser above is pretty brittle. This will help with debugging it, for
    // now.
    BOOST_LOG_TRIVIAL(error) << "Warning: skipped " << lines_read -
      lines_pushed << " of " << lines_read <<
    " talkgroup entries! Invalid format?";
    BOOST_LOG_TRIVIAL(error) <<
      "The format is very particular. See https://github.com/robotastic/trunk-recorder for example input.";
  }
  else {
    BOOST_LOG_TRIVIAL(info) << "Read " << lines_pushed << " talkgroups.";
  }
}

Talkgroup * Talkgroups::find_talkgroup(long tg_number) {
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
