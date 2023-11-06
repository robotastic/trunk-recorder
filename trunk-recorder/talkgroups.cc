#include "talkgroups.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/log/trivial.hpp>
#include <boost/tokenizer.hpp>

#include "csv_helper.h"
#include <csv-parser/csv.hpp>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>

Talkgroups::Talkgroups() {}

using namespace csv;

void Talkgroups::load_talkgroups(int sys_num, std::string filename) {
  if (filename == "") {
    return;
  } else {
    BOOST_LOG_TRIVIAL(info) << "Reading Talkgroup CSV File: " << filename;
  }

  CSVFormat format;
  format.trim({' ', '\t'});
  CSVReader reader(filename, format);
  std::vector<std::string> headers = reader.get_col_names();
  std::vector<std::string> defined_headers = {"Decimal", "Mode", "Description","Alpha Tag", "Hex", "Category", "Tag", "Priority", "Preferred NAC"};

  if (headers[0] != "Decimal") {

    BOOST_LOG_TRIVIAL(error) << "Column Headers are required for Talkgroup CSV files";
    BOOST_LOG_TRIVIAL(error) << "The first column must be 'Decimal'";
    BOOST_LOG_TRIVIAL(error) << "Required columns are: 'Decimal', 'Mode', 'Description'";
    BOOST_LOG_TRIVIAL(error) << "Optional columns are: 'Alpha Tag', 'Hex', 'Category', 'Tag', 'Priority', 'Preferred NAC'";
    exit(0);
  } else {
    BOOST_LOG_TRIVIAL(info) << "Found Columns: " << internals::format_row(reader.get_col_names(), ", ");
  }

  for(int i=0; i<headers.size(); i++) {
    if (find(defined_headers.begin(), defined_headers.end(), headers[i]) == defined_headers.end()) {
        BOOST_LOG_TRIVIAL(error) << "Unknown column header: " << headers[i];
        BOOST_LOG_TRIVIAL(error) << "Required columns are: 'Decimal', 'Mode', 'Description'";
        BOOST_LOG_TRIVIAL(error) << "Optional columns are: 'Alpha Tag', 'Hex', 'Category', 'Tag', 'Priority', 'Preferred NAC'";
        exit(0);
    }
  }

long lines_pushed = 0;
for (CSVRow& row: reader) { // Input iterator
    Talkgroup *tg = NULL;
    int priority = 1;
    unsigned long preferredNAC = 0;
    long tg_number = 0;
    std::string alpha_tag = "";
    std::string description = "";
    std::string tag = "";
    std::string group = "";
    std::string mode = "";

  if ((reader.index_of("Decimal")>=0) && !row["Decimal"].is_null() && row["Decimal"].is_int()) {
          tg_number = row["Decimal"].get<long>();
  } else {
    BOOST_LOG_TRIVIAL(error) << "'Decimal' is required for specifying the Talkgroup number - Row: " << reader.n_rows();
    exit(0);
  }

  if ((reader.index_of("Mode")>=0) && row["Mode"].is_str()) {
    mode = row["Mode"].get<std::string>();
  } else {
    BOOST_LOG_TRIVIAL(error) << "Mode is required for Row: "  << reader.n_rows();;
    exit(0);
  }

  if ((reader.index_of("Description")>=0) && row["Description"].is_str()) {
    description = row["Description"].get<std::string>();
  } else {
    BOOST_LOG_TRIVIAL(error) << "Description is required for Row: "  << reader.n_rows();;
    exit(0);
  }
  

  if ((reader.index_of("Alpha Tag")>=0) && row["Alpha Tag"].is_str()) {
    alpha_tag = row["Alpha Tag"].get<std::string>();
  }

  if ((reader.index_of("Tag")>=0) && row["Tag"].is_str()) {
    tag = row["Tag"].get<std::string>();
  }

  if ((reader.index_of("Category")>=0) && row["Category"].is_str()) {
    group = row["Category"].get<std::string>();
  }

  if ((reader.index_of("Priority")>=0) && row["Priority"].is_int()) {
    priority = row["Priority"].get<int>();
  }

  if ((reader.index_of("Preferred NAC")>=0) && row["Preferred NAC"].is_int()) {
    preferredNAC = row["Preferred NAC"].get<unsigned long>();
  }
    tg = new Talkgroup(sys_num, tg_number, mode, alpha_tag, description, tag, group, priority, preferredNAC); 
    talkgroups.push_back(tg);
    lines_pushed++;
}

    BOOST_LOG_TRIVIAL(info) << "Read " << lines_pushed << " talkgroups.";
  
}

void Talkgroups::load_channels(int sys_num, std::string filename) {
  if (filename == "") {
    return;
  }

  std::ifstream in(filename.c_str());

  if (!in.is_open()) {
    BOOST_LOG_TRIVIAL(error) << "Error Opening Channel File: " << filename << std::endl;
    return;
  }

  boost::escaped_list_separator<char> sep("\\", ",\t", "\"");
  typedef boost::tokenizer<boost::escaped_list_separator<char>> t_tokenizer;

  std::vector<std::string> vec;
  std::string line;

  int lines_read = 0;
  int lines_pushed = 0;

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
    // [7] - enable (Optional, default is True)

    if ((vec.size() != 7) && (vec.size() != 8)) {
      BOOST_LOG_TRIVIAL(error) << "Malformed channel entry at line " << lines_read << ". Found: " << vec.size() << " Expected 7 or 8";
      continue;
    }
    // TODO(nkw): more sanity checking here.
    bool enable = true;
    if (vec.size() == 8) {
      boost::trim(vec[7]);
      if (boost::iequals(vec[7], "false")) {
        enable = false;
      }
    }

    // Talkgroup(long num, double freq, double tone, std::string mode, std::string alpha_tag, std::string description, std::string tag, std::string group) {
    if (enable) {
      tg = new Talkgroup(sys_num, atoi(vec[0].c_str()), std::stod(vec[1]), std::stod(vec[2]), vec[3].c_str(), vec[4].c_str(), vec[5].c_str(), vec[6].c_str());

      talkgroups.push_back(tg);
    }
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

Talkgroup *Talkgroups::find_talkgroup(int sys_num, long tg_number) {
  Talkgroup *tg_match = NULL;

  for (std::vector<Talkgroup *>::iterator it = talkgroups.begin(); it != talkgroups.end(); ++it) {
    Talkgroup *tg = (Talkgroup *)*it;

    if ((tg->sys_num == sys_num) && (tg->number == tg_number)) {
      tg_match = tg;
      break;
    }
  }
  return tg_match;
}

Talkgroup *Talkgroups::find_talkgroup_by_freq(int sys_num, double freq) {
  Talkgroup *tg_match = NULL;

  for (std::vector<Talkgroup *>::iterator it = talkgroups.begin(); it != talkgroups.end(); ++it) {
    Talkgroup *tg = (Talkgroup *)*it;

    if ((tg->sys_num == sys_num) && (tg->freq == freq)) {
      tg_match = tg;
      break;
    }
  }
  return tg_match;
}

std::vector<Talkgroup *> Talkgroups::get_talkgroups() {
  return talkgroups;
}
