#ifndef STATE_H
#define STATE_H

enum State { MONITORING = 0,
             RECORDING = 1,
             INACTIVE = 2,
             ACTIVE = 3,
             IDLE = 4,
             COMPLETED = 5,
             STOPPED = 6,
             AVAILABLE = 7 };

enum MonitoringState {
             UNSPECIFIED = 0, 
             UNKNOWN_TG = 1,
             NO_SOURCE = 2,
             NO_RECORDER = 3,
             ENCRYPTED = 4,
             DUPLICATE = 5,
             SUPERSEDED = 6};

#endif
