#ifndef P25CONVENTIONAL_RECORDER_H
#define P25CONVENTIONAL_RECORDER_H

#define _USE_MATH_DEFINES

class p25_recorder;
class Source;
class p25conventional_recorder;

typedef boost::shared_ptr<p25conventional_recorder>p25conventional_recorder_sptr;
p25conventional_recorder_sptr make_p25conventional_recorder(Source *src);

#include "p25_recorder.h"

class p25conventional_recorder : public p25_recorder {
  friend p25conventional_recorder_sptr make_p25conventional_recorder(Source *src);

protected:

  p25conventional_recorder(Source *src);

public:

  ~p25conventional_recorder();

private:

};


#endif // ifndef P25CONVENTIONAL_RECORDER_H
