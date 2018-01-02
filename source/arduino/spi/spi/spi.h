#ifndef __SPI_H_
#define __SPI_H_

#include <stdint.h>

#define SPI_CS_BIT			(1u << 2)
#define SPI_MOSI_BIT		(1u << 3)
#define SPI_MISO_BIT		(1u << 4)
#define SPI_SCK_BIT			(1u << 5)

//speed - useful for interface with sd card
//that requires an init speed much lower
typedef enum
{
	SPI_SPEED_4_MHZ,	//prescale 4
	SPI_SPEED_1_MHZ,	//prescale 16
	SPI_SPEED_250_KHZ,	//prescale 64
	SPI_SPEED_125_KHZ,	//prescale 128
}SPISpeed_t;

void spi_delay(unsigned long t);
void spi_init(void);
void spi_setSpeed(SPISpeed_t speed);

uint8_t spi_write(uint8_t data);
void spi_writeArray(uint8_t* data, uint16_t length);
uint8_t spi_readReg(uint8_t reg);
uint8_t spi_readData(uint8_t cmd, uint16_t address);



/////////////////////////////////
//helper functions
void spi_select(void);
void spi_deselect(void);
uint8_t spi_tx(uint8_t data);
void spi_rx(uint8_t* data);



#endif
