#include "PushEncoderKnob.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <unordered_map>
#include <string>

#define TAG "PushEncoderKnob"

constexpr gpio_num_t kGpioButton = GPIO_NUM_7;
constexpr gpio_num_t kGpioEncoderA = GPIO_NUM_8;
constexpr gpio_num_t kGpioEncoderB = GPIO_NUM_9;

constexpr int kEncoderState00 = 0;
constexpr int kEncoderState01 = 1;
constexpr int kEncoderState10 = 2;
constexpr int kEncoderState11 = 3;

constexpr uint16_t kCW = airball::Telemetry::kLocalMessageIdKnobDecrement;
constexpr uint16_t kCCW = airball::Telemetry::kLocalMessageIdKnobIncrement;

constexpr std::chrono::milliseconds kTransitionWait(3);

template<>
struct std::hash<std::pair<int, int>> {
  size_t operator()(const std::pair<int, int>& p) const {
    return p.first + p.second;
  }
};

// https://people.ece.ubc.ca/edc/2117.jan2021/lab6.pdf
//
std::unordered_map<std::pair<int, int>, uint16_t> kTransitions = {
    {
        {kEncoderState00, kEncoderState01}, kCCW,
    },
    {
        {kEncoderState00, kEncoderState10}, kCW,
    },
    {
        {kEncoderState01, kEncoderState00}, kCW,
    },
    {
        {kEncoderState01, kEncoderState11}, kCCW,
    },
    {
        {kEncoderState10, kEncoderState00}, kCCW,
    },
    {
        {kEncoderState10, kEncoderState11}, kCW,
    },
    {
        {kEncoderState11, kEncoderState01}, kCW,
    },
    {
        {kEncoderState11, kEncoderState10}, kCCW,
    },
};

int readEncoderState() {
  int a = gpio_get_level(kGpioEncoderA);
  int b = gpio_get_level(kGpioEncoderB);
  switch (a) {
    case 0:
      switch (b) {
        case 0:
          return kEncoderState00;
        case 1:
          return kEncoderState01;
        default:
          ESP_LOGI(TAG, "Strange gpio_get_level(%d) = %d", kGpioEncoderB, b);
          return kEncoderState00;
      }
    case 1:
      switch (b) {
        case 0:
          return kEncoderState10;
        case 1:
          return kEncoderState11;
        default:
          ESP_LOGI(TAG, "Strange gpio_get_level(%d) = %d", kGpioEncoderB, b);
          return kEncoderState00;
      }
    default:
      ESP_LOGI(TAG, "Strange gpio_get_level(%d) = %d", kGpioEncoderA, a);
      return kEncoderState00;
  }
}

PushEncoderKnob::PushEncoderKnob()
    : encoderState_(kEncoderState00),
      buttonState_(0),
      t_last_(std::chrono::steady_clock::now()) {
  ESP_ERROR_CHECK(gpio_set_direction(kGpioEncoderA, GPIO_MODE_INPUT));
  ESP_ERROR_CHECK(gpio_set_direction(kGpioEncoderB, GPIO_MODE_INPUT));
  ESP_ERROR_CHECK(gpio_set_direction(kGpioButton, GPIO_MODE_INPUT));
}

std::string l(int encoderState) {
  switch (encoderState) {
    case kEncoderState00: return "00";
    case kEncoderState01: return "01";
    case kEncoderState10: return "10";
    case kEncoderState11: return "11";
    default: return "??";
  }
}

bool PushEncoderKnob::evaluateEncoderStep(airball::Telemetry::Message* m) {
  auto t = std::chrono::steady_clock::now();
  if ((t - t_last_) < kTransitionWait) {
    return false;
  }
  t_last_ = t;
  int encoderStateNew = readEncoderState();
  if (encoderState_ != encoderStateNew) {
    bool transition = false;
    if (encoderState_ == kEncoderState00) {
      transition = true;
      m->domain = airball::Telemetry::kMessageDomainLocal;
      m->id = kTransitions[{encoderStateNew, encoderState_}];
      switch (m->id) {
        case airball::Telemetry::kLocalMessageIdKnobIncrement:
          ESP_LOGI(TAG, "Increment");
          break;
        case airball::Telemetry::kLocalMessageIdKnobDecrement:
          ESP_LOGI(TAG, "Decrement");
          break;
      }
      memset(m->data, 0, 8);
    }
    encoderState_ = encoderStateNew;
    return transition;
  }
  return false;
}

bool PushEncoderKnob::evaluateButtonTransition(airball::Telemetry::Message* m) {
  int buttonStateNew = gpio_get_level(kGpioButton);
  if (buttonState_ != buttonStateNew) {
    m->domain = airball::Telemetry::kMessageDomainLocal;
    if (buttonStateNew) {
      m->id = airball::Telemetry::kLocalMessageIdButtonPress;
      ESP_LOGI(TAG, "Press");
    } else {
      m->id = airball::Telemetry::kLocalMessageIdButtonRelease;
      ESP_LOGI(TAG, "Release");
    }
    buttonState_ = buttonStateNew;
    return true;
  }
  return false;
}
