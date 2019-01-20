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

#ifndef EEPROM__H
#define EEPROM__H

#include <stdio.h>
#include <stdint.h>


//defines for eeprom read/write
#define READ			0x03		//read data
#define WRITE			0x02		//write data
#define WRDI			0x04		//reset write enable latch
#define WREN			0x06		//set the write enable latch
#define RDSR			0x05		//read status reg
#define WRSR			0x01		//write status reg

#define PAGE_SIZE		0x10       //16 bytes per page
#define PAGE_MIN        0x00       //min page number
#define PAGE_MAX        0x0F       //max page num 15


void eeprom_init(void);
void eeprom_writeEnable(void);
void eeprom_writeDisable(void);
void eeprom_writeByte(uint8_t data);
void eeprom_writeStatus(uint8_t data);
void eeprom_writeData(uint8_t address, uint8_t data);

uint8_t eeprom_readStatus(void);						//reads bits on status reg
uint8_t eeprom_readData(uint8_t address);				//read data at address


//storage config
void eeprom_writePage(uint8_t page, uint8_t* buffer, uint8_t len);
uint8_t eeprom_readPage(uint8_t page, uint8_t* buffer);

void eeLog_Reset(void);
uint8_t eeLog_GetPage(void);
void eeLog_WriteEntry(uint8_t* buffer, uint8_t len);



#endif

