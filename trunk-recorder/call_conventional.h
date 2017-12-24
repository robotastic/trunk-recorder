#ifndef CALL_CONVENTIONAL_H
#define CALL_CONVENTIONAL_H

class Call;
class Config;
class System;

class Call_conventional : Call {
public:

								Call_conventional( long t, double f, System *s, Config c);
								~Call_conventional();

};

#endif
