
#include <stdlib.h>
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"

#include "spi.h"
#include "paw3395.h"

static const char *TAG = "paw3395";

static uint16_t dpi; // actually its cpi

static inline void delay_ms(uint8_t nms)
{
    vTaskDelay(pdMS_TO_TICKS(nms));
}

static inline void delay_us(uint32_t nus)
{
    esp_rom_delay_us(nus);
}

static inline void delay_120ns()
{
    // Minimum delay set to 1 microsecond.
    // The target of 120 nanoseconds cannot be reliably achieved due to hardware constraints (Xtensa architecture & CPU clock cycle limits).
    delay_us(1);
}

static inline void delay_500ns()
{
    // Same to delay_120ns()
    delay_us(1);
}

static inline void cs_high(void)
{
    delay_120ns();
    gpio_set_level(CONFIG_PAW3395D_NCS_NUM, 1);
}

static inline void cs_low(void)
{
    gpio_set_level(CONFIG_PAW3395D_NCS_NUM, 0);
    delay_120ns();
}

static inline void paw3395_write(uint8_t reg_addr, uint8_t reg_data)
{
    cs_low();

    spi_write_data(reg_addr, reg_data);

    cs_high();

    delay_us(5);
}

static inline uint8_t paw3395_read(uint8_t reg_addr)
{
    uint8_t data = 0;
    cs_low();

    spi_send_read(reg_addr);

    delay_us(2);

    // Read data
    data = spi_read_data();

    cs_high();

    delay_us(2);

    return data;
}

