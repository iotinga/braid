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
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "esp_console.h"
#include "esp_log.h"
#include <driver/uart.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include "atcacert/atcacert_client.h"
#include "atcacert/atcacert_pem.h"
#include "cryptoauthlib.h"

#include "certs/atecc_utils.h"
#include "cli/serial.h"
#include "commands_atecc.h"
#include "hal/flash.h"

#define CRYPT_BUF_LEN         2400
#define CRYPT_BUF_PUB_KEY_LEN ATCA_PUB_KEY_SIZE

#ifdef CONFIG_ECU_DEBUGGING
#define ECU_DEBUG_LOG ESP_LOGI
#else
#define ECU_DEBUG_LOG(...)
#endif /* MFG_DEBUG */

static const char *TAG = "cli/commands_atecc";

static unsigned char crypt_buf_public_key[CRYPT_BUF_PUB_KEY_LEN];
static unsigned char crypt_buf_csr[CRYPT_BUF_LEN];
static unsigned char crypt_buf_cert[CRYPT_BUF_LEN];
static device_status_t atca_cli_status_object;

static esp_err_t register_init_device();
static esp_err_t register_print_chip_info();
static esp_err_t register_generate_key_pair();
static esp_err_t register_generate_csr();
static esp_err_t register_generate_pub_key();
static esp_err_t register_get_tngtls_root_cert();
static esp_err_t register_get_tngtls_signer_cert();
static esp_err_t register_get_tngtls_device_cert();
static esp_err_t register_provide_cert_def();
static esp_err_t register_program_device_cert();
static esp_err_t register_print_device_cert();
static esp_err_t register_program_signer_cert();

esp_err_t register_commands_atecc() {
    esp_err_t ret = ESP_OK;
    ret |= register_init_device();
    ret |= register_print_chip_info();
    ret |= register_generate_key_pair();
    ret |= register_generate_csr();
    ret |= register_generate_pub_key();
    ret |= register_provide_cert_def();
    ret |= register_program_device_cert();
    ret |= register_print_device_cert();
    ret |= register_program_signer_cert();
    ret |= register_get_tngtls_root_cert();
    ret |= register_get_tngtls_signer_cert();
    ret |= register_get_tngtls_device_cert();
    return ret;
}

static esp_err_t init_device(int argc, char **argv) {
    esp_err_t ret = ESP_ERR_INVALID_ARG;
    int err_code;
    atca_cli_status_object = BEGIN;
    char device_type[20] = {};

    uint8_t i2c_sda_pin = CONFIG_ATCA_I2C_SDA_PIN;
    uint8_t i2c_scl_pin = CONFIG_ATCA_I2C_SCL_PIN;
    uint8_t do_lock = false;

    switch (argc) {
    case 2:
        do_lock = atoi(argv[1]);
        // fall through
    case 1:
        ESP_LOGI(TAG, "I2C pins selected are SDA = %d, SCL = %d", i2c_sda_pin, i2c_scl_pin);
        ret = init_atecc608a(device_type, i2c_sda_pin, i2c_scl_pin, do_lock, &err_code);
        break;
    case 3:
        ESP_LOGI(TAG, "I2C pins selected are SDA = %d, SCL = %d", i2c_sda_pin, i2c_scl_pin);
        ret = init_atecc608a(device_type, i2c_sda_pin, i2c_scl_pin, true, &err_code);
        break;
    default:
        break;
    }

    SerialPrintf("Status: %s\n", ret ? "Failure" : "Success");
    atca_cli_status_object = ret ? ATECC_INIT_FAIL : ATECC_INIT_SUCCESS;

    if (ret == ESP_ERR_INVALID_ARG) {
        ESP_LOGE(TAG, "Reason: Invalid Usage");
    } else if (ret != ESP_OK) {
        ESP_LOGE(TAG,
                 "Initilization of ATECC failed, returned %02x\nPlease check that the appropriate I2C pin numbers are "
                 "provided to the python script",
                 err_code);
    } else {
        SerialPrintf("ATECC608 Device Type: %s\n", device_type);
    }

    return ESP_OK;
}

