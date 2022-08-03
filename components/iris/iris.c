#include "iris.h"

#include <string.h>
#include "logging.h"
#include "errors.h"

#include "driver/spi_common.h"
#include "driver/spi_master.h"

static bool spi_initialized = false;
static spi_device_handle_t iris_device_handle;

#define ESP32_SPI_HOST SPI2_HOST

#define MOSI_PIN    10
#define MISO_PIN    4
#define CLK_PIN     1
#define CS_PIN      0

void initialize_spi_bus() {
    if (spi_initialized) {
        LOGI("SPI Bus Already Initialized");
        return;
    }
    // start spi master
    // https://docs.espressif.com/projects/esp-idf/en/latest/esp32c3/api-reference/peripherals/spi_master.html#_CPPv416spi_bus_config_t
    spi_bus_config_t spi_bus_config;
    memset(&spi_bus_config, 0, sizeof(spi_bus_config_t));
    spi_bus_config.mosi_io_num = MOSI_PIN;
    spi_bus_config.miso_io_num = MISO_PIN;
    spi_bus_config.sclk_io_num = CLK_PIN;
    spi_bus_config.quadwp_io_num = -1;  // -1 not used
    spi_bus_config.quadhd_io_num = -1;  // -1 not used

    esp_err_t spi_bus_initialize_err = spi_bus_initialize(ESP32_SPI_HOST, &spi_bus_config, SPI_DMA_DISABLED);
    if (spi_bus_initialize_err != ESP_OK) {
        LOGI("Failed to Initialize SPI Bus");
    } else {
        spi_initialized = true;
        LOGI("SPI Bus Initialized");
    }
}

void add_device_to_spi_bus() {

    // https://docs.espressif.com/projects/esp-idf/en/latest/esp32c3/api-reference/peripherals/spi_master.html#structspi__device__interface__config__t
    spi_device_interface_config_t spi_device_interface_config;
    spi_device_interface_config.command_bits = 0;
    spi_device_interface_config.address_bits = 0;
    spi_device_interface_config.dummy_bits = 0;
    spi_device_interface_config.mode = 0;
    spi_device_interface_config.duty_cycle_pos = 128;
    spi_device_interface_config.cs_ena_pretrans = 0; // 0-16
    spi_device_interface_config.cs_ena_posttrans = 0; // 0-16
    spi_device_interface_config.clock_speed_hz = 9600;
    spi_device_interface_config.input_delay_ns = 0;
    spi_device_interface_config.spics_io_num = CS_PIN; // nSS (Chip Select)
    spi_device_interface_config.flags = 0;
    spi_device_interface_config.queue_size = 1;
    spi_device_interface_config.pre_cb = NULL;
    spi_device_interface_config.post_cb = NULL;

    esp_err_t spi_bus_add_device_err = spi_bus_add_device(ESP32_SPI_HOST, &spi_device_interface_config, &iris_device_handle);
    if (spi_bus_add_device_err != ESP_OK) {
        LOGI("Failed to add device to SPI Bus");
    } else {
        LOGI("Add Device to SPI Bus");
    }
}

void send_data_to_iris(char * payload, int payload_len) {
    // initiate transaction
    // https://docs.espressif.com/projects/esp-idf/en/latest/esp32c3/api-reference/peripherals/spi_master.html#_CPPv417spi_transaction_t
    spi_transaction_t spi_transaction;
    spi_transaction.flags = 0;
    spi_transaction.cmd = 0;
    spi_transaction.addr = 0;
    spi_transaction.length = payload_len * 8;
    spi_transaction.rxlength = 0;
    spi_transaction.user = NULL;
    spi_transaction.tx_buffer = payload;
    // spi_transaction.tx_data // can only be used if SPI_TRANS_USE_TXDATA is set
    spi_transaction.rx_buffer = NULL; // we will not worry about recieving data at this point
    // spi_transaction.rx_data // can only be used if SPI_TRANS_USE_RXDATA is set
    esp_err_t spi_device_transmit_err = spi_device_transmit(iris_device_handle, &spi_transaction);
    if (spi_device_transmit_err != ESP_OK) {
        LOGI("Failed to transmit data on SPI Bus");
    } else {
        LOGI("Transmitted data on SPI Bus");
    }
}
