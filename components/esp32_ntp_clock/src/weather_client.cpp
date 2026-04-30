#include "weather_client.h"
#include "logging.h"

#include "esp_http_client.h"
#include "cJSON.h"

#include <cstring>
#include <cstdio>

struct HttpBuf {
    char   data[1024];
    size_t len;
};

static esp_err_t httpEventHandler(esp_http_client_event_t* evt) {
    if (evt->event_id == HTTP_EVENT_ON_DATA) {
        auto* buf = static_cast<HttpBuf*>(evt->user_data);
        size_t room = sizeof(buf->data) - buf->len - 1;
        if (room > 0) {
            size_t copy = (static_cast<size_t>(evt->data_len) < room)
                          ? static_cast<size_t>(evt->data_len) : room;
            std::memcpy(buf->data + buf->len, evt->data, copy);
            buf->len += copy;
            buf->data[buf->len] = '\0';
        }
    }
    return ESP_OK;
}

static void urlEncode(char* out, size_t out_size, const char* in) {
    size_t pos = 0;
    for (; *in && pos + 3 < out_size; ++in) {
        unsigned char c = (unsigned char)*in;
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') ||
            c == '-' || c == '_' || c == '.' || c == '~' || c == ',') {
            out[pos++] = (char)c;
        } else {
            pos += std::snprintf(out + pos, out_size - pos, "%%%02X", c);
        }
    }
    out[pos] = '\0';
}

WeatherData fetchWeather(const char* apiKey, const char* city) {
    WeatherData result = {};

    if (!apiKey || !*apiKey || !city || !*city) return result;

    char cityEncoded[128];
    urlEncode(cityEncoded, sizeof(cityEncoded), city);

    char url[320];
    std::snprintf(url, sizeof(url),
        "http://api.openweathermap.org/data/2.5/weather"
        "?q=%s&appid=%s&units=imperial",
        cityEncoded, apiKey);

    HttpBuf buf = {};

    esp_http_client_config_t cfg = {};
    cfg.url           = url;
    cfg.event_handler = &httpEventHandler;
    cfg.user_data     = &buf;
    cfg.timeout_ms    = 5000;

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (!client) {
        LOGERR("weather: http_client_init failed");
        return result;
    }

    esp_err_t err = esp_http_client_perform(client);
    int status    = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    if (err != ESP_OK || status != 200 || buf.len == 0) {
        LOGERR("weather: fetch failed (err=%d status=%d)", (int)err, status);
        return result;
    }

    // Response contains: {"main":{"temp":20.5,"humidity":65,...},...}
    cJSON* root = cJSON_Parse(buf.data);
    if (!root) {
        LOGERR("weather: JSON parse failed");
        return result;
    }

    cJSON* main = cJSON_GetObjectItemCaseSensitive(root, "main");
    if (!main) {
        LOGERR("weather: 'main' field missing");
        cJSON_Delete(root);
        return result;
    }

    cJSON* temp = cJSON_GetObjectItemCaseSensitive(main, "temp");
    cJSON* hum  = cJSON_GetObjectItemCaseSensitive(main, "humidity");

    if (!cJSON_IsNumber(temp) || !cJSON_IsNumber(hum)) {
        LOGERR("weather: temp/humidity fields missing or wrong type");
        cJSON_Delete(root);
        return result;
    }

    result.tempF    = (float)temp->valuedouble;
    result.humidity = (int)hum->valuedouble;
    result.valid    = true;

    LOGINF("weather: %.1f F, %d %%RH", result.tempF, result.humidity);
    cJSON_Delete(root);
    return result;
}
