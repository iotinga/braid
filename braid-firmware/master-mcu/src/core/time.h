#pragma once

#include <esp_err.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** standard timestamp (UNIX) */
typedef uint32_t TimestampSeconds;

#define SEC_TO_MS(x)  ((x) * 1000u)
#define MS_TO_US(x)   ((x) * 1000u)
#define MS_TO_SEC(x)  ((x) / 1000u)
#define SEC_TO_US(x)  ((MS_TO_US(x) * 1000u))
#define MIN_TO_SEC(x) ((x) * 60u)

#define SECONDS_IN_MINUTE 60u
#define MINUTES_IN_HOUR   60u
#define HOURS_IN_DAY      24u

#define SMOOTH_SYNC_LIMIT_SECONDS 30u * 60u

/** rappresents a day of the week */
enum DayOfWeek {
    DOW_SUNDAY = 0,
    DOW_MONDAY,
    DOW_TUESDAY,
    DOW_WEDNESDAY,
    DOW_THURSDAY,
    DOW_FRIDAY,
    DOW_SATURDAY,
};

/**
 * @brief If time delta is less than 30 minutes, adjust time smoothly, else set time abruptly.
 * Setting time smoothly is preferred when possible to avoid errors with components that require time.
 * https://man7.org/linux/man-pages/man3/adjtime.3.html
 *
 * @param timeSeconds The absolute epoch in seconds to use for system time
 * @return `ESP_FAIL` if errors were encountered, `ESP_OK` otherwise
 */
esp_err_t Time_Set(TimestampSeconds time_seconds);

/**
 * @brief Convert a `struct tm` to epoch seconds
 *
 * @param tm The data to convert
 * @return TimestampSeconds The given time in seconds
 */
TimestampSeconds Time_GetUnixTimestampFromTm(struct tm *tm);

/**
 * @brief Returns the current time in epoch seconds
 *
 * @return TimestampSeconds The epoch seconds
 */
TimestampSeconds Time_GetUnixTimestamp(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
