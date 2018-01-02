#ifndef __SPI_H_
#define __SPI_H_

#include <stdint.h>

#define SPI_CS_BIT			(1u << 2)
#define SPI_MOSI_BIT		(1u << 3)
#define SPI_MISO_BIT		(1u << 4)
#define SPI_SCK_BIT			(1u << 5)


void spi_delay(unsigned long t);
void spi_init(void);
uint8_t spi_write(uint8_t data);
void spi_writeArray(uint8_t* data, uint16_t length);
void spi_read(uint8_t* data);



/////////////////////////////////
//helper functions
void spi_select(void);
void spi_deselect(void);
uint8_t spi_tx(uint8_t data);
void spi_rx(uint8_t* data);



#endif
