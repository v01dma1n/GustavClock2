# ESP32 NTP Clock drivers component for GustavClock2.
#
# Contains the MAX6921 VFD driver (SPI-multiplexed, IDF port) and the
# DS1307 RTC driver (I2C, shared with MoodWhisperer).

idf_component_register(
    SRCS
        "src/disp_driver_max6921.cpp"
        "src/ds1307_driver.cpp"
    INCLUDE_DIRS
        "include"
    REQUIRES
        driver
        esp32_ntp_clock
        esp_rom
)

target_compile_options(${COMPONENT_LIB} PUBLIC -std=gnu++17)
