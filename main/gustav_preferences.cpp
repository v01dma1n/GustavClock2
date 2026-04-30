#include "gustav_preferences.h"

#include <cstring>

static constexpr const char* KEY_SHOW_STARTUP = "startup_anim";
static constexpr const char* KEY_OWM_KEY      = "owm_api_key";
static constexpr const char* KEY_OWM_CITY     = "owm_city";
static constexpr const char* KEY_TEMP_UNIT    = "temp_unit";

GustavPreferences::GustavPreferences()
    : BasePreferences(config)
{
    std::memset(&config, 0, sizeof(config));
    config.showStartupAnimation = true;
    std::strncpy(config.tempUnit, "F", sizeof(config.tempUnit) - 1);
}

void GustavPreferences::getPreferences() {
    BasePreferences::getPreferences();

    if (!openNvs(false)) return;

    config.showStartupAnimation = readBool(KEY_SHOW_STARTUP, true);
    readString(KEY_OWM_KEY,   config.owm_api_key, sizeof(config.owm_api_key));
    readString(KEY_OWM_CITY,  config.owm_city,    sizeof(config.owm_city));
    readString(KEY_TEMP_UNIT, config.tempUnit,     sizeof(config.tempUnit));
    if (config.tempUnit[0] == '\0') {
        std::strncpy(config.tempUnit, "F", sizeof(config.tempUnit) - 1);
    }

    closeNvs();
}

void GustavPreferences::putPreferences() {
    BasePreferences::putPreferences();

    if (!openNvs(true)) return;

    writeBool  (KEY_SHOW_STARTUP, config.showStartupAnimation);
    writeString(KEY_OWM_KEY,      config.owm_api_key);
    writeString(KEY_OWM_CITY,     config.owm_city);
    writeString(KEY_TEMP_UNIT,    config.tempUnit);

    closeNvs();
}

void GustavPreferences::dumpPreferences() {
    BasePreferences::dumpPreferences();
    LOGDBG("Pref=%s: %s", KEY_SHOW_STARTUP, config.showStartupAnimation ? "yes" : "no");
    LOGDBG("Pref=%s: %s", KEY_OWM_KEY,      config.owm_api_key[0] ? "(set)" : "(empty)");
    LOGDBG("Pref=%s: %s", KEY_OWM_CITY,     config.owm_city);
    LOGDBG("Pref=%s: %s", KEY_TEMP_UNIT,    config.tempUnit);
}
