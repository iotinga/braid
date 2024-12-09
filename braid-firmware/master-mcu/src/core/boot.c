#include "esp_err.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "boot.h"
#include "core/time.h"
#include "hal/flash.h"
#include "hal/modem.h"

static const char *TAG = "core/boot";

static bool shutdownPending = false;
static int shutdownCount = 0;
SemaphoreHandle_t countMutex = NULL;

// Mode durations in seconds
static int durations[BOOT_MODE_MAX] = {
    INT_MAX,       // BOOT_FACTORY_DATA
    MIN_TO_SEC(5), // BOOT_GPS
    MIN_TO_SEC(1), // BOOT_MQTT
};
SemaphoreHandle_t durationsMutex = NULL;

void Boot_Init() {
    countMutex = xSemaphoreCreateMutex();
    assert(countMutex);

    durationsMutex = xSemaphoreCreateMutex();
    assert(durationsMutex);
}

void Boot_To(Boot_Mode mode) {
    ESP_LOGD(TAG, "Received boot request for mode %d", mode);
    ESP_ERROR_CHECK(Flash_Save(PARTITION_FACTORY, "boot_mode", &mode, sizeof(Boot_Mode)));
    esp_restart();
}

void Boot_GetCurrentMode(Boot_Mode *mode) {
    esp_err_t err = Flash_Load(PARTITION_FACTORY, "boot_mode", mode, sizeof(Boot_Mode));
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        *mode = BOOT_NO_FACTORY_DATA;
    }
    ESP_LOGD(TAG, "Flash contains mode %d", *mode);
}

int Boot_RegisterTask() {
    if (xSemaphoreTake(countMutex, portMAX_DELAY) == pdTRUE) {
        ESP_LOGD(TAG, "Registered task for graceful shutdown");
        shutdownCount++;
        xSemaphoreGive(countMutex);
    }

    return shutdownCount;
}

void Boot_SetShutdownPending(bool pending) {
    shutdownPending = pending;
}

bool Boot_IsShutdownPending() {
    return shutdownPending;
}

bool Boot_IsShutdownReady() {
    return shutdownCount == 0;
}

void Boot_SetShutdownReady() {
    assert(countMutex);

    if (xSemaphoreTake(countMutex, portMAX_DELAY) == pdTRUE) {
        if (shutdownCount > 0) {
            ESP_LOGD(TAG, "Task is ready for shutdown");
            shutdownCount--;
        }

        xSemaphoreGive(countMutex);
    }
}

int Boot_GetDuration(Boot_Mode mode) {
    esp_err_t err = Flash_Load(PARTITION_FACTORY, "boot_mode_dur", durations, sizeof(durations));
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        err = Flash_Save(PARTITION_FACTORY, "boot_mode_dur", durations, sizeof(durations));
    }

    return durations[mode];
}

void Boot_SetDuration(Boot_Mode mode, int seconds) {
    assert(durationsMutex);

    if (xSemaphoreTake(durationsMutex, portMAX_DELAY) == pdTRUE) {
        durations[mode] = seconds;
        xSemaphoreGive(durationsMutex);
    }

    ESP_ERROR_CHECK(Flash_Save(PARTITION_FACTORY, "boot_mode_dur", durations, sizeof(durations)));
}