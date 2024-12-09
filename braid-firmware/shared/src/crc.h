#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Computes the CRC16 checksum for a given data buffer.
 *
 * @param length The length of the data buffer
 * @param[in] data A pointer to the data buffer
 * @return The computed CRC16 checksum
 */
uint16_t Crc16(size_t length, const uint8_t *data);

/**
 * @brief Verifies the CRC16 checksum of a data buffer
 *
 * @param length The length of the data buffer, including the CRC
 * @param[in] buffer A pointer to the data buffer
 * @return `true` if the CRC is valid, `false` otherwise
 */
bool CheckCrc16(size_t length, const uint8_t *buffer);

#ifdef __cplusplus
}
#endif /* __cplusplus */
