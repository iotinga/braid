#include "esp_check.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_netif_defaults.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "lwip/dns.h"
#include "lwip/ip4_addr.h"
#include "lwip/sockets.h"
#include <stdint.h>

#include "core/time.h"
#include "defines.h"
#include "hal/gprs.h"
#include "hal/modem.h"

#define IP_TIMEOUT_INITIAL    pdMS_TO_TICKS(SEC_TO_MS(10))
#define IP_TIMEOUT_FACTOR     (2)
#define IP_TIMEOUT_FACTOR_MAX (6)

/**
 * @brief Datetime information from the network
 */
typedef struct Network_Datetime {
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;

    /** @brief Offset in quarter of an hours (i.e. if offset is 8, timezone is +2 hours) */
    int offset;
} Network_Datetime;

static const char *TAG = "hal/gprs";
const char *register_info[] = {
    "Not registered, MT is not currently searching an operator to register to.The GPRS service is disabled, the UE is "
    "allowed to attach for GPRS if requested by the user.",
    "Registered, home network.",
    "Not registered, but MT is currently trying to attach or searching an operator to register to. The GPRS service is "
    "enabled, but an allowable PLMN is currently not available. The UE will start a GPRS attach as soon as an "
    "allowable PLMN is available.",
    "Registration denied, The GPRS service is disabled, the UE is not allowed to attach for GPRS if it is requested by "
    "the user.",
    "Unknown.",
    "Registered, roaming.",
};
static EventGroupHandle_t event_group = NULL;
static const int GOT_IP_BIT = BIT0;

static void set_dns(esp_netif_t *esp_netif) {
    esp_netif_dns_info_t dns_info1;
    esp_ip4_addr_t dns_server_ip1;
    IP4_ADDR(&dns_server_ip1, 8, 8, 8, 8);
    dns_info1.ip.u_addr.ip4.addr = dns_server_ip1.addr;
    esp_netif_set_dns_info(esp_netif, ESP_NETIF_DNS_MAIN, &dns_info1);

    esp_netif_dns_info_t dns_info2;
    esp_ip4_addr_t dns_server_ip2;
    IP4_ADDR(&dns_server_ip2, 8, 8, 4, 4);
    dns_info2.ip.u_addr.ip4.addr = dns_server_ip2.addr;
    esp_netif_set_dns_info(esp_netif, ESP_NETIF_DNS_BACKUP, &dns_info2);
}

static void on_ppp_changed(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    ESP_LOGI(TAG, "PPP state changed event %" PRIu32, event_id);
    switch (event_id) {
    case NETIF_PPP_ERRORUSER:
        /* User interrupted event from esp-netif */
        esp_netif_t **p_netif = (esp_netif_t **)event_data;
        ESP_LOGI(TAG, "User interrupted event from netif:%p", *p_netif);
        break;
    default:
        break;
    }
}

static void on_ip_event(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "IP event! %" PRIu32, event_id);
    switch (event_id) {
    case IP_EVENT_PPP_GOT_IP: {
        esp_netif_dns_info_t dns_info;

        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        esp_netif_t *netif = event->esp_netif;

        set_dns(netif);

        ESP_LOGI(TAG, "Modem connected to PPP server");
        ESP_LOGD(TAG, "~~~~~~~~~~~~~~");
        ESP_LOGD(TAG, "IP          : " IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGD(TAG, "Netmask     : " IPSTR, IP2STR(&event->ip_info.netmask));
        ESP_LOGD(TAG, "Gateway     : " IPSTR, IP2STR(&event->ip_info.gw));

        char buf[32] = {0};
        esp_ip4_addr_t tdnsip;
        esp_netif_dhcpc_option(netif, ESP_NETIF_OP_GET, ESP_NETIF_DOMAIN_NAME_SERVER, &tdnsip, 4);
        sprintf(buf, IPSTR, IP2STR(&tdnsip));
        ESP_LOGD(TAG, "DNS ns      : %s", buf);

        esp_netif_get_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns_info);
        ESP_LOGD(TAG, "Name Server1: " IPSTR, IP2STR(&dns_info.ip.u_addr.ip4));
        esp_netif_get_dns_info(netif, ESP_NETIF_DNS_BACKUP, &dns_info);
        ESP_LOGD(TAG, "Name Server2: " IPSTR, IP2STR(&dns_info.ip.u_addr.ip4));
        ESP_LOGD(TAG, "~~~~~~~~~~~~~~");
        xEventGroupSetBits(event_group, GOT_IP_BIT);
        break;
    }
    case IP_EVENT_PPP_LOST_IP: {
        ESP_LOGD(TAG, "Modem Disconnect from PPP Server");
        break;
    }
    case IP_EVENT_GOT_IP6: {
        ip_event_got_ip6_t *event = (ip_event_got_ip6_t *)event_data;
        ESP_LOGD(TAG, "Got IPv6 address " IPV6STR, IPV62STR(event->ip6_info.ip));
        break;
    }
    default:
        break;
    }
}

