#pragma once

#include <esp_check.h>

#include "core/boot.h"
#include "core/factory_data.h"
#include "proto.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern ProtoCtx protoCtx;
extern FactoryData factoryData;

/**
 * @brief Early setup for critical peripherals such as the serial/UART. Should never fail
 *
 * @return `ESP_FAIL` if errors were encountered
 */
esp_err_t Hal_EarlySetup(Boot_Mode mode);

esp_err_t Hal_Setup(Boot_Mode mode);

esp_err_t Hal_StartTasks(Boot_Mode mode);

#ifdef __cplusplus
}
#endif /* __cplusplus */