#include <stdlib.h>

#include "tinyusb.h"
#include "esp_log.h"

#include "tud.h"

/**
 * @brief USB string descriptor
 */
const char *hid_string_descriptor[5] = {
    // array of pointer to string descriptors
    (char[]){0x09, 0x04}, // 0: is supported language is English (0x0409)
    CONFIG_MANUFACTURER,  // 1: Manufacturer
    CONFIG_PRODUCT,       // 2: Product
    CONFIG_SERIALS,       // 3: Serials, should use chip ID
    // CONFIG_HID,           // 4: HID
};

const uint8_t mouse_report_descriptor[] = {
    TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(HID_ITF_PROTOCOL_MOUSE)),
};

const uint8_t vendor_report_descriptor[] = {
    TUD_HID_REPORT_DESC_GENERIC_INOUT(CONFIG_TUD_EPSIZE),
};

#define TUSB_DESC_TOTAL_LEN (TUD_CONFIG_DESC_LEN + CFG_TUD_HID * TUD_HID_DESC_LEN)

static const uint8_t hid_configuration_descriptor[] = {
    TUD_CONFIG_DESCRIPTOR(1, 2, 0, TUSB_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

    TUD_HID_DESCRIPTOR(0, 0, false, sizeof(mouse_report_descriptor), 0x81, CONFIG_TUD_EPSIZE, 1),
    TUD_HID_DESCRIPTOR(1, 0, false, sizeof(vendor_report_descriptor), 0x82, CONFIG_TUD_EPSIZE, 10),
};

uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    if (instance == 0)
    {
        return mouse_report_descriptor;
    }
    else if (instance == 1)
    {
        return vendor_report_descriptor;
    }

    return NULL;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)reqlen;

    return 0;
}

extern void api_set_dpi(uint16_t dpi);
extern void api_macro(int16_t x, int16_t y);

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
    if (instance == 1)
    {
        switch (buffer[0])
        {
        case 0x02: // set dpi
            uint16_t dpi = buffer[1] | (buffer[2] << 8);
            api_set_dpi(dpi);
            break;
        case 0x03: // macro
            int16_t x = buffer[1] | (buffer[2] << 8);
            int16_t y = buffer[3] | (buffer[4] << 8);
            api_macro(x, y);
            break;
        default:
            break;
        }
    }
}

esp_err_t wake_tud(void)
{
    const tinyusb_config_t tusb_conf = {
        .device_descriptor = NULL,
        .string_descriptor = hid_string_descriptor,
        .string_descriptor_count = sizeof(hid_string_descriptor) / sizeof(hid_string_descriptor[0]),
        .external_phy = false,
#if (TUD_OPT_HIGH_SPEED)
        .fs_configuration_descriptor = hid_configuration_descriptor, // HID configuration descriptor for full-speed and high-speed are the same
        .hs_configuration_descriptor = hid_configuration_descriptor,
        .qualifier_descriptor = NULL,
#else
        .configuration_descriptor = hid_configuration_descriptor,
#endif // TUD_OPT_HIGH_SPEED
    };

    return tinyusb_driver_install(&tusb_conf);
}