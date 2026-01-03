#include <stdlib.h>

#include "esp_log.h"
#include "class/hid/hid_device.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "tinyusb.h"
#include "driver/gpio.h"
#include "esp_pm.h"

#include "ble.h"
#include "tud.h"
#include "spi.h"
#include "paw3395.h"

static const char *TAG = "main";

static inline uint8_t get_encoder_state(void)
{
    return (gpio_get_level(CONFIG_ENCODER_A_NUM) << 1) | gpio_get_level(CONFIG_ENCODER_B_NUM);
}

static inline int8_t clamp_int8(int16_t v)
{
    return v > 127 ? 127 : v < -128 ? -128
                                    : v;
}

typedef struct
{
    uint8_t bit;
    gpio_num_t gpio;
    uint64_t last_isr_tick;
} button_info_t;

typedef struct
{
    int16_t x;
    int16_t y;
    int8_t vertical;
} accum_item_t;

// micros event data
static button_info_t btn_left = {
    .bit = 0,
    .gpio = CONFIG_MICRO_PIN_L,
};

static button_info_t btn_mid = {
    .bit = 2,
    .gpio = CONFIG_MICRO_PIN_M,
};

static button_info_t btn_right = {
    .bit = 1,
    .gpio = CONFIG_MICRO_PIN_R,
};

static button_info_t btn_back = {
    .bit = 3,
    .gpio = CONFIG_MICRO_PIN_B,
};

static button_info_t btn_forward = {
    .bit = 4,
    .gpio = CONFIG_MICRO_PIN_F,
};

// buffer data
static int16_t accum_x;
static int16_t accum_y;
static int8_t accum_vertical;
static SemaphoreHandle_t accum_mutex = NULL;

static uint8_t motion_level;
static TaskHandle_t move_task_handle = NULL;

static uint8_t encoder_state;
static uint64_t last_slide_tick;

static uint8_t buttons_temp;
static uint8_t buttons;
static TaskHandle_t report_task_handle = NULL;

static QueueHandle_t accum_queue;

static void on_move(void *args)
{
    motion_level = gpio_get_level(CONFIG_PAW3395D_MOTION_NUM);

    vTaskNotifyGiveFromISR(move_task_handle, pdFALSE);
}

/**
 * @brief on micro switches click
 */
static void on_click(void *args)
{
    uint64_t now = esp_timer_get_time();

    button_info_t *btn = (button_info_t *)args;

    int level = gpio_get_level(btn->gpio);

    if (!level)
    {
        buttons_temp |= (1 << btn->bit);
    }
    else
    {
        buttons_temp &= ~(1 << btn->bit);
    }

    if (buttons_temp == buttons)
    {
        return;
    }

    if ((now - btn->last_isr_tick) >= CONFIG_MICRO_DEBOUNCE)
    {
        btn->last_isr_tick = now;
        buttons = buttons_temp;

        accum_item_t item = {};
        xQueueSendFromISR(accum_queue, &item, NULL);
    }
}

/**
 * @brief on slider(wheel) slide
 */
static void on_scroll(void *args)
{
    uint64_t now = esp_timer_get_time();

    uint8_t encoder_state_temp = get_encoder_state();
    if (encoder_state_temp == encoder_state)
    {
        return;
    }

    //
    int8_t state_transition = (encoder_state << 2) | encoder_state_temp;
    int8_t vertical;
    switch (state_transition)
    {
    case 0b0001: // 00 -> 01: CCW
    case 0b0111: // 01 -> 11: CCW
    case 0b1110: // 11 -> 10: CCW
    case 0b1000: // 10 -> 00: CCW
        vertical = -1;
        break;
    case 0b0010: // 00 -> 10: CW
    case 0b1011: // 10 -> 11: CW
    case 0b1101: // 11 -> 01: CW
    case 0b0100: // 01 -> 00: CW
        vertical = 1;
        break;
    default:
        return;
    }

    if (now - last_slide_tick >= CONFIG_ENCODER_DEBOUNCE)
    {
        last_slide_tick = now;
        accum_item_t item = {
            .vertical = vertical,
        };

        xQueueSendFromISR(accum_queue, &item, NULL);
    }

    encoder_state = encoder_state_temp;
}

/**
 * @brief our all action trigger by interrupt. this function register all isr callback
 */
static void reg_isr_handler()
{
    // PAW3395D MOTION
    gpio_config_t motion_conf = {
        .pin_bit_mask = BIT64(CONFIG_PAW3395D_MOTION_NUM),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_ANYEDGE,
    };
    gpio_config(&motion_conf);
    gpio_isr_handler_add(CONFIG_PAW3395D_MOTION_NUM, on_move, NULL);

    // micros
    gpio_config_t switch_conf = {
        .pin_bit_mask = BIT64(CONFIG_MICRO_PIN_L) | BIT64(CONFIG_MICRO_PIN_R) | BIT64(CONFIG_MICRO_PIN_M) | BIT64(CONFIG_MICRO_PIN_F) | BIT64(CONFIG_MICRO_PIN_B),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE, // micros without external pull up
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE, // up down
    };
    gpio_config(&switch_conf);
    gpio_isr_handler_add(CONFIG_MICRO_PIN_L, on_click, &btn_left);
    gpio_isr_handler_add(CONFIG_MICRO_PIN_R, on_click, &btn_right);
    gpio_isr_handler_add(CONFIG_MICRO_PIN_M, on_click, &btn_mid);
    gpio_isr_handler_add(CONFIG_MICRO_PIN_F, on_click, &btn_forward);
    gpio_isr_handler_add(CONFIG_MICRO_PIN_B, on_click, &btn_back);

    // wheel
    gpio_config_t slide_conf = {
        .pin_bit_mask = BIT64(CONFIG_ENCODER_B_NUM) | BIT64(CONFIG_ENCODER_A_NUM),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLDOWN_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE, // up down
    };
    gpio_config(&slide_conf);
    gpio_isr_handler_add(CONFIG_ENCODER_B_NUM, on_scroll, NULL);
    gpio_isr_handler_add(CONFIG_ENCODER_A_NUM, on_scroll, NULL);

    //
    encoder_state = get_encoder_state();
}

