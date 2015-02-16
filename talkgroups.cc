#include "talkgroups.h"

Talkgroups::Talkgroups() {

}

void Talkgroups::load_talkgroups(std::string filename) {
	std::ifstream in(filename.c_str());
	if (!in.is_open()) {
		std::cout << "Error Opening TG File: " << filename << std::endl;
		return;
	}
	boost::char_separator<char> sep(",");
	typedef boost::tokenizer< boost::char_separator<char> > t_tokenizer;

	std::vector< std::string > vec;
	std::string line;

	while (getline(in,line, '\r'))  // this works with the CSV files from Excel, it might be a different new line charecter for other programs
	{
		t_tokenizer tok(line, sep);

		vec.assign(tok.begin(),tok.end());
		if (vec.size() < 8) continue;

		Talkgroup *tg = new Talkgroup(atoi( vec[0].c_str()), vec[2].at(0),vec[3].c_str(),vec[4].c_str(),vec[5].c_str() ,vec[6].c_str(),atoi(vec[7].c_str()) );

		talkgroups.push_back(tg);
	}
}

Talkgroup *Talkgroups::find_talkgroup(long tg_number) {
	Talkgroup *tg_match = NULL;
	for(std::vector<Talkgroup *>::iterator it = talkgroups.begin(); it != talkgroups.end(); ++it) {
		Talkgroup *tg = (Talkgroup *) *it;
		if (tg->number == tg_number) {
			tg_match = tg;
			break;
		}

	}
	return tg_match;
}