#include "esp_modem_api.h"
#include "esp_timer.h"
#include <driver/gpio.h>
#include <driver/uart.h>
#include <esp_check.h>
#include <esp_log.h>
#include <esp_vfs_common.h>
#include <esp_vfs_dev.h>
#include <esp_vfs_usb_serial_jtag.h>
#include <stdint.h>

#include "cli/serial.h"
#include "core/factory_data.h"
#include "core/time.h"
#include "hal.h"
#include "hal/anti_tamper.h"
#include "hal/flash.h"
#include "hal/gprs.h"
#include "hal/gps.h"
#include "hal/lilygo/board.h"
#include "hal/modem.h"
#include "log_extra.h"
#include "main.h"
#include "net/mqtt.h"
#include "net/shadow.h"
#include "proto.h"
#include "proto_payload.h"
#include "sensors.h"

static esp_modem_dce_t *modem;
static const char *TAG = "hal/lilygo";

static GPRS_Params modemParams;

void Task_GPRS(void *arg);
void Task_GPS(void *arg);
void Task_Sensors(void *arg);
void Task_AntiTamper(void *arg);

esp_err_t Hal_EarlySetup(Boot_Mode mode) {
    ESP_ERROR_CHECK(SerialUARTInit(UART_NUM_0, 115200));

    return ESP_OK;
}

esp_err_t Hal_Setup(Boot_Mode mode) {
    esp_err_t status;

    // Setup antitamper on generic pins
    status = Tamper_Setup(GPIO_NUM_23);
    if (status != ESP_OK) {
        return status;
    }

    ESP_ERROR_CHECK(Sensors_UARTInit(&protoCtx, UART_NUM_2, GPIO_NUM_32, GPIO_NUM_33));

    // Create netif
    esp_netif_t *netif = GPRS_NewNetif();

    // Initialize modem
    modem = Modem_Init(netif, factoryData.simApn, BOARD_MODEM_TXD_PIN, BOARD_MODEM_RXD_PIN, BOARD_MODEM_PWR_PIN);

    if (mode == BOOT_GPS) {
        // Configure GNSS
        ESP_ERROR_CHECK(GPS_Configure(modem, GPS_GALILEO));
    } else if (mode == BOOT_MQTT) {
        modemParams = (GPRS_Params){
            .apn = factoryData.simApn,
            .pin = factoryData.simPinRequired ? factoryData.simPin : NULL,
            .networkMode = NETWORK_GSM,
            .connectionMode = CONN_CATM,
        };
        // Setup GPRS
        ESP_ERROR_CHECK(GPRS_Connect(modem, &modemParams));

        // Connect to MQTT
        ESP_ERROR_CHECK(Mqtt_Init(&factoryData));
    }

    return ESP_OK;
}

esp_err_t Hal_StartTasks(Boot_Mode mode) {
    xTaskCreate(&Task_AntiTamper, "TAMPER", DEFAULT_STACK_SIZE, NULL, DEFAULT_PRIORITY + 1, NULL);
    xTaskCreate(&Task_Sensors, "SENSORS", DEFAULT_STACK_SIZE, NULL, DEFAULT_PRIORITY, NULL);

    if (mode == BOOT_GPS) {
        xTaskCreate(&Task_GPS, "GNSS", DEFAULT_STACK_SIZE, NULL, DEFAULT_PRIORITY, NULL);
    } else if (mode == BOOT_MQTT) {
        xTaskCreate(&Task_GPRS, "GPRS", DEFAULT_STACK_SIZE * 6, NULL, DEFAULT_PRIORITY + 1, NULL);
    }

    return ESP_OK;
}

void Task_Sensors(void *arg) {
    Boot_RegisterTask();

    if (ProtoPing(&protoCtx) != PROTO_SUCCESS) {
        ESP_LOGE(TAG, "task_sensors: failed to ping");
        errorHandler();
    }

    while (true) {
        if (Boot_IsShutdownPending()) {
            ESP_LOGI(TAG, "Task Sensors shutting down");
            Sensors_SaveData();
            Boot_SetShutdownReady();
            break;
        }

        if (ProtoProcessMessage(&protoCtx) != PROTO_SUCCESS) {
            ESP_LOGE(TAG, "task_sensors: failed during cmd processing");
            errorHandler();
        }

        if (ProtoReceive(&protoCtx) != PROTO_SUCCESS) {
            ESP_LOGE(TAG, "task_sensors: failed during recv");
            errorHandler();
        }

        vTaskDelay(1);
    }

    vTaskDelete(NULL);
}

