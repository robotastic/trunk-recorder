#include "call.h"

void Call::create_filename() {
    tm *ltm = localtime(&start_time);

    
	std::stringstream path_stream;
	path_stream << boost::filesystem::current_path().string() <<  "/" << 1900 + ltm->tm_year << "/" << 1 + ltm->tm_mon << "/" << ltm->tm_mday;

	boost::filesystem::create_directories(path_stream.str());
	sprintf(filename, "%s/%ld-%ld_%g.wav", path_stream.str().c_str(),talkgroup,start_time,freq);
    sprintf(filename, "%s/%ld-%ld_%g.json", path_stream.str().c_str(),talkgroup,start_time,freq);
}
Call::Call(long t, double f) {
	talkgroup = t;
	freq = f;
	start_time = time(NULL);
	last_update = time(NULL);
	recording = false;
	debug_recording = false;
	tdma = false;
	encrypted = false;
	emergency = false;
    src_count = 0;
    this->create_filename();
}

Call::Call(TrunkMessage message) {
	talkgroup = message.talkgroup;
	freq = message.freq;
	start_time = time(NULL);
	last_update = time(NULL);
	recording = false;
	debug_recording = false;
	tdma = message.tdma;
	encrypted = message.encrypted;
	emergency = message.emergency;
	src_count = 0;
    this->create_filename();
    this->add_source(message.source);
}

void Call::end_call() {
    char shell_command[200];
    
            if (this->get_recording() == true) {

                //BOOST_LOG_TRIVIAL(info) << "\tRemoving TG: " << call->get_talkgroup() << "\tElapsed: " << call->elapsed() << std::endl;

                std::ofstream myfile (status_filename);
                if (myfile.is_open())
                {
                    myfile << "{\n";
                    myfile << "\"freq\": " << this->freq << ",\n";
                    myfile << "\"emergency\": " << this->emergency << ",\n";
                    myfile << "\"talkgroup\": " << this->talkgroup << ",\n";
                    myfile << "\"srcList\": [ ";
                    for (int i=0; i < this->src_count; i++ ){
                        if (i != 0) {
                          myfile << ", " <<  this->src_list[i];
                        } else {
                          myfile << this->src_list[i];  
                        }
                    }
                    myfile << " ]\n";
                    myfile << "}\n";
                    myfile.close();
                }
                sprintf(shell_command,"./encode-upload.sh %s > /dev/null 2>&1 &", this->get_filename());
                this->get_recorder()->deactivate();
                system(shell_command);
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

long  *Call::get_source_list() {
	return src_list;
}

long  Call::get_source_count() {
	return src_count;
}

bool Call::add_source(long src) {
    if (src==0) {
        return false;
    }
    if (src_count < 1 ) {
        src_list[src_count] = src;
        src_count++;
        return true;
    } else if ((src_count < 48) && (src_list[src_count-1] != src)) {
        src_list[src_count] = src;
        src_count++;
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
void  Call::set_recording(bool m) {
	recording = m;
}
bool  Call::get_recording() {
	return recording;
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
long  Call::elapsed() {
	return time(NULL) - start_time;
}

char *Call::get_filename() {
	return filename;
}