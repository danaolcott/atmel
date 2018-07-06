/*
 * score.h
 *
 * Created: 7/3/2018 12:56:29 AM
 *  Author: danao

   Header file for Set/Get high score for the
   game.  Also functions for reading and writing
   the player name that got the high score.  High
   score is stored on the EEPROM IC using the I2C
   interface


 */ 


#ifndef SCORE_H_
#define SCORE_H_

#include <stddef.h>
#include <stdint.h>


#define SCORE_HIGH_SCORE_ADDRESS_LOW		0x80
#define SCORE_HIGH_SCORE_ADDRESS_HIGH		0x81
#define SCORE_MAX_LEVEL_ADDRESS				0x82

#define SCORE_PLAYER_NAME_SIZE				16
#define SCORE_PLAYER_NAME_ADDRESS			0x90


int Score_Init(void);

int Score_SetHighScore(uint16_t value);
uint16_t Score_GetHighScore(void);
void Score_ClearHighScore(void);

int Score_SetMaxLevel(uint8_t level);
uint8_t Score_GetMaxLevel(void);
void Score_ClearMaxLevel(void);

void Score_SetPlayerName(uint8_t* buffer, uint8_t len);
uint8_t Score_GetPlayerName(uint8_t* buffer);
void Score_ClearPlayerName(void);

void Score_DisplayNewHighScore(uint16_t score, uint8_t level);












#endif /* SCORE_H_ */