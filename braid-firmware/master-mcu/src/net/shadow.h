#pragma once

#include <esp_check.h>

#include "hal/gps.h"
#include "proto_payload.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Represents a shadow transaction action */
typedef enum Action {
    ACTION_OPTIONS = 1,
    ACTION_GET = 2,
    ACTION_HEAD = 3,
    ACTION_POST = 4,
    ACTION_PUT = 5,
    ACTION_DELETE = 6,
    ACTION_TRACE = 7,
} Action;

/** @brief The shadow metadata header */
typedef struct ShadowHeader {
    /// @brief Timestamp in milliseconds
    uint32_t ts;

    /// @brief Shadow version, incremented by the backend
    uint8_t ver;

    /// @brief Transmission protocol, fixed to 1
    uint8_t prot;

    /// @brief Action to perform when this shadow is received
    Action action;

    /// @brief Status code of the response/response
    uint16_t status;
} ShadowHeader;

/** Represents the actual payload object contained in the shadow */
typedef struct ShadowPayload {
    /// @brief The latest sensor data
    SensorData sensorData;

    /// @brief The latest GPS position
    GPS_Position gpsPosition;

    /// @brief Controls the time between network connections/reports
    uint64_t reportDelay;
} ShadowPayload;

/**
 * @brief Encode a shadow payload to CBOR
 *
 * @param version The shadow version. Must be the same as the last shadow sent by the backend
 * @param action Remote shadow action. Refer to specification for more info
 * @param[in] payload The payload to serialize
 * @param[out] buf The output buffer
 * @param bufSize Size of the output buffer for bounds check
 * @return The number of bytes actually written to the buffer
 */
size_t Shadow_Encode(uint8_t version, Action action, const ShadowPayload *payload, uint8_t *buf, size_t bufSize);

/**
 * @brief Decodes a shadow header and payload from CBOR
 *
 * @param[in] buf The data to decode
 * @param bufSize Size of the output buffer for bounds check
 * @param[out] payload Output payload struct
 * @param[out] header Output header struct. Can be set to NULL to skip deserialization
 * @return `ESP_FAIL` if deserialization errors were encountered, `ESP_OK` otherwise
 */
esp_err_t Shadow_Decode(const uint8_t *buf, size_t bufSize, ShadowPayload *payload, ShadowHeader *header);

#ifdef __cplusplus
}
#endif