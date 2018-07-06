/*
 * score.c
 *
 * Created: 7/3/2018 12:56:12 AM
 *  Author: danao

 Header file for Set/Get high score for the
 game.  Also functions for reading and writing
 the player name that got the high score.  High
 score is stored on the EEPROM IC using the I2C
 interface

 */ 


#include <stdio.h>

#include "score.h"
#include "i2c_driver.h"
#include "timer_driver.h"			//delay function
#include "lcd_12864_dfrobot.h"		//lcd functions

//////////////////////////////////////
//Initializes the high score, level
//and player.  returns -1 if any of 
//them failed, 0 if ok
int Score_Init(void)
{
	uint8_t buffer[SCORE_PLAYER_NAME_SIZE] = {0x00};
	int n = sprintf((char*)buffer, "Rebecca");

	int ret1 = Score_SetHighScore(100);
	int ret2 = Score_SetMaxLevel(1);
	Score_SetPlayerName(buffer, n);

	if ((ret1 < 0) || (ret2 < 0))
		return -1;
	else
		return 0;
		
}


/////////////////////////////////////////
//Set High Score
//Write value to eeprom address low and high
//
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

///////////////////////////////////////
//Get High Score
//Read low and high bytes from eeprom
//and return high score.
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

///////////////////////////////////////
//Reset high score
void Score_ClearHighScore(void)
{
	Score_SetHighScore(0x00);
}

/////////////////////////////////////////
//Write max level to EEPROM storage.
//reads the value back.  returns 0 if
//OK, -1 if the value does not match
int Score_SetMaxLevel(uint8_t level)
{
	uint8_t value = level;
	uint8_t readback = 0x00;

	I2C_EEPROM_Write(SCORE_MAX_LEVEL_ADDRESS, 1, &value, 1);		//write
	I2C_EEPROM_Read(SCORE_MAX_LEVEL_ADDRESS, 1, &readback, 1);		//read back

	if (readback != value)
		return -1;

	return 0;
}

///////////////////////////////////////////
//Get Max Level
//Reads max level from EEPROM and returns value
uint8_t Score_GetMaxLevel(void)
{
	uint8_t readback = 0x00;
	I2C_EEPROM_Read(SCORE_MAX_LEVEL_ADDRESS, 1, &readback, 1);
	return readback;
}

//////////////////////////////////////
//Reset max level
void Score_ClearMaxLevel(void)
{
	Score_SetMaxLevel(0x00);
}

//////////////////////////////////////////////////
//Set player name in EEPROM.
//stores up to SCORE_PLAYER_NAME_SIZE bytes in 
//EEPROM.  all bytes not used out of max size
//are set to 0x00.  len bytes must be less than
//or equal to max size.
void Score_SetPlayerName(uint8_t* buffer, uint8_t len)
{
	uint8_t numWrites = len;
	uint8_t temp = 0x00;
	if (len > SCORE_PLAYER_NAME_SIZE)
		numWrites = SCORE_PLAYER_NAME_SIZE;
		
	Score_ClearPlayerName();

	for (uint16_t i = 0 ; i < numWrites ; i++)
	{	
		temp = buffer[i];	
		I2C_EEPROM_Write(SCORE_PLAYER_NAME_ADDRESS + i, 1, &temp, 1);
	}
}

////////////////////////////////////////////////
//read contents of eeprom over max player name
//size, evaluate the length of non-0x00 chars, 
//return the length.  Assumes buffer is large
//enough to hold the player name
uint8_t Score_GetPlayerName(uint8_t* buffer)
{
	uint8_t temp[SCORE_PLAYER_NAME_SIZE] = {0x00};
	uint8_t counter = 0x00;

	//read contents into temp
	I2C_EEPROM_Read(SCORE_PLAYER_NAME_ADDRESS, 1, temp, SCORE_PLAYER_NAME_SIZE);

	//copy temp into buffer
	for (uint8_t i = 0 ; i < SCORE_PLAYER_NAME_SIZE ; i++)
	{
		buffer[i] = temp[i];
		counter++;

		if (buffer[i] == 0x00)
			break;
	}

	return counter;
}


////////////////////////////////////////////////
//Score_ClearPlayerName
//write 0x00 to the player name space in EEPROM
void Score_ClearPlayerName(void)
{
	uint8_t value[SCORE_PLAYER_NAME_SIZE] = {0x00};
	I2C_EEPROM_Write(SCORE_PLAYER_NAME_ADDRESS, 1, value, SCORE_PLAYER_NAME_SIZE);
}



//////////////////////////////////////////////
//We have a new high score.
//Display something on the lcd and update the
//high score
void Score_DisplayNewHighScore(uint16_t score, uint8_t level)
{
	uint8_t buffer[SCORE_PLAYER_NAME_SIZE] = {0x00};
	uint8_t buffer2[16] = {0x00};
	uint16_t oldScore = Score_GetHighScore();
	uint8_t oldLevel = Score_GetMaxLevel();
	uint8_t len = Score_GetPlayerName(buffer);

	LCD_Clear(0x00);

	LCD_DrawStringKernLength(0, 3, buffer, len);			//name
	LCD_DrawStringKern(1, 3, "Old Stats");					//header

	int n = sprintf((char*)buffer2, "Score:%d", oldScore);	
	LCD_DrawStringKernLength(2, 3, buffer2, n);				//old

	n = sprintf((char*)buffer2, "Level:%d", oldLevel);		//old
	LCD_DrawStringKernLength(3, 3, buffer2, n);

	LCD_DrawStringKern(5, 3, "New Stats");					//header

	n = sprintf((char*)buffer2, "Score:%d", score);
	LCD_DrawStringKernLength(6, 3, buffer2, n);				//new

	n = sprintf((char*)buffer2, "Level:%d", level);			//new
	LCD_DrawStringKernLength(7, 3, buffer2, n);
}



