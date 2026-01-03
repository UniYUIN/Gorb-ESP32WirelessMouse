
#include <stdlib.h>

#include "esp_bt.h"

#include "host/ble_hs.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "esp_hidd.h"
#include "services/gap/ble_svc_gap.h"
#include "esp_log.h"

#include "esp_hid_gap.h"
#include "ble.h"

static const char *TAG = "ble";

extern void api_set_dpi(uint16_t dpi);
extern void api_macro(int16_t x, int16_t y);

static const uint8_t mouse_report_map[] = {
    // Application Collection: Mouse
    0x05,
    0x01, // Usage Page (Generic Desktop)
    0x09,
    0x02, // Usage (Mouse)
    0xA1,
    0x01, // Collection (Application)

    // Report ID 1: Mouse Input (Device -> Host)
    0x85,
    0x01, //   Report ID (1)
    0x09,
    0x01, //   Usage (Pointer)
    0xA1,
    0x00, //   Collection (Physical)

    // Button bits (5 buttons)
    0x05,
    0x09, //     Usage Page (Button)
    0x19,
    0x01, //     Usage Minimum (Button 1)
    0x29,
    0x05, //     Usage Maximum (Button 5)
    0x15,
    0x00, //     Logical Minimum (0)
    0x25,
    0x01, //     Logical Maximum (1)
    0x75,
    0x01, //     Report Size (1)
    0x95,
    0x05, //     Report Count (5)
    0x81,
    0x02, //     Input (Data,Var,Abs) - Button states

    // Padding to fill 1 byte
    0x75,
    0x03, //     Report Size (3)
    0x95,
    0x01, //     Report Count (1)
    0x81,
    0x03, //     Input (Const,Var,Abs) - Padding

    // X and Y movement (relative)
    0x05,
    0x01, //     Usage Page (Generic Desktop)
    0x09,
    0x30, //     Usage (X)
    0x09,
    0x31, //     Usage (Y)
    0x15,
    0x81, //     Logical Minimum (-127)
    0x25,
    0x7F, //     Logical Maximum (127)
    0x75,
    0x08, //     Report Size (8)
    0x95,
    0x02, //     Report Count (2)
    0x81,
    0x06, //     Input (Data,Var,Rel) - X,Y relative movement

    // Vertical wheel
    0x09,
    0x38, //     Usage (Wheel)
    0x15,
    0x81, //     Logical Minimum (-127)
    0x25,
    0x7F, //     Logical Maximum (127)
    0x75,
    0x08, //     Report Size (8)
    0x95,
    0x01, //     Report Count (1)
    0x81,
    0x06, //     Input (Data,Var,Rel) - Vertical wheel

    0xC0, //   End Collection (Physical)

    // Report ID 2: Host -> Device (Set DPI)
    0x85,
    0x02, //   Report ID (2)
    0x05,
    0x01, //   Usage Page (Generic Desktop)
    0x09,
    0x00, //   Usage (Undefined)
    0x15,
    0x00, //   Logical Minimum (0)
    0x26,
    0xFF,
    0xFF, //   Logical Maximum (65535)
    0x75,
    0x10, //   Report Size (16)
    0x95,
    0x01, //   Report Count (1)
    0x91,
    0x02, //   Output (Data,Var,Abs) - uint16_t data

    // Report ID 3: Host -> Device (Macro)
    0x85,
    0x03, //   Report ID (3)
    0x05,
    0x01, //   Usage Page (Generic Desktop)
    0x09,
    0x00, //   Usage (Undefined)
    0x16,
    0x00,
    0x80, //   Logical Minimum (-32768)
    0x26,
    0xFF,
    0x7F, //   Logical Maximum (32767)
    0x75,
    0x10, //   Report Size (16)
    0x95,
    0x02, //   Report Count (2)
    0x91,
    0x02, //   Output (Data,Var,Abs) - two int16_t data

    0xC0, // End Collection (Application)
};

static esp_hid_raw_report_map_t ble_report_maps[] = {
    {
        .data = mouse_report_map,
        .len = sizeof(mouse_report_map),
    },
};

static esp_hid_device_config_t ble_hid_config = {
    .device_name = CONFIG_TINYUSB_DESC_PRODUCT_STRING,
    .manufacturer_name = CONFIG_TINYUSB_DESC_MANUFACTURER_STRING,
    .serial_number = CONFIG_TINYUSB_DESC_SERIAL_STRING,
    .report_maps = ble_report_maps,
    .report_maps_len = 1,
};

/**
 * ble state: 0:shutdown 1:startup
 */
static uint8_t ble_hid_task_state;

static esp_hidd_dev_t *hid_dev;

void ble_hid_task_start_up(void)
{
    ble_hid_task_state = 1;
    // esp_hidd_dev_battery_set(hid_dev, 100); // we can not get real bat. there always set 100
    ESP_LOGI(TAG, "hid start up");
}

void ble_hid_task_shut_down(void)
{
    ble_hid_task_state = 0;
    ESP_LOGI(TAG, "hid shut down");
}

