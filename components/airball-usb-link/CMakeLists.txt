idf_component_register(
    SRCS "./airball-usb-link/src/Base16Encoding.cpp" "./airball-usb-link/src/Telemetry.cpp"
    INCLUDE_DIRS "./airball-usb-link/src"
    REQUIRES driver)
