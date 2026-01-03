#ifndef TUD_H
#define TUD_H

#include "tinyusb.h"

#define TUSB_DESC_TOTAL_LEN (TUD_CONFIG_DESC_LEN + CFG_TUD_HID * TUD_HID_DESC_LEN)

esp_err_t wake_tud(void);

#endif