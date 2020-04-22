#ifndef OPENMHZ_UPLOADER_H
#define OPENMHZ_UPLOADER_H


class Uploader;

#include "uploader.h"

class OpenmhzUploader : public Uploader {
public:
    int upload(struct call_data_t *call);
};

#endif
