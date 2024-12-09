#include "freertos/FreeRTOS.h"
#include <esp_console.h>
#include <esp_system.h>
#include <string.h>

#include "build_config.h"
#include "commands_system.h"
#include "core/boot.h"
#include "core/factory_data.h"
#include "hal/anti_tamper.h"
#include "hal/flash.h"
#include "serial.h"

#define FILE_BUF_LEN (2048)

static const char *TAG = "cli/commands_system";

static unsigned char file_buf[FILE_BUF_LEN];

static esp_err_t register_log();
static esp_err_t register_reboot();
static esp_err_t register_version();
static esp_err_t register_boot_mode_dur();
static esp_err_t register_factory_write();
static esp_err_t register_root_ca_write();
static esp_err_t register_device_cert_write();
static esp_err_t register_print_tamper();
static esp_err_t register_erase_tamper();

esp_err_t register_commands_system() {
    esp_err_t ret = ESP_OK;
    ret |= register_log();
    ret |= register_reboot();
    ret |= register_version();
    ret |= register_boot_mode_dur();
    ret |= register_factory_write();
    ret |= register_root_ca_write();
    ret |= register_device_cert_write();
    ret |= register_print_tamper();
    ret |= register_erase_tamper();
    return ret;
}

static esp_err_t set_log_level(int argc, char **argv) {
    esp_err_t ret = ESP_ERR_INVALID_ARG;

    if (argc == 2) {
        int level = atoi(argv[1]);

        if (level <= ESP_LOG_VERBOSE) {
            esp_log_level_set("*", (esp_log_level_t)level);
            ESP_LOGD(TAG, "Setting log level to %d", level);
            ret = ESP_OK;
        }
    }

    return ret;
}

static esp_err_t register_log() {
    const esp_console_cmd_t cmd = {
        .command = "log",
        .help = "Set the log level\n"
                "  0 = ESP_LOG_NONE\n"
                "  1 = ESP_LOG_ERROR\n"
                "  2 = ESP_LOG_WARN\n"
                "  3 = ESP_LOG_INFO\n"
                "  4 = ESP_LOG_DEBUG\n"
                "  5 = ESP_LOG_VERBOSE\n"
                "  Usage:   log <level>\n"
                "  Example: log 1",
        .hint = NULL,
        .func = &set_log_level,
    };
    return esp_console_cmd_register(&cmd);
}

static esp_err_t print_version(int argc, char **argv) {
    SerialPrintf("%s\n", VERSION_STR);

    return ESP_OK;
}

static esp_err_t register_version() {
    const esp_console_cmd_t cmd = {
        .command = "version",
        .help = "Prints version information\n"
                "  Usage: version",
        .hint = NULL,
        .func = &print_version,
    };
    return esp_console_cmd_register(&cmd);
}

static esp_err_t reboot(int argc, char **argv) {
    esp_err_t ret = ESP_ERR_INVALID_ARG;
    if (argc == 2) {
        int mode = atoi(argv[1]);

        if (mode >= 0 && mode < BOOT_MODE_MAX) {
            Boot_SetShutdownPending(true);

            while (!Boot_IsShutdownReady()) {
                vTaskDelay(pdMS_TO_TICKS(100));
            }

            Boot_To(mode);
        }
    } else {
        esp_restart();
    }
    while (1) {
    }

    return ret;
}

static esp_err_t register_reboot() {
    const esp_console_cmd_t cmd = {
        .command = "reboot",
        .help = "Restarts the system.\n"
                "  If <mode> is provided, change boot mode."
                "  Usage: reboot <mode>",
        .hint = NULL,
        .func = &reboot,
    };
    return esp_console_cmd_register(&cmd);
}

static esp_err_t boot_mode_dur(int argc, char **argv) {
    esp_err_t ret = ESP_OK;
    if (argc == 2) {
        int mode = atoi(argv[1]);

        if (mode < 0 || mode >= BOOT_MODE_MAX) {
            ESP_LOGE(TAG, "Invalid boot mode");
            return ESP_ERR_INVALID_ARG;
        }

        int seconds = Boot_GetDuration(mode);
        SerialPrintf("Mode %d: %d\n", mode, seconds);
    } else if (argc == 3) {
        int mode = atoi(argv[1]);
        int dur = atoi(argv[2]);

        if (mode < 0 || mode >= BOOT_MODE_MAX) {
            ESP_LOGE(TAG, "Invalid boot mode");
            return ESP_ERR_INVALID_ARG;
        }

        if (dur < 30) {
            ESP_LOGE(TAG, "Duration must be more than 30 seconds");
            return ESP_ERR_INVALID_ARG;
        }

        Boot_SetDuration(mode, dur);
    }

    return ret;
}