static esp_err_t register_init_device() {
    const esp_console_cmd_t cmd = {
        .command = "init",
        .help = "Initialize the ATECC chip connection\n"
                "  (Optional) If <do_lock> is 1, locks config and data zones if not locked already.\n"
                "  Usage:   init <do_lock?>\n"
                "  Example: init",
        .hint = NULL,
        .func = &init_device,
    };
    return esp_console_cmd_register(&cmd);
}

static esp_err_t print_chip_info(int argc, char **argv) {
    esp_err_t ret = ESP_ERR_INVALID_ARG;
    int err_code;
    uint8_t sn[9] = {};
    if (atca_cli_status_object >= ATECC_INIT_SUCCESS) {
        ret = atecc_print_info(sn, &err_code);
        SerialPrintf("Status: %s\n", ret ? "Failure" : "Success");
    }
    if (atca_cli_status_object < ATECC_INIT_SUCCESS) {
        ESP_LOGE(TAG, "Please Initialize device before calling this function");
    } else if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error in generating keys, returned %02x", err_code);
    } else {
        SerialPrintf("Serial Number:");
        for (int count = 0; count < 9; count++) {
            SerialPrintf("%02x ", sn[count]);
        }
        SerialPrintf("\r\n");
    }

    return ESP_OK;
}

static esp_err_t register_print_chip_info() {
    const esp_console_cmd_t cmd = {
        .command = "print-chip-info",
        .help = "Print the Serial Number of the atecc608a chip\n"
                "  Serial number is a 9 byte number, unique to every chip\n"
                "  Usage:   print-chip-info\n"
                "  Example: print-chip-info",
        .func = &print_chip_info,
    };
    return esp_console_cmd_register(&cmd);
}

static esp_err_t generate_key_pair(int argc, char **argv) {
    esp_err_t ret = ESP_ERR_INVALID_ARG;
    int err_code;
    if (atca_cli_status_object >= ATECC_INIT_SUCCESS) {
        if (argc == 2 && (atoi(argv[1]) < 3)) {
            ret = atecc_keygen(atoi(argv[1]), crypt_buf_public_key, CRYPT_BUF_PUB_KEY_LEN, &err_code);
        }
        SerialPrintf("Status: %s\n", ret ? "Failure" : "Success");
        atca_cli_status_object = ret ? KEY_PAIR_GEN_FAIL : KEY_PAIR_GEN_SUCCESS;
    }

    if (atca_cli_status_object < ATECC_INIT_SUCCESS) {
        ESP_LOGE(TAG, "Please Initialize device before calling this function");
    } else if (ret == ESP_ERR_INVALID_ARG) {
        ESP_LOGE(TAG, "Reason: Invalid Usage");
    } else if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error in generating keys, returned %02x", err_code);
    } else {
        SerialPrintf("\nPublic Key:\n");
        for (int count = 0; count < CRYPT_BUF_PUB_KEY_LEN; count++) {
            SerialPrintf("%02x", crypt_buf_public_key[count]);
        }
        SerialPrintf("\n");
    }

    return ESP_OK;
}

static esp_err_t register_generate_key_pair() {
    const esp_console_cmd_t cmd = {
        .command = "generate-keys",
        .help = "Generates an internal ECC private key inside the given slot of ATECC608, returns its public key \n"
                "  By default only slots 0,1,2 are supported"
                "  Usage:   generate-keys <slot number>\n"
                "  Example: generate-keys 0",
        .func = &generate_key_pair,
    };
    return esp_console_cmd_register(&cmd);
}

