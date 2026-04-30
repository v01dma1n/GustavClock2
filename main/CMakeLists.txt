# GustavClock2 main component.

idf_component_register(
    SRCS
        "main.cpp"
        "gustav_app.cpp"
        "gustav_preferences.cpp"
        "gustav_access_point_manager.cpp"
        "gustav_weather_manager.cpp"
    INCLUDE_DIRS
        "."
    REQUIRES
        esp32_ntp_clock
        esp32_ntp_clock_drivers
        nvs_flash
        driver
)

target_compile_options(${COMPONENT_LIB} PUBLIC -std=gnu++17)
