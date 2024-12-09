#define PIN(x) (gpio_num_t)(x)

#define ESP_RET_CHECK(x)                                                                                               \
    do {                                                                                                               \
        esp_err_t err_rc_ = (x);                                                                                       \
        if (unlikely(err_rc_ != ESP_OK)) {                                                                             \
            return err_rc_;                                                                                            \
        }                                                                                                              \
    } while (0)

#define DEFAULT_STACK_SIZE (1024u * 4u)
#define DEFAULT_PRIORITY   (tskIDLE_PRIORITY + 1)

/** Antitamper sampling time in microseconds */
#define ANTITAMPER_TIME_US (10000u)

#define MODEM_AT_TIMEOUT (1000u)