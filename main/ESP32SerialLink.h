#ifndef MAIN_ESP32SERIALLINK_H
#define MAIN_ESP32SERIALLINK_H

#include "Telemetry.h"
#include "AbstractSerialLink.h"

class ESP32SerialLink : public airball::AbstractSerialLink<sizeof(airball::Telemetry::Message)> {
public:
  static ESP32SerialLink* instance();
};

#endif //MAIN_ESP32SERIALLINK_H
