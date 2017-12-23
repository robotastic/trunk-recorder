#include "imbe_decoder.h"

imbe_decoder::~imbe_decoder()
{
}

imbe_decoder::imbe_decoder() :
   d_audio()
{
}

audio_samples*
imbe_decoder::audio()
{
   return &d_audio;
}
