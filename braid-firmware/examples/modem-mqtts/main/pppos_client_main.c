/*
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
/* PPPoS Client Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_modem_api.h"
#include "esp_netif.h"
#include "esp_netif_ppp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "mqtt_client.h"
#include "sdkconfig.h"
#include <string.h>

#include "certs.h"

#if defined(CONFIG_EXAMPLE_FLOW_CONTROL_NONE)
#define EXAMPLE_FLOW_CONTROL ESP_MODEM_FLOW_CONTROL_NONE
#elif defined(CONFIG_EXAMPLE_FLOW_CONTROL_SW)
#define EXAMPLE_FLOW_CONTROL ESP_MODEM_FLOW_CONTROL_SW
#elif defined(CONFIG_EXAMPLE_FLOW_CONTROL_HW)
#define EXAMPLE_FLOW_CONTROL ESP_MODEM_FLOW_CONTROL_HW
#endif

#define MODEM_POWER_PIN 4

static const char *TAG = "pppos_example";
static EventGroupHandle_t event_group = NULL;
static const int CONNECT_BIT = BIT0;
static const int GOT_DATA_BIT = BIT2;
static const int USB_DISCONNECTED_BIT = BIT3; // Used only with USB DTE but we define it unconditionally, to avoid
                                              // too many #ifdefs in the code

#ifdef CONFIG_EXAMPLE_MODEM_DEVICE_CUSTOM
esp_err_t esp_modem_get_time(esp_modem_dce_t *dce_wrap, char *p_time);
#endif

#define esp_modem_at_log(modem, cmd, timeout)                                                                          \
    do {                                                                                                               \
        char __buf[512] = {0};                                                                                         \
        esp_err_t __err = esp_modem_at(modem, cmd, __buf, timeout);                                                    \
        ESP_ERROR_CHECK(__err);                                                                                        \
        ESP_LOGI(TAG, "%s --> %s", cmd, __buf);                                                                        \
    } while (0);

static void wakeup_modem_sync(esp_modem_dce_t *modem, gpio_num_t power_pin) {
    int attempt = 0;
    esp_err_t status = ESP_ERR_TIMEOUT;
    do {
        status = esp_modem_at(modem, "+++", NULL, 1000);
        status = esp_modem_at(modem, "AT", NULL, 700);
        ESP_LOGD(TAG, "Polling modem");
        attempt++;
    } while ((attempt < 3) && (status != ESP_OK));

    if (status == ESP_ERR_TIMEOUT) {
        /* Power on the modem */
        ESP_LOGI(TAG, "Modem is not responding, powering on");
        gpio_set_level(power_pin, 1);
        vTaskDelay(pdMS_TO_TICKS(1000));
        gpio_set_level(power_pin, 0);
        while (esp_modem_sync(modem) != ESP_OK) {
        }
    }
}

