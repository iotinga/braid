#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct SensorData {
    int32_t temperature;
    int32_t humidity;
    float acceleration_mg[3];
} __attribute__((packed, aligned(4))) SensorData;

typedef struct SensorPayload {
    /** @brief The actual data */
    SensorData data;

    /** @brief The HMAC SHA256 hash of this message */
    uint8_t hash[32];
} __attribute__((packed, aligned(4))) SensorPayload;

/**
 * @brief Verifies a payload containing a HMAC-SHA256 hash
 *
 * @param payload Payload to verify
 * @param[in] key The key used for hashing
 * @param keyLength Length of the key
 * @return - `true` if the payload hash and the calculated hash are the same
 * @return - `false` if the payload hash and the calculated hash DO NOT match
 */
bool PayloadVerify(SensorPayload *payload, const uint8_t *key, const size_t keyLength);

/**
 * @brief Hashes the data and stores the hash in the `hash` field of `SensorPayload`
 *
 * @param[in, out] payload Payload to hash data in
 * @param[in] key The key used for hashing
 * @param keyLength Length of the key
 */
void PayloadHash(SensorPayload *payload, const void *key, const size_t keyLength);

#ifdef __cplusplus
}
#endif /* __cplusplus */