esp_netif_t *GPRS_NewNetif() {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &on_ip_event, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(NETIF_PPP_STATUS, ESP_EVENT_ANY_ID, &on_ppp_changed, NULL));
    esp_netif_config_t netif_ppp_config = ESP_NETIF_DEFAULT_PPP();

    esp_netif_t *esp_netif = esp_netif_new(&netif_ppp_config);
    assert(esp_netif);

    set_dns(esp_netif);
    ESP_ERROR_CHECK(esp_netif_set_default_netif(esp_netif));

    ESP_LOGI(TAG, "Created netif");

    event_group = xEventGroupCreate();
    assert(event_group);

    return esp_netif;
}

esp_err_t GPRS_Connect(esp_modem_dce_t *modem, GPRS_Params *init) {
    esp_err_t err = ESP_OK;

    vTaskDelay(pdMS_TO_TICKS(500));
    xEventGroupClearBits(event_group, GOT_IP_BIT);

    esp_modem_at_log(modem, "AT+CGPIO=0,48,1,0", 5000);
    esp_modem_at_log(modem, "AT+CPIN?", 5000);
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_modem_at_log(modem, "AT+CFUN=0", 10000);
    esp_modem_set_preferred_mode(modem, init->connectionMode);
    esp_modem_set_network_mode(modem, init->networkMode);
    esp_modem_at_log(modem, "AT+CFUN=1", 10000);

    char cmdBuf[128] = {0};
    sprintf(cmdBuf, "AT+CGDCONT=1,\"IP\",\"%s\",\"0.0.0.0\",0,0,0,0", init->apn);
    esp_modem_at_log(modem, cmdBuf, 5000);
    sprintf(cmdBuf, "AT+CGDCONT=13,\"IP\",\"%s\",\"0.0.0.0\",0,0,0,0", init->apn);
    esp_modem_at_log(modem, cmdBuf, 5000);
    memset(cmdBuf, 0, sizeof(cmdBuf));

    // Wait for signal
    int rssi = 99, ber;
    ESP_LOGI(TAG, "Waiting for signal lock");
    while (rssi >= 99) {
        esp_err_t err = esp_modem_get_signal_quality(modem, &rssi, &ber);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_modem_get_signal_quality failed with %d %s", err, esp_err_to_name(err));
            ESP_RET_CHECK(err);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    ESP_LOGI(TAG, "Signal quality: rssi=%d, ber=%d", rssi, ber);

    ESP_LOGI(TAG, "Waiting for network");
    while (1) {
        char outputBuf[512] = {0};
        esp_err_t err = esp_modem_at(modem, "AT+CGREG?", outputBuf, 1000);
        ESP_ERROR_CHECK(err);
        //"+CGREG: 0,5"
        if (outputBuf[10] == '5' || outputBuf[10] == '1') {
            break;
        }
    }

    esp_modem_at_log(modem, "AT+CIPSHUT", 5000);
    esp_modem_at_log(modem, "AT+CGATT=0", 5000);

    if (init->networkMode == NETWORK_GSM || init->networkMode == NETWORK_GSM) {
        esp_modem_at_log(modem, "AT+SAPBR=3,1,\"Contype\",\"GPRS\"", 5000);
        sprintf(cmdBuf, "AT+SAPBR=3,1,\"APN\",\"%s\"", init->apn);
        esp_modem_at_log(modem, cmdBuf, 5000);
        memset(cmdBuf, 0, sizeof(cmdBuf));
    }
    esp_modem_at_log(modem, "AT+CPSI?", 5000);

    err = esp_modem_set_mode(modem, ESP_MODEM_MODE_DATA);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_modem_set_mode(ESP_MODEM_MODE_DATA) failed with err %d", err);
        return err;
    }

    /* Wait for IP address */
    ESP_LOGD(TAG, "Waiting for IP address");
    xEventGroupWaitBits(event_group, GOT_IP_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    return ESP_OK;
}

esp_err_t GPRS_Disconnect(esp_modem_dce_t *modem) {
    esp_err_t ret = esp_modem_sync(modem);
    if (ret != ESP_OK) {
        ESP_LOGI(TAG, "Switching to command mode");
        esp_modem_set_mode(modem, ESP_MODEM_MODE_COMMAND);
        ESP_LOGI(TAG, "Retry sync 3 times");
        for (int i = 0; i < 3; ++i) {
            ret = esp_modem_sync(modem);
            if (ret == ESP_OK) {
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
        if (ret != ESP_OK) {
            ESP_ERROR_CHECK(ret);
        }
    }
    ESP_LOGI(TAG, "Manual hang-up before reconnecting");
    esp_modem_at(modem, "ATH", NULL, 2000);

    esp_modem_at_log(modem, "AT+CIPSHUT", MODEM_AT_TIMEOUT * 5);
    esp_modem_sync(modem);
    esp_modem_at_log(modem, "AT+CGATT=0", MODEM_AT_TIMEOUT * 10);
    return ESP_OK;
}

void GPRS_PrintConnInfo(esp_modem_dce_t *modem) {
    char buf[256] = {0};

    int att = 0;
    esp_modem_get_network_attachment_state(modem, &att);
    ESP_LOGI(TAG, "GPRS status: %s", att ? "connected" : "not connected");

    esp_modem_at(modem, "AT+CCID", buf, MODEM_AT_TIMEOUT);
    ESP_LOGI(TAG, "CCID: %s", buf);

    esp_modem_get_imei(modem, buf);
    ESP_LOGI(TAG, "IMEI: %s", buf);

    esp_modem_get_imsi(modem, buf);
    ESP_LOGI(TAG, "IMSI: %s", buf);

    esp_modem_get_operator_name(modem, buf, NULL);
    ESP_LOGI(TAG, "Operator: %s", buf);

    int rssi = 99, ber = 99;
    esp_modem_get_signal_quality(modem, &rssi, &ber);
    ESP_LOGI(TAG, "RSSI: %d  BER: %d", rssi, ber);
}

time_t GPRS_GetDatetime(esp_modem_dce_t *modem) {
    struct tm t;
    Network_Datetime dt = {0, 0, 0, 0, 0, 0, 0};

    char buf[256] = {0};
    esp_modem_at(modem, "AT+CCLK?", buf, MODEM_AT_TIMEOUT);

    // CCLK returns a string in the format `yy/MM/dd,hh:mm:ss±zz`
    sscanf(buf, "%d/%d/%d,%d:%d:%d%d", &dt.year, &dt.month, &dt.day, &dt.hour, &dt.minute, &dt.second, &dt.offset);

    int offsetDays = dt.offset / 96;
    dt.offset %= 96;
    int offsetHours = dt.offset / 4;
    dt.offset %= 4;
    int offsetMinutes = dt.offset * 15;

    t.tm_year = dt.year + 100;
    t.tm_mon = dt.month - 1;
    t.tm_mday = dt.day + offsetDays;
    t.tm_hour = dt.hour + offsetHours;
    t.tm_min = dt.minute + offsetMinutes;
    t.tm_sec = dt.second;
    t.tm_isdst = -1;

    return mktime(&t);
}
