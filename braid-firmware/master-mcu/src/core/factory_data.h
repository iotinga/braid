#pragma once

#include <esp_check.h>
#include <stdbool.h>
#include <stdint.h>

#define HMAC_KEY_LENGTH   (128u)
#define FACTORY_DATA_SIZE (sizeof(FactoryData))

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @brief Represents factory data stored in the NVS */
typedef struct FactoryData {
    /** @brief Private key for protocol HMAC */
    uint8_t key[HMAC_KEY_LENGTH];

    /** @brief Device ID */
    char deviceId[32];

    /** @brief Full URI of the MQTT broker */
    char mqttUri[256];

    /** @brief APN for the SIM */
    char simApn[32];

    /** @brief `true` if a pin is set, `false` otherwise */
    bool simPinRequired;

    /** @brief PIN to unlock the SIM */
    char simPin[8];
} FactoryData;

/**
 * @brief Validate the given factory data object
 *
 * @param[inout] data Object to validate and modify
 * @return `ESP_ERR_INVALID_ARGUMENT` if the data contains errors
 */
esp_err_t FactoryData_Validate(FactoryData *data);

/**
 * @brief Reads factory data from NVS
 *
 * @param[out] data Data structure to be read into
 * @return - `ESP_OK` if the value was retrieved successfully
 * @return - `ESP_FAIL` if there is an internal error; most likely due to corrupted NVS partition
 * @return - `ESP_ERR_NVS_NOT_FOUND` if factory data doesn't exist in the NVS
 */
esp_err_t FactoryData_Load(FactoryData *data);

/**
 * @brief Erases factory data from NVS
 *
 * @return - `ESP_OK` if the value was erased successfully
 * @return - `ESP_FAIL` if there is an internal error; most likely due to corrupted NVS partition
 * @return - `ESP_ERR_NVS_NOT_FOUND` if factory data doesn't exist in the NVS
 */
esp_err_t FactoryData_Erase();

#ifdef __cplusplus
}
#endif /* __cplusplus */