#include <string.h>

#include "esp_log.h"
#include <driver/uart.h>
#include <esp_console.h>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include "commands_atecc.h"
#include "commands_system.h"
#include "serial.h"

#define CMD_BUFFER_SIZE 1024

static const char *TAG = "cli/task";

static void cli_loop() {
    int i, cmd_ret;
    static char previousChar = '\0';
    uint8_t linebuf[CMD_BUFFER_SIZE];

    SerialPrintf("CLI started\n");
    while (true) {
        bzero(linebuf, sizeof(linebuf));
        i = 0;

        while (true) {
            char ch = '\0';
            int readBytes = SerialRead((uint8_t *)&ch, 1);
            if (readBytes == 1) {
                if ((ch == '\n') && (previousChar == '\r')) {
                    previousChar = '\0';
                    SerialPrintf("\r\n");
                    break;
                } else if (ch == '\b' || ch == 127) {
                    if (i > 0) {
                        linebuf[i] = '\0';
                        SerialPrintf("\b \b");
                        i--;
                    }
                } else {
                    linebuf[i] = ch;
                    SerialWrite((uint8_t *)&ch, 1);
                    i++;
                    previousChar = ch;
                }
            } else if (readBytes < 0) {
            }
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        /* Remove the truncating \r\n */
        linebuf[strlen((char *)linebuf) - 1] = '\0';

        if (esp_console_run((char *)linebuf, &cmd_ret) == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Command not found: %s", linebuf);
        }
        if (cmd_ret != ESP_OK) {
            ESP_LOGE(TAG, "Command returned error 0x%04x", cmd_ret);
        }
        SerialPrintf("COMMAND END\n");
    }
}

void task_cli(void *arg) {
    esp_console_config_t console_config;
    console_config.max_cmdline_args = 8;
    console_config.max_cmdline_length = CMD_BUFFER_SIZE;

    esp_console_init(&console_config);
    esp_console_register_help_command();

    esp_err_t status = ESP_OK;
    status |= register_commands_atecc();
    status |= register_commands_system();
    if (status == ESP_OK) {
        cli_loop();
    } else {
        ESP_LOGE(TAG, "Failed to register all commands");
    }

    ESP_LOGI(TAG, "Stopping the CLI");
    vTaskDelete(NULL);
}