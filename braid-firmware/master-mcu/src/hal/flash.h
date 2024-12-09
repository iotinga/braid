#pragma once

#include <esp_check.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <stdbool.h>

#define FACTORY_DATA_PARTITION_NAME "factory-data"
#define USER_DATA_PARTITION_NAME    "nvs"

#define FLASH_DEFAULT_NAMESPACE "braid"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @brief Defines different partition types */
typedef enum FlashPartition {
    PARTITION_USER,
    PARTITION_FACTORY,
    PARTITION_COUNT,
} FlashPartition;

/**
 * @brief Initalizes the NVS flash for a partition
 *
 * @param partition The partition to be initialized
 * @return esp_err_t `ESP_OK` if no errors were encountered, otherwise a relevant error code
 */
esp_err_t Flash_Init(FlashPartition partition);

/**
 * @brief Saves binary data to flash
 *
 * @param partition The partition in use
 * @param[in] key The file name/key
 * @param[in] data The data buffer
 * @param length Length of the data in bytes
 * @return esp_err_t `ESP_OK` if no errors were encountered, otherwise a relevant error code
 */
esp_err_t Flash_Save(FlashPartition partition, const char *key, const void *data, size_t length);

/**
 * @brief Reads a file from the given partition into the given buffer
 *
 * @param partition The partition in use
 * @param[in] key The file name/key
 * @param[out] data The data buffer
 * @param[out] length Length of the buffer loaded from flash
 * @return esp_err_t `ESP_OK` if no errors were encountered, otherwise a relevant error code
 */
esp_err_t Flash_Load(FlashPartition partition, const char *key, void *data, size_t length);

/**
 * @brief Erases all data for the given key in the given partition
 *
 * @param partition The partition in use
 * @param key The key to erase
 * @return esp_err_t `ESP_OK` if no errors were encountered, otherwise a relevant error code
 */
esp_err_t Flash_Erase(FlashPartition partition, const char *key);

/**
 * @brief Checks if the given key exists in the given partition
 *
 * @param partition The partition in use
 * @param key The key to search for
 * @return `true` if the key was found, `false` otherwise
 */
bool Flash_Exists(FlashPartition partition, const char *key);

#ifdef __cplusplus
}
#endif /* __cplusplus */