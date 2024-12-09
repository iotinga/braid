/* Cryptoauthlib Configuration File */
#pragma once

/* Include HALS */
#define ATCA_HAL_I2C

/* Included device support */
#define ATCA_ATECC608_SUPPORT

#ifndef ATCA_POST_DELAY_MSEC
/**
 * @brief How long to wait after an initial wake failure for the POST to complete. If Power-on self test (POST) is
 * enabled, the self test will run on waking from sleep or during power-on, which delays the wake reply.
 */
#define ATCA_POST_DELAY_MSEC 25
#endif