static esp_err_t generate_csr(int argc, char **argv) {
    esp_err_t ret = ESP_ERR_INVALID_ARG;
    int err_code;
    if (atca_cli_status_object >= ATECC_INIT_SUCCESS) {
        if (argc == 1) {
            ret = atecc_csr_gen(crypt_buf_csr, CRYPT_BUF_LEN, &err_code);
        }
        SerialPrintf("Status: %s\n", ret ? "Failure" : "Success");
        atca_cli_status_object = ret ? CSR_GEN_FAIL : CSR_GEN_SUCCESS;
    }

    if (atca_cli_status_object < ATECC_INIT_SUCCESS) {
        ESP_LOGE(TAG, "Please Initialize device before calling this function");
    } else if (ret == ESP_ERR_INVALID_ARG) {
        ESP_LOGE(TAG, "Reason: Invalid Usage");
    } else if (ret != ESP_OK) {
        ESP_LOGE(TAG,
                 "Generating CSR failed, returned %02x, \nNote: please check that you have called \n generate_keys "
                 "command on slot mentioned in cert_def_3_device_csr.c in component/cryptoauthlib/port/ \nat least "
                 "once before you debug error code",
                 err_code);
    } else {
        SerialPrintf("%s\n", crypt_buf_csr);
    }

    return ESP_OK;
}

static esp_err_t register_generate_csr() {
    const esp_console_cmd_t cmd = {
        .command = "generate-csr",
        .help =
            "Generates a CSR from private key in specified slot.\n"
            "  Private key must be genereated at least once on specified slot for this command to succeed. Information "
            "such as CN,O as well as the priv key slot etc. is picked from cert_def generated by python script.\n"
            "  Usage:   generate-csr\n"
            "  Example: generate-csr",
        .func = &generate_csr,
    };
    return esp_console_cmd_register(&cmd);
}

static esp_err_t generate_pub_key(int argc, char **argv) {
    esp_err_t ret = ESP_ERR_INVALID_ARG;
    int err_code;
    if (atca_cli_status_object >= ATECC_INIT_SUCCESS) {
        if (argc == 2) {
            ret = atecc_gen_pubkey(atoi(argv[1]), crypt_buf_public_key, CRYPT_BUF_PUB_KEY_LEN, &err_code);
        }
        SerialPrintf("Status: %s\n", ret ? "Failure" : "Success");
        atca_cli_status_object = ret ? PUBKEY_GEN_FAIL : PUBKEY_GEN_SUCCESS;
    }

    if (atca_cli_status_object < ATECC_INIT_SUCCESS) {
        ESP_LOGE(TAG, "Please Initialize device before calling this function");
    } else if (ret == ESP_ERR_INVALID_ARG) {
        ESP_LOGE(TAG, "Reason: Invalid Usage");
    } else if (ret != ESP_OK) {
        ESP_LOGE(TAG,
                 "Generating Public key failed, returned %02x, \nNote: please check that you have "
                 "called\ngenerate_keys command on slot %d at least once before you debug error code",
                 err_code,
                 atoi(argv[1]));
    } else {
        SerialPrintf("\nPublic Key:\n");
        for (int count = 0; count < CRYPT_BUF_PUB_KEY_LEN; count++) {
            SerialPrintf("%02x ", crypt_buf_public_key[count]);
        }
        SerialPrintf("\n");
    }

    return ESP_OK;
}

static esp_err_t register_generate_pub_key() {
    const esp_console_cmd_t cmd = {
        .command = "generate-pubkey",
        .help = "Generates a public key from the present key-pair\n"
                "  generate-keys must be executed at least once on specified slot to succeed\n"
                "  Usage:   generate-pubkey <slot number>\n"
                "  Example: generate-pubkey 0",
        .func = &generate_pub_key,
    };
    return esp_console_cmd_register(&cmd);
}

