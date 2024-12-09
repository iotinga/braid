#pragma once

#include "esp_err.h"
#include "esp_modem_api.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Defines GPS support constellations/operating mode
 */
typedef enum {
    /// @brief Base GPS, must always be set
    GPS_ONLY = 1 << 4,

    /// @brief GLONASS constellation support
    GPS_GLONASS = 1 << 3,

    /// @brief BEIDOU constellation support
    GPS_BEIDOU = 1 << 2,

    /// @brief GALILEO constellation support
    GPS_GALILEO = 1 << 1,
} GPS_Mode;

/**
 * @brief Represents a position obtained from the GPS module
 */
typedef struct GPS_Position {
    /// @brief Set to `true` if the modem received a lock confirmation
    bool locked;

    /// @brief Direction of movement in degress with 0 being north
    float direction;

    /// @brief Latitude
    float lat;

    /// @brief Longitude
    float lon;

    /// @brief Speed
    float speed;

    /// @brief Altitude
    float alt;

    /// @brief Satellites Used
    int usat;

    /// @brief Horizontal dilution of precision
    float accuracy;
} GPS_Position;

/**
 * @brief Represents a datetime obtained from the GPS module
 */
typedef struct GPS_Date {
    /// @brief Year
    int year;

    /// @brief Month
    int month;

    /// @brief Day
    int day;

    /// @brief Hour
    int hour;

    /// @brief Minute
    int min;

    /// @brief Second
    int sec;
} GPS_Date;

/**
 * Print `GPS_Position` and `GPS_Date` in readable format to serial console.
 *
 * @param gpsPos Data to be printed
 * @param gpsDate Data to be printed
 */
void GPS_PrintData(GPS_Position *gpsPos, GPS_Date *gpsDate);

/**
 * @brief Writes GPS data to flash
 *
 * @param[in] gpsPos The position to write
 * @return `ESP_OK` if writing was successful, otherwise a relevant error code
 */
esp_err_t GPS_SaveData(GPS_Position *gpsPos);

/**
 * @brief Reads GPS data from flash
 *
 * @param[out] gpsPos Pointer to struct to populate
 * @return `ESP_OK` if reading was successful, otherwise a relevant error code
 */
esp_err_t GPS_LoadData(GPS_Position *gpsPos);

/**
 * Fetch GPS data from the modem and fill the given data struct. Not blocking, must be called in a loop until `true` is
 * returned. `false` is returned while the search is in progress.
 *
 * @param modem   Modem instance to use for GPS acquisition
 * @param gpsPos  Position to be populated
 * @param gpsDate Datetime to be populated (can be left to NULL)
 * @return `ESP_OK` if the current position was found, `ESP_FAIL` otherwise
 */
esp_err_t GPS_GetData(esp_modem_dce_t *modem, GPS_Position *gpsPos, GPS_Date *gpsDate);

/**
 * Configures the GNSS service on the given modem with the desired parameters.
 *
 * @param modem Modem instance to configure the GNSS service on
 * @param mode  GPS operation mode/constellation to use
 * @return `ESP_FAIL` if intialization errors were encountered, `ESP_OK` otherwise
 */
esp_err_t GPS_Configure(esp_modem_dce_t *modem, GPS_Mode mode);

/**
 * Deletes the Xtra file from the modem's flash.
 *
 * @param modem Modem instance to use
 * @return `ESP_FAIL` if errors were encountered, `ESP_OK` otherwise
 */
esp_err_t GPS_DeleteXtra(esp_modem_dce_t *modem);

/**
 * Downloads the Xtra file from the network. The network MUST be connected and registered to download the file.
 *
 * @param modem Modem instance to use
 * @param mode  GPS operation mode/constellation in use
 * @return `ESP_FAIL` if errors were encountered, `ESP_OK` otherwise
 */
esp_err_t GPS_DownloadXtra(esp_modem_dce_t *modem, GPS_Mode mode);

/**
 * Enables Xtra mode for the GNSS module. Will download the file if necessary.
 *
 * @param modem Modem instance to use
 * @param mode  GPS operation mode/constellation in use
 * @return `ESP_FAIL` if errors were encountered, `ESP_OK` otherwise
 */
esp_err_t GPS_ConfigureXtra(esp_modem_dce_t *modem, GPS_Mode mode);

#ifdef __cplusplus
}
#endif /* __cplusplus */
