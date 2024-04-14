#include "LeaderSelectSense.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <string>

#define TAG "LeaderSelectSense"

constexpr gpio_num_t kGpioLeaderSelect = GPIO_NUM_6;

LeaderSelectSense::LeaderSelectSense() {
  ESP_ERROR_CHECK(gpio_set_direction(kGpioLeaderSelect, GPIO_MODE_INPUT));
}

void LeaderSelectSense::reportLeaderSelect(airball::Telemetry::Message* m) {
  m->domain = airball::Telemetry::kMessageDomainLocal;
  if (gpio_get_level(kGpioLeaderSelect)) {
    ESP_LOGI(TAG, "This Node Is Leader");
    m->id = airball::Telemetry::kLocalMessageIdThisNodeIsLeader;
  } else {
    ESP_LOGI(TAG, "This Node Is Follower");
    m->id = airball::Telemetry::kLocalMessageIdThisNodeIsFollower;
  }
}