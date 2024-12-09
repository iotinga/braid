#include "sensors/lis2dh.h"
#include "i2c.h"
#include "main.h"

void lis2dh_init(stmdev_ctx_t *ctx) {
    ctx->handle = &hi2c1;
    ctx->read_reg = platform_read;
    ctx->write_reg = platform_write;
    ctx->mdelay = platform_delay;

    // Enable Block Data Update
    lis2dh12_block_data_update_set(ctx, 1);
    // Set Output Data Rate to 50Hz
    lis2dh12_data_rate_set(ctx, LIS2DH12_ODR_50Hz);
    // Set full scale to 2g
    lis2dh12_full_scale_set(ctx, LIS2DH12_2g);
    // Disable temperature sensor
    lis2dh12_temperature_meas_set(ctx, LIS2DH12_TEMP_DISABLE);
    // Set device in continuous mode with 12 bit resolution
    lis2dh12_operating_mode_set(ctx, LIS2DH12_HR_12bit);
    // Enable internal pullup for SA0 (required for i2c)
    lis2dh12_pin_sdo_sa0_mode_set(ctx, LIS2DH12_PULL_UP_CONNECT);
}

int8_t lis2dh_probe(const stmdev_ctx_t *ctx) {
    uint8_t deviceId;
    lis2dh12_device_id_get(ctx, &deviceId);
    if (deviceId != LIS2DH12_ID) {
        return 1;
    }

    return 0;
}

int32_t platform_write(void *handle, uint8_t reg, const uint8_t *bufp, uint16_t len) {
    /* Write multiple command */
    reg |= 0x80;
    return (int32_t)HAL_I2C_Mem_Write((I2C_HandleTypeDef *)handle,
                                      LIS2DH12_I2C_ADD_L,
                                      reg,
                                      I2C_MEMADD_SIZE_8BIT,
                                      (uint8_t *)bufp,
                                      len,
                                      DEFAULT_I2C_TIMEOUT);
}

int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len) {
    /* Read multiple command */
    reg |= 0x80;
    return (int32_t)HAL_I2C_Mem_Read(
        (I2C_HandleTypeDef *)handle, LIS2DH12_I2C_ADD_L, reg, I2C_MEMADD_SIZE_8BIT, bufp, len, DEFAULT_I2C_TIMEOUT);
}

void platform_delay(uint32_t ms) {
    HAL_Delay(ms);
}

void tx_com(uint8_t *tx_buffer, uint16_t len) {
}

void platform_init(void) {
}