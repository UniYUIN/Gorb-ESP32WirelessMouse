#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "spi.h"

static const char *TAG = "spi";

static spi_device_handle_t spi_handle;

esp_err_t wake_spi()
{
    esp_err_t ret;

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = CONFIG_PAW3395D_MOSI_NUM,
        .miso_io_num = CONFIG_PAW3395D_MISO_NUM,
        .sclk_io_num = CONFIG_PAW3395D_SCLK_NUM,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32,
    };

    ret = spi_bus_initialize(SPI_HOST, &bus_cfg, SPI_DMA_DISABLED);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "SPI bus initialize failed: %s", esp_err_to_name(ret));
        return ret;
    }

    spi_device_interface_config_t dev_cfg = {
        .command_bits = 0,
        .address_bits = 0,
        .dummy_bits = 0,
        .mode = 3,
        .clock_speed_hz = CONFIG_SPI_CLOCK_SPEED_HZ,
        .spics_io_num = -1, // handle manually
        .flags = 0,
        .queue_size = 1,
        .pre_cb = NULL,
        .post_cb = NULL,
    };

    ret = spi_bus_add_device(SPI_HOST, &dev_cfg, &spi_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "SPI device add failed: %s", esp_err_to_name(ret));
        return ret;
    }

    gpio_set_direction(CONFIG_PAW3395D_NCS_NUM, GPIO_MODE_OUTPUT);
    gpio_set_level(CONFIG_PAW3395D_NCS_NUM, 1);

    ESP_LOGI(TAG, "SPI wake up.");

    return ret;
}

void spi_write_data(uint8_t reg, uint8_t data)
{
    uint8_t tx_data[2] = {reg | 0x80, data};

    spi_transaction_t trans = {
        .length = 16,
        .tx_buffer = tx_data,
        .rx_buffer = NULL,
    };

    esp_err_t ret = spi_device_transmit(spi_handle, &trans);
    if (ret != ESP_OK)
    {
        ESP_LOGE("TAG", "Write command failed: 0x%02x", reg);
    }
}

void spi_send_read(uint8_t reg)
{
    uint8_t addr = reg & 0x7F;

    spi_transaction_t trans = {
        .length = 8,
        .tx_buffer = &addr,
        .rx_buffer = NULL,
    };

    esp_err_t ret = spi_device_transmit(spi_handle, &trans);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Read command failed: 0x%02x %s", reg, esp_err_to_name(ret));
    }
}

uint8_t spi_read_data()
{
    uint8_t dummy = 0x00;
    uint8_t rx_data[1] = {0};

    spi_transaction_t trans = {
        .length = 8,
        .tx_buffer = &dummy,
        .rx_buffer = rx_data,
    };

    esp_err_t ret = spi_device_transmit(spi_handle, &trans);
    if (ret != ESP_OK)
    {
        ESP_LOGE("TAG", "Read data failed: %s", esp_err_to_name(ret));
        return 0;
    }

    return rx_data[0];
}
