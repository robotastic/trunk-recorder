#ifndef BROADCASTIFY_UPLOADER_H
#define BROADCASTIFY_UPLOADER_H


class Uploader;

#include "uploader.h"

class BroadcastifyUploader : public Uploader {
public:
    int upload(struct call_data_t *call);
private:
    CURLcode upload_audio_file(std::string converted, std::string url);
};


#endif
