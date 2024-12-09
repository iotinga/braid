#include <string.h>

#include "crypto_hmac.h"
#include "proto_payload.h"
#include "sha256.h"

bool PayloadVerify(SensorPayload *payload, const uint8_t *key, const size_t keyLength) {
    uint8_t hash[SHA256_HASH_SIZE];
    uint8_t *dataBytes = (uint8_t *)&payload->data;
    size_t written = 0;

    written = Crypto_HMAC(key, keyLength, dataBytes, sizeof(SensorData), hash, SHA256_HASH_SIZE);

    return memcmp(hash, payload->hash, written) == 0;
}

void PayloadHash(SensorPayload *payload, const void *key, const size_t keyLength) {
    const uint8_t *dataBytes = (const uint8_t *)&payload->data;

    memset(payload->hash, 0, SHA256_HASH_SIZE);
    Crypto_HMAC(key, keyLength, dataBytes, sizeof(SensorData), payload->hash, SHA256_HASH_SIZE);
}
