/*
 * Copyright 2021 Espressif Systems (Shanghai) CO LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifdef CONFIG_ECU_DEBUGGING
#define ECU_DEBUG_LOG ESP_LOGI
#else
#define ECU_DEBUG_LOG(...)
#endif /* MFG_DEBUG */

#include "stdio.h"
#include <string.h>

#include "driver/uart.h"
#include "esp_err.h"
#include "esp_flash_partitions.h"
#include "esp_log.h"
#include "esp_partition.h"
#include "mbedtls/atca_mbedtls_wrap.h"
#include "mbedtls/base64.h"
#include "spi_flash_mmap.h"

#include "atecc_utils.h"

/* Cryptoauthlib includes */
#include "atcacert/atcacert_client.h"
#include "atcacert/atcacert_pem.h"
#include "certs/cert_def_1_signer.h"
#include "certs/cert_def_2_device.h"
#include "certs/cert_def_3_device_csr.h"
#include "cryptoauthlib.h"
#include "tng_atcacert_client.h"

static const char *TAG = "cli/handlers";
static bool is_atcab_init = false;

extern QueueHandle_t uart_queue;

static atcacert_def_t g_cert_def_common;
uint8_t *g_cert_template_device;

static const uint8_t public_key_x509_header[] = {
    0x30, 0x59, 0x30, 0x13, 0x06, 0x07, 0x2A, 0x86, 0x48,
    0xCE, 0x3D, 0x02, 0x01, 0x06, 0x08, 0x2A, 0x86, 0x48,
    0xCE, 0x3D, 0x03, 0x01, 0x07, 0x03, 0x42, 0x00, 0x04};

int convert_pem_to_der(const unsigned char *input, size_t input_len,
                       unsigned char *output, size_t *output_len) {
    int ret;
    const unsigned char *s1, *s2, *end = input + input_len;
    size_t len = 0;

    s1 = (unsigned char *)strstr((const char *)input, "-----BEGIN");
    if (s1 == NULL) {
        ESP_LOGE(TAG, "input (of size %d) does not contain 'BEGIN': %s",
                 input_len, input);
        return -1;
    }

    s2 = (unsigned char *)strstr((const char *)input, "-----END");
    if (s2 == NULL) {
        ESP_LOGE(TAG, "input (of size %d) does not contain 'END': %s",
                 input_len, input);
        return -1;
    }

    s1 += 10;
    while (s1 < end && *s1 != '-') {
        s1++;
    }
    while (s1 < end && *s1 == '-') {
        s1++;
    }
    if (*s1 == '\r') {
        s1++;
    }
    if (*s1 == '\n') {
        s1++;
    }

    if (s2 <= s1) {
        ESP_LOGE(TAG, "END found before BEGIN");
        return -1;
    }

    if (s2 > end) {
        ESP_LOGE(TAG, "%s", input);
        ESP_LOGE(TAG, "%s", s1);
        ESP_LOGE(TAG, "%s", s2);
        ESP_LOGE(TAG, "END is out of bounds (%d > %d)", s2 - input,
                 end - input);
        return -1;
    }

    ret = mbedtls_base64_decode(NULL, 0, &len, (const unsigned char *)s1,
                                s2 - s1);
    if (ret == MBEDTLS_ERR_BASE64_INVALID_CHARACTER) {
        return ret;
    }

    if (len > *output_len) {
        return -1;
    }
    if ((ret = mbedtls_base64_decode(
             output, len, &len, (const unsigned char *)s1, s2 - s1)) != 0) {
        return ret;
    }

    *output_len = len;

    return 0;
}

void convert_der_to_pem(uint8_t *der, size_t der_len, char *pem,
                        size_t pem_max_len) {
    const char *header = "-----BEGIN CERTIFICATE-----\n";
    const char *footer = "\n-----END CERTIFICATE-----\n";
    size_t pem_len = pem_max_len;

    // Copy header
    strcpy(pem, header);

    // Encode DER to Base64 and add line breaks every 64 characters
    atcab_base64encode(der, der_len, pem + strlen(header), &pem_len);

    // Append footer
    strcat(pem, footer);
}

