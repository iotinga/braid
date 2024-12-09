#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*!
 * Message structure
 *
 * +------+-----+-----+---------+-----+
 * | 0xA5 | TYP | LEN | PAYLOAD | CRC |
 * +------+-----+-----+---------+-----+
 *
 * 0xA5     - start byte
 * TYP      - message type (1 byte)
 * LEN      - payload length (1 byte)
 * PAYLOAD  - optional payload of LEN bytes
 * CRC      - CRC 16 of TYP, LEN and PAYLOAD (MSB)
 */

/// @brief Message start magic byte
#define PROTO_MSG_START_BYTE 0xA5
/// @brief Maximum length in bytes of the payload
#define PROTO_MSG_PAYLOAD_MAX_LEN (0xff)
/// @brief Maximum length in bytes of the whole message
#define PROTO_MSG_MAX_LEN (1 + 1 + 1 + PROTO_MSG_PAYLOAD_MAX_LEN + 2)

/// @brief Index for the message type byte (the magic byte is discarded)
#define PROTO_MSG_TYP_OFFSET 0
/// @brief Index for the message length byte (the magic byte is discarded)
#define PROTO_MSG_LEN_OFFSET 1
/// @brief Index for the beginning of the payload (the magic byte is discarded)
#define PROTO_MSG_PAYLOAD_OFFSET 2

/**
 * @enum ProtoMsgType
 * @brief Describes the various message types. This value can be found at `PROTO_MSG_TYP_OFFSET` inside the message
 * packet
 */
typedef enum ProtoMsgType {
    /** Command response. Payload should be the status code as 1 byte */
    PROTO_MSG_TYPE_RESPONSE = 0x00,

    /** Firmware started. Payload should be the fimware version as 3 bytes (major, minor, patch) */
    PROTO_MSG_TYPE_PING = 0x01,

    /** Sensor request command */
    PROTO_MSG_TYPE_SENSOR_REQUEST = 0x02,
} ProtoMsgType;

/**
 * @struct ProtoCtx
 * @brief Implementation context for the protocol functions
 */
typedef struct ProtoCtx {
    /**
     * @brief HAL implementation for the UART write function.
     * Must return the number of bytes written for error checking
     *
     * @param writeCtx This context
     * @param length Length of the data buffer
     * @param data The data buffer to be written
     * @return The number of bytes written
     */
    int (*write)(void *writeCtx, size_t length, const uint8_t *data);

    /**
     * @brief HAL implementation for the UART read function.
     * Must return the number of bytes read for error checking.
     *
     * @note Can return:
     * @note - `0` in case there were no bytes to read (FIFO empty)
     * @note - `-1` in case of error. In this case `ProtoReceive` will return `PROTO_ERROR_HAL`
     *
     * @param writeCtx This context
     * @param length Length of the data buffer
     * @param data The data buffer to be filled
     * @return The number of bytes written
     */
    int (*read)(void *readCtx, size_t length, uint8_t *data);

    /**
     * @brief Callback called when a valid packet is received and checked for integrity.
     * Will never be called if set to `NULL`
     *
     * @param msgType The message type from the enum
     * @param payload The payload section is `memcpy`ed here
     */
    void (*messageCallback)(void *messageCallbackCtx, ProtoMsgType msgType, uint8_t *payload);

    /** @brief Extra arguments for the write function */
    void *writeCtx;

    /** @brief Extra arguments for the read function */
    void *readCtx;

    /** @brief Extra arguments for the message callback function */
    void *messageCallbackCtx;
} ProtoCtx;

/**
 * @enum ProtoErrorCode
 * @brief Enumeration for the protocol error codes
 */
typedef enum ProtoErrorCode {
    /** Operation successful */
    PROTO_SUCCESS = 0x00,

    /** CRC error detected */
    PROTO_ERROR_CRC = 0x01,

    /** Invalid command received */
    PROTO_ERROR_INVALID_CMD = 0x02,

    /** Invalid argument passed to function */
    PROTO_ERROR_INVALID_ARG = 0x03,

    /** Invalid state encountered */
    PROTO_ERROR_INVALID_STATE = 0x04,

    /** HAL error occurred */
    PROTO_ERROR_HAL = 0x05,

    /** General failure */
    PROTO_ERROR_FAILURE = 0xff,
} ProtoErrorCode;

/**
 * @brief Processes the current message packet, if any
 *
 * @param ctx The protocol context
 * @return ProtoErrorCode indicating success or type of failure
 */
ProtoErrorCode ProtoProcessMessage(ProtoCtx *ctx);

/**
 * @brief Sends a ping message using the protocol context
 *
 * @param ctx The protocol context
 * @return ProtoErrorCode indicating success or type of failure
 */
ProtoErrorCode ProtoPing(ProtoCtx *ctx);

/**
 * @brief Sends a message using the protocol context
 *
 * @param ctx The protocol context
 * @param type The message type
 * @param payloadLength The length of the payload
 * @param payload The payload data to be sent
 * @return ProtoErrorCode indicating success or type of failure
 */
ProtoErrorCode ProtoSend(ProtoCtx *ctx, ProtoMsgType type, uint8_t payloadLength, const uint8_t *payload);

/**
 * @brief Receives a message using the protocol context. Must be called in a loop.
 *
 * @param ctx The protocol context
 * @return ProtoErrorCode indicating success or type of failure
 */
ProtoErrorCode ProtoReceive(ProtoCtx *ctx);

#ifdef __cplusplus
}
#endif /* __cplusplus */