static void ble_hidd_event_callback(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{
    esp_hidd_event_t event = (esp_hidd_event_t)id;
    esp_hidd_event_data_t *param = (esp_hidd_event_data_t *)event_data;

    switch (event)
    {
    case ESP_HIDD_START_EVENT:
    {
        ESP_LOGI(TAG, "START");
        esp_hid_ble_gap_adv_start();
        break;
    }
    case ESP_HIDD_CONNECT_EVENT:
    {
        ESP_LOGI(TAG, "CONNECT");
        break;
    }
    case ESP_HIDD_PROTOCOL_MODE_EVENT:
    {
        ESP_LOGI(TAG, "PROTOCOL MODE[%u]: %s", param->protocol_mode.map_index, param->protocol_mode.protocol_mode ? "REPORT" : "BOOT");
        break;
    }
    case ESP_HIDD_CONTROL_EVENT:
    {
        ESP_LOGI(TAG, "CONTROL[%u]: %sSUSPEND", param->control.map_index, param->control.control ? "EXIT_" : "");
        if (param->control.control)
        {
            // exit suspend
            ble_hid_task_start_up();
        }
        else
        {
            // suspend
            ble_hid_task_shut_down();
        }
        break;
    }
    case ESP_HIDD_OUTPUT_EVENT:
    {
        ESP_LOGI(TAG, "OUTPUT[%u]: %8s ID: %2u, Len: %d, Data:", param->output.map_index, esp_hid_usage_str(param->output.usage), param->output.report_id, param->output.length);
        ESP_LOG_BUFFER_HEX(TAG, param->output.data, param->output.length);

        // TODO: ble output
        switch (param->output.report_id)
        {
        case 2: // set dpi
            if (param->output.length >= 2)
            {
                int16_t dpi = (uint16_t)(param->output.data[0] | (param->output.data[1] << 8));
                api_set_dpi(dpi);
            }
            break;

        case 3: // macro
            if (param->output.length >= 4)
            {
                int16_t x = (int16_t)(param->output.data[0] | (param->output.data[1] << 8));
                int16_t y = (int16_t)(param->output.data[2] | (param->output.data[3] << 8));
                api_macro(x, y);
            }
            break;

        default:
            ESP_LOGW(TAG, "error Output Report ID: %d", param->output.report_id);
            break;
        }

        break;
    }
    case ESP_HIDD_FEATURE_EVENT:
    {
        ESP_LOGI(TAG, "FEATURE[%u]: %8s ID: %2u, Len: %d, Data:", param->feature.map_index, esp_hid_usage_str(param->feature.usage), param->feature.report_id, param->feature.length);
        ESP_LOG_BUFFER_HEX(TAG, param->feature.data, param->feature.length);
        break;
    }
    case ESP_HIDD_DISCONNECT_EVENT:
    {
        ESP_LOGI(TAG, "DISCONNECT: %s", esp_hid_disconnect_reason_str(esp_hidd_dev_transport_get(param->disconnect.dev), param->disconnect.reason));
        ble_hid_task_shut_down();
        esp_hid_ble_gap_adv_start();
        break;
    }
    case ESP_HIDD_STOP_EVENT:
    {
        ESP_LOGI(TAG, "STOP");
        break;
    }
    default:
        break;
    }
    return;
}

bool ble_mounted(void)
{
    if (hid_dev == NULL)
    {
        return false;
    }

    return ble_hid_task_state;
}

void ble_hid_device_host_task(void *param)
{
    ESP_LOGI(TAG, "BLE Host Task Started");
    nimble_port_run();

    nimble_port_freertos_deinit();
}

void ble_store_config_init(void);

// close bat service
void ble_svc_bas_init(void)
{
}

esp_err_t ble_svc_bas_battery_level_set(uint8_t level)
{
    return ESP_OK;
}

esp_err_t wake_ble(void)
{
    esp_err_t ret;

    ret = esp_hid_gap_init(HIDD_BLE_MODE);
    ESP_ERROR_CHECK(ret);

    ret = esp_hid_ble_gap_adv_init(ESP_HID_APPEARANCE_MOUSE, ble_hid_config.device_name);
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "setting ble device");
    ESP_ERROR_CHECK(
        esp_hidd_dev_init(&ble_hid_config, ESP_HID_TRANSPORT_BLE, ble_hidd_event_callback, &hid_dev));

    ble_store_config_init();

    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;
    /* Starting nimble task after gatts is initialized*/
    ret = esp_nimble_enable(ble_hid_device_host_task);
    if (ret)
    {
        ESP_LOGE(TAG, "esp_nimble_enable failed: %d", ret);
    }

    ble_svc_gap_device_name_set(CONFIG_TINYUSB_DESC_PRODUCT_STRING);

    return ret;
}

esp_err_t sleep_ble(void)
{
    esp_err_t ret;

    if (hid_dev != NULL)
    {
        ret = esp_hidd_dev_deinit(hid_dev);
        ESP_ERROR_CHECK(ret);
    }

    ret = esp_nimble_disable();
    ESP_ERROR_CHECK(ret);

    return ret;
}

void ble_hid_mouse_report(uint8_t buttons, char x, char y, char vertical)
{
    static uint8_t buffer[4] = {0};
    buffer[0] = buttons;
    buffer[1] = x;
    buffer[2] = y;
    buffer[3] = vertical;
    esp_hidd_dev_input_set(hid_dev, 0, 1, buffer, 4);
}