esp_err_t atecc_device_type(char *device_type) {
    int ret = 0;

    cfg_ateccx08a_i2c_default.atcai2c.address = 0xC0;
    ret = atcab_init(&cfg_ateccx08a_i2c_default);
    if (ret == ATCA_SUCCESS) {
        ESP_LOGI(TAG, "Device is of type TrustCustom");
        sprintf(device_type, "%s", "TrustCustom");
        return ESP_OK;
    }

    cfg_ateccx08a_i2c_default.atcai2c.address = 0x6A;
    ret = atcab_init(&cfg_ateccx08a_i2c_default);
    if (ret == ATCA_SUCCESS) {
        ESP_LOGI(TAG, "Device is of type TrustnGo");
        sprintf(device_type, "%s", "Trust&Go");
        return ESP_OK;
    }

    cfg_ateccx08a_i2c_default.atcai2c.address = 0x6C;
    ret = atcab_init(&cfg_ateccx08a_i2c_default);
    if (ret == ATCA_SUCCESS) {
        ESP_LOGI(TAG, "Device is of type TrustFlex");
        sprintf(device_type, "%s", "TrustFlex");
        return ESP_OK;
    }

    return ESP_FAIL;
}

esp_err_t init_atecc608a(char *device_type, uint8_t i2c_sda_pin,
                         uint8_t i2c_scl_pin, uint8_t do_lock, int *err_ret) {
    int ret = 0;
    bool is_zone_locked = false;
    ECU_DEBUG_LOG(TAG, "Initialize the ATECC interface...");
    ESP_LOGI(TAG, "I2C pins selected are SDA = %d, SCL = %d", i2c_sda_pin,
             i2c_scl_pin);

    esp_err_t esp_ret = atecc_device_type(device_type);
    if (esp_ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize atca device");
    }

    ECU_DEBUG_LOG(TAG, "\t\t OK");

    if (do_lock == 1) {
        if (ATCA_SUCCESS !=
            (ret = atcab_is_locked(LOCK_ZONE_CONFIG, &is_zone_locked))) {
            ESP_LOGE(TAG, " failed\n  ! atcab_is_locked returned 0x%02x", ret);
            goto exit;
        }

        if (!is_zone_locked) {
            ret = atcab_lock_config_zone();
            if (ret != ATCA_SUCCESS) {
                ESP_LOGE(TAG, "error in locking config zone, ret = 0x%02x",
                         ret);
                goto exit;
            }
            ECU_DEBUG_LOG(TAG, "success in locking config zone");
        } else {
            ECU_DEBUG_LOG(TAG, "config zone is Locked ..\tOK");
        }

        is_zone_locked = false;

        if (ATCA_SUCCESS !=
            (ret = atcab_is_locked(LOCK_ZONE_DATA, &is_zone_locked))) {
            ESP_LOGE(TAG, " failed\n  ! atcab_is_locked returned 0x%02x", ret);
            goto exit;
        }

        if (!is_zone_locked) {
            ret = atcab_lock_data_zone();
            if (ret != ATCA_SUCCESS) {
                ESP_LOGE(TAG, "\nerror in locking data zone, ret = 0x%02x",
                         ret);
                goto exit;
            }
            ECU_DEBUG_LOG(TAG, "success in locking data zone");
        } else {
            ECU_DEBUG_LOG(TAG, "data zone is Locked ..\tOK");
        }
    }

    is_atcab_init = true;
    *err_ret = ret;
    return ESP_OK;
exit:
    *err_ret = ret;
    return ESP_FAIL;
}

esp_err_t atecc_print_info(uint8_t *serial_no, int *err_ret) {
    uint8_t rev_info[4] = {};
    int ret = -1;
    if (ATCA_SUCCESS != (ret = atcab_info(rev_info))) {
        ESP_LOGE(TAG, "Error in reading revision information, ret is 0x%02x",
                 ret);
        goto exit;
    }
    ESP_LOG_BUFFER_HEX("Revision", rev_info, 4);
    if (rev_info[3] == 0x03) {
        ESP_LOGI(TAG, "Since the last byte of chip revision is 0x03. This is "
                      "an ATECC608B chip");
    } else if (rev_info[3] == 0x02) {
        ESP_LOGI(TAG, "Since the last byte of chip revision is 0x02. This is a "
                      "ATECC608A chip");
    }

    if (ATCA_SUCCESS != (ret = atcab_read_serial_number(serial_no))) {
        ESP_LOGE(TAG, "Error in reading serial number, ret is 0x%02x", ret);
        goto exit;
    }
    ESP_LOG_BUFFER_HEX("Serial", serial_no, 9);
    *err_ret = ret;
    return ESP_OK;
exit:
    *err_ret = ret;
    return ESP_FAIL;
}