void report(uint8_t accum_buttons_temp, int16_t accum_x_temp, int16_t accum_y_temp, int8_t accum_vertical_temp)
{
    do
    {
        // hid can only report int8_t once.
        int8_t x_send = clamp_int8(accum_x_temp);
        int8_t y_send = clamp_int8(accum_y_temp);

        if (tud_mounted())
        {
            tud_hid_mouse_report(HID_ITF_PROTOCOL_MOUSE, accum_buttons_temp, x_send, y_send, accum_vertical_temp, 0);
            vTaskDelay(pdMS_TO_TICKS(CONFIG_STOP_INTERVAL));
        }
        else if (ble_mounted())
        {
            ble_hid_mouse_report(accum_buttons_temp, x_send, y_send, accum_vertical_temp);
            vTaskDelay(pdMS_TO_TICKS(CONFIG_STOP_INTERVAL_BLE));
        }
        accum_x_temp -= x_send;
        accum_y_temp -= y_send;
        accum_vertical_temp = 0;
    } while (accum_x_temp != 0 || accum_y_temp != 0 || accum_vertical_temp != 0);
}

static void report_loop_task()
{
    uint8_t accum_buttons_temp = 0;
    int16_t accum_x_temp = 0;
    int16_t accum_y_temp = 0;
    int8_t accum_vertical_temp = 0;

    while (1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (xSemaphoreTake(accum_mutex, portMAX_DELAY) == pdTRUE)
        {
            // copy accum data
            accum_x_temp = accum_x;
            accum_y_temp = accum_y;
            accum_vertical_temp = accum_vertical;
            accum_buttons_temp = buttons; //

            accum_x = 0;
            accum_y = 0;
            accum_vertical = 0;

            xSemaphoreGive(accum_mutex);

            report(accum_buttons_temp, accum_x_temp, accum_y_temp, accum_vertical_temp);
        }
    }

    vTaskDelete(NULL);
}

static void accum_loop_task(void *args)
{
    accum_item_t item;
    while (1)
    {
        if (xQueueReceive(accum_queue, &item, portMAX_DELAY) == pdPASS)
        {
            if (xSemaphoreTake(accum_mutex, portMAX_DELAY) == pdTRUE)
            {
                accum_x += item.x;
                accum_y += item.y;
                accum_vertical += item.vertical;

                xSemaphoreGive(accum_mutex);

                xTaskNotifyGive(report_task_handle);
            }
        }
    }

    vTaskDelete(NULL);
}

static void move_loop_task(void *args)
{
    int16_t x;
    int16_t y;
    while (1)
    {
        // pass on paw3395 motion interrupt
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        while (motion_level == 0)
        {
            // loop read
            read_move(&x, &y);
            if (x != 0 || y != 0)
            {
                accum_item_t item = {
                    .x = x,
                    .y = y,
                };
                xQueueSend(accum_queue, &item, 0);

                x = y = 0;
            }

            vTaskDelay(pdMS_TO_TICKS(CONFIG_PAW3395_READ_INTERVAL));
        }

        // read again for ensure all data send
        read_move(&x, &y);
        if (x != 0 || y != 0)
        {
            accum_item_t item = {
                .x = x,
                .y = y,
            };
            xQueueSend(accum_queue, &item, 0);
        }

        // clear
        x = y = 0;
    }

    vTaskDelete(NULL);
}

void api_set_dpi(uint16_t dpi)
{
    set_dpi(dpi);
}

void api_macro(int16_t x, int16_t y)
{
    accum_item_t item = {
        .x = x,
        .y = y,
    };

    xQueueSend(accum_queue, &item, 0);
}

void app_main(void)
{
    esp_err_t ret;

    // init nvs flash to save data
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ret = wake_tud();
    ESP_ERROR_CHECK(ret);

    ret = wake_ble();
    ESP_ERROR_CHECK(ret);

    ret = wake_spi();
    ESP_ERROR_CHECK(ret);

    ret = gpio_install_isr_service(0);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE)
    {
        ESP_LOGE(TAG, "GPIO ISR service install failed: %s", esp_err_to_name(ret));
    }
    ESP_LOGI(TAG, "ISR handler service installed.");

    wake_paw3395();

    reg_isr_handler();
    ESP_LOGI(TAG, "ISR handlers ready.");

    accum_queue = xQueueCreate(32, sizeof(accum_item_t));
    accum_mutex = xSemaphoreCreateMutex();

    // create loop tasks
    xTaskCreate(report_loop_task, "report_loop_task", 2048, NULL, 1, &report_task_handle);
    xTaskCreate(accum_loop_task, "accum_loop_task", 2048, NULL, 2, NULL);
    xTaskCreate(move_loop_task, "move_loop_task", 2048, NULL, 1, &move_task_handle);
}