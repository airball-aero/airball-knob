#ifndef MAIN_PUSHENCODERKNOB_H
#define MAIN_PUSHENCODERKNOB_H

#include "Telemetry.h"
#include <chrono>

class PushEncoderKnob {
public:
  PushEncoderKnob();

  bool evaluateEncoderStep(airball::Telemetry::Message* m);
  bool evaluateButtonTransition(airball::Telemetry::Message* m);

private:
  int encoderState_;
  int buttonState_;
  std::chrono::steady_clock::time_point t_last_;
};

#endif //MAIN_PUSHENCODERKNOB_H
