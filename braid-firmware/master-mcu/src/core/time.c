#include "core/time.h"

#include "esp_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

static const char *TAG = "core/time";

esp_err_t Time_Set(TimestampSeconds timeSeconds) {
    int result = 0;
    struct timeval correctTime = {
        .tv_sec = timeSeconds,
        .tv_usec = 0,
    };
    struct timeval currentTime;
    result = gettimeofday(&currentTime, NULL);

    if (result != 0) {
        return ESP_FAIL;
    }

    struct timeval delta = {
        .tv_sec = correctTime.tv_sec - currentTime.tv_sec,
        .tv_usec = correctTime.tv_usec - currentTime.tv_usec,
    };

    if (delta.tv_sec < SMOOTH_SYNC_LIMIT_SECONDS) {
        struct timeval oldDelta;
        result = adjtime(&delta, &oldDelta);
    } else {
        result = settimeofday(&correctTime, NULL);
    }

    if (result != 0) {
        return ESP_FAIL;
    }

    return ESP_OK;
}

// esp_err_t Time_GetLocalTime(const Shadow *shadow, TimestampSeconds unixTimestamp, struct tm *tm) {
//     uint32_t offsetMinutes = Shadow_GetInt(shadow, SYS_TIMEZONE_OFFSET_MINUTES);
//     time_t localTimestamp = unixTimestamp + offsetMinutes * 60;
//     if (gmtime_r(&localTimestamp, tm) == NULL) {
//         return ESP_ERR_INVALID_RESPONSE;
//     }

//     return ESP_OK;
// }

TimestampSeconds Time_GetUnixTimestampFromTm(struct tm *tm) {
    return mktime(tm);
}

TimestampSeconds Time_GetUnixTimestamp(void) {
    TimestampSeconds result = 0;

    struct timeval currentTime;
    int error = gettimeofday(&currentTime, NULL);
    if (error != 0) {
        ESP_LOGE(TAG, "gettimeofday failure");
    } else {
        result = currentTime.tv_sec;
    }

    return result;
}
