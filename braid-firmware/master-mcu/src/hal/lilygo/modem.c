#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_ppp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "lwip/dns.h"
#include "lwip/sockets.h"
#include <driver/gpio.h>

#include "board.h"
#include "defines.h"
#include "hal/modem.h"

static const char *TAG = "hal/modem";

static void Modem_ConfigGpio(gpio_num_t powerPin) {
    gpio_config_t io_conf = {}; // zero-initialize the config structure.

    io_conf.intr_type = GPIO_INTR_DISABLE;        // disable interrupt
    io_conf.mode = GPIO_MODE_OUTPUT;              // set as output mode
    io_conf.pin_bit_mask = (1ULL << powerPin);    // bit mask of the pins that you
                                                  // want to set,e.g.GPIO18/19
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE; // disable pull-down mode
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;     // disable pull-up mode

    gpio_config(&io_conf); // configure GPIO with the given settings
}

static void wakeup_modem_sync(esp_modem_dce_t *modem, gpio_num_t powerPin) {
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
        gpio_set_level(powerPin, 1);
        vTaskDelay(pdMS_TO_TICKS(1000));
        gpio_set_level(powerPin, 0);
        while (esp_modem_sync(modem) != ESP_OK) {
        }
    }
}

esp_modem_dce_t *Modem_Init(
    esp_netif_t *netif, const char *apn, gpio_num_t txPin, gpio_num_t rxPin, gpio_num_t powerPin) {
    Modem_ConfigGpio(powerPin);

    /* Configure the PPP netif */
    esp_modem_dce_config_t dce_config = ESP_MODEM_DCE_DEFAULT_CONFIG(apn);
    esp_modem_dte_config_t dte_config = ESP_MODEM_DTE_DEFAULT_CONFIG();
    /* setup UART specific configuration based on kconfig options */
    dte_config.uart_config.port_num = UART_NUM_1;
    dte_config.uart_config.tx_io_num = txPin;
    dte_config.uart_config.rx_io_num = rxPin;
    dte_config.uart_config.rts_io_num = -1;
    dte_config.uart_config.cts_io_num = -1;
    dte_config.uart_config.flow_control = ESP_MODEM_FLOW_CONTROL_NONE;
    dte_config.uart_config.rx_buffer_size = MODEM_UART_RX_BUFFER_SIZE;
    dte_config.uart_config.tx_buffer_size = MODEM_UART_TX_BUFFER_SIZE;
    dte_config.uart_config.event_queue_size = MODEM_UART_EVENT_QUEUE_SIZE;
    dte_config.task_stack_size = MODEM_UART_EVENT_TASK_STACK_SIZE;
    dte_config.task_priority = MODEM_UART_EVENT_TASK_PRIORITY;
    dte_config.dte_buffer_size = MODEM_UART_RX_BUFFER_SIZE / 2;

    esp_modem_dce_t *modem = esp_modem_new_dev(ESP_MODEM_DCE_SIM7000, &dte_config, &dce_config, netif);
    assert(modem);

    wakeup_modem_sync(modem, powerPin);

    return modem;
}

esp_err_t Modem_Poweroff(esp_modem_dce_t *modem, gpio_num_t powerPin) {
    ESP_LOGD(TAG, "Modem poweroff started");
    gpio_set_level(powerPin, 1);
    vTaskDelay(pdMS_TO_TICKS(1200));
    gpio_set_level(powerPin, 0);
    while (esp_modem_sync(modem) == ESP_OK) {
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    return ESP_OK;
}
