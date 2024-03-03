#ifndef MAIN_PUSHENCODERKNOB_H
#define MAIN_PUSHENCODERKNOB_H

#include "Telemetry.h"

class PushEncoderKnob {
public:
  PushEncoderKnob();

  bool evaluateEncoderStep(airball::Telemetry::Message* m);
  bool evaluateButtonTransition(airball::Telemetry::Message* m);
private:
  int gpioStateEncoderA_;
  int gpioStateEncoderB_;
  int gpioStateButton_;
};

#endif //MAIN_PUSHENCODERKNOB_H
