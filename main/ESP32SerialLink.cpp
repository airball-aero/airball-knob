#include "ESP32SerialLink.h"

#include <thread>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_system.h"
#include "tinyusb.h"
#include "tusb_cdc_acm.h"

#include "sdkconfig.h"
#include <queue.h>
#include <thread>
#include <functional>

#include <mutex>

static const char *kTag = "ESP32SerialLink";
static constexpr UBaseType_t kQueueLength = 1024;

char const* string_desc_arr [] = {
    (const char[]) { 0x09, 0x04 }, // 0: is supported language is English (0x0409)
    "Airball.aero",                        // 1: Manufacturer
    "USB Knob+CANBus Peripheral",          // 2: Product
    "0",                                   // 3: Serials, should use chip ID
    "Telemetry serial",                    // 4: CDC Interface
};

static const tusb_desc_device_t cdc_device_descriptor = {
    .bLength = sizeof(cdc_device_descriptor),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = USB_ESPRESSIF_VID,
    .idProduct = 0x4002,
    .bcdDevice = 0x0100,
    .iManufacturer = 0x01,
    .iProduct = 0x02,
    .iSerialNumber = 0x03,
    .bNumConfigurations = 0x01
};

class ESP32SerialLinkImpl : public ESP32SerialLink {
public:
  ESP32SerialLinkImpl();

  // Exposed for interrupt handling; not for public use
  QueueHandle_t rx_queue() { return rx_queue_; }

protected:
  void push(uint8_t c) override;
  bool pull(uint8_t* c) override;

private:
  QueueHandle_t rx_queue_;
};

std::mutex instance_mu_;
static ESP32SerialLinkImpl* instance_ = nullptr;

ESP32SerialLink* ESP32SerialLink::instance() {
  std::unique_lock<std::mutex> lock(instance_mu_);
  if (instance_ == nullptr) {
    instance_ = new ESP32SerialLinkImpl();
  }
  return instance_;
}

void tusb_cdcacm_callback(int itf, cdcacm_event_t *event) {
  if (itf == TINYUSB_CDC_ACM_0 && event->type == CDC_EVENT_RX) {
    uint8_t rx_buf[CONFIG_TINYUSB_CDC_RX_BUFSIZE];
    size_t rx_size;
    tinyusb_cdcacm_read(TINYUSB_CDC_ACM_0,
                        rx_buf,
                        CONFIG_TINYUSB_CDC_RX_BUFSIZE,
                        &rx_size);
    QueueHandle_t q = ESP32SerialLinkImpl::instance()->rx_queue();
    for (int i = 0; i < rx_size; i++) {
      xQueueSendToBack(q, &(rx_buf[i]), 0);
      tinyusb_cdcacm_write_queue(TINYUSB_CDC_ACM_0, &(rx_buf[i]), 1);
      tinyusb_cdcacm_write_flush(TINYUSB_CDC_ACM_0, 0);
    }
  }
}

ESP32SerialLinkImpl::ESP32SerialLinkImpl() {
  ESP_LOGI(kTag, "USB initialization");
  const tinyusb_config_t tusb_cfg = {
      .device_descriptor = &cdc_device_descriptor,
      .string_descriptor = string_desc_arr,
      .external_phy = false,
      .configuration_descriptor = nullptr,
  };
  ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
  tinyusb_config_cdcacm_t acm_cfg = {
      .usb_dev = TINYUSB_USBDEV_0,
      .cdc_port = TINYUSB_CDC_ACM_0,
      .callback_rx = &tusb_cdcacm_callback,
      .callback_rx_wanted_char = nullptr,
      .callback_line_state_changed = nullptr,
      .callback_line_coding_changed = nullptr,
  };
  ESP_ERROR_CHECK(tusb_cdc_acm_init(&acm_cfg));
  ESP_LOGI(kTag, "USB initialization done");

  rx_queue_ = xQueueCreate(kQueueLength, 1);
}

void ESP32SerialLinkImpl::push(uint8_t c) {
  tinyusb_cdcacm_write_queue(TINYUSB_CDC_ACM_0, &c, 1);
  tinyusb_cdcacm_write_flush(TINYUSB_CDC_ACM_0, 0);
}

bool ESP32SerialLinkImpl::pull(uint8_t* c) {
  return xQueueReceive(rx_queue_, &c, 0);
}
