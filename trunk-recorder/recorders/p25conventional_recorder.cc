#include "p25_recorder.h"
#include "p25conventional_recorder.h"


p25conventional_recorder_sptr make_p25conventional_recorder(Source *src)
{
  return gnuradio::get_initial_sptr(new p25conventional_recorder(src));
}

p25conventional_recorder::p25conventional_recorder(Source *src) : p25_recorder(src)  
{
  
}

p25conventional_recorder::~p25conventional_recorder() {}
