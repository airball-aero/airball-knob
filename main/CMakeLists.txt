idf_component_register(
  SRCS "main.cpp" "ESP32SerialLink.cpp" "CanBus.cpp" "PushEncoderKnob.cpp"
  INCLUDE_DIRS "."
  REQUIRES driver airball-usb-link)
