// gustav_app.cpp — GustavClock2 application implementation.

#include "gustav_app.h"
#include "version.h"
#include "vfd_hardware_map.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "sntp_client.h"
#include "weather_client.h"

#include <cstdio>
#include <cstring>
#include <ctime>

// --- WiFi activity LED -------------------------------------------------------

static esp_timer_handle_t s_ledOffTimer = nullptr;

static void ledOffCallback(void*) {
    gpio_set_level(static_cast<gpio_num_t>(LED_GPIO), 0);
}

static void ledWifiEventHandler(void*, esp_event_base_t, int32_t, void*) {
    if (LED_GPIO < 0 || !s_ledOffTimer) return;
    gpio_set_level(static_cast<gpio_num_t>(LED_GPIO), 1);
    esp_timer_stop(s_ledOffTimer);
    esp_timer_start_once(s_ledOffTimer, 80 * 1000ULL);
}

// --- Scene playlist data getters --------------------------------------------

static float gustav_getTimeData()     { return UNSET_VALUE; }
static float gustav_getTempData()     { return GustavApp::getInstance().getTempData(); }
static float gustav_getHumidityData() { return GustavApp::getInstance().getHumidityData(); }

// Scene playlist for the 10-digit VFD.
// Matches the original GustavClock sequence: date / time / temp / humidity.
static const DisplayScene s_scenePlaylist[] = {
    { "Date",        "%Y-%m-%d",     MATRIX,       true,  false, 10000, 350,  50, &gustav_getTimeData     },
    { "Time",        " %k. %M.  %S", SLOT_MACHINE, true,  true,  10000, 200,  50, &gustav_getTimeData     },
    { "Date",        "%b.%d %Y",     MATRIX,       true,  false, 10000, 350,  50, &gustav_getTimeData     },
    { "Time",        " %k. %M.  %S", SLOT_MACHINE, true,  true,  10000, 200,  50, &gustav_getTimeData     },
    { "Temperature", "T %3.0f F",    MATRIX,       false, false,  7000, 250,  40, &gustav_getTempData     },
    { "Time",        " %k. %M.  %S", SLOT_MACHINE, true,  true,  10000, 200,  50, &gustav_getTimeData     },
    { "Humidity",    "H %2.0f PCT",  MATRIX,       false, false,  7000, 250,  40, &gustav_getHumidityData },
    { "Time",        " %k. %M.  %S", SLOT_MACHINE, true,  true,  10000, 200,  50, &gustav_getTimeData     },
};
static const int s_numScenes = static_cast<int>(sizeof(s_scenePlaylist) / sizeof(s_scenePlaylist[0]));

// --- Singleton ---------------------------------------------------------------

GustavApp& GustavApp::getInstance() {
    static GustavApp instance;
    return instance;
}

GustavApp::~GustavApp() = default;

GustavApp::GustavApp()
    : _display(VSPI_SCLK, VSPI_MISO, VSPI_MOSI, VSPI_SS, VSPI_BLANK,
               VFD_GRIDS, VFD_DIGIT_COUNT, VFD_SEGMENT_MAP, VFD_SPI_HOST),
      _appPrefs(),
      _apManagerConcrete(_appPrefs),
      _rtc(RTC_I2C_PORT, RTC_I2C_SDA, RTC_I2C_SCL, RTC_I2C_ADDR)
{
    _displayManager = std::make_unique<DisplayManager>(_display);
    _prefs          = &_appPrefs;
    _apManager      = &_apManagerConcrete;
}

// --- Hardware & lifecycle ----------------------------------------------------

