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
//NOTE:
//Since the SPI is being shared with another device, reimplement the SPI 
//controls here.
//
#include "eeprom.h"
#include "spi.h"
#include "register.h"



//////////////////////////////////////////
//EEPROM Variables
static uint8_t eepromPage = 0x00;

//////////////////////////////////////
//SPI assumed to be enabled already
//Confiugre the CS pin - PD6 - Pin 6
void eeprom_init()
{
    //CS Pin - PD6 - Pin 6
    PORTD_DIR_R |= BIT6; 	//pin 6 as output
    PORTD_DATA_R |= BIT6;	//disable cs pin

    eepromPage = 0x00;      //current page to write to

    eeprom_writeEnable();
}


void eeprom_select()
{
    PORTD_DATA_R &=~ BIT6;
}

void eeprom_deselect()
{
    PORTD_DATA_R |= BIT6;
}

void eeprom_writeEnable()
{
    eeprom_writeByte(WREN);
}

void eeprom_writeDisable()
{
    eeprom_writeByte(WRDI);
}

//////////////////////////////////////
//
void eeprom_writeByte(uint8_t data)
{
    eeprom_select();
    SPI_tx(data);
    eeprom_deselect();
}

////////////////////////////////////////////////
//Send 2 bytes:
//WRSR 
//data
void eeprom_writeStatus(uint8_t data)
{
    eeprom_select();
    SPI_tx(WRSR);
    SPI_tx(data);
    eeprom_deselect();
}

////////////////////////////////////////////////
//Write one byte data to address
void eeprom_writeData(uint8_t address, uint8_t data)
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

    eeprom_select();

    SPI_tx(WRITE);
    SPI_tx(address);
    SPI_tx(data);

    eeprom_deselect();
}

////////////////////////////////////////////////
//read status bits
//no need to poll the WIP bit because
//you can read this while a write is in progress
uint8_t eeprom_readStatus(void)
{
    uint8_t status = 0x00;

	eeprom_select();        //select
	SPI_tx(RDSR);           //send register - RDSR
	status = SPI_rx();      //read
	eeprom_deselect();      //deselect

    return status;
}

///////////////////////////////////////////////
//send the READ command, address, read result
uint8_t eeprom_readData(uint8_t address)
{
    uint8_t result = 0x00;

    eeprom_select();

    SPI_tx(READ);
    SPI_tx(address);
    result = SPI_rx();

    eeprom_deselect();

    return result;
}


/////////////////////////////////////////////
//EEPROM - Write Page.
//Writes 16 byte sequence, page aligned.
//Valid page from PAGE_MIN to PAGE_MAX
//If buffer length is < 16 bytes, the 
//remaining is filled with 0x00.  
//Generally same as byte write, just that 16 byes
//are written.
void eeprom_writePage(uint8_t page, uint8_t* buffer, uint8_t len)
{
    uint8_t remainder, i = 0x00;
    uint8_t address = page * PAGE_SIZE;

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

    if (page <= PAGE_MAX)
    {
        eeprom_select();

        SPI_tx(WRITE);
        SPI_tx(address);

        if (len == PAGE_SIZE)
        {
            for (i = 0 ; i < len ; i++)
                SPI_tx(buffer[i]);
        }

        else if (len < PAGE_SIZE)
        {
            remainder = PAGE_SIZE - len;

            for (i = 0 ; i < len ; i++)
                SPI_tx(buffer[i]);

            for (i = 0 ; i < remainder ; i++)
                SPI_tx(0x00);
        }

        eeprom_deselect();
    }
}


/////////////////////////////////////////////
//EEPROM - Read Page
//Reads 16 bytes into buffer.  Returns the
//length of non-0x00 bytes.  
uint8_t eeprom_readPage(uint8_t page, uint8_t* buffer)
{
    uint8_t i = 0x00;
    uint8_t counter = 0x00;
    uint8_t address = page * PAGE_SIZE;

    eeprom_select();

    SPI_tx(READ);
    SPI_tx(address);

    //read 16 bytes
    for (i = 0 ; i < PAGE_SIZE ; i++)
    {
        buffer[i] = SPI_rx();

        if (buffer[i] != 0x00)
            counter++;
    }

    eeprom_deselect();

    return counter;    
}


////////////////////////////////////////////////
//Resets the current page count, clears all
//pages in EEPROM with 0x00;
void eeLog_Reset(void)
{
    uint8_t i = 0x00;
    uint8_t buffer[16] = {0x00};

    eepromPage = 0x00;

    for (i = 0 ; i < PAGE_MAX + 1 ; i++)
    {
        eeprom_writePage(i, buffer, 16);
    }
}

////////////////////////////////////////////////
//Returns the next available page for 
//logging data to.  Subsequent calls to get 
//page increment the next available page.
uint8_t eeLog_GetPage(void)
{
    if (eepromPage < PAGE_MAX)
        eepromPage++;
    else
        eepromPage = 0x00;

    return eepromPage;
}

////////////////////////////////////////////////
//Write 16 byte data entry into page.  Uses 
//GetPage to get the next available page.
void eeLog_WriteEntry(uint8_t* buffer, uint8_t len)
{
    uint8_t nextPage = eeLog_GetPage();

    if (len <= PAGE_SIZE)
        eeprom_writePage(nextPage, buffer, len);

    else
        eeprom_writePage(nextPage, buffer, PAGE_SIZE);
}




