#include "gustav_access_point_manager.h"

static const PrefSelectOption s_tempUnitOptions[] = {
    { "Fahrenheit", "F" },
    { "Celsius",    "C" },
};
static const int s_numTempUnitOptions =
    static_cast<int>(sizeof(s_tempUnitOptions) / sizeof(s_tempUnitOptions[0]));

void GustavAccessPointManager::initializeFormFields() {
    BaseAccessPointManager::initializeFormFields();

    auto& cfg = static_cast<GustavPreferences&>(_prefs).config;

    _formFields.push_back(FormField{
        "startup_anim", "Show Startup Animation", false, VALIDATION_NONE,
        PREF_BOOL, { .bool_pref = &cfg.showStartupAnimation },
        nullptr, 0,
    });

    _formFields.push_back(FormField{
        "owm_city", "OWM City (e.g. Chicago,IL,US)", false, VALIDATION_NONE,
        PREF_STRING, { .str_pref = cfg.owm_city }, nullptr, 0,
    });

    _formFields.push_back(FormField{
        "owm_api_key", "OWM API Key", true, VALIDATION_NONE,
        PREF_STRING, { .str_pref = cfg.owm_api_key }, nullptr, 0,
    });

    _formFields.push_back(FormField{
        "temp_unit", "Temperature Unit", false, VALIDATION_NONE,
        PREF_SELECT, { .str_pref = cfg.tempUnit },
        s_tempUnitOptions, s_numTempUnitOptions,
    });
}
