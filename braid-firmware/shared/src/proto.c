#include <stdio.h>
#include <string.h>

#include "build_config.h"
#include "crc.h"
#include "proto.h"

size_t uartRxBufferIdx;
uint8_t uartRxBuffer[PROTO_MSG_MAX_LEN];
uint8_t uartPayloadBuffer[PROTO_MSG_PAYLOAD_MAX_LEN];

static volatile bool txDone;

enum UartState {
    UART_STATE_WAIT_START,
    UART_STATE_RECV,
    UART_STATE_EXEC,
    UART_STATE_CRC_ERROR,
};
volatile enum UartState uartState = UART_STATE_WAIT_START;

static ProtoErrorCode Respond(ProtoCtx *ctx, ProtoErrorCode response) {
    return ProtoSend(ctx, PROTO_MSG_TYPE_RESPONSE, 1, (uint8_t *)&response);
}

ProtoErrorCode ProtoProcessMessage(ProtoCtx *ctx) {
    ProtoErrorCode result = PROTO_SUCCESS;
    int payloadLength = 0;

    /* check if there is a command to execute */
    switch (uartState) {
    case UART_STATE_EXEC: {
        ProtoMsgType msgType = uartRxBuffer[PROTO_MSG_TYP_OFFSET];
        /* process the message here */
        switch (msgType) {
        case PROTO_MSG_TYPE_RESPONSE:
            // fall through
        case PROTO_MSG_TYPE_PING:
            // fall through
        case PROTO_MSG_TYPE_SENSOR_REQUEST:
            payloadLength = uartRxBuffer[PROTO_MSG_LEN_OFFSET];
            memcpy(uartPayloadBuffer, &uartRxBuffer[PROTO_MSG_PAYLOAD_OFFSET], payloadLength);
            if (ctx->messageCallback != NULL) {
                ctx->messageCallback(ctx->messageCallbackCtx, msgType, uartPayloadBuffer);
            }
            break;
        default:
            /* unknown command */
            result = PROTO_ERROR_INVALID_CMD;
            break;
        }

        /* make the Innophase know that the command was successfully accepted/rejected */
        result = Respond(ctx, result);

        /* done processing: wait for another command to start */
        uartState = UART_STATE_WAIT_START;

        break;
    }
    case UART_STATE_CRC_ERROR:
        result = Respond(ctx, PROTO_ERROR_CRC);
        uartState = UART_STATE_WAIT_START;
        break;

    default:
        break;
    }

    return result;
}

ProtoErrorCode ProtoSend(ProtoCtx *ctx, ProtoMsgType type, uint8_t payloadLength, const uint8_t *payload) {
    static uint8_t sendBuffer[PROTO_MSG_MAX_LEN];

    /* add start byte */
    sendBuffer[0] = PROTO_MSG_START_BYTE;
    sendBuffer[1 + PROTO_MSG_TYP_OFFSET] = (uint8_t)type;
    sendBuffer[1 + PROTO_MSG_LEN_OFFSET] = payloadLength;

    /* add payload bytes */
    for (size_t i = 0; i < payloadLength; i++)
        sendBuffer[i + PROTO_MSG_LEN_OFFSET + 2] = payload[i];

    /* compute CRC */
    uint16_t crc = Crc16(payloadLength + 2, sendBuffer + 1);
    sendBuffer[payloadLength + 3] = (uint8_t)((crc >> 8) & 0xFF);
    sendBuffer[payloadLength + 4] = (uint8_t)(crc & 0xFF);

    /* send payload */
    int messageLength = payloadLength + 5;
    if (ctx->write(ctx->writeCtx, messageLength, sendBuffer) != messageLength) {
        return PROTO_ERROR_HAL;
    }

    return PROTO_SUCCESS;
}

/**
 * After detecting the packet start byte, read all bytes and check the CRC
 */
static ProtoErrorCode RecvLoop(ProtoCtx *ctx) {
    uint8_t rxChar = 0;

    while (true) {
        int readRet = ctx->read(ctx->readCtx, 1, &rxChar);
        if (readRet < 0) {
            return PROTO_ERROR_HAL;
        } else if (readRet > 0) {
            uartRxBuffer[uartRxBufferIdx++] = rxChar;
        }

        if (uartRxBufferIdx > PROTO_MSG_LEN_OFFSET &&
            uartRxBufferIdx == (size_t)(4 + uartRxBuffer[PROTO_MSG_LEN_OFFSET])) {
            if (CheckCrc16(uartRxBufferIdx, uartRxBuffer)) {
                uartState = UART_STATE_EXEC;
            } else {
                uartState = UART_STATE_CRC_ERROR;
                return PROTO_ERROR_CRC;
            }
            break;
        }
    }

    return PROTO_SUCCESS;
}

ProtoErrorCode ProtoReceive(ProtoCtx *ctx) {
    uint8_t rxChar = 0;

    switch (uartState) {
    case UART_STATE_WAIT_START:
        if (ctx->read(ctx->readCtx, 1, &rxChar) < 0) {
            return PROTO_ERROR_HAL;
        }

        if (rxChar == PROTO_MSG_START_BYTE) {
            uartRxBufferIdx = 0;
            uartState = UART_STATE_RECV;
        }
        break;
    case UART_STATE_RECV:
        return RecvLoop(ctx);
    case UART_STATE_EXEC:
        break;
    case UART_STATE_CRC_ERROR:
        return PROTO_ERROR_CRC;
    }

    return PROTO_SUCCESS;
}

ProtoErrorCode ProtoPing(ProtoCtx *ctx) {
    uint8_t version[3] = {CFG_FW_VERSION_MAJOR, CFG_FW_VERSION_MINOR, CFG_FW_VERSION_PATCH};

    return ProtoSend(ctx, PROTO_MSG_TYPE_PING, 3, version);
}
