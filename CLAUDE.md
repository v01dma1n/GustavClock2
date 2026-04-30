# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Flash

```bash
idf.py set-target esp32          # first-time only
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

`sdkconfig.defaults` is applied automatically; no manual `menuconfig` needed for a normal build.

## Architecture

GustavClock2 is an ESP-IDF v5 / C++17 project for a 10-digit VFD clock on an ESP32-WROOM board.

### Component layout

```
main/                        — app-specific glue (GustavApp singleton, scene playlist, preferences)
components/
  esp32_ntp_clock/           — submodule: FSM engine, WiFi/NTP/weather/animation framework
  esp32_ntp_clock_drivers/   — submodule: MAX6921 VFD driver, DS1307 RTC driver, PT6315 driver
```

Both `components/` subdirectories are git submodules (upstream: v01dma1n/ESP32NTPClock2 and v01dma1n/ESP32NTPClockDrivers2). Changes to shared behavior belong in those repos; changes specific to this clock belong in `main/`.

### Task / core layout

`app_main` (main.cpp) pins work across both cores deliberately:

| Task | Core | Purpose |
|------|------|---------|
| AppTask | 0 | `setup()` / `loop()`: WiFi, NTP, weather — may block |
| DisplayTask | 1 | `dm.update()` at 50 Hz — animation frame ticks |
| RefreshTask | 1 | `display.writeNextDigit()` at ~100 Hz — MAX6921 digit multiplexing |

Display tasks on core 1 are isolated from WiFi/NTP work on core 0 so a slow network operation never causes VFD flicker.

### Display multiplexing

MAX6921 has no internal multiplexing — the host cycles all 10 grid lines externally. RefreshTask loops every ~10 ms and advances one digit per call; each digit gets ~1 ms of on-time, giving 10 Hz effective per-digit refresh (perceived as flicker-free). `vfd_hardware_map.h` defines the 20-bit SPI words for each grid and segment.

### Scene playlist (gustav_app.cpp `setup()`)

Eight scenes cycle: Date → Time → Date → Time → Temperature → Time → Humidity → Time. Each scene pairs a content getter (e.g., `getTempData()`) with an animation type (SLOT_MACHINE for time, MATRIX for date/weather). Temperature is always fetched in °C from `weather_client` and converted to °F here if `tempUnit == "F"`.

### Config / NVS

`GustavPreferences` (extends `BaseConfig`) stores: `startup_anim`, `owm_api_key`, `owm_city`, `temp_unit`. These are set via the captive-portal web form served by `GustavAccessPointManager` at 192.168.4.1 when in AP mode.

AP mode is entered by double-resetting the board within ~3 s (BootManager uses RTC memory for detection). GPIO 0 is the MAX6921 BLANK pin and cannot be used as a button, so there is no physical AP-trigger button.

### Key hardware pins (vfd_hardware_map.h)

| Pin | Function |
|-----|----------|
| 18/19/23/5 | VSPI SCLK/MISO/MOSI/SS |
| 0 | MAX6921 BLANK (LOW = display on) |
| 21/22 | I2C SDA/SCL (DS1307 RTC — supported but not currently installed) |
| 2 | WiFi activity LED (ESP32-WROOM blue) |

### DS1307 RTC

Driver is wired in and initialized in `setupHardware()`. If the oscillator is running it seeds the system clock at boot, providing resilience during WiFi outages. The chip is not currently installed on this board.
