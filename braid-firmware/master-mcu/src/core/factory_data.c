#include "esp_mac.h"
#include <esp_log.h>
#include <string.h>

#include "factory_data.h"
#include "hal/flash.h"

#define MAC_ADDR_MAGIC_STR "__MAC_ADDR__"

static const char *TAG = "core/factory_data";
static const char *KEY = "factory";

esp_err_t FactoryData_Validate(FactoryData *data) {
    if (strcmp(MAC_ADDR_MAGIC_STR, data->deviceId) == 0) {
        ESP_LOGD(TAG, "MAC address magic string detected");

        uint8_t mac[6];
        esp_efuse_mac_get_default(mac);

        // Format the MAC address as a string
        snprintf(data->deviceId,
                 sizeof(data->deviceId),
                 "%02x:%02x:%02x:%02x:%02x:%02x",
                 mac[0],
                 mac[1],
                 mac[2],
                 mac[3],
                 mac[4],
                 mac[5]);
    }

    return ESP_OK;
}

esp_err_t FactoryData_Load(FactoryData *data) {
    esp_err_t status = ESP_OK;
    status = Flash_Load(PARTITION_FACTORY, KEY, data, FACTORY_DATA_SIZE);
    // If data was erased/not found, use some sane defaults
    if (status == ESP_ERR_NVS_NOT_FOUND) {
        memset(data->key, 0, HMAC_KEY_LENGTH);
    } else if (status != ESP_OK) {
        ESP_LOGD(TAG, "Failed loading factory data, error 0x%04x", status);
    }
    return status;
}

esp_err_t FactoryData_Erase() {
    esp_err_t status = ESP_OK;
    status = Flash_Erase(PARTITION_FACTORY, KEY);
    if (status != ESP_OK) {
        ESP_LOGD(TAG, "Failed loading factory data, error 0x%04x", status);
    }
    return status;
}