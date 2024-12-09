#include <string.h>

#include "factory_data.h"
#include "rtc.h"

extern RTC_HandleTypeDef hrtc;

HAL_StatusTypeDef FactoryData_Save(const FactoryData *data) {
    HAL_StatusTypeDef status = HAL_ERROR;
    const uint8_t *dataBytes = (const uint8_t *)data;
    size_t dataSize = sizeof(FactoryData);

    status = HAL_FLASHEx_DATAEEPROM_Unlock();
    if (status != HAL_OK) {
        return status;
    }

    memcpy((void *)DATA_EEPROM_BASE, dataBytes, dataSize);

    status = HAL_FLASHEx_DATAEEPROM_Lock();
    if (status != HAL_OK) {
        return status;
    }

    return HAL_OK;
}

HAL_StatusTypeDef FactoryData_Load(FactoryData *data) {
    HAL_StatusTypeDef status = HAL_ERROR;
    uint8_t *dataBytes = (uint8_t *)data;
    size_t dataSize = sizeof(FactoryData);

    // Zero the data buffer
    memset(dataBytes, 0, sizeof(FactoryData));

    status = HAL_FLASHEx_DATAEEPROM_Unlock();
    if (status != HAL_OK) {
        return status;
    }

    memcpy(data, (void *)DATA_EEPROM_BASE, dataSize);

    status = HAL_FLASHEx_DATAEEPROM_Lock();
    if (status != HAL_OK) {
        return status;
    }

    return HAL_OK;
}

void FactoryData_Unload(FactoryData *data) {
    memset(data, 0, sizeof(FactoryData));
}

HAL_StatusTypeDef FactoryData_EraseSecrets(FactoryData *data) {
    HAL_StatusTypeDef status = HAL_OK;

    memset(data->key, 0, HMAC_KEY_LENGTH);
    status = FactoryData_Save(data);

    return status;
}
