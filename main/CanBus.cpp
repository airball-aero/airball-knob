#include "CanBus.h"

#include "driver/twai.h"
#include <driver/gpio.h>
#include "esp_log.h"

#define TAG "CanBus"

CanBus::CanBus() {
  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_18, GPIO_NUM_17, TWAI_MODE_NORMAL);
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
  if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
    ESP_LOGI(TAG, "TWAI driver installed");
  }
  if (twai_start() == ESP_OK) {
    ESP_LOGI(TAG, "TWAI driver started");
  }
}

void toTwaiMessage(const airball::Telemetry::Message* m, twai_message_t *twai) {
  memset(twai, 0, sizeof(twai_message_t));
  twai->ss = 1;
  twai->identifier = m->id;
  twai->data_length_code = 0;
  memcpy(twai->data, m->data, 8);
}

void toAirballMessage(const twai_message_t* twai, airball::Telemetry::Message* m) {
  memset(m, 0, sizeof(airball::Telemetry::Message));
  m->domain = airball::Telemetry::kMessageDomainCanBus;
  m->id = twai->identifier;
  memcpy(m->data, twai->data, 8);
}

void CanBus::send(airball::Telemetry::Message m) {
  twai_message_t twai;
  toTwaiMessage(&m, &twai);
  auto r = twai_transmit(&twai, pdMS_TO_TICKS(10));
  if (r != ESP_OK) {
    ESP_LOGI(TAG, "Error in twai_transmit: %d", r);
  } else {
    ESP_LOGI(TAG, "Successful twai_transmit");
  }
}

bool CanBus::recv(airball::Telemetry::Message* m) {
  twai_message_t twai;
  auto r = twai_receive(&twai, 0);
  if (r == ESP_OK) {
    toAirballMessage(&twai, m);
    ESP_LOGI(TAG, "Successful twai_receive");
    return true;
  }
  return false;
}
