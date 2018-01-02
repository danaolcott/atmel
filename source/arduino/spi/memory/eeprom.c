////////////////////////////////////////////
//Dana Olcott
//Prepared somewhere around 2017ish~ updated
//
//EEPROM Driver File
//Controller file for the Microchip 
//25AA640A/25LC640A Serial EEPROM IC
//SPI interface
//Idle clock low
//Data on the leading edge
//
//This version of the eeprom driver file
//is written for the ATMega328p Atmel
//processor - same one used in the Arduino
//board.  Uses spi pins PB2 - PB5 (pins 10-13)
//
//See spi.h/.c for spi write/reads
//
//
//For EEPROM Writes:
//Read the status bits - can do this even while a write is in progress
//Important to poll the WIP - Write in Progress bit - needs to be low before you can write
//Important to poll the WEl - Write Enable Latch - needs to be high before you can write
//

#include "eeprom.h"
#include "spi.h"

void eeprom_init()
{
    eeprom_writeEnable();
}

void eeprom_writeEnable()
{
    eeprom_writeByte(WREN);
}

void eeprom_writeDisable()
{
    eeprom_writeByte(WRDI);
}

void eeprom_writeByte(uint8_t data)
{
    spi_write(data);
}

///////////////////////////////////
//Send 2 bytes:
//WRSR 
//data
void eeprom_writeStatus(uint8_t data)
{
	uint8_t val[2] = {WRSR, data};
	spi_writeArray(val, 2);
}

void eeprom_writeData(uint16_t address, uint8_t data)
{

    //check the status - Write in progress
    //WEL - bit 1 = must be high - write enable latch
    //WIP - bit 0 = must be low - write in progress
    uint8_t status = eeprom_readStatus();

    //wait for WIP to go low
    while ((status & 0x01) == 1)
    {
        status = eeprom_readStatus();
    }

    //set the write enable latch and poll for WEL bit 1 == 1
    eeprom_writeEnable();
    status = eeprom_readStatus();

    //wait while bit 1 is not high
    while ((status & 0x02) != 2)
    {
        status = eeprom_readStatus();
    }

    //split the high and low bytes
    uint8_t lowByte = address & 0x00FF;
    uint8_t highByte = (address >> 8) & 0xFF;

    //write data sequence consists of:
    //send WRITE command
    //send high byte address
    //send low byte address
    //send data byte
    uint8_t result[4] = {WRITE, highByte, lowByte, data};

    spi_writeArray(result, 4);

}

////////////////////////////////////
//read status bits
//no need to poll the WIP bit because
//you can read this while a write is in progress
uint8_t eeprom_readStatus(void)
{
    uint8_t status = 0x00;

    //read RDSR
    status = spi_readReg(RDSR);

    //return the 8 bit value
    return status;

}

//////////////////////////////////////
//send the READ command, address, read result
uint8_t eeprom_readData(uint16_t address)
{
    uint8_t result = 0x00;
    result = spi_readData(READ, address);

    return result;
}



