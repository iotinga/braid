#include <esp_intr_alloc.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <string.h>

#include "anti_tamper.h"
#include "flash.h"
#include "log_extra.h"

#define ESP_INTR_FLAG_BASE 0
#define min(a, b)                                                                                                      \
    ({                                                                                                                 \
        __typeof__(a) _a = (a);                                                                                        \
        __typeof__(b) _b = (b);                                                                                        \
        _a < _b ? _a : _b;                                                                                             \
    })

static const char *TAG = "hal/anti_tamper";
static const char *KEY = "antitamper";

static QueueHandle_t gpioQueue = NULL;
static TamperData tamperData = (TamperData){
    .events = {{0}, {0}, {0}, {0}, {0}},
    .lastEventIndex = -1,
};

esp_err_t TamperData_Load(TamperData *data);
esp_err_t TamperData_Save(TamperData *data);

static void IRAM_ATTR gpio_isr_handler(void *arg) {
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(gpioQueue, &gpio_num, NULL);
}

esp_err_t Tamper_Setup(gpio_num_t pin) {
    esp_err_t status = ESP_OK;
    gpio_config_t ioConf = {
        .pin_bit_mask = (1ULL << pin),
        .intr_type = GPIO_INTR_NEGEDGE,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
    };
    status = gpio_config(&ioConf);

    if (status == ESP_OK) {
        status = gpio_install_isr_service(ESP_INTR_FLAG_BASE);
    }

    if (status == ESP_OK || status == ESP_ERR_INVALID_STATE) {
        status = gpio_isr_handler_add(pin, gpio_isr_handler, (void *)pin);
    }

    gpioQueue = xQueueCreate(10, sizeof(uint32_t));

    if (!Flash_Exists(PARTITION_USER, KEY)) {
        ESP_LOGD(TAG, "Initial antitamper data does not exist, creating data");
        TamperData_Save(&tamperData);
    }

    return status;
}

int Tamper_CheckForTamper(gpio_num_t *pin) {
    return xQueueReceive(gpioQueue, pin, portMAX_DELAY);
}

esp_err_t TamperData_Load(TamperData *data) {
    esp_err_t status = ESP_OK;
    status = Flash_Load(PARTITION_USER, KEY, data, TAMPER_DATA_SIZE);
    if (status != ESP_OK) {
        ESP_LOGD(TAG, "Failed loading tamper data, error 0x%04x", status);
    }
    return status;
}

esp_err_t TamperData_Erase() {
    esp_err_t status = ESP_OK;
    tamperData = (TamperData){
        .events = {{0}, {0}, {0}, {0}, {0}},
        .lastEventIndex = -1,
    };

    status = TamperData_Save(&tamperData);
    if (status == ESP_OK) {
        ESP_LOGI(TAG, "Erased anti-tamper data");
    }

    return status;
}

esp_err_t TamperData_Print() {
    esp_err_t status = ESP_OK;
    uint8_t index = 0;
    TamperEvent *evt;
    status = TamperData_Load(&tamperData);

    if (status == ESP_OK) {
        ESP_LOG_LINE_BEGIN(ESP_LOG_INFO, TAG, "tamperData{ ");
        ESP_LOG_LINE(TAG, "lastIdx:%d evts:[ ", tamperData.lastEventIndex);
        for (index = 0; index < min(tamperData.lastEventIndex + 1, TAMPER_MAX_EVENTS - 1); index++) {
            evt = &tamperData.events[index];
            ESP_LOG_LINE(TAG, "{pin:%d ts:%llu} ", evt->pin, evt->timestamp);
        }
        ESP_LOG_LINE_END(TAG, "] }");
    }
    memset(&tamperData, 0, TAMPER_DATA_SIZE);

    return status;
}

esp_err_t TamperData_Save(TamperData *data) {
    esp_err_t status = ESP_OK;

    status = Flash_Save(PARTITION_USER, KEY, data, TAMPER_DATA_SIZE);
    if (status != ESP_OK) {
        ESP_LOGD(TAG, "Failed saving tamper data, error 0x%04x", status);
    }
    memset(data, 0, TAMPER_DATA_SIZE);

    return status;
}

esp_err_t Tamper_RegisterEvent(TamperEvent *event) {
    esp_err_t status = ESP_OK;
    uint8_t index = 0;
    size_t offset = 0;
    status = TamperData_Load(&tamperData);

    if (status == ESP_OK) {
        index = min(tamperData.lastEventIndex + 1, TAMPER_MAX_EVENTS - 1);
        offset = index * sizeof(TamperEvent);
        memcpy(tamperData.events + offset, event, sizeof(TamperEvent));
        tamperData.lastEventIndex = index;
        status = TamperData_Save(&tamperData);
    }

    return status;
}