static void config_gpio(gpio_num_t power_pin) {
    gpio_config_t io_conf = {}; // zero-initialize the config structure.

    io_conf.intr_type = GPIO_INTR_DISABLE;        // disable interrupt
    io_conf.mode = GPIO_MODE_OUTPUT;              // set as output mode
    io_conf.pin_bit_mask = (1ULL << power_pin);   // bit mask of the pins that you
                                                  // want to set,e.g.GPIO18/19
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE; // disable pull-down mode
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;     // disable pull-up mode

    gpio_config(&io_conf); // configure GPIO with the given settings
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIu32, base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        msg_id = esp_mqtt_client_subscribe(client, CONFIG_EXAMPLE_MQTT_TEST_TOPIC, 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        msg_id =
            esp_mqtt_client_publish(client, CONFIG_EXAMPLE_MQTT_TEST_TOPIC, CONFIG_EXAMPLE_MQTT_TEST_DATA, 0, 0, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        xEventGroupSetBits(event_group, GOT_DATA_BIT);

        vTaskDelay(pdMS_TO_TICKS(2000));
        esp_mqtt_client_publish(client, CONFIG_EXAMPLE_MQTT_TEST_TOPIC, CONFIG_EXAMPLE_MQTT_TEST_DATA, 0, 0, 0);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;
    default:
        ESP_LOGI(TAG, "MQTT other event id: %d", event->event_id);
        break;
    }
}

static void on_ppp_changed(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    ESP_LOGI(TAG, "PPP state changed event %" PRIu32, event_id);
    if (event_id == NETIF_PPP_ERRORUSER) {
        /* User interrupted event from esp-netif */
        esp_netif_t **p_netif = event_data;
        ESP_LOGI(TAG, "User interrupted event from netif:%p", *p_netif);
    }
}

static void on_ip_event(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "IP event! %" PRIu32, event_id);
    if (event_id == IP_EVENT_PPP_GOT_IP) {
        esp_netif_dns_info_t dns_info;

        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        esp_netif_t *netif = event->esp_netif;

        ESP_LOGI(TAG, "Modem Connect to PPP Server");
        ESP_LOGI(TAG, "~~~~~~~~~~~~~~");
        ESP_LOGI(TAG, "IP          : " IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "Netmask     : " IPSTR, IP2STR(&event->ip_info.netmask));
        ESP_LOGI(TAG, "Gateway     : " IPSTR, IP2STR(&event->ip_info.gw));
        esp_netif_get_dns_info(netif, 0, &dns_info);
        ESP_LOGI(TAG, "Name Server1: " IPSTR, IP2STR(&dns_info.ip.u_addr.ip4));
        esp_netif_get_dns_info(netif, 1, &dns_info);
        ESP_LOGI(TAG, "Name Server2: " IPSTR, IP2STR(&dns_info.ip.u_addr.ip4));
        ESP_LOGI(TAG, "~~~~~~~~~~~~~~");
        xEventGroupSetBits(event_group, CONNECT_BIT);

        ESP_LOGI(TAG, "GOT ip event!!!");
    } else if (event_id == IP_EVENT_PPP_LOST_IP) {
        ESP_LOGI(TAG, "Modem Disconnect from PPP Server");
    } else if (event_id == IP_EVENT_GOT_IP6) {
        ESP_LOGI(TAG, "GOT IPv6 event!");

        ip_event_got_ip6_t *event = (ip_event_got_ip6_t *)event_data;
        ESP_LOGI(TAG, "Got IPv6 address " IPV6STR, IPV62STR(event->ip6_info.ip));
    }
}

void app_main(void) {
    config_gpio(MODEM_POWER_PIN);

    /* Init and register system/core components */
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &on_ip_event, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(NETIF_PPP_STATUS, ESP_EVENT_ANY_ID, &on_ppp_changed, NULL));

    /* Configure the PPP netif */
    esp_modem_dce_config_t dce_config = ESP_MODEM_DCE_DEFAULT_CONFIG(CONFIG_EXAMPLE_MODEM_PPP_APN);
    esp_netif_config_t netif_ppp_config = ESP_NETIF_DEFAULT_PPP();
    esp_netif_t *esp_netif = esp_netif_new(&netif_ppp_config);
    assert(esp_netif);

    event_group = xEventGroupCreate();

    /* Configure the DTE */
    esp_modem_dte_config_t dte_config = ESP_MODEM_DTE_DEFAULT_CONFIG();
    /* setup UART specific configuration based on kconfig options */
    dte_config.uart_config.tx_io_num = CONFIG_EXAMPLE_MODEM_UART_TX_PIN;
    dte_config.uart_config.rx_io_num = CONFIG_EXAMPLE_MODEM_UART_RX_PIN;
    dte_config.uart_config.rts_io_num = CONFIG_EXAMPLE_MODEM_UART_RTS_PIN;
    dte_config.uart_config.cts_io_num = CONFIG_EXAMPLE_MODEM_UART_CTS_PIN;
    dte_config.uart_config.flow_control = EXAMPLE_FLOW_CONTROL;
    dte_config.uart_config.rx_buffer_size = CONFIG_EXAMPLE_MODEM_UART_RX_BUFFER_SIZE;
    dte_config.uart_config.tx_buffer_size = CONFIG_EXAMPLE_MODEM_UART_TX_BUFFER_SIZE;
    dte_config.uart_config.event_queue_size = CONFIG_EXAMPLE_MODEM_UART_EVENT_QUEUE_SIZE;
    dte_config.task_stack_size = CONFIG_EXAMPLE_MODEM_UART_EVENT_TASK_STACK_SIZE;
    dte_config.task_priority = CONFIG_EXAMPLE_MODEM_UART_EVENT_TASK_PRIORITY;
    dte_config.dte_buffer_size = CONFIG_EXAMPLE_MODEM_UART_RX_BUFFER_SIZE / 2;

