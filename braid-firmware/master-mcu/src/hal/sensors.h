#pragma once

#include "proto.h"

#define SENSORS_UART_BUFFER_SIZE (2048u)

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Initializes the UART connecion towards the sensors MCU
 *
 * @param[out] protoCtx The `ProtoCtx` to initialize
 * @param uartNum UART bus number to use for connection
 * @param txPin The TX pin
 * @param rxPin The RX pin
 * @return `ESP_FAIL` if errors were encountered
 */
esp_err_t Sensors_UARTInit(ProtoCtx *protoCtx, int uartNum, gpio_num_t txPin, gpio_num_t rxPin);

/**
 * @brief Copies the most recent sensor data into the given struct
 *
 * @param[out] out Reference to the data to initialize
 * @return `ESP_ERR_NOT_ALLOWED` if a data read from the sensors is already in progress
 */
esp_err_t Sensors_GetLastData(SensorData *out);

/**
 * @brief Writes sensors data to flash
 *
 * @return `ESP_OK` if writing was successful, otherwise a relevant error code
 */
esp_err_t Sensors_SaveData();

/**
 * @brief Reads sensors data from flash
 *
 * @return `ESP_OK` if reading was successful, otherwise a relevant error code
 */
esp_err_t Sensors_LoadData();

#ifdef __cplusplus
}
#endif /* __cplusplus */