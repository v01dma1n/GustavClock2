#include "gustav_weather_manager.h"
#include "gustav_app.h"
#include "logging.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

GustavWeatherManager::GustavWeatherManager(GustavApp& app)
    : _app(app), _data{}, _lastFetchMs(-(FETCH_INTERVAL_MS))
{}

void GustavWeatherManager::update() {
    if (!_app.isOkToRunScenes()) return;

    int64_t nowMs = (int64_t)xTaskGetTickCount() * portTICK_PERIOD_MS;
    if (nowMs - _lastFetchMs < FETCH_INTERVAL_MS) return;
    _lastFetchMs = nowMs;

    const GustavConfig& cfg = _app.getPrefs().config;
    if (cfg.owm_api_key[0] == '\0' || cfg.owm_city[0] == '\0') {
        LOGINF("OWM not configured — skipping weather fetch");
        return;
    }

    LOGINF("Fetching weather for: %s", cfg.owm_city);
    _data = fetchWeather(cfg.owm_api_key, cfg.owm_city);
}
