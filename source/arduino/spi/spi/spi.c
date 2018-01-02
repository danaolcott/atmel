/*
SPI driver file for use with the ATMega328p
processor (same one on Arduino board).  Configures
pins PB2-PB5 (pins 10 to 13) as spi.  Provides
commands for reading and writing over the SPI
interface.

SPI configured as master, 1mhz clock, MSB first,
idle clock low, data on a leading edge.

 pin name:
 slave reset: 10:  PB2
 MOSI:        11:  PB3
 MISO:        12:  PB4
 SCK:         13:  PB5

Change clock speed with spi_setSpeed, configurable
to 4mhz, 1mhz, 250khz, 125khz.

 */
///////////////////////////////////////////////

#include <stdint.h>
#include "register.h"		//register defs
#include <avr/io.h>         //macros
#include "spi.h"


///////////////////////////////////////
//dummy delay
void spi_delay(unsigned long t)
{
	volatile unsigned long p = t;
	while (p > 0)
		p--;
}


///////////////////////////////////
//Initialize SPI on PB2-PB5 (10-13)
//SPI speed 1mhz, idle clock low, data
//on leading edge, MSB first.
//
void spi_init(void)
{
	//config pin directions per datasheet

	//Pin 10 - pb2 cs pin - normal io
	PORTB_DIR_R |= SPI_CS_BIT;		//pb2 - pin 10
	PORTB_DIR_R |= SPI_MOSI_BIT;	//pb3 - pin 11
	PORTB_DIR_R &=~ SPI_MISO_BIT;	//pb4 - pin 12
	PORTB_DIR_R |= SPI_SCK_BIT;		//pb5 - pin 13

	PORTB_DATA_R |= SPI_CS_BIT;   	 //set high
	PORTB_DATA_R &=~ SPI_MOSI_BIT;   //set low
	PORTB_DATA_R &=~ SPI_MISO_BIT;   //set low
	PORTB_DATA_R &=~ SPI_SCK_BIT;    //set low

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

	//dummy write and clear the if flag
	SPI_DATA_R = 0x00;
	spi_delay(1000);			//wait a bit...
	while (!(SPI_STATUS_R & SPI_IF_BIT)){}
	spi_delay(1000);			//wait a bit...

}


//////////////////////////////////
//Configure the clock speed.
//
void spi_setSpeed(SPISpeed_t speed)
{
	SPI_CONTROL_R &=~ BIT6;		//disable spi
	SPI_CONTROL_R &=~ 0x03;		//clear speed bits

	switch(speed)
	{
		case SPI_SPEED_4_MHZ:	SPI_CONTROL_R |= 0x00;	break;	//prescale 4
		case SPI_SPEED_1_MHZ:	SPI_CONTROL_R |= 0x01;	break;	//prescale 16
		case SPI_SPEED_250_KHZ:	SPI_CONTROL_R |= 0x02;	break;	//prescale 64
		case SPI_SPEED_125_KHZ:	SPI_CONTROL_R |= 0x03;	break;	//prescale 128
	}

	SPI_CONTROL_R |= BIT6;		//enable spi

}


/////////////////////////////////
//Write 8 bits
uint8_t spi_write(uint8_t data)
{
	uint8_t res = 0x00;
	spi_select();
	res = spi_tx(data);
	spi_deselect();

	return res;
}

////////////////////////////////////
//Write array of 8 bit values
void spi_writeArray(uint8_t* data, uint16_t length)
{
	uint16_t i = 0;

	spi_select();

	for (i = 0 ; i < length ; i++)
		spi_tx(data[i]);

	spi_deselect();
}


/////////////////////////////////////
//Write 8 bit register value
//and read the result.
//
uint8_t spi_readReg(uint8_t reg)
{
	uint8_t data = 0x00;

	spi_select();

	spi_tx(reg);		//send the register
	spi_rx(&data);		//read into data
	spi_deselect();

	return data;
}


////////////////////////////////////
//Send 8 bit command followed by
//16 bit address (MSB first), read
//result.
//
uint8_t spi_readData(uint8_t cmd, uint16_t address)
{
	uint8_t data = 0x00;
	uint8_t high = (address >> 8) & 0xFF;
	uint8_t low = (address & 0xFF);

	spi_select();

	spi_tx(cmd);		//send the command
	spi_tx(high);		//send high
	spi_tx(low);		//send low

	spi_rx(&data);		//read into data
	spi_deselect();

	return data;
}



////////////////////////////////
//CS pin low
void spi_select(void)
{
   PORTB_DATA_R &=~ SPI_CS_BIT;
}


////////////////////////////////
//CS pin high
void spi_deselect(void)
{
   PORTB_DATA_R |= SPI_CS_BIT;
}

//////////////////////////////
//helper - tx data
//
uint8_t spi_tx(uint8_t data)
{
	//write data and wait
	SPI_DATA_R = data;
	while (!(SPI_STATUS_R & SPI_IF_BIT)){}

	return ((uint8_t)SPI_DATA_R);
}

/////////////////////////////////
//send dummy 0xFF and poll IF flag
//return result
void spi_rx(uint8_t* data)
{
	//write dummy 0xFF
	SPI_DATA_R = 0xFF;
	while (!(SPI_STATUS_R & SPI_IF_BIT)){}
	*data = SPI_DATA_R;
}