static void load_powerup_reg_setting()
{
    paw3395_write(0x7F, 0x07);
    paw3395_write(0x40, 0x41);
    paw3395_write(0x7F, 0x00);
    paw3395_write(0x40, 0x80);
    paw3395_write(0x7F, 0x0E);
    paw3395_write(0x55, 0x0D);
    paw3395_write(0x56, 0x1B);
    paw3395_write(0x57, 0xE8);
    paw3395_write(0x58, 0xD5);
    paw3395_write(0x7F, 0x14);
    paw3395_write(0x42, 0xBC);
    paw3395_write(0x43, 0x74);
    paw3395_write(0x4B, 0x20);
    paw3395_write(0x4D, 0x00);
    paw3395_write(0x53, 0x0E);
    paw3395_write(0x7F, 0x05);
    paw3395_write(0x44, 0x04);
    paw3395_write(0x4D, 0x06);
    paw3395_write(0x51, 0x40);
    paw3395_write(0x53, 0x40);
    paw3395_write(0x55, 0xCA);
    paw3395_write(0x5A, 0xE8);
    paw3395_write(0x5B, 0xEA);
    paw3395_write(0x61, 0x31);
    paw3395_write(0x62, 0x64);
    paw3395_write(0x6D, 0xB8);
    paw3395_write(0x6E, 0x0F);

    paw3395_write(0x70, 0x02);
    paw3395_write(0x4A, 0x2A);
    paw3395_write(0x60, 0x26);
    paw3395_write(0x7F, 0x06);
    paw3395_write(0x6D, 0x70);
    paw3395_write(0x6E, 0x60);
    paw3395_write(0x6F, 0x04);
    paw3395_write(0x53, 0x02);
    paw3395_write(0x55, 0x11);
    paw3395_write(0x7A, 0x01);
    paw3395_write(0x7D, 0x51);
    paw3395_write(0x7F, 0x07);
    paw3395_write(0x41, 0x10);
    paw3395_write(0x42, 0x32);
    paw3395_write(0x43, 0x00);
    paw3395_write(0x7F, 0x08);
    paw3395_write(0x71, 0x4F);
    paw3395_write(0x7F, 0x09);
    paw3395_write(0x62, 0x1F);
    paw3395_write(0x63, 0x1F);
    paw3395_write(0x65, 0x03);
    paw3395_write(0x66, 0x03);
    paw3395_write(0x67, 0x1F);
    paw3395_write(0x68, 0x1F);
    paw3395_write(0x69, 0x03);
    paw3395_write(0x6A, 0x03);
    paw3395_write(0x6C, 0x1F);

    paw3395_write(0x6D, 0x1F);
    paw3395_write(0x51, 0x04);
    paw3395_write(0x53, 0x20);
    paw3395_write(0x54, 0x20);
    paw3395_write(0x71, 0x0C);
    paw3395_write(0x72, 0x07);
    paw3395_write(0x73, 0x07);
    paw3395_write(0x7F, 0x0A);
    paw3395_write(0x4A, 0x14);
    paw3395_write(0x4C, 0x14);
    paw3395_write(0x55, 0x19);
    paw3395_write(0x7F, 0x14);
    paw3395_write(0x4B, 0x30);
    paw3395_write(0x4C, 0x03);
    paw3395_write(0x61, 0x0B);
    paw3395_write(0x62, 0x0A);
    paw3395_write(0x63, 0x02);
    paw3395_write(0x7F, 0x15);
    paw3395_write(0x4C, 0x02);
    paw3395_write(0x56, 0x02);
    paw3395_write(0x41, 0x91);
    paw3395_write(0x4D, 0x0A);
    paw3395_write(0x7F, 0x0C);
    paw3395_write(0x4A, 0x10);
    paw3395_write(0x4B, 0x0C);
    paw3395_write(0x4C, 0x40);
    paw3395_write(0x41, 0x25);
    paw3395_write(0x55, 0x18);
    paw3395_write(0x56, 0x14);
    paw3395_write(0x49, 0x0A);
    paw3395_write(0x42, 0x00);
    paw3395_write(0x43, 0x2D);
    paw3395_write(0x44, 0x0C);
    paw3395_write(0x54, 0x1A);
    paw3395_write(0x5A, 0x0D);
    paw3395_write(0x5F, 0x1E);
    paw3395_write(0x5B, 0x05);
    paw3395_write(0x5E, 0x0F);
    paw3395_write(0x7F, 0x0D);
    paw3395_write(0x48, 0xDD);
    paw3395_write(0x4F, 0x03);
    paw3395_write(0x52, 0x49);

    paw3395_write(0x51, 0x00);
    paw3395_write(0x54, 0x5B);
    paw3395_write(0x53, 0x00);

    paw3395_write(0x56, 0x64);
    paw3395_write(0x55, 0x00);
    paw3395_write(0x58, 0xA5);
    paw3395_write(0x57, 0x02);
    paw3395_write(0x5A, 0x29);
    paw3395_write(0x5B, 0x47);
    paw3395_write(0x5C, 0x81);
    paw3395_write(0x5D, 0x40);
    paw3395_write(0x71, 0xDC);
    paw3395_write(0x70, 0x07);
    paw3395_write(0x73, 0x00);
    paw3395_write(0x72, 0x08);
    paw3395_write(0x75, 0xDC);
    paw3395_write(0x74, 0x07);
    paw3395_write(0x77, 0x00);
    paw3395_write(0x76, 0x08);
    paw3395_write(0x7F, 0x10);
    paw3395_write(0x4C, 0xD0);
    paw3395_write(0x7F, 0x00);
    paw3395_write(0x4F, 0x63);
    paw3395_write(0x4E, 0x00);
    paw3395_write(0x52, 0x63);
    paw3395_write(0x51, 0x00);
    paw3395_write(0x54, 0x54);
    paw3395_write(0x5A, 0x10);
    paw3395_write(0x77, 0x4F);
    paw3395_write(0x47, 0x01);
    paw3395_write(0x5B, 0x40);
    paw3395_write(0x64, 0x60);
    paw3395_write(0x65, 0x06);
    paw3395_write(0x66, 0x13);
    paw3395_write(0x67, 0x0F);
    paw3395_write(0x78, 0x01);
    paw3395_write(0x79, 0x9C);
    paw3395_write(0x40, 0x00);
    paw3395_write(0x55, 0x02);
    paw3395_write(0x23, 0x70);
    paw3395_write(0x22, 0x01);

    delay_ms(1);

    // Read register 0x6C at 1ms interval until value
    // 0x80 is obtained or read up to 60 times, this
    // register read interval must be carried out at 1ms
    // interval with timing tolerance of +-1%
    uint8_t i;
    for (i = 0; i < 60; i++)
    {
        if (paw3395_read(0x6C) == 0x80)
        {
            break;
        }
        delay_ms(1);
    }

    if (i == 60)
    {
        paw3395_write(0x7F, 0x14);
        paw3395_write(0x6C, 0x00);
        paw3395_write(0x7F, 0x00);
    }

    paw3395_write(0x22, 0x00);
    paw3395_write(0x55, 0x00);
    paw3395_write(0x7F, 0x07);
    paw3395_write(0x40, 0x40);
    paw3395_write(0x7F, 0x00);
}

