#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @brief Hashes some data with a key using HMAC and SHA256
 *
 * @param key The key. Should be at least 32 bytes long for optimal security
 * @param keylen Length of the key buffer
 * @param[in] data The data to hash alongside the key
 * @param datalen Length of the data buffer
 * @param[out] out The output hash. Should be 32 bytes long. If it's less than 32 bytes, the resulting hash will be
 * truncated to the specified length
 * @param outlen Length of the output buffer
 * @return The number of bytes written to `out`
 */
size_t Crypto_HMAC(
    const void *key, const size_t keylen, const uint8_t *data, const size_t datalen, uint8_t *out, const size_t outlen);

#ifdef __cplusplus
}
#endif // __cplusplus