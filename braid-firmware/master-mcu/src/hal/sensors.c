#include <driver/gpio.h>
#include <driver/uart.h>
#include <esp_check.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <string.h>

#include "core/factory_data.h"
#include "hal/flash.h"
#include "log_extra.h"
#include "proto_payload.h"
#include "sensors.h"

static const char *TAG = "hal/sensors";

static QueueHandle_t uart_queue;
static SensorData sensorData = {
    .humidity = 0.0f,
    .temperature = 0.0f,
    .acceleration_mg = {0, 0, 0},
};
static SemaphoreHandle_t sensorDataMutex;

static int Sensors_ProtoWrite(void *writeCtx, size_t length, const uint8_t *data);
static int Sensors_ProtoRead(void *readCtx, size_t length, uint8_t *data);
static void Sensors_MsgCallback(void *cbCtx, ProtoMsgType msgType, uint8_t *payload);

esp_err_t Sensors_UARTInit(ProtoCtx *protoCtx, int uartNum, gpio_num_t txPin, gpio_num_t rxPin) {
    uart_config_t uart_config;
    uart_config.baud_rate = 115200;
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.parity = UART_PARITY_DISABLE;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uart_config.source_clk = UART_SCLK_DEFAULT;

    ESP_ERROR_CHECK(uart_param_config(uartNum, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(uartNum, txPin, rxPin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(uartNum, SENSORS_UART_BUFFER_SIZE, 0, 10, &uart_queue, 0));
    ESP_LOGD(TAG, "Created sensors UART");

    sensorDataMutex = xSemaphoreCreateMutex();
    if (sensorDataMutex == NULL) {
        ESP_LOGE(TAG, "Error creating mutex");
        return ESP_FAIL;
    }

    protoCtx->write = Sensors_ProtoWrite;
    protoCtx->read = Sensors_ProtoRead;
    protoCtx->messageCallback = Sensors_MsgCallback;

    return ESP_OK;
}

esp_err_t Sensors_GetLastData(SensorData *out) {
    esp_err_t status = ESP_ERR_NOT_ALLOWED;

    if (xSemaphoreTake(sensorDataMutex, portMAX_DELAY) == pdTRUE) {
        memcpy(out, &sensorData, sizeof(SensorData));
        xSemaphoreGive(sensorDataMutex);
        status = ESP_OK;
    }

    return status;
}

esp_err_t Sensors_SaveData() {
    if (xSemaphoreTake(sensorDataMutex, portMAX_DELAY) == pdTRUE) {
        esp_err_t status = Flash_Save(PARTITION_USER, "sensors", &sensorData, sizeof(SensorData));
        xSemaphoreGive(sensorDataMutex);
        return status;
    }

    return ESP_ERR_TIMEOUT;
}

esp_err_t Sensors_LoadData() {
    if (xSemaphoreTake(sensorDataMutex, portMAX_DELAY) == pdTRUE) {
        esp_err_t status = Flash_Load(PARTITION_USER, "sensors", &sensorData, sizeof(SensorData));
        xSemaphoreGive(sensorDataMutex);
        return status;
    }
    return ESP_ERR_TIMEOUT;
}

/**
 * @brief Implementation for `ProtoCtx.write`
 */
static int Sensors_ProtoWrite(void *writeCtx, size_t length, const uint8_t *data) {
    return uart_write_bytes(UART_NUM_2, data, length);
}

/**
 * @brief Implementation for `ProtoCtx.read`
 */
static int Sensors_ProtoRead(void *readCtx, size_t length, uint8_t *data) {
    return uart_read_bytes(UART_NUM_2, data, length, 100);
}

static void printData(SensorPayload *payload) {
    ESP_LOGD(TAG,
             "temp: %.3f\thum: %.3f\taccel: % 0.3fg (x) % 0.3fg (y) % 0.3fg (z)",
             payload->data.temperature / 1000.0,
             payload->data.humidity / 1000.0,
             payload->data.acceleration_mg[0] / 1000.0,
             payload->data.acceleration_mg[1] / 1000.0,
             payload->data.acceleration_mg[2] / 1000.0);
}

static void printPayload(uint8_t *payload) {
    size_t offset = 0;

    ESP_LOG_LINE_BEGIN(ESP_LOG_VERBOSE, TAG, "data[ ");
    for (offset = 0; offset < sizeof(SensorData); offset++) {
        ESP_LOG_LINE(TAG, "%02x ", *(payload + offset));
    }
    ESP_LOG_LINE(TAG, "]\t");

    ESP_LOG_LINE(TAG, "hash[ ");
    for (offset = 0; offset < 32; offset++) {
        ESP_LOG_LINE(TAG, "%02x ", *(payload + sizeof(SensorData) + offset));
    }
    ESP_LOG_LINE_END(TAG, "]");
}

/**
 * @brief Implementation for `ProtoCtx.messageCallback`
 */
static void Sensors_MsgCallback(void *cbCtx, ProtoMsgType msgType, uint8_t *payload) {
    FactoryData *factoryData = (FactoryData *)cbCtx;
    bool verified = PayloadVerify((SensorPayload *)payload, factoryData->key, HMAC_KEY_LENGTH);
    ESP_LOGV(TAG, "Received message of type 0x%02x: payload is %s", msgType, verified ? "verified" : "invalid");
    if (!verified) {
        ESP_LOGD(TAG, "Discarding invalid message of type 0x%02x", msgType);
        return;
    }

    switch (msgType) {
    case PROTO_MSG_TYPE_PING:
        ESP_LOGI(TAG, "Connected to sensors MCU");
        break;
    case PROTO_MSG_TYPE_RESPONSE:
        printPayload(payload);
        printData((SensorPayload *)payload);

        if (xSemaphoreTake(sensorDataMutex, portMAX_DELAY) == pdTRUE) {
            sensorData = ((SensorPayload *)payload)->data;
            xSemaphoreGive(sensorDataMutex);
        }
        break;
    default:
        break;
    }
}