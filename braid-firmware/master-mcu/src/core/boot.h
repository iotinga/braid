#pragma once

#include "stdbool.h"

#define BOOT_MODE_MAX 3

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/// @brief Describes various boot modes
typedef enum Boot_Mode {
    BOOT_NO_FACTORY_DATA = 0,
    BOOT_GPS,
    BOOT_MQTT,
} Boot_Mode;

/**
 * @brief Initializes internal resources
 */
void Boot_Init();

/**
 * @brief Reboots the system into a different mode
 *
 * @param mode The new boot mode
 */
void Boot_To(Boot_Mode mode);

/**
 * @brief Returns the currently active boot mode
 *
 * @param[out] mode The current boot mode
 * @return The current boot mode
 */
void Boot_GetCurrentMode(Boot_Mode *mode);

/**
 * @brief Checks whether a shutdown is pending
 *
 * @return true if a shutdown is pending
 */
bool Boot_IsShutdownPending();
void Boot_SetShutdownPending(bool pending);

/**
 * @brief Checks whether all internal systems are ready for shutdown
 *
 * @return true
 * @return false
 */
bool Boot_IsShutdownReady();

/**
 * @brief Registers the current task to the graceful shutdown system
 *
 * @return int The number of registered task, including the newly registered one
 */
int Boot_RegisterTask();

/**
 * @brief Sets the current task as ready for shutdown
 */
void Boot_SetShutdownReady();

/**
 * @brief Returns the durations for a specific boot mode, i.e. how much time that boot mode must last
 *
 * @param mode The mode to get duration for
 * @return The duration in seconds
 */
int Boot_GetDuration(Boot_Mode mode);

/**
 * @brief Sets the duration for a specific boot mode
 *
 * @param mode The mode to change duration for
 * @param seconds Duration in seconds
 */
void Boot_SetDuration(Boot_Mode mode, int seconds);

#ifdef __cplusplus
}
#endif /* __cplusplus */