static esp_err_t register_boot_mode_dur() {
    const esp_console_cmd_t cmd = {
        .command = "boot-mode-dur",
        .help = "Reads or sets a boot mode duration (in seconds).\n"
                "  If <duration> is provided, overwrites the duration with the new one."
                "  Usage: boot-mode-dur <mode> <duration>",
        .hint = NULL,
        .func = &boot_mode_dur,
    };
    return esp_console_cmd_register(&cmd);
}

static esp_err_t factory_write(int argc, char **argv) {
    esp_err_t ret = ESP_ERR_INVALID_ARG;

    if (argc == 2) {
        size_t fileSize = atoi(argv[1]);

        memset(file_buf, 0, FILE_BUF_LEN);
        ESP_LOGD(TAG, "Reading %d bytes", fileSize);
        SerialRead(file_buf, fileSize);

        ret = FactoryData_Validate((FactoryData *)file_buf);
        if (ret == ESP_OK) {
            ret = Flash_Save(PARTITION_FACTORY, "factory", file_buf, fileSize);
        }
        SerialPrintf("Status: %s\n", (ret != ESP_OK) ? "Failure" : "Success");
    }

    return ret;
}

static esp_err_t register_factory_write() {
    const esp_console_cmd_t cmd = {
        .command = "factory-write",
        .help = "Writes a file to the factory flash\n"
                "  Usage:   factory-write <size_bytes>\n"
                "  Example: factory-write 512",
        .hint = NULL,
        .func = &factory_write,
    };
    return esp_console_cmd_register(&cmd);
}

static esp_err_t root_ca_write(int argc, char **argv) {
    esp_err_t ret = ESP_ERR_INVALID_ARG;

    if (argc == 2) {
        size_t fileSize = atoi(argv[1]);

        memset(file_buf, 0, FILE_BUF_LEN);
        ESP_LOGD(TAG, "Reading %d bytes", fileSize);
        SerialRead(file_buf, fileSize);

        ret = Flash_Save(PARTITION_FACTORY, "root_ca", file_buf, fileSize);
        SerialPrintf("Status: %s\n", (ret != ESP_OK) ? "Failure" : "Success");
    }

    return ret;
}

static esp_err_t register_root_ca_write() {
    const esp_console_cmd_t cmd = {
        .command = "root-ca-write",
        .help = "Writes the root CA to flash\n"
                "  Usage:   root-ca-write <size_bytes>\n"
                "  Example: root-ca-write 512",
        .hint = NULL,
        .func = &root_ca_write,
    };
    return esp_console_cmd_register(&cmd);
}

static esp_err_t device_cert_write(int argc, char **argv) {
    esp_err_t ret = ESP_ERR_INVALID_ARG;

    if (argc == 2) {
        size_t fileSize = atoi(argv[1]);

        memset(file_buf, 0, FILE_BUF_LEN);
        ESP_LOGD(TAG, "Reading %d bytes", fileSize);
        SerialRead(file_buf, fileSize);

        ret = Flash_Save(PARTITION_FACTORY, "device_cert", file_buf, fileSize);
        SerialPrintf("Status: %s\n", (ret != ESP_OK) ? "Failure" : "Success");
    }

    return ret;
}

static esp_err_t register_device_cert_write() {
    const esp_console_cmd_t cmd = {
        .command = "device-cert-write",
        .help = "Writes the device certificate to flash\n"
                "  Usage:   device-cert-write <size_bytes>\n"
                "  Example: device-cert-write 512",
        .hint = NULL,
        .func = &device_cert_write,
    };
    return esp_console_cmd_register(&cmd);
}

static esp_err_t print_tamper(int argc, char **argv) {
    return TamperData_Print();
}

static esp_err_t register_print_tamper() {
    const esp_console_cmd_t cmd = {
        .command = "print-tamper",
        .help = "Print stored antitamper data\n"
                "  Usage:   print-tamper\n"
                "  Example: print-tamper",
        .hint = NULL,
        .func = &print_tamper,
    };
    return esp_console_cmd_register(&cmd);
}

static esp_err_t erase_tamper(int argc, char **argv) {
    return TamperData_Erase();
}

static esp_err_t register_erase_tamper() {
    const esp_console_cmd_t cmd = {
        .command = "erase-tamper",
        .help = "Erase antitamper data\n"
                "  Usage:   erase-tamper\n"
                "  Example: erase-tamper",
        .hint = NULL,
        .func = &erase_tamper,
    };
    return esp_console_cmd_register(&cmd);
}