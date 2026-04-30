// gustav_app.h — GustavClock2 application.
//
// Mirrors MoodWhisperer's WhispererApp structure: one class inheriting
// BaseNtpClockApp that owns a DispDriverMAX6921 and a DisplayManager,
// and supplies the IBaseClock implementations the engine wants.
//
// GustavClock2 specifics:
//   * 10-digit VFD driven by MAX6921 over VSPI (external multiplexing).
//   * DS1307 RTC for time resilience across WiFi outages.
//   * OpenWeatherMap weather data displayed in a scene playlist.

#pragma once

#include "ESP32NTPClock.h"

#include "disp_driver_max6921.h"
#include "ds1307_driver.h"
#include "gustav_preferences.h"
#include "gustav_access_point_manager.h"
#include "gustav_weather_manager.h"

#include <memory>

static constexpr const char* GUSTAV_HOST_NAME = "gustav-clock";

class GustavApp : public BaseNtpClockApp {
public:
    static GustavApp& getInstance();

    ~GustavApp() override;

    void setup() override;
    void loop()  override;
    void setupHardware() override;

    GustavPreferences& getPrefs() { return _appPrefs; }

    // Weather data accessors for scene playlist data getters.
    float getTempData()     const;
    float getHumidityData() const;

    // --- IBaseClock ----------------------------------------------------------
    const char* getAppName()  const override { return GUSTAV_HOST_NAME; }
    const char* getSsid()     const override { return _appPrefs.config.ssid; }
    const char* getPassword() const override { return _appPrefs.config.password; }
    const char* getTimezone() const override { return _appPrefs.config.time_zone; }

    IDisplayDriver& getDisplay() override { return _display; }
    DisplayManager& getClock()   override { return *_displayManager; }

    bool isOkToRunScenes() const override;
    bool hasRtcTime()     const override { return _rtcAvailable; }
    void activateAccessPoint() override;
    void formatTime(char* txt, unsigned txt_size,
                    const char* format, time_t now) override;

private:
    GustavApp();

    DispDriverMAX6921                _display;
    std::unique_ptr<DisplayManager>  _displayManager;

    GustavPreferences                _appPrefs;
    GustavAccessPointManager         _apManagerConcrete;

    Ds1307Driver                     _rtc;
    bool                             _rtcPresent   = false;
    bool                             _rtcAvailable = false;
    bool                             _rtcSynced    = false;

    std::unique_ptr<GustavWeatherManager> _weatherManager;

    int64_t _apTriggerHeldSinceUs = 0;
};