static void print_public_key(uint8_t pubkey[ATCA_PUB_KEY_SIZE]) {
    uint8_t buf[128];
    uint8_t *tmp;
    size_t buf_len = sizeof(buf);

    /* Calculate where the raw data will fit into the buffer */
    tmp =
        buf + sizeof(buf) - ATCA_PUB_KEY_SIZE - sizeof(public_key_x509_header);

    /* Copy the header */
    memcpy(tmp, public_key_x509_header, sizeof(public_key_x509_header));

    /* Copy the key bytes */
    memcpy(tmp + sizeof(public_key_x509_header), pubkey, ATCA_PUB_KEY_SIZE);

    /* Convert to base 64 */
    (void)atcab_base64encode(tmp,
                             ATCA_PUB_KEY_SIZE + sizeof(public_key_x509_header),
                             (char *)buf, &buf_len);

    /* Add a null terminator */
    buf[buf_len] = 0;

    /* Print out the key */
    ECU_DEBUG_LOG(
        TAG,
        "\r\n-----BEGIN PUBLIC KEY-----\r\n%s\r\n-----END PUBLIC KEY-----\r\n",
        buf);
}

esp_err_t atecc_keygen(int slot, unsigned char *pub_key_buf,
                       int pub_key_buf_len, int *err_ret) {
    int ret = 0;
    bzero(pub_key_buf, pub_key_buf_len);
    if (!is_atcab_init) {
        ESP_LOGE(TAG, "gevice is not initialized");
        goto exit;
    }
    ECU_DEBUG_LOG(TAG, "generating priv key ..");

    if (ATCA_SUCCESS != (ret = atcab_genkey(slot, pub_key_buf))) {
        ESP_LOGE(TAG, "failed\n !atcab_genkey returned -0x%02x", -ret);
        goto exit;
    }
    ECU_DEBUG_LOG(TAG, "\t\t OK");
    print_public_key(pub_key_buf);
    *err_ret = ret;
    return ESP_OK;

exit:
    ESP_LOGE(TAG, "failure in generating Key");
    *err_ret = ret;
    return ESP_FAIL;
}

esp_err_t atecc_gen_pubkey(int slot, unsigned char *pub_key_buf,
                           int pub_key_buf_len, int *err_ret) {
    int ret = -1;
    if (!is_atcab_init) {
        ESP_LOGE(TAG, "\ndevice is not initialized");
        goto exit;
    }
    bzero(pub_key_buf, pub_key_buf_len);
    ECU_DEBUG_LOG(TAG, "Get the public key...");
    if (0 != (ret = atcab_get_pubkey(slot, pub_key_buf))) {
        ESP_LOGE(TAG, " failed\n  ! atcab_get_pubkey returned 0x%02x", ret);
        goto exit;
    }
    ECU_DEBUG_LOG("\t\t OK\n");
    print_public_key(pub_key_buf);
    *err_ret = ret;
    return ESP_OK;

exit:
    *err_ret = ret;
    ESP_LOGE(TAG, "\ngenerate public key failed");
    return ESP_FAIL;
}

esp_err_t atecc_csr_gen(unsigned char *csr_buf, size_t csr_buf_len,
                        int *err_ret) {
    int ret = 0;
    if (!is_atcab_init) {
        ESP_LOGE(TAG, "device is not initialized");
        goto exit;
    }
    bzero(csr_buf, csr_buf_len);
    ECU_DEBUG_LOG(TAG, "generating csr ..");
    ret = atcacert_create_csr_pem(&g_csr_def_3_device, (char *)csr_buf,
                                  &csr_buf_len);
    if (ret != ATCA_SUCCESS) {
        ESP_LOGE(TAG, "create csr pem failed, returned 0x%02x", ret);
        goto exit;
    }
    ECU_DEBUG_LOG(TAG, "\t\t OK");
    *err_ret = ret;
    return ESP_OK;

exit:
    ESP_LOGE(TAG, "Failure, Exiting , ret is 0x%02x", ret);
    *err_ret = ret;
    return ESP_FAIL;
}

esp_err_t receive_cert_def(unsigned char *cert_def_array, size_t data_len,
                           cert_type_t cert_type) {
    return ESP_OK;
}

