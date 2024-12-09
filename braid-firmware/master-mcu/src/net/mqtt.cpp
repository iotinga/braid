#include "esp_crt_bundle.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "mqtt_client.h"
#include <functional>
#include <map>
#include <string>

#include "certs/atecc_utils.h"
#include "core/time.h"
#include "defines.h"
#include "hal/flash.h"
#include "mqtt.h"

static const char *TAG = "net/mqtt";

static EventGroupHandle_t event_group = NULL;
static const int CONNECTED_BIT = BIT0;
static const int DISCONNECTED_BIT = BIT1;
static const int RX_DATA_BIT = BIT2;
static const int TX_DATA_BIT = BIT3;

static char root_ca_buf[1536] = {0};
static char dev_cert_buf[1536] = {0};
static esp_mqtt5_client_handle_t mqtt_client = NULL;
std::map<std::string, std::function<void(const char *, const uint8_t *, size_t)>> topicHandlers;

static const char *topicTypes[] = {
    [MQTT_TOPIC_TYPE_AGENT] = "agent",
    [MQTT_TOPIC_TYPE_ENTITY] = "entity",
};

static const char *topicsPub[] = {
    [MQTT_TOPIC_REPORTED] = "shadow/reported/",
    [MQTT_TOPIC_REPORTED_BATCH] = "shadow/reported/batch",
    [MQTT_TOPIC_REPORTED_LIVE] = "shadow/reported/live",
    [MQTT_TOPIC_COMMAND_PUB] = "command",
};

static const char *topicsSub[] = {
    [MQTT_TOPIC_DESIRED] = "shadow/desired",
    [MQTT_TOPIC_DESIRED_BATCH] = "shadow/desired/batch",
    [MQTT_TOPIC_COMMAND_SUB] = "command",
};

static void Mqtt_UnknownTopicHandler(const uint8_t *data, size_t length) {
    ESP_LOGW(TAG, "Received message (of length %d) for unknown topic", length);
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIu32, base, event_id);
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGD(TAG, "MQTT_EVENT_CONNECTED");
        xEventGroupSetBits(event_group, CONNECTED_BIT);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGD(TAG, "MQTT_EVENT_DISCONNECTED");
        xEventGroupSetBits(event_group, DISCONNECTED_BIT);
        break;
    case MQTT_EVENT_SUBSCRIBED: {
        ESP_LOGD(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    }
    case MQTT_EVENT_UNSUBSCRIBED: {
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED topic=%.*s msg_id=%d", event->topic_len, event->topic, event->msg_id);
        break;
    }
    case MQTT_EVENT_PUBLISHED: {
        xEventGroupClearBits(event_group, TX_DATA_BIT);
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    }
    case MQTT_EVENT_DATA: {
        ESP_LOGI(TAG, "MQTT_EVENT_DATA topic=%.*s len=%d", event->topic_len, event->topic, event->data_len);
        xEventGroupSetBits(event_group, RX_DATA_BIT);
        // Dispatch to appropriate handler
        auto it = topicHandlers.find(event->topic);
        if (it != topicHandlers.end()) {
            it->second(event->topic, (const uint8_t *)event->data, event->data_len);
        } else {
            // Handle unknown topics
            Mqtt_UnknownTopicHandler((const uint8_t *)event->data, event->data_len);
        }
        break;
    }
    case MQTT_EVENT_ERROR: {
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;
    }
    default: {
        ESP_LOGI(TAG, "MQTT other event id: %d", event->event_id);
        break;
    }
    }
}

