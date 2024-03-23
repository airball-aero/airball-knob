#ifndef MAIN_CANBUS_H
#define MAIN_CANBUS_H

#include "Telemetry.h"

class CanBus {
public:
  CanBus();

  void send(airball::Telemetry::Message m);
  bool recv(airball::Telemetry::Message* m);
};

#endif // MAIN_CANBUS_H
