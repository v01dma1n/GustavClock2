// gustav_access_point_manager.h — adds app-specific rows to the captive portal.

#pragma once

#include "ESP32NTPClock.h"
#include "gustav_preferences.h"

class GustavAccessPointManager : public BaseAccessPointManager {
public:
    explicit GustavAccessPointManager(GustavPreferences& prefs)
        : BaseAccessPointManager(prefs) {}

protected:
    void initializeFormFields() override;
};
