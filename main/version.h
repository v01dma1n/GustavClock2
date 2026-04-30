#pragma once

#define APP_NAME   "Gustav VFD Clock"
#define APP_AUTHOR "v01dma1n"
#define APP_DATE   "2026-04-29"

#define VER_MAJOR 2
#define VER_MINOR 0
#define VER_BUILD 0

#define APP_GREETING "GUSTAV VFD CLOCK"
#define APP_MESSAGE  "The clock connects to WiFi. Hold BOOT for 3 s for Access Point."

#include <string>

#define VERSION_STRING \
    (std::to_string(VER_MAJOR) + "." + std::to_string(VER_MINOR) + "." + std::to_string(VER_BUILD))