#if CONFIG_EXAMPLE_MODEM_DEVICE_BG96 == 1
    ESP_LOGI(TAG, "Initializing esp_modem for the BG96 module...");
    esp_modem_dce_t *dce = esp_modem_new_dev(ESP_MODEM_DCE_BG96, &dte_config, &dce_config, esp_netif);
#elif CONFIG_EXAMPLE_MODEM_DEVICE_SIM800 == 1
    ESP_LOGI(TAG, "Initializing esp_modem for the SIM800 module...");
    esp_modem_dce_t *dce = esp_modem_new_dev(ESP_MODEM_DCE_SIM800, &dte_config, &dce_config, esp_netif);
#elif CONFIG_EXAMPLE_MODEM_DEVICE_SIM7000 == 1
    ESP_LOGI(TAG, "Initializing esp_modem for the SIM7000 module...");
    esp_modem_dce_t *dce = esp_modem_new_dev(ESP_MODEM_DCE_SIM7000, &dte_config, &dce_config, esp_netif);
#elif CONFIG_EXAMPLE_MODEM_DEVICE_SIM7070 == 1
    ESP_LOGI(TAG, "Initializing esp_modem for the SIM7070 module...");
    esp_modem_dce_t *dce = esp_modem_new_dev(ESP_MODEM_DCE_SIM7070, &dte_config, &dce_config, esp_netif);
#elif CONFIG_EXAMPLE_MODEM_DEVICE_SIM7600 == 1
    ESP_LOGI(TAG, "Initializing esp_modem for the SIM7600 module...");
    esp_modem_dce_t *dce = esp_modem_new_dev(ESP_MODEM_DCE_SIM7600, &dte_config, &dce_config, esp_netif);
#elif CONFIG_EXAMPLE_MODEM_DEVICE_CUSTOM == 1
    ESP_LOGI(TAG, "Initializing esp_modem with custom module...");
    esp_modem_dce_t *dce = esp_modem_new_dev(ESP_MODEM_DCE_CUSTOM, &dte_config, &dce_config, esp_netif);
#else
    ESP_LOGI(TAG, "Initializing esp_modem for a generic module...");
    esp_modem_dce_t *dce = esp_modem_new(&dte_config, &dce_config, esp_netif);
#endif
    assert(dce);
    wakeup_modem_sync(dce, MODEM_POWER_PIN);

    esp_modem_at_log(modem, "AT+CGPIO=0,48,1,0", 5000);
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_modem_at_log(modem, "AT+CPIN?", 5000);
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_modem_at_log(modem, "AT+CFUN=0", 5000);

    esp_modem_at_log(modem, "AT+CNMP=13", 5000);
    esp_modem_at_log(modem, "AT+CMNB=1", 5000);
    esp_modem_at_log(modem, "AT+CFUN=1", 5000);

    if (dte_config.uart_config.flow_control == ESP_MODEM_FLOW_CONTROL_HW) {
        esp_err_t err = esp_modem_set_flow_control(dce, 2, 2); // 2/2 means HW Flow Control.
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to set the set_flow_control mode");
            return;
        }
        ESP_LOGI(TAG, "HW set_flow_control OK");
    }

    xEventGroupClearBits(event_group, CONNECT_BIT | GOT_DATA_BIT | USB_DISCONNECTED_BIT);

    /* Run the modem demo app */
#if CONFIG_EXAMPLE_NEED_SIM_PIN == 1
    // check if PIN needed
    bool pin_ok = false;
    if (esp_modem_read_pin(dce, &pin_ok) == ESP_OK && pin_ok == false) {
        if (esp_modem_set_pin(dce, CONFIG_EXAMPLE_SIM_PIN) == ESP_OK) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        } else {
            abort();
        }
    }
