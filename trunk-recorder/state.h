#ifndef STATE_H
#define STATE_H

enum State { MONITORING = 0,
             RECORDING = 1,
             INACTIVE = 2,
             ACTIVE = 3,
             IDLE = 4,
             STOPPED = 6,
             AVAILABLE = 7,
             IGNORE = 8 };

enum MonitoringState {
             UNSPECIFIED = 0, 
             UNKNOWN_TG = 1,
             IGNORED_TG = 2,
             NO_SOURCE = 3,
             NO_RECORDER = 4,
             ENCRYPTED = 5,
             DUPLICATE = 6,
             SUPERSEDED = 7};

#endif
