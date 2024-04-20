#include <esp_log.h>
#include "ESP32SerialLink.h"
#include "Telemetry.h"
#include "CanBus.h"
#include "PushEncoderKnob.h"
#include "LeaderSelectSense.h"

class App {
public:
  App()
      : host_link_(ESP32SerialLink::instance()),
        can_bus_(),
        knob_(),
        leader_select_()
  {}

  void run() { mainLoop(); }

private:
  [[noreturn]] void mainLoop() {
    airball::Telemetry::Message m {};
    while (true) {
      while (host_link_.recv(&m)) {
        processMessageFromHost(m);
      }
      while (can_bus_.recv(&m)) {
        processMessageFromCanBus(m);
      }
      if (knob_.evaluateEncoderStep(&m)) {
        host_link_.send(m);
        can_bus_.send(m);
      }
      if (knob_.evaluateButtonTransition(&m)) {
        host_link_.send(m);
        can_bus_.send(m);
      }
    }
  }

  void processMessageFromHost(airball::Telemetry::Message incoming) {
    switch (incoming.domain) {
      case airball::Telemetry::kMessageDomainCanBus:
        can_bus_.send(incoming);
        break;
      case airball::Telemetry::kMessageDomainLocal:
        switch (incoming.id) {
          case airball::Telemetry::kLocalMessageIdIdentifyLeaderFollower: {
            airball::Telemetry::Message m = { 0 };
            leader_select_.reportLeaderSelect(&m);
            host_link_.send(m);
            break;
          }
          default:
            break;
        }
        break;
      default:
        break;
    }
  }

  void processMessageFromCanBus(airball::Telemetry::Message incoming) {
    switch (incoming.domain) {
      case airball::Telemetry::kMessageDomainCanBus:
        host_link_.send(incoming);
        break;
      default:
        break;
    }
  }

  airball::Telemetry host_link_;
  CanBus can_bus_;
  PushEncoderKnob knob_;
  LeaderSelectSense leader_select_;
};

extern "C" void app_main(void) {
  App app;
  app.run();
}