static uint8_t motion_burst_buffer[12] = {0};

static void read_motion()
{
    cs_low();

    spi_send_read(MOTION_BURST_ADR);

    delay_us(2);

    for (uint8_t i = 0; i < 12; i++)
    {
        motion_burst_buffer[i] = spi_read_data();
    }

    cs_high();
    delay_500ns();
}

void resume_dpi(void);

void wake_paw3395()
{
    ESP_LOGI(TAG, "Wake paw3395 begin.");

    delay_ms(50); // wait 50 ms

    // reset SPI
    cs_high();
    cs_low();

    // write 0x5A to POWER_UP_RESET
    paw3395_write(0x3A, 0x5A);
    delay_ms(5);

    // load Power-up initialization register setting.
    load_powerup_reg_setting();

    // read registers 0x02, 0x03, 0x04, 0x05 and 0x06 one tiime regardless of the motion bit state.
    paw3395_read(0x02);
    paw3395_read(0x03);
    paw3395_read(0x04);
    paw3395_read(0x05);
    paw3395_read(0x06);

    ESP_LOGI(TAG, "PRODUCT_ID:0x%02x", paw3395_read(0x00));

    // our paw3395 install reverse.so we need reverse axis_x
    paw3395_write(0x5B, 0x20);

    ESP_LOGI(TAG, "Wake paw3395 end.");

    resume_dpi();
}

void read_move(int16_t *x, int16_t *y)
{
    read_motion();

    *x += (int16_t)(motion_burst_buffer[2] + (motion_burst_buffer[3] << 8));
    *y += (int16_t)(motion_burst_buffer[4] + (motion_burst_buffer[5] << 8));
}

void set_dpi(uint16_t new_dpi)
{
    if (new_dpi < CPI_MIN)
    {
        new_dpi = CPI_MIN;
    }

    if (new_dpi > CPI_MAX)
    {
        new_dpi = CPI_MIN;
    }

    if (new_dpi == dpi)
    {
        return;
    }

    uint16_t reg_value = (new_dpi / CPI_MIN) - 1;

    uint8_t high_byte = (reg_value >> 8) & 0x0F;
    uint8_t low_byte = reg_value & 0xFF;

    // both x y resolution set by RESOLUTION_X_L,RESOLUTION_X_h
    paw3395_write(MOTION_CTRL, 0x00);

    paw3395_write(RESOLUTION_X_LOW, low_byte);
    paw3395_write(RESOLUTION_X_HIGH, high_byte);

    // refresh
    paw3395_write(SET_RESOLUTION, 0x01);

    delay_500ns();

    dpi = new_dpi;

    ESP_LOGI(TAG, "current DPI(CPI):%d", dpi);

    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "NVS open failed: %s", esp_err_to_name(ret));
    }

    ret = nvs_set_u16(nvs_handle, "dpi", dpi);
    nvs_close(nvs_handle);
}

/**
 * @brief resume dpi to last storage value
 */
void resume_dpi(void)
{
    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "NVS open failed: %s", esp_err_to_name(ret));
        return;
    }

    uint16_t dpi;
    ret = nvs_get_u16(nvs_handle, "dpi", &dpi);
    nvs_close(nvs_handle);

    if (ret == ESP_OK)
    {
        set_dpi(dpi);
    }
    else
    {
        set_dpi(CONFIG_DEFAULT_DPI);
    }
}