/*
 * score.c
 *
 * Created: 7/3/2018 12:56:12 AM
 *  Author: danao

 Header file for Set/Get high score for the
 game.  Also functions for reading and writing
 the player name that got the high score.

 */ 

#include "i2c_driver.h"

//#define SCORE_HIGH_SCORE_ADDRESS_LOW		0x80
//#define SCORE_HIGH_SCORE_ADDRESS_HIGH		0x81
//#define SCORE_MAX_LEVEL_ADDRESS				0x82

//#define SCORE_PLAYER_NAME_SIZE				16
//#define SCORE_PLAYER_NAME_ADDRESS			0x90


/////////////////////////////////////////
//write value to address low and high
//to EEPROM
int Score_SetHighScore(uint16_t value)
{
	uint8_t low = value & 0xFF;
	uint8_t high = (value >> 8) & 0xFF;
	
	I2C_EEPROM_Write(SCORE_HIGH_SCORE_ADDRESS_LOW, 1, &low, 1);
	I2C_EEPROM_Write(SCORE_HIGH_SCORE_ADDRESS_HIGH, 1, &high, 1);

	//read it back and compare
	uint8_t resLow = 0x00;
	uint8_t resHigh = 0x00;

	I2C_EEPROM_Read(SCORE_HIGH_SCORE_ADDRESS_LOW, 1, &resLow, 1);
	I2C_EEPROM_Read(SCORE_HIGH_SCORE_ADDRESS_HIGH, 1, &resHigh, 1);

	uint16_t result = resLow;
	result |= (((uint16_t)resHigh) << 8);

	//compare
	if (result != value)
		return -1;			//error

	return 0;
}

uint16_t Score_GetHighScore(void)
{
	uint8_t resLow = 0x00;
	uint8_t resHigh = 0x00;

	I2C_EEPROM_Read(SCORE_HIGH_SCORE_ADDRESS_LOW, 1, &resLow, 1);
	I2C_EEPROM_Read(SCORE_HIGH_SCORE_ADDRESS_HIGH, 1, &resHigh, 1);

	uint16_t result = resLow;
	result |= (((uint16_t)resHigh) << 8);

	return result;
}

void Score_ClearHighScore(void)
{
	Score_SetHighScore(0x00);
}

/////////////////////////////////////////
//writes max level to eeprom storage.
//reads the value back.  returns 0 if
//ok, -1 if the value does not match
int Score_SetMaxLevel(uint8_t level)
{
	uint8_t value = level;
	uint8_t readback = 0x00;

	I2C_EEPROM_Write(SCORE_MAX_LEVEL_ADDRESS, 1, &value, 1);

	//read it back
	I2C_EEPROM_Read(SCORE_MAX_LEVEL_ADDRESS, 1, &readback, 1);

	if (readback != value)
		return -1;

	return 0;
}

uint8_t Score_GetMaxLevel(void)
{
	uint8_t readback = 0x00;
	I2C_EEPROM_Read(SCORE_MAX_LEVEL_ADDRESS, 1, &readback, 1);
	return readback;
}

void Score_ClearMaxLevel(void)
{
	Score_SetMaxLevel(0x00);
}

void Score_SetPlayerName(uint8_t* buffer, uint8_t len)
{


}

uint8_t Score_GetPlayerName(uint8_t* buffer)
{


}








