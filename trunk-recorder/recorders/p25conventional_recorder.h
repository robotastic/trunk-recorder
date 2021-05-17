#ifndef P25CONVENTIONAL_RECORDER_H
#define P25CONVENTIONAL_RECORDER_H

#define _USE_MATH_DEFINES
#include <boost/shared_ptr.hpp>

class p25_recorder;
class Source;
class p25conventional_recorder;
class nonstop_wavfile_delayopen_sink_impl;

typedef boost::shared_ptr<p25conventional_recorder>p25conventional_recorder_sptr;
p25conventional_recorder_sptr make_p25conventional_recorder(Source * src, bool delayopen);

#include "p25_recorder.h"

class p25conventional_recorder : public p25_recorder {
  friend p25conventional_recorder_sptr make_p25conventional_recorder(Source * src, bool delayopen);

protected:

  p25conventional_recorder(bool delayopen);

public:

  ~p25conventional_recorder();

  void start(Call *call);
  void recording_started();
  char * get_filename();

private:
  bool d_delayopen;
};


#endif // ifndef P25CONVENTIONAL_RECORDER_H
