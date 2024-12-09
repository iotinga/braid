#pragma once

#include "lis2dh12_reg.h"
#include "stm32l0xx.h"

/*
 * @brief  Initialize a platform context
 *
 * @param  ctx    context to be initialized
 */
void lis2dh_init(stmdev_ctx_t *ctx);

/*
 * @brief  Tries to connect to the sensor
 *
 * @param  ctx    Context handle
 */
int8_t lis2dh_probe(const stmdev_ctx_t *ctx);

/*
 * @brief  Write generic device register (platform dependent)
 *
 * @param  handle    customizable argument. In this examples is used in
 *                   order to select the correct sensor bus handler.
 * @param  reg       register to write
 * @param  bufp      pointer to data to write in register reg
 * @param  len       number of consecutive register to write
 */
int32_t platform_write(void *handle, uint8_t reg, const uint8_t *bufp, uint16_t len);

/*
 * @brief  Read generic device register (platform dependent)
 *
 * @param  handle    customizable argument. In this examples is used in
 *                   order to select the correct sensor bus handler.
 * @param  reg       register to read
 * @param  bufp      pointer to buffer that store the data read
 * @param  len       number of consecutive register to read
 */
int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len);

/*
 * @brief  Send buffer to console (platform dependent)
 *
 * @param  tx_buffer     buffer to transmit
 * @param  len           number of byte to send
 */
void tx_com(uint8_t *tx_buffer, uint16_t len);

/*
 * @brief  platform specific delay (platform dependent)
 *
 * @param  ms        delay in ms
 */
void platform_delay(uint32_t ms);

/*
 * @brief  platform specific initialization (platform dependent)
 */
void platform_init(void);