static esp_err_t provide_cert_def(int argc, char **argv) {
    int ret = -1;
    if (atca_cli_status_object >= ATECC_INIT_SUCCESS) {
        if (argc == 3) {
            size_t buf_len = atoi(argv[2]);
            if (atoi(argv[1]) == 0) {
                ret = receive_cert_def(crypt_buf_cert, buf_len, CERT_TYPE_DEVICE);
            } else if (atoi(argv[1]) == 1) {
                ret = receive_cert_def(crypt_buf_cert, buf_len, CERT_TYPE_SIGNER);
            }
        }
        SerialPrintf("Status: %s\n", ret ? "Failure" : "Success");
        atca_cli_status_object = ret ? GET_CERT_DEF_SUCCESS : GET_CERT_DEF_FAIL;
    }

    if (atca_cli_status_object < ATECC_INIT_SUCCESS) {
        ESP_LOGE(TAG, "Please Initialize device before calling this function");
    } else if (ret == ESP_ERR_INVALID_ARG) {
        ESP_LOGE(TAG, "Reason: Invalid Usage");
    } else if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failure code %d", atca_cli_status_object);
    }

    return ESP_OK;
}

static esp_err_t register_provide_cert_def() {
    const esp_console_cmd_t cmd = {
        .command = "provide-cert-def",
        .help = "Provides the cert definition of device cert of signer cert to the ATECC chip\n"
                "  <device> must be 0 for device cert or 1 for signer cert\n"
                "  Usage:   provide-cert-def <device> <data_length>\n"
                "  Example: provide-cert-def 0 1024",
        .func = &provide_cert_def,
    };
    return esp_console_cmd_register(&cmd);
}

static esp_err_t program_device_cert(int argc, char **argv) {
    esp_err_t ret = ESP_ERR_INVALID_ARG;
    int err_code;
    if (atca_cli_status_object >= CSR_GEN_SUCCESS) {
        if (argc == 3) {
            size_t buf_len = atoi(argv[2]);
            bool lock = false;
            if (atoi(argv[1]) == 1) {
                lock = true;
            }

            ret = atecc_input_cert(crypt_buf_cert, buf_len, CERT_TYPE_DEVICE, lock, &err_code);
            if (!ret) {
                esp_err_t err = Flash_Save(PARTITION_FACTORY, "device_cert", crypt_buf_cert, buf_len);
                atca_cli_status_object = err != ESP_OK ? PROGRAM_CERT_FAIL : PROGRAM_CERT_SUCCESS;
            }
        }

        SerialPrintf("Status: %s\n", ret ? "Failure" : "Success");
        atca_cli_status_object = ret ? PROGRAM_CERT_FAIL : PROGRAM_CERT_SUCCESS;
    }
    if (atca_cli_status_object < CSR_GEN_SUCCESS) {
        ESP_LOGE(TAG, "Generate the CSR before calling this function.");
    } else if (ret == ESP_ERR_INVALID_ARG) {
        ESP_LOGE(TAG, "Reason: Invalid Usage");
    } else if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Programming device cert failed, returned %d", err_code);
    }

    return ESP_OK;
}

static esp_err_t register_program_device_cert() {
    const esp_console_cmd_t cmd = {
        .command = "program-dev-cert",
        .help = "Sets the UART command handler to input the device certificate.\n"
                "  Usage: program-dev-cert <lock> <data_length>",
        .func = &program_device_cert,
    };
    return esp_console_cmd_register(&cmd);
}

static esp_err_t print_device_cert(int argc, char **argv) {
    int ret = -1;
    if (atca_cli_status_object >= ATECC_INIT_SUCCESS) {
        char dev_cert_buf[2048] = {0};
        ret = atecc_read_cert(CERT_TYPE_DEVICE, dev_cert_buf, sizeof(dev_cert_buf) * sizeof(char));
        SerialPrintf("%s\n", dev_cert_buf);

        SerialPrintf("Status: %s\n", ret != ESP_OK ? "Failure" : "Success");
        atca_cli_status_object = ret == ESP_OK ? GET_CERT_DEF_SUCCESS : GET_CERT_DEF_FAIL;
    }

    if (atca_cli_status_object < ATECC_INIT_SUCCESS) {
        ESP_LOGE(TAG, "Please Initialize device before calling this function");
    } else if (ret == ESP_ERR_INVALID_ARG) {
        ESP_LOGE(TAG, "Reason: Invalid Usage");
    } else if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failure code %d", atca_cli_status_object);
    }

    return ESP_OK;
}

