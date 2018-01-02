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

#define PAGE1			0x100;
#define PAGE2			0x200;


void eeprom_init(void);
void eeprom_writeEnable(void);
void eeprom_writeDisable(void);
void eeprom_writeByte(uint8_t data);
void eeprom_writeStatus(uint8_t data);
void eeprom_writeData(uint16_t address, uint8_t data);
uint8_t eeprom_readStatus(void);						//reads bits on status reg
uint8_t eeprom_readData(uint16_t address);				//read data at address





#endif

