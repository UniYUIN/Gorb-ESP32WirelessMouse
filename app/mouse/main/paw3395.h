#ifndef PAW3395_H
#define PAW3395_H

#define MOTION_BURST_ADR 0x16

#define MOTION_CTRL 0x5C

#define CPI_MIN 50
#define CPI_MAX 26000

#define SET_RESOLUTION 0x47
#define RESOLUTION_X_LOW 0x48
#define RESOLUTION_X_HIGH 0x49

void wake_paw3395();

void read_move(int16_t *x, int16_t *y);

void set_dpi(uint16_t new_dpi);

#endif