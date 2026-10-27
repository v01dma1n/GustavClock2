// main.cpp — ESP-IDF entry point for GustavClock2.
//
// Task layout:
//   Core 0 "AppTask"      — setup() + loop(): WiFi, NTP, portal, scene FSM.
//   Core 1 "DisplayTask"  — dm.update() at 50 Hz (animation ticks).
//   Core 1 "RefreshTask"  — writeNextDigit() at 1 ms (MAX6921 multiplexing).

#include "gustav_app.h"
#include "logging.h"

#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static constexpr int APP_TASK_STACK = 12288;
static constexpr int APP_TASK_PRIO  = 5;
static constexpr int APP_TASK_CORE  = 0;

static void logTaskWatermarks() {
    static char buf[800];
    vTaskList(buf);
    LOGINF("--- task watermarks (name / state / prio / free_stack_words / num) ---");
    char* line = buf;
    for (char* p = buf; ; ++p) {
        if (*p == '\n' || *p == '\0') {
            bool done = (*p == '\0');
            *p = '\0';
            if (*line) LOGINF("  %s", line);
            if (done) break;
            line = p + 1;
        }
    }
    LOGINF("--- end watermarks ---");
}

// Drives animation ticks at 50 Hz on Core 1 so AppTask can block in
// WiFi/NTP without stalling the glass.
static void displayTask(void* /*pvParameters*/) {
    DisplayManager& dm = GustavApp::getInstance().getClock();
    for (;;) {
        dm.update();
        vTaskDelay(pdMS_TO_TICKS(20));  // 50 Hz
    }
}

// Drives MAX6921 digit multiplexing at ~100 Hz.
// Writes all digits back-to-back in a tight loop — each writeNextDigit() call
// uses esp_rom_delay_us() internally so every digit gets the same on-time
// regardless of FreeRTOS tick granularity. The display is left blanked after
// the loop, so task scheduling only affects the (dark) inter-frame gap.
static void refreshTask(void* /*pvParameters*/) {
    IDisplayDriver& display = GustavApp::getInstance().getDisplay();
    const int n = display.getDisplaySize();
    TickType_t xLastWakeTime = xTaskGetTickCount();
    for (;;) {
        for (int i = 0; i < n; i++) {
            display.writeNextDigit();
        }
        // Display is blanked here. Yield until start of next 10 ms frame so
        // DisplayTask can update the frame buffer between frames.
        xTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10));
    }
}

static void appTask(void* /*pvParameters*/) {
    auto& app = GustavApp::getInstance();
    app.setup();

    xTaskCreatePinnedToCore(
        displayTask, "DisplayTask",
        4096, nullptr, 6, nullptr, /*core=*/1);

    // MAX6921 needs continuous multiplexing.
    xTaskCreatePinnedToCore(
        refreshTask, "RefreshTask",
        4096, nullptr, 10, nullptr, /*core=*/1);

    logTaskWatermarks();

    static TickType_t s_lastDump = 0;
    for (;;) {
        app.loop();

        TickType_t now = xTaskGetTickCount();
        if (now - s_lastDump >= pdMS_TO_TICKS(30000)) {
            s_lastDump = now;
            logTaskWatermarks();
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

extern "C" void app_main(void) {
    LOGINF(">>> GustavClock2 booting");

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
        err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    } else {
        ESP_ERROR_CHECK(err);
    }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    xTaskCreatePinnedToCore(
        appTask, "AppTask",
        APP_TASK_STACK, nullptr, APP_TASK_PRIO, nullptr, APP_TASK_CORE);

    LOGINF(">>> tasks running; app_main returning to idle");
}
