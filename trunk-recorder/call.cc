#include "call.h"

void Call::create_filename() {
    tm *ltm = localtime(&start_time);


	std::stringstream path_stream;
	path_stream << this->config.capture_dir <<  "/" << 1900 + ltm->tm_year << "/" << 1 + ltm->tm_mon << "/" << ltm->tm_mday;

	boost::filesystem::create_directories(path_stream.str());
  sprintf(filename, "%s/%ld-%ld_%g.wav", path_stream.str().c_str(),talkgroup,start_time,freq);
  sprintf(status_filename, "%s/%ld-%ld_%g.json", path_stream.str().c_str(),talkgroup,start_time,freq);
	//sprintf(filename, "%s/%ld-%ld.wav", path_stream.str().c_str(),talkgroup,start_time);
  //sprintf(status_filename, "%s/%ld-%ld.json", path_stream.str().c_str(),talkgroup,start_time);
}
Call::Call(long t, double f, Config c) {
  config = c;
	talkgroup = t;
	freq = f;
	start_time = time(NULL);
	last_update = time(NULL);
  state = monitoring;
	debug_recording = false;
  recorder = NULL;
	tdma = false;
	encrypted = false;
	emergency = false;
    src_count = 0;
    this->create_filename();
}

Call::Call(TrunkMessage message, Config c) {
  config = c;
	talkgroup = message.talkgroup;
	freq = message.freq;
	start_time = time(NULL);
	last_update = time(NULL);
  state = monitoring;
	debug_recording = false;
  recorder = NULL;
	tdma = message.tdma;
	encrypted = message.encrypted;
	emergency = message.emergency;
	src_count = 0;
    this->create_filename();
    this->add_source(message.source);
}

Call::~Call() {
  //  BOOST_LOG_TRIVIAL(info) << " This call is over!!";
}

void Call::close_call() {
  if (state == recording) {
    state = closing;
    closing_time = time(NULL);
    this->get_recorder()->close();
  }  else {
    BOOST_LOG_TRIVIAL(info) << "\tRemoving closing Call \tTG: " << this->get_talkgroup() << "\tElapsed: " << this->elapsed();
  }
}
void Call::end_call() {
    char shell_command[200];

            if (state == closing) {

                BOOST_LOG_TRIVIAL(info) << "\tRemoving Recorded Call \tTG: " << this->get_talkgroup() << "\tLast Update: " << this->since_last_update() << "Elapsed: " << this->elapsed() << std::endl;

                std::ofstream myfile (status_filename);
                if (myfile.is_open())
                {
                    myfile << "{\n";
                    myfile << "\"freq\": " << this->freq << ",\n";
                    myfile << "\"start_time\": " << this->start_time << ",\n";
                    myfile << "\"emergency\": " << this->emergency << ",\n";
                    myfile << "\"talkgroup\": " << this->talkgroup << ",\n";
                    myfile << "\"srcList\": [ ";
                    for (int i=0; i < this->src_count; i++ ){
                        if (i != 0) {
                          myfile << ", " <<  this->src_list[i].source;
                        } else {
                          myfile << this->src_list[i].source;
                        }
                    }
                    myfile << " ]\n";
                    myfile << "}\n";
                    myfile.close();
                }
                sprintf(shell_command,"./encode-upload.sh %s &", this->get_filename());
                this->get_recorder()->deactivate();
                int rc = system(shell_command);
                if (this->config.upload_server != "") {
                  send_call(this, config);
                }
                BOOST_LOG_TRIVIAL(info) << "\tFinished Removing Call \tTG: " << this->get_talkgroup() << "\tElapsed: " << this->elapsed() << std::endl;

            } else {
              //BOOST_LOG_TRIVIAL(info) << "\tRemoving closing Call \tTG: " << this->get_talkgroup() << "\tElapsed: " << this->elapsed();

            }
            if (this->get_debug_recording() == true) {
                this->get_debug_recorder()->deactivate();
            }

            //BOOST_LOG_TRIVIAL(trace) << "\tRemoving TG: " << call->get_talkgroup() << "\tElapsed: " << call->elapsed();

}


void  Call::set_debug_recorder(Recorder *r) {
	debug_recorder = r;
}

Recorder *  Call::get_debug_recorder() {
	return debug_recorder;
}

void  Call::set_recorder(Recorder *r) {
	recorder = r;
}
Recorder *  Call::get_recorder() {
	return recorder;
}
double  Call::get_freq() {
	return freq;
}
void Call::set_freq(double f) {
	freq = f;
}
long  Call::get_talkgroup() {
	return talkgroup;
}

Call_Source  *Call::get_source_list() {
	return src_list;
}

long  Call::get_source_count() {
	return src_count;
}

bool Call::add_source(long src) {
    double position = 0;
    if (src==0) {
        return false;
    }
    if (recorder!=NULL) {
      position = recorder->get_current_length();

    }
    Call_Source call_source = {src, position};

    if (src_count < 1 ) {
        src_list[src_count] = call_source;
        src_count++;
        if (state == recording){
        	BOOST_LOG_TRIVIAL(info) << "1st src: " << src << " Pos: " << position << " Elapsed:  " << this->elapsed() << " TG: " << this->talkgroup << std::endl;
        }
        return true;
    } else if ((src_count < 48) && (src_list[src_count-1].source != src)) {
        src_list[src_count] = call_source;
        src_count++;
        if (state == recording){
        	BOOST_LOG_TRIVIAL(info) << "adding src: " << src << " Pos: " << position << " Elapsed:  " << this->elapsed() << " TG: " << this->talkgroup << " Rec_num: " << this->recorder->num << std::endl;
        }
        return true;
    }
    return false;
}
void  Call::set_debug_recording(bool m) {
	debug_recording = m;
}
bool  Call::get_debug_recording() {
	return debug_recording;
}

void Call::set_state(State s){
  state = s;
}
State Call::get_state() {
  return state;
}
void  Call::set_encrypted(bool m) {
	encrypted = m;
}
bool  Call::get_encrypted() {
	return encrypted;
}
void  Call::set_emergency(bool m) {
	emergency = m;
}
bool  Call::get_emergency() {
	return emergency;
}
void  Call::set_tdma(int m) {
	tdma = m;
}
int  Call::get_tdma() {
	return tdma;
}
void  Call::update(TrunkMessage message) {

    this->add_source(message.source);
	last_update = time(NULL);
}
int  Call::since_last_update() {
	return time(NULL) - last_update;
}
long Call::closing_elapsed() {
	return time(NULL) - closing_time;
}
long  Call::elapsed() {
	return time(NULL) - start_time;
}
	long Call::get_start_time() {
    return start_time;
  }
char *Call::get_filename() {
	return filename;
}
