#pragma once

#include "stm32l0xx.h"
#include <stdbool.h>

#define EEPROM_WORD_SIZE (32u)
#define HMAC_KEY_LENGTH  (128u)

/** @brief Represents data stored in the EEPROM */
typedef struct FactoryData {
    /** @brief Private key for protocol HMAC */
    uint8_t key[HMAC_KEY_LENGTH];

    /** @brief Device ID */
    uint32_t id;

    /** @brief Device type/capabilities bitmask */
    uint32_t type;

    /** @brief Tamper detection for TAMPER2 */
    uint8_t tamper2;

    /** @brief Tamper detection for TAMPER3 */
    uint8_t tamper3;
} FactoryData;

/**
 * @brief Writes structured data to the EEPROM
 *
 * @param[in] data Data to write
 * @return HAL_StatusTypeDef `HAL_OK` if no errors were encountered, `HAL_ERR` otherwise
 */
HAL_StatusTypeDef FactoryData_Save(const FactoryData *data);

/**
 * @brief Reads structured data stored in the EEPROM. Must always be followed by `FactoryData_Unload` to ensure secure
 * access to the data
 *
 * @param[out] data Data to write
 * @return HAL_StatusTypeDef `HAL_OK` if no errors were encountered, `HAL_ERR` otherwise
 */
HAL_StatusTypeDef FactoryData_Load(FactoryData *data);

/**
 * @brief Removes factory data struct from memory
 *
 * @param[in] data Data to write
 */
void FactoryData_Unload(FactoryData *data);

/**
 * @brief Erases all secrets from the given factory data and saves it to EEPROM
 *
 * @param[in] data Data to write
 */
HAL_StatusTypeDef FactoryData_EraseSecrets(FactoryData *data);