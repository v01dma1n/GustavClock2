#pragma once

// Current weather conditions fetched from OpenWeatherMap v2.5.
// Requires a free OWM account; the API key and city are stored in
// app preferences and passed here on each call.

struct WeatherData {
    float tempF;    // air temperature, Fahrenheit
    int   humidity; // relative humidity, percent 0-100
    bool  valid;    // false if the last fetch failed
};

// Fetch current conditions for `city` (e.g. "Warsaw,PL").
// Blocks for up to ~5 s. Returns valid=false on network or parse error.
WeatherData fetchWeather(const char* apiKey, const char* city);