#define ATECC608A_DEVICE_CERT_SLOT 10
#define ATECC608A_SIGNER_CERT_SLOT 12
esp_err_t atecc_input_cert(unsigned char *cert_buf, size_t cert_len,
                           cert_type_t cert_type, bool lock, int *err_ret) {
    return ESP_OK;
}

esp_err_t atecc_read_cert(cert_type_t cert_type, char *pem,
                          size_t pem_max_len) {
    uint8_t cert_der[1024];
    size_t cert_der_len = sizeof(cert_der);

    atcacert_def_t cert_def = g_cert_def_2_device;
    if (cert_type == CERT_TYPE_SIGNER) {
        cert_def = g_cert_def_1_signer;
    }

    // Read certificate in DER format from the specified slot
    int status = atcacert_read_cert(&cert_def, NULL, cert_der, &cert_der_len);
    if (status != ATCACERT_E_SUCCESS) {
        ESP_LOGE(TAG, "Error reading certificate: %d\n", status);
        return ESP_FAIL;
    }

    // Convert the DER certificate to PEM format
    convert_der_to_pem(cert_der, cert_der_len, pem, pem_max_len);

    return ESP_OK;
}

esp_err_t atecc_get_tngtls_root_cert(unsigned char *cert_buf, size_t *cert_len,
                                     int *err_ret) {
    int ret;
    ECU_DEBUG_LOG(TAG, "atecc_get_tngtls_root_cert start");
    if (ATCA_SUCCESS != (ret = tng_atcacert_root_cert_size(cert_len))) {
        ESP_LOGE(TAG,
                 "failed to get tng_atcacert_root_cert_size, returned 0x%02x",
                 ret);
        goto exit;
    }
    if (ATCA_SUCCESS != (ret = tng_atcacert_root_cert(cert_buf, cert_len))) {
        ESP_LOGE(TAG, "failed to read tng_atcacert_root_cert, returned 0x%02x",
                 ret);
        goto exit;
    }
    ECU_DEBUG_LOG(TAG, "atecc_get_tngtls_root_cert end");
    *err_ret = ret;
    return ESP_OK;

exit:
    *err_ret = ret;
    return ESP_FAIL;
}

esp_err_t atecc_get_tngtls_signer_cert(unsigned char *cert_buf,
                                       size_t *cert_len, int *err_ret) {
    int ret;
    ECU_DEBUG_LOG(TAG, "atecc_get_tngtls_signer_cert start");
    if (ATCA_SUCCESS != (ret = tng_atcacert_max_signer_cert_size(cert_len))) {
        ESP_LOGE(TAG,
                 "failed to get tng_atcacert_signer_cert_size, returned 0x%02x",
                 ret);
        goto exit;
    }
    if (ATCA_SUCCESS !=
        (ret = tng_atcacert_read_signer_cert(cert_buf, cert_len))) {
        ESP_LOGE(TAG,
                 "failed to read tng_atcacert_signer_cert, returned 0x%02x",
                 ret);
        goto exit;
    }
    ECU_DEBUG_LOG(TAG, "atecc_get_tngtls_signer_cert end");
    *err_ret = ret;
    return ESP_OK;

exit:
    *err_ret = ret;
    return ESP_FAIL;
}

esp_err_t atecc_get_tngtls_device_cert(unsigned char *cert_buf,
                                       size_t *cert_len, int *err_ret) {
    int ret;
    ECU_DEBUG_LOG(TAG, "atecc_get_tngtls_signer_cert start");
    if (ATCA_SUCCESS != (ret = tng_atcacert_max_device_cert_size(cert_len))) {
        ESP_LOGE(TAG,
                 "Failed to get tng_atcacert_device_cert_size, returned 0x%02x",
                 ret);
        goto exit;
    }
    if (ATCA_SUCCESS !=
        (ret = tng_atcacert_read_device_cert(cert_buf, cert_len, NULL))) {
        ESP_LOGE(TAG,
                 "failed to read tng_atcacert_device_cert, returned 0x%02x",
                 ret);
        goto exit;
    }
    ECU_DEBUG_LOG(TAG, "atecc_get_tngtls_signer_cert end");

    *err_ret = ret;
    return ESP_OK;

exit:
    *err_ret = ret;
    return ESP_FAIL;
}
