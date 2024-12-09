#include "esp_log.h"
#include "esp_modem_api.h"
#include "string.h"

#include "defines.h"
#include "hal/flash.h"
#include "hal/gps.h"
#include "hal/modem.h"

static const char *TAG = "hal/gps";

static const char *MODE_STR_GPS_ONLY = "1,0,0,0";
static const char *MODE_STR_GLONASS = "1,1,0,0";
static const char *MODE_STR_BEIDOU = "1,0,1,0";
static const char *MODE_STR_GALILEO = "1,0,0,1";

static const char *gpsModeToString(GPS_Mode mode) {
    switch (mode) {
    case GPS_GLONASS:
        return MODE_STR_GLONASS;
    case GPS_BEIDOU:
        return MODE_STR_BEIDOU;
    case GPS_GALILEO:
        return MODE_STR_GALILEO;
    case GPS_ONLY:
    default:
        return MODE_STR_GPS_ONLY;
    }
}

void GPS_PrintData(GPS_Position *gpsPos, GPS_Date *gpsDate) {
    ESP_LOGD(TAG, "lat: %.8f\tlon: %.8f", gpsPos->lat, gpsPos->lon);
    ESP_LOGD(TAG, "speed: %.3f\taltitude: %.3f", gpsPos->speed, gpsPos->alt);

    if (gpsDate != NULL) {
        ESP_LOGD(TAG, "date: %04d/%02d/%02d", gpsDate->year, gpsDate->month, gpsDate->day);
        ESP_LOGD(TAG, "time: %02d:%02d:%02d", gpsDate->hour, gpsDate->min, gpsDate->sec);
    }
}

esp_err_t GPS_SaveData(GPS_Position *gpsPos) {
    return Flash_Save(PARTITION_USER, "gps_pos", gpsPos, sizeof(GPS_Position));
}

esp_err_t GPS_LoadData(GPS_Position *gpsPos) {
    return Flash_Load(PARTITION_USER, "gps_pos", gpsPos, sizeof(GPS_Position));
}

esp_err_t GPS_GetData(esp_modem_dce_t *modem, GPS_Position *gpsPos, GPS_Date *gpsDate) {
    // Turn on power to the antenna
    ESP_ERROR_CHECK(esp_modem_at(modem, "AT+CGPIO=0,48,1,1", NULL, MODEM_AT_TIMEOUT));

    char buf[512] = {0};
    ESP_RET_CHECK(esp_modem_at(modem, "AT+CGNSINF", buf, MODEM_AT_TIMEOUT));

    int fixStatus = 0;
    sscanf(buf,
           "%*d,%d,%04d%02d%02d%02d%02d%02d.%*d,%f,%f,%f,%f,%f,%*d,,%f,%*f,%*f,,%*d,%d,%*d,,%*d,%*f,%*f",
           &fixStatus,
           &gpsDate->year,
           &gpsDate->month,
           &gpsDate->day,
           &gpsDate->hour,
           &gpsDate->min,
           &gpsDate->sec,
           &gpsPos->lat,
           &gpsPos->lon,
           &gpsPos->alt,
           &gpsPos->speed,
           &gpsPos->direction,
           &gpsPos->accuracy,
           &gpsPos->usat);
    gpsPos->locked = (bool)fixStatus;

    // Turn on power to the antenna
    ESP_ERROR_CHECK(esp_modem_at(modem, "AT+CGPIO=0,48,1,0", NULL, MODEM_AT_TIMEOUT));

    return ESP_OK;
}

