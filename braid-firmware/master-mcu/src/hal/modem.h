#pragma once

#include "esp_modem_api.h"
#include "esp_netif.h"
#include <driver/gpio.h>
#include <stdbool.h>

#define MODEM_UART_RX_BUFFER_SIZE        (2048u)
#define MODEM_UART_TX_BUFFER_SIZE        (2048u)
#define MODEM_UART_EVENT_QUEUE_SIZE      (30u)
#define MODEM_UART_EVENT_TASK_STACK_SIZE (4096u)
#define MODEM_UART_EVENT_TASK_PRIORITY   (10u)

#define esp_modem_at_scanf(modem, cmd, out_fmt, ...)                                                                   \
    do {                                                                                                               \
        char __buf[256] = {0};                                                                                         \
        ESP_RET_CHECK(esp_modem_at(modem, cmd, __buf, MODEM_AT_TIMEOUT));                                              \
        sscanf(__buf, out_fmt, ##__VA_ARGS__);                                                                         \
    } while (0)

#define esp_modem_at_log(modem, cmd, timeout)                                                                          \
    do {                                                                                                               \
        char __buf[512] = {0};                                                                                         \
        esp_err_t __err = esp_modem_at(modem, cmd, __buf, timeout);                                                    \
        ESP_ERROR_CHECK(__err);                                                                                        \
        ESP_LOGD(TAG, "%s --> %s", cmd, __buf);                                                                        \
    } while (0);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Initializes and configures the SIM module through serial
 *
 * @param[in] netif   Already initialized `esp_netif` to bind to modem
 * @param apn         APN for the SIM
 * @param txPin       TX pin connected to the modem UART
 * @param rxPin       RX pin connected to the modem UART
 * @param powerPin    Power pin/PWRKEY of the modem
 * @return Pointer to initialized modem object
 */
esp_modem_dce_t *Modem_Init(
    esp_netif_t *netif, const char *apn, gpio_num_t txPin, gpio_num_t rxPin, gpio_num_t powerPin);

/**
 * @brief Physically turns off the modem
 *
 * @param modem The modem instance in use
 * @return `ESP_OK` if the modem was powered off
 */
esp_err_t Modem_Poweroff(esp_modem_dce_t *modem, gpio_num_t powerPin);

#ifdef __cplusplus
}
#endif /* __cplusplus */
