#ifndef CALL_CONCLUDER_H
#define CALL_CONCLUDER_H
#include <queue>
#include <list>
#include <vector>
#include <thread>
#include <future>
#include <boost/regex.hpp>
#include <ctime>

#include "../call.h"
#include "../global_structs.h"
#include "../formatter.h"
#include "../systems/system.h"
#include "../systems/system_impl.h"
/*
class Uploader;
#include "../uploaders/uploader.h"
#include "../uploaders/call_uploader.h"
#include "../uploaders/broadcastify_uploader.h"
#include "../uploaders/openmhz_uploader.h"*/




Call_Data_t upload_call_worker(Call_Data_t call_info);

class Call_Concluder {
static const int MAX_RETRY = 2;
public:
static std::list<Call_Data_t> retry_call_list;
static std::list<std::future<Call_Data_t>> call_data_workers;

static Call_Data_t create_call_data(Call *call, System *sys, Config config);

static void conclude_call(Call *call, System *sys, Config config);

static void manage_call_data_workers();

};




#endif