static esp_err_t register_print_device_cert() {
    const esp_console_cmd_t cmd = {
        .command = "print-device-cert",
        .help = "Prints the device cert\n"
                "  Example: print-device-cert",
        .func = &print_device_cert,
    };
    return esp_console_cmd_register(&cmd);
}

static esp_err_t program_signer_cert(int argc, char **argv) {
    esp_err_t ret = ESP_ERR_INVALID_ARG;
    int err_code;
    if (atca_cli_status_object >= CSR_GEN_SUCCESS) {
        if (argc == 3) {
            size_t buf_len = atoi(argv[2]);
            bool lock = false;
            if (argc == 2) {
                if (atoi(argv[1]) == 1) {
                    lock = true;
                }
            }
            ret = atecc_input_cert(crypt_buf_cert, buf_len, CERT_TYPE_SIGNER, lock, &err_code);
        }

        SerialPrintf("Status: %s\n", ret ? "Failure" : "Success");
        atca_cli_status_object = ret ? PROGRAM_CERT_FAIL : PROGRAM_CERT_SUCCESS;
    }
    if (atca_cli_status_object < CSR_GEN_SUCCESS) {
        ESP_LOGE(TAG, "Generate the CSR before calling this function.");
    } else if (ret == ESP_ERR_INVALID_ARG) {
        ESP_LOGE(TAG, "Reason: Invalid Usage");
    } else if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Programming signer cert failed, returned %d", err_code);
    }

    return ESP_OK;
}

static esp_err_t register_program_signer_cert() {
    const esp_console_cmd_t cmd = {
        .command = "program-signer-cert",
        .help = "Sets the UART command handler to input the signer certificate.\n"
                "  Usage: program-signer-cert <lock> <data_length>",
        .func = &program_signer_cert,
    };
    return esp_console_cmd_register(&cmd);
}

static esp_err_t get_tngtls_root_cert(int argc, char **argv) {
    esp_err_t ret = ESP_ERR_INVALID_ARG;
    int err_code;
    size_t cert_size = CRYPT_BUF_LEN;
    if (atca_cli_status_object >= ATECC_INIT_SUCCESS) {
        if (argc == 1) {
            ret = atecc_get_tngtls_root_cert(crypt_buf_cert, &cert_size, &err_code);
        }
        SerialPrintf("Status: %s\n", ret ? "Failure" : "Success");
        atca_cli_status_object = ret ? TNGTLS_ROOT_CERT_FAIL : TNGTLS_ROOT_CERT_SUCCESS;
    }
    if (atca_cli_status_object < ATECC_INIT_SUCCESS) {
        ESP_LOGE(TAG, "Please Initialize device before calling this function");
    } else if (ret == ESP_ERR_INVALID_ARG) {
        ESP_LOGE(TAG, "Reason: Invalid Usage");
    } else if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to obtain tngtls_root cert, returned %02x", err_code);
    } else {
        SerialPrintf("\n Root Cert Len:%d\n", cert_size);
        SerialPrintf("\nCertificate:\n");
        for (int count = 0; count < cert_size; count++) {
            SerialPrintf("%02x", crypt_buf_cert[count]);
        }
        SerialPrintf("\n");
    }

    return ESP_OK;
}

static esp_err_t register_get_tngtls_root_cert() {
    const esp_console_cmd_t cmd = {
        .command = "get-tngtls-root-cert",
        .help = "get tngtls root cert, which already stored on the device\n"
                "  The ATECC608 device type should be TNG (Trust & Go)\n"
                "  Usage:   get-tngtls-root-cert\n"
                "  Example: get-tngtls-root-cert",
        .func = &get_tngtls_root_cert,
    };
    return esp_console_cmd_register(&cmd);
}

