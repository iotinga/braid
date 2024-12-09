#pragma once

#include <driver/gpio.h>
#include <esp_err.h>
#include <stdbool.h>

#define TAMPER_MAX_EVENTS (5u)
#define TAMPER_DATA_SIZE  (sizeof(TamperData))

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @brief Represents tamper event */
typedef struct TamperEvent {
    /** @brief The pin which generated the event */
    gpio_num_t pin;

    /** @brief System timestamp for the event */
    uint64_t timestamp;
} TamperEvent;

/** @brief Represents tamper data saved in the NVS */
typedef struct TamperData {
    /** @brief All tamper events */
    TamperEvent events[TAMPER_MAX_EVENTS];

    /** @brief Points to the last tamper event registered */
    int8_t lastEventIndex;
} TamperData;

/**
 * @brief Initializes anti tamper on the given pin
 * The pin must normally be connected to Vin (3.3V). An event is generated when the pin is disconnected from Vin.
 * @note No debouncing is applied
 *
 * @param pin The pin to check on
 * @return - `ESP_OK` Success
 * @return - `ESP_ERR_NO_MEM` No memory to install the GPIO service
 */
esp_err_t Tamper_Setup(gpio_num_t pin);

/**
 * @brief Checks for messages on the internal message queue
 *
 * @param[out] pin The pin which generated the alert
 * @return - `0` if no message was received at the time of checking
 * @return - `>0` if there was a message in the queue
 */
int Tamper_CheckForTamper(gpio_num_t *pin);

/**
 * @brief Saves a tamper event into NVS
 *
 * @param[in] event The event to save
 * @return - `ESP_OK` if the value was saved successfully
 * @return - `ESP_FAIL` if there is an internal error; most likely due to corrupted NVS partition
 */
esp_err_t Tamper_RegisterEvent(TamperEvent *event);

/**
 * @brief Prints antitamper data stored in NVS to console
 *
 * @return esp_err_t `ESP_OK` if no errors were encountered, otherwise a relevant error code
 */
esp_err_t TamperData_Print();

/**
 * @brief Erases stored antitamper data
 *
 * @return esp_err_t `ESP_OK` if no errors were encountered, otherwise a relevant error code
 */
esp_err_t TamperData_Erase();

#ifdef __cplusplus
}
#endif /* __cplusplus */