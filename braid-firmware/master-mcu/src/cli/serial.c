#include "serial.h"
#include "defines.h"

#include <driver/gpio.h>
#include <driver/uart.h>
#include <driver/usb_serial_jtag.h>
#include <esp_adc/adc_oneshot.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_vfs_common.h>
#include <esp_vfs_dev.h>
#include <esp_vfs_usb_serial_jtag.h>

static const char *TAG = "cli/serial";

esp_err_t SerialUSBInit() {
    ESP_LOGI(TAG, "initializing serial");

    /* flush and sync buffers to avoid losing console messages when installing UART driver */
    (void)fflush(stdout);
    (void)fflush(stderr);

    (void)fsync(fileno(stdout));
    (void)fsync(fileno(stdout));

    usb_serial_jtag_driver_config_t config = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
    config.rx_buffer_size = 4096;
    config.tx_buffer_size = 512;

    esp_vfs_dev_usb_serial_jtag_set_rx_line_endings(ESP_LINE_ENDINGS_LF);
    esp_err_t result = usb_serial_jtag_driver_install(&config);
    if (result != ESP_OK) {
        ESP_LOGW(TAG, "serial driver install failed (%d)", result);
        return ESP_FAIL;
    }
    esp_vfs_usb_serial_jtag_use_driver();

    return ESP_OK;
}

esp_err_t SerialUARTInit(int uartNum, int baudRate) {
    ESP_LOGI(TAG, "initializing serial");

    /* flush and sync buffers to avoid losing console messages when installing UART driver */
    (void)fflush(stdout);
    (void)fflush(stderr);

    (void)fsync(fileno(stdout));
    (void)fsync(fileno(stdout));

    uart_sclk_t clk_source = UART_SCLK_REF_TICK;
    // REF_TICK clock can't provide a high baudrate
    if (baudRate > 1 * 1000 * 1000) {
        clk_source = UART_SCLK_DEFAULT;
        ESP_LOGW(TAG, "light sleep UART wakeup might not work at the configured baud rate");
    }
    const uart_config_t uart_config = {
        .baud_rate = baudRate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = clk_source,
    };
    ESP_RET_CHECK(uart_param_config(uartNum, &uart_config));
    ESP_RET_CHECK(uart_set_pin(uartNum, GPIO_NUM_1, GPIO_NUM_3, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    esp_vfs_dev_uart_port_set_rx_line_endings(uartNum, ESP_LINE_ENDINGS_LF);
    esp_err_t result = uart_driver_install(uartNum, 4096, 512, 10, NULL, 0);
    if (result != ESP_OK) {
        ESP_LOGW(TAG, "serial driver install failed (%d)", result);
        return ESP_FAIL;
    }

    esp_vfs_dev_uart_use_driver(uartNum);
    ESP_LOGI(TAG, "serial init ok");
    return ESP_OK;
}

void SerialPrintf(const char *fmt, ...) {
    static char buffer[2048];

    va_list args;
    va_start(args, fmt);

    int length = vsnprintf(buffer, sizeof(buffer), fmt, args);
    if (length > 0) {
        SerialWrite((const uint8_t *)buffer, length);
    }

    va_end(args);
}

int SerialRead(void *buffer, size_t size) {
    uint32_t totalBytes = 0;

    while (totalBytes < size) {
        int readBytes = read(STDIN_FILENO, buffer + totalBytes, size - totalBytes);
        if (readBytes < 0) {
            ESP_LOGE(TAG, "failed to read bytes");
            return readBytes;
        }
        totalBytes += readBytes;
    }

    return totalBytes;
}

int SerialWrite(const void *buffer, size_t size) {
    uint32_t totalBytes = 0;

    // write using the VFS layer, since it has locking inside to prevent console messages interleaving with command
    // output.
    fflush(stdout);
    fflush(stderr);
    fsync(STDOUT_FILENO);
    fsync(STDERR_FILENO);

    // Setup the VFS layer do not do any line ending translation
    esp_vfs_dev_uart_port_set_tx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_LF);

    // Move the caret to the beginning of the next line on '\n'
    while (totalBytes < size) {
        int writtenBytes = write(STDIN_FILENO, buffer + totalBytes, size - totalBytes);
        if (writtenBytes < 0) {
            ESP_LOGE(TAG, "failed to write bytes");
            return writtenBytes;
        }
        totalBytes += writtenBytes;
    }

    // Restore previous VFS setting
    esp_vfs_dev_uart_port_set_tx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CRLF);

    return totalBytes;
}