static esp_err_t get_tngtls_signer_cert(int argc, char **argv) {
    esp_err_t ret = ESP_ERR_INVALID_ARG;
    int err_code;
    size_t cert_size = CRYPT_BUF_LEN;
    if (atca_cli_status_object >= ATECC_INIT_SUCCESS) {
        if (argc == 1) {
            ret = atecc_get_tngtls_signer_cert(crypt_buf_cert, &cert_size, &err_code);
        }
        SerialPrintf("Status: %s\n", ret ? "Failure" : "Success");
        atca_cli_status_object = ret ? TNGTLS_SIGNER_CERT_FAIL : TNGTLS_SIGNER_CERT_SUCCESS;
    }
    if (atca_cli_status_object < ATECC_INIT_SUCCESS) {
        ESP_LOGE(TAG, "Please Initialize device before calling this function");
    } else if (ret == ESP_ERR_INVALID_ARG) {
        ESP_LOGE(TAG, "Reason: Invalid Usage");
    } else if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to obtain tngtls_signer cert, returned %02x", err_code);
    } else {
        SerialPrintf("\n Signer Cert Len:%d\n", cert_size);
        SerialPrintf("\nCertificate:\n");
        for (int count = 0; count < cert_size; count++) {
            SerialPrintf("%02x", crypt_buf_cert[count]);
        }
        SerialPrintf("\n");
    }

    return ESP_OK;
}

static esp_err_t register_get_tngtls_signer_cert() {
    const esp_console_cmd_t cmd = {
        .command = "get-tngtls-signer-cert",
        .help = "get tngtls signer cert, which already stored on the device\n"
                "  The ATECC608 device type should be TNG (Trust & Go)\n"
                "  Usage:   get-tngtls-signer-cert\n"
                "  Example: get-tngtls-signer-cert",
        .func = &get_tngtls_signer_cert,
    };
    return esp_console_cmd_register(&cmd);
}

static esp_err_t get_tngtls_device_cert(int argc, char **argv) {
    esp_err_t ret = ESP_ERR_INVALID_ARG;
    int err_code;
    size_t cert_size = CRYPT_BUF_LEN;
    if (atca_cli_status_object >= TNGTLS_SIGNER_CERT_SUCCESS) {
        if (argc == 1) {
            ret = atecc_get_tngtls_device_cert(crypt_buf_cert, &cert_size, &err_code);
        }
        SerialPrintf("Status: %s\n", ret ? "Failure" : "Success");
        atca_cli_status_object = ret ? TNGTLS_DEVICE_CERT_FAIL : TNGTLS_DEVICE_CERT_SUCCESS;
    }
    if (atca_cli_status_object < ATECC_INIT_SUCCESS) {
        ESP_LOGE(TAG, "Please Initialize device before calling this function");
    } else if (atca_cli_status_object < TNGTLS_SIGNER_CERT_SUCCESS) {
        ESP_LOGE(TAG, "Please execute get-tngtls-signer-cert command as this command requires signer cert");
    } else if (ret == ESP_ERR_INVALID_ARG) {
        ESP_LOGE(TAG, "Reason: Invalid Usage");
    } else if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to obtain tngtls_siger cert, returned %02x", err_code);
    } else {
        SerialPrintf("\n Device Cert Len:%d\n", cert_size);
        SerialPrintf("\nCertificate:\n");
        for (int count = 0; count < cert_size; count++) {
            SerialPrintf("%02x", crypt_buf_cert[count]);
        }
        SerialPrintf("\n");
    }

    return ESP_OK;
}

static esp_err_t register_get_tngtls_device_cert() {
    const esp_console_cmd_t cmd = {
        .command = "get-tngtls-device-cert",
        .help = "get tngtls device cert, which already stored on the device\n"
                "  The ATECC608 device type should be TNG (Trust & Go)\n"
                "  Usage:  get-tngtls-device-cert\n"
                "  Example get-tngtls-device-cert",
        .func = &get_tngtls_device_cert,
    };
    return esp_console_cmd_register(&cmd);
}
