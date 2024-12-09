/* Enable debug messages */
#define ATCA_PRINTF
#include "cryptoauthlib.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include <esp_vfs_common.h>
#include <esp_vfs_dev.h>
#include <esp_vfs_usb_serial_jtag.h>

#include "cli/serial.h"
#include "cli/task.h"
#include "core/boot.h"
#include "core/time.h"
#include "hal/flash.h"
#include "hal/hal.h"
#include "main.h"

static const char *TAG = "main";

static Boot_Mode bootMode = BOOT_GPS;
ProtoCtx protoCtx;
FactoryData factoryData;

static ATCAIfaceCfg cfg_ateccx08a_i2c = {
    ATCA_I2C_IFACE, ATECC608A, CONFIG_ATCA_I2C_ADDRESS, 0, CONFIG_ATCA_I2C_BAUD_RATE, 1500, 20, NULL};

extern "C" void app_main() {
    esp_err_t status;

    // Initialize security processor
    auto ret = atcab_init(&cfg_ateccx08a_i2c);
    if (ret != ATCA_SUCCESS) {
        ESP_LOGE(TAG, "Error initializing ATCA: %d", ret);
        errorHandler();
    }

    ESP_ERROR_CHECK(Flash_Init(PARTITION_FACTORY));
    ESP_ERROR_CHECK(Flash_Init(PARTITION_USER));

    Boot_Init();
    Boot_GetCurrentMode(&bootMode);

    status = FactoryData_Load(&factoryData);
    if (status == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "Factory data not found");
        bootMode = BOOT_NO_FACTORY_DATA;
    } else {
        ESP_ERROR_CHECK(status);
        if (bootMode == BOOT_NO_FACTORY_DATA) {
            bootMode = BOOT_GPS;
        }
    }

    ESP_LOGI(TAG, "Booting in mode %d", bootMode);
    ESP_ERROR_CHECK(Hal_EarlySetup(bootMode));

    switch (bootMode) {
    case BOOT_NO_FACTORY_DATA:
        break;

    case BOOT_GPS:
        // fall through
    case BOOT_MQTT:
        protoCtx.messageCallbackCtx = &factoryData;

        ESP_ERROR_CHECK(Hal_Setup(bootMode));
    }

    xTaskCreatePinnedToCore(&task_cli, "CLI", DEFAULT_STACK_SIZE * 2u, NULL, DEFAULT_PRIORITY + 10, NULL, 0);

    switch (bootMode) {
    case BOOT_NO_FACTORY_DATA:
        break;

    case BOOT_GPS:
        // fall through
    case BOOT_MQTT:
        ESP_ERROR_CHECK(Hal_StartTasks(bootMode));
        break;
    }

    int seconds = Boot_GetDuration(bootMode);
    ESP_LOGD(TAG, "Holding mode for %d seconds", seconds);
    vTaskDelay(pdMS_TO_TICKS(SEC_TO_MS(seconds)));

    ESP_LOGD(TAG, "Shutdown pending");
    Boot_SetShutdownPending(true);

    ESP_LOGD(TAG, "Waiting for graceful shutdown");
    while (!Boot_IsShutdownReady()) {
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    if (bootMode == BOOT_GPS) {
        Boot_To(BOOT_MQTT);
    } else if (bootMode == BOOT_MQTT) {
        Boot_To(BOOT_GPS);
    }
}

void errorHandler() {
    ESP_LOGE(TAG, "Reached error state");
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}