esp_err_t Mqtt_Init(FactoryData *factoryData) {
    event_group = xEventGroupCreate();
    assert(event_group);

    ESP_ERROR_CHECK(Flash_Load(PARTITION_FACTORY, "root_ca", root_ca_buf, sizeof(root_ca_buf)));
    ESP_LOGD(TAG, "Loaded root CA");

    ESP_ERROR_CHECK(Flash_Load(PARTITION_FACTORY, "device_cert", dev_cert_buf, sizeof(dev_cert_buf)));
    ESP_LOGD(TAG, "Loaded device cert");

    esp_mqtt_client_config_t mqtt_config;
    mqtt_config.broker.address.uri = factoryData->mqttUri;
    mqtt_config.broker.verification.skip_cert_common_name_check = true;
    mqtt_config.broker.verification.certificate = root_ca_buf;
    mqtt_config.credentials.username = factoryData->deviceId;
    mqtt_config.credentials.client_id = factoryData->deviceId;
    mqtt_config.credentials.authentication.use_secure_element = true;
    mqtt_config.credentials.authentication.certificate = dev_cert_buf;
    mqtt_config.session.keepalive = 60;
    mqtt_config.session.protocol_ver = MQTT_PROTOCOL_V_5;

    mqtt_client = esp_mqtt_client_init(&mqtt_config);
    assert(mqtt_client);
    ESP_ERROR_CHECK(
        esp_mqtt_client_register_event(mqtt_client, (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID, mqtt_event_handler, NULL));

    xEventGroupClearBits(event_group, CONNECTED_BIT | DISCONNECTED_BIT | RX_DATA_BIT | TX_DATA_BIT);

    return ESP_OK;
}

esp_err_t Mqtt_Connect() {
    assert(mqtt_client);
    xEventGroupClearBits(event_group, CONNECTED_BIT | RX_DATA_BIT);
    esp_err_t status = esp_mqtt_client_start(mqtt_client);
    xEventGroupWaitBits(event_group, CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    if (status == ESP_OK) {
        ESP_LOGI(TAG, "Connected to MQTT broker");
    }
    return status;
}

esp_err_t Mqtt_Disconnect() {
    assert(mqtt_client);
    xEventGroupWaitBits(event_group, TX_DATA_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    xEventGroupClearBits(event_group, DISCONNECTED_BIT);
    esp_err_t status = esp_mqtt_client_stop(mqtt_client);
    xEventGroupWaitBits(
        event_group, DISCONNECTED_BIT, pdFALSE, pdFALSE, pdMS_TO_TICKS(SEC_TO_MS(MQTT_TIMEOUT_SECONDS)));
    if (status == ESP_OK) {
        ESP_LOGI(TAG, "Disconnected from MQTT broker");
    }
    return status;
}

esp_err_t Mqtt_HandleMessages(uint32_t seconds) {
    uint32_t start = esp_timer_get_time() / 1000;
    uint32_t ms = SEC_TO_MS(seconds);

    do {
        xEventGroupWaitBits(event_group, RX_DATA_BIT, pdFALSE, pdFALSE, pdMS_TO_TICKS(ms));
        xEventGroupClearBits(event_group, RX_DATA_BIT);
    } while (((esp_timer_get_time() / 1000) - start) < ms);

    return ESP_OK;
}

int Mqtt_ComposeTopicPub(const char *deviceId, Mqtt_TopicType type, Mqtt_TopicPub topic, char *out, size_t outSize) {
    return snprintf(out, outSize, MQTT_TOPIC_ROOT "/%s/%s/%s", topicTypes[type], deviceId, topicsPub[topic]);
}

int Mqtt_ComposeTopicSub(
    const char *deviceId, Mqtt_TopicType type, Mqtt_TopicSub topic, const char *role, char *out, size_t outSize) {
    return snprintf(out, outSize, MQTT_TOPIC_ROOT "/%s/%s/%s/%s", topicTypes[type], deviceId, topicsSub[topic], role);
}

esp_err_t Mqtt_Pub(const char *topic, const void *buf, size_t bufSize) {
    esp_err_t status = ESP_OK;

    ESP_LOGD(TAG, "Publishing buffer of size %d on '%s'", bufSize, topic);
    xEventGroupSetBits(event_group, TX_DATA_BIT);
    int msg_id = esp_mqtt_client_publish(mqtt_client, topic, (const char *)buf, bufSize, 0, 0);
    switch (msg_id) {
    case -1:
        xEventGroupClearBits(event_group, TX_DATA_BIT);
        return ESP_FAIL;
    case -2:
        xEventGroupClearBits(event_group, TX_DATA_BIT);
        ESP_LOGW(TAG, "Outbox for '%s' is full", topic);
        return ESP_FAIL;
    }

    return status;
}

esp_err_t Mqtt_Sub(const char *topic, void (*handler)(const char *, const uint8_t *, size_t)) {
    int msg_id = esp_mqtt_client_subscribe_single(mqtt_client, topic, 0);
    switch (msg_id) {
    case -1:
        return ESP_FAIL;
    case -2:
        ESP_LOGW(TAG, "Outbox for '%s' is full", topic);
        return ESP_FAIL;
    }

    topicHandlers[topic] = handler;
    ESP_LOGD(TAG, "Registered handler for topic '%s'", topic);

    return ESP_OK;
}

esp_err_t Mqtt_Unsub(const char *topic) {
    int msg_id = esp_mqtt_client_unsubscribe(mqtt_client, topic);
    if (msg_id == -1) {
        return ESP_FAIL;
    }

    if (topicHandlers.erase(topic) > 0) {
        ESP_LOGD(TAG, "Deregistered handler for topic '%s'", topic);
    }

    return ESP_OK;
}
