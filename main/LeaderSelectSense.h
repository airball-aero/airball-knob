#ifndef MAIN_LEADERSELECTSENSE_H
#define MAIN_LEADERSELECTSENSE_H

#include "Telemetry.h"

class LeaderSelectSense {
public:
  LeaderSelectSense();
  void reportLeaderSelect(airball::Telemetry::Message* m);
};


#endif //MAIN_LEADERSELECTSENSE_H
