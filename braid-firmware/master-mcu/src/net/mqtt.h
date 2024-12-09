#pragma once

#include <esp_check.h>
#include <stdint.h>

#include "core/factory_data.h"

/** @brief The absolute root of all topics */
#define MQTT_TOPIC_ROOT "braid"

#define MQTT_TIMEOUT_SECONDS (5)

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Represents the different types of topics */
typedef enum Mqtt_TopicType {
    MQTT_TOPIC_TYPE_AGENT,
    MQTT_TOPIC_TYPE_ENTITY,
} Mqtt_TopicType;

/** @brief Represents publish-only topics */
typedef enum Mqtt_TopicPub {
    MQTT_TOPIC_REPORTED,
    MQTT_TOPIC_REPORTED_BATCH,
    MQTT_TOPIC_REPORTED_LIVE,
    MQTT_TOPIC_COMMAND_PUB,
} Mqtt_TopicPub;

/** @brief Represents subscription-only topics */
typedef enum Mqtt_TopicSub {
    MQTT_TOPIC_DESIRED,
    MQTT_TOPIC_DESIRED_BATCH,
    MQTT_TOPIC_COMMAND_SUB,
} Mqtt_TopicSub;

/**
 * @brief Initializes the MQTT subsystem and connects to the given broker
 *
 * @param[in] factoryData Factory data from the flash
 * @return `ESP_FAIL` if the modem returned an error, `ESP_OK` otherwise
 */
esp_err_t Mqtt_Init(FactoryData *factoryData);

/**
 * @brief Connects to the MQTT broker
 *
 * @return `ESP_FAIL` if client is in invalid state, `ESP_OK` otherwise
 */
esp_err_t Mqtt_Connect();

/**
 * @brief Disconnects from the MQTT broker
 *
 * @return `ESP_FAIL` if client is in invalid state, `ESP_OK` otherwise
 */
esp_err_t Mqtt_Disconnect();

/**
 * @brief Composes a topic string for a publish topic
 *
 * @param[in] deviceId The device ID in use
 * @param type Type of the topic
 * @param topic The topic specification type (see the enum)
 * @param[out] out The output string
 * @param outSize Output buffer size for bounds check
 * @return The number of characters written
 */
int Mqtt_ComposeTopicPub(const char *deviceId, Mqtt_TopicType type, Mqtt_TopicPub topic, char *out, size_t outSize);

/**
 * @brief Composes a topic string for a subscription topic
 *
 * @param[in] deviceId The device ID in use
 * @param type Type of the topic
 * @param topic The topic specification type (see the enum)
 * @param role The role to respond to (see specification for more info on roles)
 * @param[out] out The output string
 * @param outSize Output buffer size for bounds check
 * @return The number of characters written
 */
int Mqtt_ComposeTopicSub(
    const char *deviceId, Mqtt_TopicType type, Mqtt_TopicSub topic, const char *role, char *out, size_t outSize);

/**
 * @brief Publishes a message on a topic
 *
 * @param topic The topic to publish on
 * @param[in] buf The data to publish
 * @param bufSize Size of the data to publish
 * @return `ESP_FAIL` if the modem returned an error, `ESP_OK` otherwise
 */
esp_err_t Mqtt_Pub(const char *topic, const void *buf, size_t bufSize);

/**
 * @brief Subscribes to a topic and registers the given handler
 *
 * @param topic The topic to subscribe to
 * @param handler Function to call upon receiving a message on the topic
 * @return `ESP_FAIL` if the modem returned an error, `ESP_OK` otherwise
 */
esp_err_t Mqtt_Sub(const char *topic, void (*handler)(const char *, const uint8_t *, size_t));

/**
 * @brief Unsubscribes from a topic and deregisters the associated handler
 *
 * @param topic The topic to unsubscribe from
 * @return `ESP_FAIL` if the modem returned an error, `ESP_OK` otherwise
 */
esp_err_t Mqtt_Unsub(const char *topic);

/**
 * @brief Listens to all subscription messages for the given number of seconds (blocking)
 *
 * @param seconds Seconds to block for
 * @return `ESP_FAIL` if the data returned is too much (the modem will truncate it)
 */
esp_err_t Mqtt_HandleMessages(uint32_t seconds);

#ifdef __cplusplus
}
#endif