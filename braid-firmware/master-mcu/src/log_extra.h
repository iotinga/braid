#include <esp_log.h>

#define LOG_FORMAT_BEGIN_LINE(letter, format) LOG_COLOR_##letter #letter " (%" PRIu32 ") %s: " format LOG_RESET_COLOR
#define LOG_FORMAT_END_LINE(letter, format)   LOG_COLOR_##letter format LOG_RESET_COLOR "\n"
#define LOG_FORMAT_LINE(letter, format)       LOG_COLOR_##letter format LOG_RESET_COLOR

/**
 * @brief Begins a single-line logging context.
 * Also prints the given arguments like all other logs, with the given log `level`
 *
 * @note This macro must not be used when interrupts are disabled or inside an ISR.
 *
 * @param level Level of the log line
 * @param tag Tag of the log, which can be used to change the log level by ``esp_log_level_set`` at runtime.
 * @param format `printf`-compatible formatting string
 * @param[in] ... Arguments for the internal `printf`
 *
 * @see ``printf``
 */
#define ESP_LOG_LINE_BEGIN(level, tag, format, ...)                                                                    \
    {                                                                                                                  \
        esp_log_level_t __ctx_level = level;                                                                           \
        do {                                                                                                           \
            if (level == ESP_LOG_ERROR) {                                                                              \
                esp_log_write(                                                                                         \
                    ESP_LOG_ERROR, tag, LOG_FORMAT_BEGIN_LINE(E, format), esp_log_timestamp(), tag, ##__VA_ARGS__);    \
            } else if (level == ESP_LOG_WARN) {                                                                        \
                esp_log_write(                                                                                         \
                    ESP_LOG_WARN, tag, LOG_FORMAT_BEGIN_LINE(W, format), esp_log_timestamp(), tag, ##__VA_ARGS__);     \
            } else if (level == ESP_LOG_DEBUG) {                                                                       \
                esp_log_write(                                                                                         \
                    ESP_LOG_DEBUG, tag, LOG_FORMAT_BEGIN_LINE(D, format), esp_log_timestamp(), tag, ##__VA_ARGS__);    \
            } else if (level == ESP_LOG_VERBOSE) {                                                                     \
                esp_log_write(                                                                                         \
                    ESP_LOG_VERBOSE, tag, LOG_FORMAT_BEGIN_LINE(V, format), esp_log_timestamp(), tag, ##__VA_ARGS__);  \
            } else {                                                                                                   \
                esp_log_write(                                                                                         \
                    ESP_LOG_INFO, tag, LOG_FORMAT_BEGIN_LINE(I, format), esp_log_timestamp(), tag, ##__VA_ARGS__);     \
            }                                                                                                          \
        } while (0)

/**
 * @brief Ends a single-line logging context
 * Also prints the given arguments like all other logs, retaining the log level set with `ESP_LOG_LINE_BEGIN`
 *
 * @note This macro must not be used when interrupts are disabled or inside an ISR.
 *
 * @param tag Tag of the log, which can be used to change the log level by ``esp_log_level_set`` at runtime.
 * @param format `printf`-compatible formatting string
 * @param[in] ... Arguments for the internal `printf`
 *
 * @see ``printf``
 */
#define ESP_LOG_LINE_END(tag, format, ...)                                                                             \
    do {                                                                                                               \
        if (__ctx_level == ESP_LOG_ERROR) {                                                                            \
            esp_log_write(__ctx_level, tag, LOG_FORMAT_END_LINE(E, format), ##__VA_ARGS__);                            \
        } else if (__ctx_level == ESP_LOG_WARN) {                                                                      \
            esp_log_write(__ctx_level, tag, LOG_FORMAT_END_LINE(W, format), ##__VA_ARGS__);                            \
        } else if (__ctx_level == ESP_LOG_DEBUG) {                                                                     \
            esp_log_write(__ctx_level, tag, LOG_FORMAT_END_LINE(D, format), ##__VA_ARGS__);                            \
        } else if (__ctx_level == ESP_LOG_VERBOSE) {                                                                   \
            esp_log_write(__ctx_level, tag, LOG_FORMAT_END_LINE(V, format), ##__VA_ARGS__);                            \
        } else {                                                                                                       \
            esp_log_write(__ctx_level, tag, LOG_FORMAT_END_LINE(I, format), ##__VA_ARGS__);                            \
        }                                                                                                              \
    } while (0);                                                                                                       \
    }

/**
 * @brief Writes a string in a line logging context started with `ESP_LOG_LINE_BEGIN`
 *
 * @note This macro must not be used when interrupts are disabled or inside an ISR.
 *
 * @param tag Tag of the log, which can be used to change the log level by ``esp_log_level_set`` at runtime.
 * @param format `printf`-compatible formatting string
 * @param[in] ... Arguments for the internal `printf`
 *
 * @see ``printf``
 */
#define ESP_LOG_LINE(tag, format, ...)                                                                                 \
    do {                                                                                                               \
        if (__ctx_level == ESP_LOG_ERROR) {                                                                            \
            esp_log_write(ESP_LOG_ERROR, tag, LOG_FORMAT_LINE(E, format), ##__VA_ARGS__);                              \
        } else if (__ctx_level == ESP_LOG_WARN) {                                                                      \
            esp_log_write(ESP_LOG_WARN, tag, LOG_FORMAT_LINE(W, format), ##__VA_ARGS__);                               \
        } else if (__ctx_level == ESP_LOG_DEBUG) {                                                                     \
            esp_log_write(ESP_LOG_DEBUG, tag, LOG_FORMAT_LINE(D, format), ##__VA_ARGS__);                              \
        } else if (__ctx_level == ESP_LOG_VERBOSE) {                                                                   \
            esp_log_write(ESP_LOG_VERBOSE, tag, LOG_FORMAT_LINE(V, format), ##__VA_ARGS__);                            \
        } else {                                                                                                       \
            esp_log_write(ESP_LOG_INFO, tag, LOG_FORMAT_LINE(I, format), ##__VA_ARGS__);                               \
        }                                                                                                              \
    } while (0);
