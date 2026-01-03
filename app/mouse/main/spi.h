#ifndef SPI_H
#define SPI_H

#define SPI_HOST SPI2_HOST

/**
  * @brief Wake up esp spi bus
  * @throw
      std::runtime_error
*/
esp_err_t wake_spi();

void spi_write_data(uint8_t reg, uint8_t data);

void spi_send_read(uint8_t reg);

uint8_t spi_read_data();

#endif