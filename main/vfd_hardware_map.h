// vfd_hardware_map.h — GPIO + SPI pin assignments for the GustavClock2 hardware.
//
// The GustavClock uses a MAX6921 VFD driver chip over VSPI (SPI3_HOST).
// 10-digit VFD with each digit multiplexed via a grid-select line.
// The segment lines and grid lines are each routed through one MAX6921 output.
//
// Change here if porting to a different ESP32 board or wiring choice.

#pragma once

#include "disp_driver_max6921.h"
#include "driver/spi_common.h"
#include "driver/i2c.h"

// --- SPI pins (VSPI = SPI3_HOST on classic ESP32) ----------------------------
constexpr int VSPI_SCLK  = 18;
constexpr int VSPI_MISO  = 19;
constexpr int VSPI_MOSI  = 23;
constexpr int VSPI_SS    =  5;
constexpr int VSPI_BLANK =  0;

constexpr spi_host_device_t VFD_SPI_HOST = SPI3_HOST;

// --- Grid (digit) pin mapping ------------------------------------------------
// Each entry is a 20-bit mask that activates one grid line through MAX6921.
static const unsigned long VFD_GRIDS[] = {
    0b00000000010000000000, // GRD_01 (Digit 0)
    0b00000000100000000000, // GRD_02 (Digit 1)
    0b00000010000000000000, // GRD_03 (Digit 2)
    0b00010000000000000000, // GRD_04 (Digit 3)
    0b10000000000000000000, // GRD_05 (Digit 4)
    0b00000000000000000010, // GRD_06 (Digit 5)
    0b00000000000000000100, // GRD_07 (Digit 6)
    0b00000000000000100000, // GRD_08 (Digit 7)
    0b00000000000100000000, // GRD_09 (Digit 8)
    0b00000000001000000000  // GRD_10 (Digit 9)
};
constexpr int VFD_DIGIT_COUNT = static_cast<int>(sizeof(VFD_GRIDS) / sizeof(VFD_GRIDS[0]));

// --- Segment bitmask mapping -------------------------------------------------
// Each field is the 20-bit MAX6921 output mask that lights that segment.
// The display combines decimal dot and comma into a single segment pair.
static const Max6921SegmentMap VFD_SEGMENT_MAP = {
    .segA   = 0b00000000000010000000,
    .segB   = 0b00000000000001000000,
    .segC   = 0b00000000000000010000,
    .segD   = 0b01000000000000000000,
    .segE   = 0b00001000000000000000,
    .segF   = 0b00000100000000000000,
    .segG   = 0b00000001000000000000,
    .segDot = 0b00100000000000001000, // combined dot+comma on this glass
};

// --- AP trigger --------------------------------------------------------------
// GPIO 0 is the BLANK line for the MAX6921 VFD driver (VSPI_BLANK above) so
// it cannot double as a button input. Use double-reset (BootManager) to enter
// AP mode instead — same mechanism as the original Arduino GustavClock.
constexpr int AP_TRIGGER_GPIO = -1;

// --- Built-in LED ------------------------------------------------------------
// GPIO 2 = blue LED on ESP32-WROOM dev board. Flashes on WiFi activity.
// Set to -1 to disable.
constexpr int LED_GPIO = 2;

// --- I2C bus (DS1307 RTC) ---------------------------------------------------
constexpr int        RTC_I2C_SDA  = 21;
constexpr int        RTC_I2C_SCL  = 22;
constexpr i2c_port_t RTC_I2C_PORT = I2C_NUM_0;
constexpr uint8_t    RTC_I2C_ADDR = 0x68;
