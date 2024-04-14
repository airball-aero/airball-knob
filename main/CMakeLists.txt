idf_component_register(
  SRCS "main.cpp" "ESP32SerialLink.cpp" "CanBus.cpp" "PushEncoderKnob.cpp" "LeaderSelectSense.cpp"
  INCLUDE_DIRS "."
  REQUIRES driver airball-usb-link)
