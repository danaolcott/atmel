
#include <stdint.h>
#include "register.h"

#include <avr/io.h>         //macros

#include "spi.h"


///////////////////////////////
//Configure gpio pins 10-13
//as spi.  configure cs pin as normal
//gpio.  spi is on pins 10-13
//pb

/*
 *
 *
// pin name:    not-mega:         mega(1280 and 2560)
// slave reset: 10:  pb2             53
// MOSI:        11:  pb3             51
// MISO:        12:  pb4             50
// SCK:         13:  pb5             52
 *
 */

void spi_delay(unsigned long t)
{
	volatile unsigned long p = t;
	while (p > 0)
		p--;
}


/////////////////////////////////
//sck, mosi, cs - output
//miso input

void spi_init(void)
{

	//Pin 10 - pb2 cs pin - normal io
	PORTB_DIR_R |= SPI_CS_BIT;		//pb2 - pin 10
	PORTB_DIR_R |= SPI_MOSI_BIT;		//pb3 - pin 11
	PORTB_DIR_R &=~ SPI_MISO_BIT;	//pb4 - pin 12
	PORTB_DIR_R |= SPI_SCK_BIT;		//pb5 - pin 13

	PORTB_DATA_R |= SPI_CS_BIT;   	//set high
	PORTB_DATA_R &=~ SPI_MOSI_BIT;   //set low
	PORTB_DATA_R &=~ SPI_MISO_BIT;   //set low
	PORTB_DATA_R &=~ SPI_SCK_BIT;   	//set low

	//configure SPI_CONTROL_R
	//bits 0 and 1 set the freq/prescale
	//assume double speed bit set 0
	//00 - prescale = 4
	//01 - prescale = 16
	//10 - 64
	//11 - 128

	//prescale = 16
	SPI_CONTROL_R |= BIT0;
	SPI_CONTROL_R &=~ BIT1;

	//double speed bit in status reg
	SPI_STATUS_R &=~ BIT0;

	SPI_CONTROL_R &=~ BIT2;		//0 - data leading edge
	SPI_CONTROL_R &=~ BIT3;		//0 - idle clock low
	SPI_CONTROL_R |= BIT4;		//1 - master mode
	SPI_CONTROL_R &=~ BIT5;		//0 - msb first
	SPI_CONTROL_R |= BIT6;		//1 - enable spi
	SPI_CONTROL_R &=~ BIT7;		//0 - interrupt disable

	SPI_DATA_R = 0x00;
	spi_delay(1000);			//wait a bit...
	//wait while status bit is low - complete when set
	while (!(SPI_STATUS_R & SPI_IF_BIT)){}
	spi_delay(1000);			//wait a bit...

}

uint8_t spi_write(uint8_t data)
{
	uint8_t res = 0x00;
	spi_select();
	res = spi_tx(data);
	spi_deselect();

	return res;
}

void spi_writeArray(uint8_t* data, uint16_t length)
{
	uint16_t i = 0;

	spi_select();

	for (i = 0 ; i < length ; i++)
		spi_tx(data[i]);

	spi_deselect();
}

void spi_read(uint8_t* data)
{
	spi_select();
	spi_rx(data);
	spi_deselect();
}


void spi_select(void)
{
   PORTB_DATA_R &=~ SPI_CS_BIT;
}


void spi_deselect(void)
{
   PORTB_DATA_R |= SPI_CS_BIT;
}

//////////////////////////////
//helper - tx data
uint8_t spi_tx(uint8_t data)
{
	//write data and wait
	SPI_DATA_R = data;
	while (!(SPI_STATUS_R & SPI_IF_BIT)){}

	//return contents of what ever is in data reg
	return ((uint8_t)SPI_DATA_R);
}

void spi_rx(uint8_t* data)
{
	//write dummy 0xFF
	SPI_DATA_R = 0xFF;
	while (!(SPI_STATUS_R & SPI_IF_BIT)){}

	*data = SPI_DATA_R;

}


