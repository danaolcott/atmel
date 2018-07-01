/*
* i2c_driver.c
*
* Created: 6/21/2018 2:23:08 AM
*  Author: danao

Configure files for the i2c peripheral on the
SAME70-xplained board.
I2C interface is used to communicate with the EEPROM
chip on the board.  

PA3 - TWD0 - SDA(Serial Data) - Connected to Arudio header
PA4 - TWCK0 - SCL(Serial Clock Line) - Connected to Arduino header
PC11 - GPIO Pin - write protect pin - pulled high

EEPROM IC - AT24MAC402
I2C address = 0x37h
Schematic on the user manual shows pins 1,2, 3 pulled high (a0=a1=a2 = high)
PC11 - connected to pin 7 - WP pin, pulled low normally.

NOTE: No pullups are shown for data/clock lines.

For now, start with just reading the chip id and read/write 
data to register.


*/ 

#include <string.h>

#include "asf.h"
#include "conf_board.h"
#include "conf_clock.h"

#include "twihs.h"
#include "i2c_driver.h"


#define ioport_set_pin_peripheral_mode(pin, mode) \
	do {\
		ioport_set_pin_mode(pin, mode);\
		ioport_disable_pin(pin);\
	} while (0)


uint8_t i2c_txBuffer[I2C_BUFFER_SIZE];
uint8_t i2c_rxBuffer[I2C_BUFFER_SIZE];



/////////////////////////////////////////////////
void I2C_DummyDelay(uint32_t delay)
{
	volatile uint32_t temp = delay;
	while (temp > 0)
		temp--;
}

////////////////////////////////////////////////
//Configures i2c peripheral on PA3 and PA4
//connected to the EEPROM IC.  
//PC11 - GPIO Pin - write protect pin - pulled high
//
void I2C_Config(void)
{
	twihs_options_t opt;

	memset(i2c_txBuffer, 0x00, 64);
	memset(i2c_rxBuffer, 0x00, 64);

	//enable peripheral clocks - i2c and WP pin
	pmc_enable_periph_clk(BOARD_ID_TWIHS_EEPROM);
	pmc_enable_periph_clk(ID_PIOC);				//wp pin

	//PC11 - pulled high
	ioport_set_pin_dir(PIO_PC11_IDX, IOPORT_DIR_OUTPUT);
	ioport_set_pin_level(PIO_PC11_IDX, true);


	//configure the I2C pins
	ioport_set_pin_peripheral_mode(TWIHS0_DATA_GPIO, TWIHS0_DATA_FLAGS);
	ioport_set_pin_peripheral_mode(TWIHS0_CLK_GPIO, TWIHS0_CLK_FLAGS);



	/* Configure the options of TWI driver */
	opt.master_clk = sysclk_get_peripheral_hz();
	opt.speed      = TWIHS_CLK;

	//init the i2c peripheral
	if (twihs_master_init(BOARD_BASE_TWIHS_EEPROM, &opt) != TWIHS_SUCCESS) 
	{
		while (1){};		
	}
}


void I2C_EEPROM_WP_Enable(void)
{
	ioport_set_pin_level(PIO_PC11_IDX, true);
}

void I2C_EEPROM_WP_Disable(void)
{
	ioport_set_pin_level(PIO_PC11_IDX, false);
}


////////////////////////////////////////////////////////
//Write length bytes from array data to memaddress
//memaddress is either 1 byte or two
void I2C_EEPROM_Write(uint16_t memaddress, uint8_t memaddlen, uint8_t* data, uint16_t length)
{
	twihs_packet_t packet;

	I2C_EEPROM_WP_Disable();

	packet.chip        = AT24C_ADDRESS;

	//1 byte address
	if (memaddlen == 1)
	{
		packet.addr[0]     = memaddress & 0xFF;
		packet.addr[1]     = memaddress & 0xFF;
		packet.addr_length = 1;
	}
	//max 2 byte address
	else
	{
		packet.addr[0]     = ((memaddress >> 8) && 0xFF);			//msb first
		packet.addr[1]     = (memaddress & 0xFF);
		packet.addr_length = 2;
	}

	//point packet to data array
	packet.buffer      = (uint8_t *) data;	//buffer
	packet.length      = length;			//length

	//send the data to the EEPROM IC
	if (twihs_master_write(BOARD_BASE_TWIHS_EEPROM, &packet) != TWIHS_SUCCESS) 
	{
		while (1) {}
	}

	I2C_DummyDelay(100000);

	I2C_EEPROM_WP_Enable();

}

///////////////////////////////////////////////////////
//read from eeprom.  memaddress is either 1 
//byte or 2
void I2C_EEPROM_Read(uint16_t memaddress, uint8_t memaddlen, uint8_t* data, uint16_t length)
{
	twihs_packet_t packet;

	packet.chip        = AT24C_ADDRESS;

	//1 byte address
	if (memaddlen == 1)
	{
		packet.addr[0]     = memaddress & 0xFF;
		packet.addr[1]     = memaddress & 0xFF;
		packet.addr_length = 1;
	}
	//max 2 byte address
	else
	{
		packet.addr[0]     = ((memaddress >> 8) && 0xFF);			//msb first
		packet.addr[1]     = (memaddress & 0xFF);
		packet.addr_length = 2;
	}

	//point packet to data array
	packet.buffer      = (uint8_t *) data;	//buffer
	packet.length      = length;			//length

	//read data from eeprom
	if (twihs_master_read(BOARD_BASE_TWIHS_EEPROM, &packet) != TWIHS_SUCCESS) 
	{
		while (1){}
	}


	I2C_DummyDelay(100000);
}