esp_err_t GPS_Configure(esp_modem_dce_t *modem, GPS_Mode mode) {
    // Turn on power to the antenna
    ESP_ERROR_CHECK(esp_modem_at(modem, "AT+CGPIO=0,48,1,1", NULL, MODEM_AT_TIMEOUT));

    //  When configuring GNSS, we need to stop GPS first
    ESP_ERROR_CHECK(esp_modem_at(modem, "AT+CGNSPWR=0", NULL, MODEM_AT_TIMEOUT));

    /*
    ! GNSS Work Mode Set
    <gps mode> GPS work mode, must be 1 to work
    <glo mode> GLONASS constellation support
    <bd mode> BEIDOU constellation support
    <gal mode> GALILEO constellation support
    */

    // GNSs SetMODe
    char cmd_buf[128] = {0};
    sprintf(cmd_buf, "AT+CGNSMOD=%s", gpsModeToString(mode));
    ESP_ERROR_CHECK(esp_modem_at(modem, cmd_buf, NULL, MODEM_AT_TIMEOUT));

    ESP_LOGI(TAG, "GNSS configured");

    while (esp_modem_at(modem, "AT+CGNSPWR=1", NULL, MODEM_AT_TIMEOUT * 10) != ESP_OK) {
        ESP_LOGE(TAG, "Modem GPS enable failed, retrying");
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

    return ESP_OK;
}

esp_err_t GPS_DeleteXtra(esp_modem_dce_t *modem) {
    ESP_RET_CHECK(esp_modem_at(modem, "AT+CFSINIT", NULL, MODEM_AT_TIMEOUT));
    ESP_RET_CHECK(esp_modem_at(modem, "AT+CFSDFILE=3,\"Xtra3.bin\"", NULL, MODEM_AT_TIMEOUT));
    ESP_RET_CHECK(esp_modem_at(modem, "AT+CFSTERM", NULL, MODEM_AT_TIMEOUT));

    return ESP_OK;
}

esp_err_t GPS_DownloadXtra(esp_modem_dce_t *modem, GPS_Mode mode) {
    // Download the correct Xtra file
    switch (mode) {
    case GPS_GLONASS:
        ESP_RET_CHECK(esp_modem_at(modem,
                                   "AT+HTTPTOFS=\"http://iot2.xtracloud.net/xtra3gr_72h.bin\",\"/customer/Xtra3.bin\"",
                                   NULL,
                                   MODEM_AT_TIMEOUT));
        break;
    case GPS_BEIDOU:
        ESP_RET_CHECK(esp_modem_at(modem,
                                   "AT+HTTPTOFS=\"http://iot2.xtracloud.net/xtra3gc_72h.bin\",\"/customer/Xtra3.bin\"",
                                   NULL,
                                   MODEM_AT_TIMEOUT));
        break;
    case GPS_GALILEO:
        ESP_RET_CHECK(esp_modem_at(modem,
                                   "AT+HTTPTOFS=\"http://iot2.xtracloud.net/xtra3ge_72h.bin\",\"/customer/Xtra3.bin\"",
                                   NULL,
                                   MODEM_AT_TIMEOUT));
        break;
    case GPS_ONLY:
        // fall through
    default:
        ESP_RET_CHECK(esp_modem_at(modem,
                                   "AT+HTTPTOFS=\"http://iot2.xtracloud.net/xtra3g_72h.bin\",\"/customer/Xtra3.bin\"",
                                   NULL,
                                   MODEM_AT_TIMEOUT));
    }

    // Wait for download to finish
    int downloadStatus = 0;
    do {
        char cmd_buf[128] = {0};
        ESP_RET_CHECK(esp_modem_at(modem, "AT+HTTPTOFS?", cmd_buf, MODEM_AT_TIMEOUT));
        sscanf(cmd_buf, "%d,%*s,%*s", &downloadStatus);
        esp_modem_at_scanf(modem, "AT+HTTPTOFS?", "%d,%*s,%*s", &downloadStatus);
        vTaskDelay(pdMS_TO_TICKS(500));
    } while (downloadStatus == 1);

    // Load Xtra file
    ESP_RET_CHECK(esp_modem_at(modem, "AT+CGNSCPY", NULL, MODEM_AT_TIMEOUT));

    // Verify Xtra file
    ESP_RET_CHECK(esp_modem_at(modem, "AT+CGNSXTRA", NULL, MODEM_AT_TIMEOUT));

    return ESP_OK;
}

esp_err_t GPS_ConfigureXtra(esp_modem_dce_t *modem, GPS_Mode mode) {
    // Sync network time
    ESP_RET_CHECK(esp_modem_at(modem, "AT+CLTS=1", NULL, MODEM_AT_TIMEOUT));

    // Check Xtra file existence
    bool fileExists = (esp_modem_at(modem, "AT+CFSGFIS=3,\"Xtra3.bin\"", NULL, MODEM_AT_TIMEOUT * 4) == ESP_OK);

    // Verify Xtra file
    bool xtraValid = (esp_modem_at(modem, "AT+CGNSXTRA", NULL, MODEM_AT_TIMEOUT * 4) == ESP_OK);

    if (xtraValid && fileExists) {
        ESP_LOGI(TAG, "Current Xtra file is valid");
    } else {
        ESP_LOGI(TAG, "Downloading fresh Xtra file");
        GPS_DownloadXtra(modem, mode);
    }

    // Enable Xtra
    ESP_RET_CHECK(esp_modem_at(modem, "AT+CGNSXTRA=1", NULL, MODEM_AT_TIMEOUT * 4));

    return ESP_OK;
}
