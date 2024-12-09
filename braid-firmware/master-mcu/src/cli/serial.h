#pragma once

#include <esp_check.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialized the serial console over the USBCDC device
 *
 * @return - `ESP_OK` if the operation was successful
 * @return - `ESP_FAIL` if the driver install failed
 */
esp_err_t SerialUSBInit();

/**
 * @brief Initialized the serial console over the USB to UART device
 *
 * @param uartNum UART bus to use, must be free
 * @param baudRate The baud rate for the new UART
 * @return - `ESP_OK` if the operation was successful
 * @return - `ESP_FAIL` if the driver install failed
 */
esp_err_t SerialUARTInit(int uartNum, int baudRate);

/**
 * @brief Prints a string to the serial console
 *
 * @param fmt `printf`-compatible formatting string
 * @param[in] ... Arguments for the internal `printf`
 *
 * @see ``printf``
 */
void SerialPrintf(const char *fmt, ...);

/**
 * @brief Reads raw data from the serial console
 *
 * @param[out] buffer Data buffer to write into
 * @param size The expected size of data to be read
 * @return int The number of bytes read
 */
int SerialRead(void *buffer, size_t size);

/**
 * @brief Writes raw data to the serial console
 *
 * @param[in] buffer Data buffer to write from
 * @param size The size of the data to write
 * @return int The number of bytes written successfully
 */
int SerialWrite(const void *buffer, size_t size);

#ifdef __cplusplus
}
#endif