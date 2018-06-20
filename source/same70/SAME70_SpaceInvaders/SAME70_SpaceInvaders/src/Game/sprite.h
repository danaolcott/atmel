/*
Sprite header file

*/

#ifndef __SPRITE_H
#define __SPRITE_H

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

#include "bitmap.h"			//ImageData type


///////////////////////////////////
//defines
#define NUM_ENEMY		12
#define NUM_ENEMY_ROWS	2
#define NUM_ENEMY_COLS	6
#define ENEMY_IMAGE_PADDING   ((uint16_t)2)

#define PLAYER_DEFAULT_LIVES    5
#define PLAYER_DEFAULT_X        60
#define PLAYER_DEFAULT_Y        50
#define PLAYER_MIN_X			4				//padding
#define PLAYER_MAX_X			110				//LCD width - player sizex, padding check this
#define PLAYER_DX				4

#define PLAYER_IMAGE_PADDING   ((uint16_t)2)

#define NUM_MISSILE    8

#define SPRITE_MAX_X        120
#define SPRITE_MIN_X        10
#define SPRITE_MAX_Y        48
#define SPRITE_MIN_Y        8

////////////////////////////////

typedef enum
{
	SPRITE_DIRECTION_LEFT,
	SPRITE_DIRECTION_RIGHT,
}SpriteDirection_t;

typedef enum
{
	SPRITE_VERTICAL_DOWN,
	SPRITE_VERTICAL_UP,
}SpriteVerticalDirection_t;



//player
typedef struct
{
	uint8_t numLives;
	uint32_t x;
	uint32_t y;
	uint32_t sizeX;
	uint32_t sizeY;
	const ImageData* image;
}PlayerStruct;


//enemy
typedef struct
{
	uint8_t life;
	uint32_t x;
	uint32_t y;
	uint32_t sizeX;
	uint32_t sizeY;
	uint16_t points;
    SpriteDirection_t horizDirection;
    SpriteVerticalDirection_t vertDirection;
	const ImageData* image;
}EnemyStruct;


//missile struct
typedef struct
{
	uint8_t life;               //active / not active
	uint32_t x;
	uint32_t y;
	uint32_t sizeX;
	uint32_t sizeY;
	const ImageData* image;
}MissileStruct;



void Sprite_DummyDelay(uint32_t delay);

void Sprite_Init(void);
void Sprite_Player_Init(void);
void Sprite_Enemy_Init(void);
void Sprite_Missile_Init(void);

void Sprite_Player_Move(void);
void Sprite_Enemy_Move(void);
void Sprite_Missle_Move(void);

void Sprite_Player_Missle_Launch(void);
void Sprite_Enemy_Missle_Launch(void);

int Sprite_Score_EnemyHit(uint8_t enemyIndex, uint8_t missileIndex);
int Sprite_Score_PlayerHit(uint8_t missileIndex);

uint16_t Sprite_GetGameScore(void);
uint16_t Sprite_GetGameLevel(void);

void Sprite_ResetGameScore(void);
uint8_t Sprite_GetNumPlayers(void);


int Sprite_GetNumEnemy(void);
int Sprite_GetRandomEnemy(void);


int Sprite_Player_GetNextMissile(void);
int Sprite_Enemy_GetNextMissile(void);

void Sprite_SetPlayerMissileLaunchFlag(void);
uint8_t Sprite_GetPlayerMissileLaunchFlag(void);
void Sprite_ClearPlayerMissileLaunchFlag(void);

uint8_t Sprite_GetGameOverFlag(void);
void Sprite_ClearGameOverFlag(void);


void Sprite_UpdateDisplay(void);
void Sprite_Player_Draw(void);
void Sprite_Enemy_Draw(void);
void Sprite_Missle_Draw(void);


void Sprite_Player_Explode(uint16_t x, uint16_t y);

#endif
