#include <sys/cdefs.h>
#include <stdint.h>

#include <thread>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_system.h"
#include "tinyusb.h"
#include "tusb_cdc_acm.h"

#include "sdkconfig.h"

#include "AbstractSerialLink.h"
#include "Telemetry.h"

class ESP32SerialLink : public airball::AbstractSerialLink<sizeof(airball::Telemetry::Message)> {
public:
  ESP32SerialLink() {
    rx_queue_ = xQueueCreate(1024, 1);
    read_thread_ = std::thread([this]() {
      while (true) {
        size_t rx_size = 0;
        tinyusb_cdcacm_read(TINYUSB_CDC_ACM_0,
                            rx_buf_,
                            CONFIG_TINYUSB_CDC_RX_BUFSIZE,
                            &rx_size);
        for (int i = 0; i < rx_size; i++) {
          xQueueSendToBack(rx_queue_, &(rx_buf_[i]), 0);
          tinyusb_cdcacm_write_queue(TINYUSB_CDC_ACM_0, &(rx_buf_[i]), 1);
          tinyusb_cdcacm_write_flush(TINYUSB_CDC_ACM_0, 0);
        }
      }
    });
  }

protected:
  void push(uint8_t c) override {
    tinyusb_cdcacm_write_queue(TINYUSB_CDC_ACM_0, &c, 1);
    tinyusb_cdcacm_write_flush(TINYUSB_CDC_ACM_0, 0);
  }

  uint8_t pull() override {
    uint8_t c;
    while (xQueueReceive(rx_queue_, &c, 5 / portTICK_PERIOD_MS) != pdTRUE) { }
    return c;
  }

private:
  std::thread read_thread_;
  uint8_t rx_buf_[CONFIG_TINYUSB_CDC_RX_BUFSIZE];
  QueueHandle_t rx_queue_;
};

static const char *TAG = "Airball Knob+CANBus";

/*
static uint8_t buf[CONFIG_TINYUSB_CDC_RX_BUFSIZE + 1];
*/

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

/*
* @brief Receive data from CDC interface
*
* @param[in] itf           Index of CDC interface
* @param[out] out_buf      Data buffer
* @param[in] out_buf_sz    Data buffer size in bytes
* @param[out] rx_data_size Number of bytes written to out_buf
* @return esp_err_t ESP_OK, ESP_FAIL or ESP_ERR_INVALID_STATE
*/
// esp_err_t tinyusb_cdcacm_read(tinyusb_cdcacm_itf_t itf, uint8_t *out_buf, size_t out_buf_sz, size_t *rx_data_size);

/*
void rx_task(void* data) {
    size_t rx_size = 0;

  while (true) {
    esp_err_t ret = tinyusb_cdcacm_read(TINYUSB_CDC_ACM_0, buf, CONFIG_TINYUSB_CDC_RX_BUFSIZE, &rx_size);
    if (ret == ESP_OK) {
      ESP_LOGI(TAG, "Data from channel %d:", TINYUSB_CDC_ACM_0);
      ESP_LOG_BUFFER_HEXDUMP(TAG, buf, rx_size, ESP_LOG_INFO);
    } else {
      ESP_LOGE(TAG, "Read error");
    }

    // write back
    tinyusb_cdcacm_write_queue(TINYUSB_CDC_ACM_0, buf, rx_size);
    tinyusb_cdcacm_write_flush(TINYUSB_CDC_ACM_0, 0);
  }
}
 */


void tinyusb_cdc_line_state_changed_callback(int itf, cdcacm_event_t *event) {
  int dtr = event->line_state_changed_data.dtr;
  int rts = event->line_state_changed_data.rts;
  ESP_LOGI(TAG, "Line state changed on channel %d: DTR:%d, RTS:%d", itf, dtr, rts);
}

/*
_Noreturn static void tx_task(void *arg) {
  char* p = "hello world\r\n";
  while (true) {
    tinyusb_cdcacm_write_queue(TINYUSB_CDC_ACM_0, (uint8_t *) p, strlen(p));
    tinyusb_cdcacm_write_flush(TINYUSB_CDC_ACM_0, 0);
    ESP_LOGI(TAG, "Sent %s", p);
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}
 */

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "USB initialization");
    const tinyusb_config_t tusb_cfg = {
        .device_descriptor = &cdc_device_descriptor,
        .string_descriptor = string_desc_arr,
        .external_phy = false,
        .configuration_descriptor = NULL,
    };

    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

    tinyusb_config_cdcacm_t acm_cfg = {
        .usb_dev = TINYUSB_USBDEV_0,
        .cdc_port = TINYUSB_CDC_ACM_0,
        .callback_rx = NULL, // &tinyusb_cdc_rx_callback, // the first way to register a callback
        .callback_rx_wanted_char = NULL,
        .callback_line_state_changed = NULL,
        .callback_line_coding_changed = NULL
    };

    ESP_ERROR_CHECK(tusb_cdc_acm_init(&acm_cfg));
    /* the second way to register a callback */
    ESP_ERROR_CHECK(tinyusb_cdcacm_register_callback(
                        TINYUSB_CDC_ACM_0,
                        CDC_EVENT_LINE_STATE_CHANGED,
                        &tinyusb_cdc_line_state_changed_callback));

    ESP_LOGI(TAG, "USB initialization DONE");

    /*
    xTaskCreate(rx_task,
                "rx_task",
                1024 * 3,
                NULL,
                configMAX_PRIORITIES - 1,
                NULL);

    std::thread txt([]() {
      tx_task(nullptr);
    });
    txt.join();
     */

  ESP32SerialLink link;
  airball::Telemetry t(&link);

  while (true) {
    // vTaskDelay( pdMS_TO_TICKS( 100 ) );
    airball::Telemetry::Message m = t.recv();
  }
}