void GustavApp::setupHardware() {
    // AP trigger (BOOT button, hold 3 s).
    if (AP_TRIGGER_GPIO >= 0) {
        gpio_config_t io = {};
        io.pin_bit_mask = 1ULL << AP_TRIGGER_GPIO;
        io.mode         = GPIO_MODE_INPUT;
        io.pull_up_en   = GPIO_PULLUP_ENABLE;
        io.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io.intr_type    = GPIO_INTR_DISABLE;
        gpio_config(&io);
        LOGINF("AP trigger on GPIO %d (hold 3 s for AP mode)", AP_TRIGGER_GPIO);
    }

    // WiFi activity LED.
    if (LED_GPIO >= 0) {
        gpio_config_t io = {};
        io.pin_bit_mask = 1ULL << LED_GPIO;
        io.mode         = GPIO_MODE_OUTPUT;
        io.pull_up_en   = GPIO_PULLUP_DISABLE;
        io.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io.intr_type    = GPIO_INTR_DISABLE;
        gpio_config(&io);
        gpio_set_level(static_cast<gpio_num_t>(LED_GPIO), 0);

        esp_timer_create_args_t ta = {};
        ta.callback = ledOffCallback;
        ta.name     = "led_off";
        esp_timer_create(&ta, &s_ledOffTimer);

        esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, ledWifiEventHandler, nullptr);
        esp_event_handler_register(IP_EVENT,   ESP_EVENT_ANY_ID, ledWifiEventHandler, nullptr);
        LOGINF("WiFi activity LED on GPIO %d", LED_GPIO);
    }

    // DS1307 RTC — seed system clock from battery-backed time if available.
    _rtcPresent = _rtc.init();
    if (_rtcPresent && _rtc.isRunning()) {
        time_t t = _rtc.readTime();
        if (t != (time_t)-1) {
            struct timeval tv = { .tv_sec = t, .tv_usec = 0 };
            settimeofday(&tv, nullptr);
            _rtcAvailable = true;
            LOGINF("System clock seeded from DS1307");
        }
    }

    // VFD display init + optional startup animation.
    _displayManager->begin();

    if (_appPrefs.config.showStartupAnimation) {
        _displayManager->setAnimation(
            std::make_unique<ScrollingTextAnimation>(APP_GREETING, 150, false));
    } else {
        _displayManager->setAnimation(
            std::make_unique<StaticTextAnimation>(APP_GREETING));
    }
}

void GustavApp::setup() {
    BaseNtpClockApp::setup();

    _weatherManager = std::make_unique<GustavWeatherManager>(*this);

    if (_sceneManager) _sceneManager->setup(s_scenePlaylist, s_numScenes);

    LOGINF("GustavClock2 ready");
}

void GustavApp::loop() {
    BaseNtpClockApp::loop();

    // AP trigger: hold BOOT button for 3 s to enter AP mode from normal run.
    if (AP_TRIGGER_GPIO >= 0 && _fsmManager && !_fsmManager->isInState("AP_MODE")) {
        if (gpio_get_level(static_cast<gpio_num_t>(AP_TRIGGER_GPIO)) == 0) {
            if (_apTriggerHeldSinceUs == 0)
                _apTriggerHeldSinceUs = esp_timer_get_time();
            else if (esp_timer_get_time() - _apTriggerHeldSinceUs >= 3'000'000) {
                LOGINF("AP trigger held 3 s — requesting AP mode");
                _apTriggerHeldSinceUs = 0;
                _fsmManager->requestApMode();
            }
        } else {
            _apTriggerHeldSinceUs = 0;
        }
    }

    // Weather polling (15 min, only when configured and WiFi is up).
    if (_weatherManager) _weatherManager->update();

    // Write NTP time back to DS1307 once on first sync.
    if (_rtcPresent && !_rtcSynced && timeAvail) {
        _rtc.writeTime(time(nullptr));
        _rtcSynced = true;
        LOGINF("DS1307 synchronized with NTP time");
    }
}

// --- IBaseClock implementations ----------------------------------------------

bool GustavApp::isOkToRunScenes() const {
    return _fsmManager && _fsmManager->isInState("RUNNING_NORMAL");
}

void GustavApp::formatTime(char* txt, unsigned txt_size,
                           const char* format, time_t now) {
    struct tm ti;
    localtime_r(&now, &ti);
    strftime(txt, txt_size, format, &ti);
}

void GustavApp::activateAccessPoint() {
    _apManagerConcrete.setup(GUSTAV_HOST_NAME);

    char waiting[64];
    snprintf(waiting, sizeof(waiting), "SETUP MODE - JOIN %s", GUSTAV_HOST_NAME);
    char connected[] = "GOTO 192.168.4.1";

    _displayManager->setAnimation(
        std::make_unique<ScrollingTextAnimation>(waiting, 180, false));
    _apManagerConcrete.runBlockingLoop(*_displayManager, waiting, connected);
}

// --- Weather data getters ----------------------------------------------------

float GustavApp::getTempData() const {
    if (!_weatherManager) return UNSET_VALUE;
    const WeatherData& w = _weatherManager->getWeatherData();
    if (!w.valid) return UNSET_VALUE;
    // Convert to Celsius if configured; weather_client always returns °F.
    if (_appPrefs.config.tempUnit[0] == 'C') {
        return (w.tempF - 32.0f) * 5.0f / 9.0f;
    }
    return w.tempF;
}

float GustavApp::getHumidityData() const {
    if (!_weatherManager) return UNSET_VALUE;
    const WeatherData& w = _weatherManager->getWeatherData();
    return w.valid ? static_cast<float>(w.humidity) : UNSET_VALUE;
}
