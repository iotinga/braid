#pragma once

#include "esp_modem_api.h"
#include "esp_netif.h"
#include "esp_netif_ppp.h"
#include "time.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief The modem GPRS operating mode
 */
typedef enum GPRS_Connectivity {
    /// @brief Cat-M1 only
    CONN_CATM = 1,
    /// @brief NBIoT only
    CONN_NB_IOT,
    /// @brief Cat-M1 and NBIoT
    CONN_CATM_NBIOT,
} GPRS_Connectivity;

/**
 * @brief The modem GPRS network physical protocol
 */
typedef enum GPRS_Network {
    /// @brief Automatic
    NETWORK_AUTO = 2,
    /// @brief GSM only
    NETWORK_GSM = 13,
    /// @brief LTE only
    NETWORK_LTE = 38,
    /// @brief GSM and LTE
    NETWORK_GSM_LTE = 51,
} GPRS_Network;

/**
 * @brief Initialization parameters for the modem
 */
typedef struct GPRS_Params {
    /// @brief The APN for the SIM provider. If `nullptr`, APN is not configured during setup
    const char *apn;

    /// @brief The PIN for the SIM. If `nullptr`, the SIM must be unprotected to work
    const char *pin;

    /// @brief The preferred connectivity protocol
    GPRS_Connectivity connectionMode;

    /// @brief The network to use
    GPRS_Network networkMode;
} GPRS_Params;

/**
 * @brief Creates an instance of `esp_netif_t` to use with the modem
 *
 * @return pointer to esp-netif object on success - NULL otherwise
 */
esp_netif_t *GPRS_NewNetif();

/**
 * @brief Configures the network connection and attaches to it
 *
 * @param modem  Modem instance configure the service on
 * @param init   Initialization parameters
 * @return `ESP_OK` if no errors were encountered
 */
esp_err_t GPRS_Connect(esp_modem_dce_t *modem, GPRS_Params *init);

/**
 * @brief Disconnects the modem from the network
 *
 * @param modem  Modem instance configure the service on
 * @return `ESP_OK` if no errors were encountered
 */
esp_err_t GPRS_Disconnect(esp_modem_dce_t *modem);

/**
 * Prints connection status for the given modem to serial console.
 *
 * @param modem      Modem instance to get data from
 */
void GPRS_PrintConnInfo(esp_modem_dce_t *modem);

/**
 * Fetch UTC datetime from the network.
 *
 * @param modem   Modem instance to use for data acquisition
 * @return the current epoch in milliseconds
 */
time_t GPRS_GetDatetime(esp_modem_dce_t *modem);

#ifdef __cplusplus
}
#endif /* __cplusplus */
