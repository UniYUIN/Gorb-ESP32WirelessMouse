#ifndef BLE_H
#define BLE_H

#include <stdlib.h>

esp_err_t wake_ble(void);

esp_err_t sleep_ble(void);

bool ble_mounted(void);

void ble_hid_mouse_report(uint8_t buttons, char x, char y, char vertical);

void ble_power_save();

#endif