void Task_GPS(void *arg) {
    GPS_Position gpsPos;
    GPS_Date gpsDate;

    Boot_RegisterTask();

    while (true) {
        if (Boot_IsShutdownPending()) {
            ESP_LOGI(TAG, "Task GPS shutting down");
            GPS_SaveData(&gpsPos);
            Modem_Poweroff(modem, BOARD_MODEM_PWR_PIN);
            vTaskDelay(pdMS_TO_TICKS(3000));
            Boot_SetShutdownReady();
            break;
        }

        // Acquire GPS position
        int attempt = 0;
        while (!GPS_GetData(modem, &gpsPos, &gpsDate) && attempt++ < 5) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
        GPS_PrintData(&gpsPos, NULL);
    }

    vTaskDelete(NULL);
}

static void Mqtt_ShadowHandler(const char *topic, const uint8_t *data, size_t length) {
    ESP_LOGD(TAG, "Received message (of length %d) on topic %s", length, topic);
    ShadowPayload pl;
    ShadowHeader h;

    if (Shadow_Decode(data, length, &pl, &h) != ESP_OK) {
        ESP_LOGW(TAG, "Received invalid shadow");
        return;
    }

    ESP_LOG_LINE_BEGIN(ESP_LOG_DEBUG, TAG, "received shadow: { ");
    ESP_LOG_LINE(TAG, "ts:%lu act:%d st:%d pl:", h.ts, h.action, h.status);
    ESP_LOG_LINE(TAG, "{ d:%llu }", pl.reportDelay);
    ESP_LOG_LINE_END(TAG, " }");

    // The time between reports depends on the duration of the GPS task
    Boot_SetDuration(BOOT_GPS, pl.reportDelay);
}

void Task_GPRS(void *arg) {
    time_t epoch;
    static uint8_t shadowBuf[1024];
    char topicBuf[256];
    ShadowPayload payload;

    Boot_RegisterTask();

    while (true) {
        // Time sync
        epoch = GPRS_GetDatetime(modem);
        ESP_LOGD(TAG, "Current time is %lld", epoch);
        Time_Set(epoch);

        // Fetch sensor data
        Sensors_LoadData();
        Sensors_GetLastData(&payload.sensorData);
        // Populate GPS position
        GPS_LoadData(&payload.gpsPosition);
        // Encode the shadow
        int actualSize = Shadow_Encode(1, ACTION_PUT, &payload, shadowBuf, sizeof(shadowBuf));
        if (actualSize > 0) {
            ESP_LOGD(TAG, "Successfully encoded shadow");
        }

        // Connect to the broker
        Mqtt_Connect();

        // Subscribe to relevant topics
        memset(topicBuf, 0, sizeof(topicBuf));
        Mqtt_ComposeTopicSub(
            factoryData.deviceId, MQTT_TOPIC_TYPE_AGENT, MQTT_TOPIC_DESIRED, "ADMIN", topicBuf, sizeof(topicBuf));
        Mqtt_Sub(topicBuf, Mqtt_ShadowHandler);

        // Publish messages
        memset(topicBuf, 0, sizeof(topicBuf));
        Mqtt_ComposeTopicPub(
            factoryData.deviceId, MQTT_TOPIC_TYPE_AGENT, MQTT_TOPIC_REPORTED, topicBuf, sizeof(topicBuf));
        ESP_ERROR_CHECK(Mqtt_Pub(topicBuf, shadowBuf, actualSize));

        while (true) {
            if (Boot_IsShutdownPending()) {
                ESP_LOGI(TAG, "Task GPRS shutting down");
                Mqtt_Disconnect();
                Modem_Poweroff(modem, BOARD_MODEM_PWR_PIN);
                vTaskDelay(pdMS_TO_TICKS(3000));
                Boot_SetShutdownReady();
            }

            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    vTaskDelete(NULL);
}

void Task_AntiTamper(void *arg) {
    gpio_num_t pin;
    int last_pin_state = -1;
    int current_pin_state;
    int64_t last_event_time = 0;
    int64_t current_time_us;
    TamperEvent event;

    while (true) {
        if (Tamper_CheckForTamper(&pin)) {
            current_time_us = esp_timer_get_time();
            current_pin_state = gpio_get_level(pin);

            // Debounce the antitamper pin
            if (current_time_us - last_event_time > ANTITAMPER_TIME_US) {
                if (current_pin_state != last_pin_state) {
                    last_pin_state = current_pin_state;
                    last_event_time = current_time_us;

                    if (current_pin_state == 0) {
                        event.pin = pin;
                        event.timestamp = current_time_us;

                        ESP_ERROR_CHECK(Tamper_RegisterEvent(&event));
                        ESP_LOGI(TAG, "Tamper detected, erasing factory data");
                        ESP_ERROR_CHECK(FactoryData_Erase());
                        esp_restart();
                    }
                }
            }
        }
        vTaskDelay(1);
    }
    vTaskDelete(NULL);
}
