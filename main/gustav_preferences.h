// gustav_preferences.h — application-specific NVS preferences for GustavClock2.
//
// Extends BaseConfig with weather and display preferences.

#pragma once

#include "ESP32NTPClock.h"

struct GustavConfig : public BaseConfig {
    bool showStartupAnimation;
    char owm_api_key[MAX_PREF_STRING_LEN];
    // City query string passed to OpenWeatherMap, e.g. "Chicago,IL,US".
    char owm_city[MAX_PREF_STRING_LEN];
    // "F" for Fahrenheit (default), "C" for Celsius. The IDF weather client
    // always returns Fahrenheit; C is derived by converting on display.
    char tempUnit[MAX_PREF_STRING_LEN];
};

class GustavPreferences : public BasePreferences {
public:
    GustavPreferences();

    void getPreferences() override;
    void putPreferences() override;
    void dumpPreferences() override;

    GustavConfig config;
};
