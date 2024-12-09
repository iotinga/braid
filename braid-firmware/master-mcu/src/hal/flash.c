#include <esp_efuse.h>
#include <esp_efuse_table.h>
#include <esp_log.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <string.h>

#include "flash.h"

static const char *TAG = "hal/flash";

static nvs_handle_t nvsHandles[PARTITION_COUNT] = {
    [PARTITION_USER] = 0,
    [PARTITION_FACTORY] = 0,
};

static const char *nvsPartitionName[PARTITION_COUNT] = {
    [PARTITION_USER] = USER_DATA_PARTITION_NAME,
    [PARTITION_FACTORY] = FACTORY_DATA_PARTITION_NAME,
};

esp_err_t Flash_Init(FlashPartition partition) {
    esp_err_t status = ESP_OK;
    const char *name = nvsPartitionName[partition];

    status = nvs_flash_init_partition(name);
    if (status == ESP_ERR_NVS_NO_FREE_PAGES || status == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase_partition(name));
        status = nvs_flash_init_partition(name);
    }
    if (status != ESP_OK) {
        return status;
    }
    ESP_LOGD(TAG, "init flash partition %s", name);

#ifdef CONFIG_NVS_NECRYPTION
    const esp_partition_t *part =
        esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, name);
    if (part == NULL) {
        ESP_LOGE(TAG, "Partition not found");
        return ESP_ERR_NOT_FOUND;
    }

    nvs_sec_cfg_t securityCfg = {0};
    error = nvs_flash_read_security_cfg(part, &securityCfg);
    if (error != ESP_OK) {
        ESP_LOGW(TAG, "Failed to read NVS security cfg: %d. Generate new keys", error);
        error = nvs_flash_generate_keys(part, &securityCfg);
        if (error != ESP_OK) {
            ESP_LOGE(TAG, "failed to generate NVS encr-keys: %d", error);

            return error;
        }
#ifdef BURN_EFUSES
        else {
            ESP_LOGD(TAG, "created credentials. Burn read-protect efuse");
            error = esp_efuse_write_field_bit(ESP_EFUSE_WR_DIS_RD_DIS);
            if (error != ESP_OK) {
                ESP_LOGE(TAG, "error burning efuse %d", error);
            }
        }
#endif
    }

    error = nvs_flash_secure_init_partition(name, &securityCfg);
    if ((error == ESP_ERR_NVS_NO_FREE_PAGES) || (error == ESP_ERR_NVS_NEW_VERSION_FOUND)) {
        /* this is suggested from ESP documentation */
        ESP_LOGW(TAG, "partition %s was truncated, erasing...", name);
        error = nvs_flash_erase_partition(name);
        if (error != ESP_OK) {
            ESP_LOGE(TAG, "partition erase error: %d", error);
        }

        error = nvs_flash_secure_init_partition(name, &securityCfg);
        if (error != ESP_OK) {
            ESP_LOGE(TAG, "partition secure init error: %d", error);

            return ESP_FAIL;
        }
    }
#endif

    status = nvs_open_from_partition(name, FLASH_DEFAULT_NAMESPACE, NVS_READWRITE, &nvsHandles[partition]);
    if (status != ESP_OK) {
        ESP_LOGE(TAG, "partition %s open error: 0x%04x", name, status);
    }

    return status;
}

esp_err_t Flash_UnsafeSave(FlashPartition partition, const char *key, const void *data, size_t length) {
    esp_err_t status = ESP_OK;
    if (data == NULL || key == NULL) {
        status = ESP_ERR_INVALID_ARG;
    }

    if (status == ESP_OK) {
        status = nvs_set_blob(nvsHandles[partition], key, data, length);
    }

    if (status == ESP_OK) {
        status = nvs_commit(nvsHandles[partition]);
    }

    return status;
}

esp_err_t Flash_Erase(FlashPartition partition, const char *key) {
    esp_err_t status = ESP_OK;
    if (key == NULL) {
        status = ESP_ERR_INVALID_ARG;
    }

    if (status == ESP_OK) {
        status = nvs_erase_key(nvsHandles[partition], key);
    }

    if (status == ESP_OK) {
        status = nvs_commit(nvsHandles[partition]);
    }

    return status;
}

esp_err_t Flash_Save(FlashPartition partition, const char *key, const void *data, size_t length) {
    uint8_t readData[length];
    esp_err_t status = Flash_UnsafeSave(partition, key, data, length);

    if (status == ESP_OK) {
        status = Flash_Load(partition, key, readData, length);
    }

    if (status == ESP_OK) {
        if (memcmp(data, readData, length) == 0) {
            ESP_LOGD(TAG, "File %s saved successfully", key);
        } else {
            status = ESP_ERR_NVS_CONTENT_DIFFERS;
        }
    }

    return status;
}

esp_err_t Flash_Load(FlashPartition partition, const char *key, void *data, size_t length) {
    size_t actualLength = length;
    esp_err_t status = ESP_OK;
    if (key == NULL) {
        status = ESP_ERR_INVALID_ARG;
    }

    status = nvs_get_blob(nvsHandles[partition], key, data, &actualLength);
    if (status == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGD(TAG, "key %d:%s not found in NVS", partition, key);
    } else if (status != ESP_OK) {
        ESP_LOGD(TAG, "error loading from flash: 0x%04x (%s)", status, esp_err_to_name(status));
    }

    if (length < actualLength && length != 0) {
        ESP_LOGW(
            TAG, "given length is not enough, file will be truncated (got %d, want at least %d)", length, actualLength);
    }

    return status;
}

bool Flash_Exists(FlashPartition partition, const char *key) {
    esp_err_t status = ESP_OK;
    size_t length = 0;
    uint8_t *data = {0};

    status = Flash_Load(partition, key, data, length);
    return status == ESP_OK && length > 0;
}