#endif

    esp_modem_at_log(dce, "AT+CGDCONT=1,\"IP\",\"TM\",\"0.0.0.0\",0,0,0,0", 5000);
    esp_modem_at_log(dce, "AT+CGDCONT=13,\"IP\",\"TM\",\"0.0.0.0\",0,0,0,0", 5000);

    ESP_LOGI(TAG, "Waiting for network");
    while (1) {
        char __buf[512] = {0};
        esp_err_t __err = esp_modem_at(dce, "AT+CGREG?", __buf, 1000);
        ESP_ERROR_CHECK(__err);
        //"+CGREG: 0,5"
        if (__buf[10] == '5' || __buf[10] == '1') {
            break;
        }
    }

    esp_modem_at_log(dce, "AT+CIPSHUT", 5000);
    esp_modem_at_log(dce, "AT+CGATT=0", 5000);
    esp_modem_at_log(dce, "AT+SAPBR=3,1,\"Contype\",\"GPRS\"", 5000);
    esp_modem_at_log(dce, "AT+SAPBR=3,1,\"APN\",\"TM\"", 5000);
    esp_modem_at_log(dce, "AT+CGDCONT=1,\"IP\",\"TM\"", 5000);

    esp_modem_at_log(dce, "AT+CPSI?", 5000);

    int rssi = 99, ber;
    ESP_LOGI(TAG, "Waiting for signal lock");
    while (rssi >= 99) {
        esp_err_t err = esp_modem_get_signal_quality(dce, &rssi, &ber);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_modem_get_signal_quality failed with %d %s", err, esp_err_to_name(err));
            ESP_ERROR_CHECK(err);
        }
    }
    ESP_LOGI(TAG, "Signal quality: rssi=%d, ber=%d", rssi, ber);

#ifdef CONFIG_EXAMPLE_MODEM_DEVICE_CUSTOM
    {
        char time[64];
        err = esp_modem_get_time(dce, time);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_modem_get_time failed with %d %s", err, esp_err_to_name(err));
            return;
        }
        ESP_LOGI(TAG, "esp_modem_get_time: %s", time);
    }
#endif

    esp_err_t err = esp_modem_set_mode(dce, ESP_MODEM_MODE_DATA);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_modem_set_mode(ESP_MODEM_MODE_DATA) failed with %d", err);
        return;
    }
    /* Wait for IP address */
    ESP_LOGI(TAG, "Waiting for IP address");
    xEventGroupWaitBits(event_group, CONNECT_BIT | USB_DISCONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    /* Config MQTT */
    esp_mqtt_client_config_t mqtt_config = {
        .broker =
            {
                .address.uri = "mqtts://178.175.208.233:8883",
                .verification.certificate = (const char *)ROOT_CA,
                .verification.skip_cert_common_name_check = true,
            },
        .credentials =
            {
                .authentication.use_secure_element = true,
                .authentication.certificate = (const char *)DEVICE_CERT_ATECC,
                .authentication.certificate_len = strlen(DEVICE_CERT_ATECC) + 1,
                // .authentication.key = (const char *)DEVICE_KEY,
                .username = "gabibbo",
                .client_id = "gabibbo",
            },
        .session =
            {
                .keepalive = 60,
                .protocol_ver = MQTT_PROTOCOL_V_5,
            },
    };
    esp_mqtt_client_handle_t mqtt_client = esp_mqtt_client_init(&mqtt_config);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
    ESP_LOGI(TAG, "Waiting for MQTT data");
    xEventGroupWaitBits(event_group, GOT_DATA_BIT | USB_DISCONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    ESP_LOGI(TAG, "Connected to MQTT");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    // err = esp_modem_set_mode(dce, ESP_MODEM_MODE_COMMAND);
    // if (err != ESP_OK) {
    //     ESP_LOGE(TAG,
    //              "esp_modem_set_mode(ESP_MODEM_MODE_COMMAND) failed with %d",
    //              err);
    //     return;
    // }
    // char imsi[32];
    // err = esp_modem_get_imsi(dce, imsi);
    // if (err != ESP_OK) {
    //     ESP_LOGE(TAG, "esp_modem_get_imsi failed with %d", err);
    //     return;
    // }
    // ESP_LOGI(TAG, "IMSI=%s", imsi);

    // esp_mqtt_client_disconnect(mqtt_client);
    // esp_mqtt_client_stop(mqtt_client);
    // UART DTE clean-up
    // esp_modem_destroy(dce);
    // esp_netif_destroy(esp_netif);
}
