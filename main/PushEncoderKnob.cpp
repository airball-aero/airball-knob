#include "PushEncoderKnob.h"
#include "driver/gpio.h"
#include <unordered_map>

constexpr gpio_num_t kGpioEncoderA = GPIO_NUM_0;
constexpr gpio_num_t kGpioEncoderB = GPIO_NUM_0;
constexpr gpio_num_t kGpioButton = GPIO_NUM_0;

// From https://github.com/PaulStoffregen/Encoder/blob/master/Encoder.h
//                           _______         _______
//               Pin1 ______|       |_______|       |______ Pin1
// negative <---         _______         _______         __      --> positive
//               Pin2 __|       |_______|       |_______|   Pin2
//
//      new     new     old     old
//      pin2    pin1    pin2    pin1    Result
//      ----    ----    ----    ----    ------
//      0       0       0       0       no movement
//      0       0       0       1       +1
//      0       0       1       0       -1
//      0       0       1       1       +2  (assume pin1 edges only)
//      0       1       0       0       -1
//      0       1       0       1       no movement
//      0       1       1       0       -2  (assume pin1 edges only)
//      0       1       1       1       +1
//      1       0       0       0       +1
//      1       0       0       1       -2  (assume pin1 edges only)
//      1       0       1       0       no movement
//      1       0       1       1       -1
//      1       1       0       0       +2  (assume pin1 edges only)
//      1       1       0       1       -1
//      1       1       1       0       +1
//      1       1       1       1       no movement

struct Pins {
  Pins(int _bNew, int _aNew, int _bOld, int _aOld) {
    bNew = _bNew;
    aNew = _aNew;
    bOld = _bOld;
    aOld = _aOld;
  }

  int bNew;
  int aNew;
  int bOld;
  int aOld;

  bool operator==(const Pins &p) const {
    return
        bNew == p.bNew &&
        aNew == p.aNew &&
        bOld == p.bOld &&
        aOld == p.aOld;
  }
};

template <>
struct std::hash<Pins>
{
  std::size_t operator()(const Pins& k) const {
    return k.bNew + 2 * k.aNew + 4 * k.bOld + 8 * k.aOld;
  }
};

std::unordered_map<Pins, int> kPinTransitions = {
    { Pins(0, 0, 0, 0),  0},
    { Pins(0, 0, 0, 1),  1},
    { Pins(0, 0, 1, 0), -1},
    { Pins(0, 0, 1, 1),  0}, // ignore double transitions

    { Pins(0, 1, 0, 0), -1},
    { Pins(0, 1, 0, 1),  0},
    { Pins(0, 1, 1, 0),  0}, // ignore double transitions
    { Pins(0, 1, 1, 1),  1},

    { Pins(1, 0, 0, 0),  1},
    { Pins(1, 0, 0, 1),  0}, // ignore double transitions
    { Pins(1, 0, 1, 0),  0},
    { Pins(1, 0, 1, 1), -1},

    { Pins(1, 1, 0, 0),  0}, // ignore double transitions
    { Pins(1, 1, 0, 1), -1},
    { Pins(1, 1, 1, 0),  1},
    { Pins(1, 1, 1, 1),  0},
};

PushEncoderKnob::PushEncoderKnob()
    : gpioStateEncoderA_(0),
      gpioStateEncoderB_(0),
      gpioStateButton_(0)
{
  ESP_ERROR_CHECK(gpio_set_direction(kGpioEncoderA, GPIO_MODE_INPUT));
  ESP_ERROR_CHECK(gpio_set_direction(kGpioEncoderB, GPIO_MODE_INPUT));
  ESP_ERROR_CHECK(gpio_set_direction(kGpioButton, GPIO_MODE_INPUT));
}

bool PushEncoderKnob::evaluateEncoderStep(airball::Telemetry::Message* m) {
  int gpioStateEncoderANew = gpio_get_level(kGpioEncoderA);
  int gpioStateEncoderBNew = gpio_get_level(kGpioEncoderB);
  if (gpioStateEncoderANew != gpioStateEncoderA_ || gpioStateEncoderBNew != gpioStateEncoderB_) {

    gpioStateEncoderA_ = gpioStateEncoderANew;
    gpioStateEncoderB_ = gpioStateEncoderBNew;
    return true;
  }
  return false;
}

bool PushEncoderKnob::evaluateButtonTransition(airball::Telemetry::Message* m) {
  int gpioStateButtonNew = gpio_get_level(kGpioEncoderB);
  if (gpioStateButton_ != gpioStateButtonNew) {
    m->domain = airball::Telemetry::kMessageDomainLocal;
    if (gpioStateButtonNew) {
      m->id = airball::Telemetry::kLocalMessageIdButtonPress;
    } else {
      m->id = airball::Telemetry::kLocalMessageIdButtonRelease;
    }
    gpioStateButton_ = gpioStateButtonNew;
    return true;
  }
  return false;
}
