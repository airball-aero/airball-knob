#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ESP32SerialLink.h"
#include "Telemetry.h"
#include "CanBus.h"
#include "PushEncoderKnob.h"

class App {
public:
  App()
      : host_link_(ESP32SerialLink::instance()) {}

  void run() { mainLoop(); }

private:
  [[noreturn]] void mainLoop() {
    airball::Telemetry::Message m {};
    while (true) {
      bool work = false;
      while (host_link_.recv(&m)) {
        work = true;
        processMessageFromHost(m);
      }
      while (can_bus_.recv(&m)) {
        work = true;
        processMessageFromCanBus(m);
      }
      if (knob_.evaluateEncoderStep(&m)) {
        work = true;
        host_link_.send(m);
      }
      if (knob_.evaluateButtonTransition(&m)) {
        work = true;
        host_link_.send(m);
      }
      if (!work) {
        vTaskDelay(pdMS_TO_TICKS(1));
      }
    }
  }

  void processMessageFromHost(airball::Telemetry::Message m) {
    switch (m.domain) {
      case airball::Telemetry::kMessageDomainCanBus:
        can_bus_.send(m);
        break;
      default:
        break;
    }
  }

  void processMessageFromCanBus(airball::Telemetry::Message m) {
    switch (m.domain) {
      case airball::Telemetry::kMessageDomainCanBus:
        host_link_.send(m);
        break;
      default:
        break;
    }
  }

  airball::Telemetry host_link_;
  CanBus can_bus_;
  PushEncoderKnob knob_;
};

extern "C" void app_main(void) {
  App app;
  app.run();
}
