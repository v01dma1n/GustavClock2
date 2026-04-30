// gustav_weather_manager.h — polls OpenWeatherMap every 15 minutes.

#pragma once

#include "weather_client.h"

#include <cstdint>

class GustavApp;

class GustavWeatherManager {
public:
    explicit GustavWeatherManager(GustavApp& app);
    void update();

    WeatherData getWeatherData() const { return _data; }

private:
    GustavApp&  _app;
    WeatherData _data;
    int64_t     _lastFetchMs;
    static constexpr int64_t FETCH_INTERVAL_MS = 15LL * 60 * 1000;
};
