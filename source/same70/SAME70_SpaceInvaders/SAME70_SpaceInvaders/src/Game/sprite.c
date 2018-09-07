/*
////////////////////////////////////////////////////////
Sprite Files
Provides control and access to the sprites in the system.
Main sprites include the following:
Player - single player that moves in x direction
Enemy - moves left and right, drops down one row each time
missile - moves up or down depending on who's shooting the missile
drone - drones cross the LCD periodically and fire at the player.

image names:
imagePlayer - 24x10
imageEnemy - 16x16
imageMissile - 8 x 8
drone image 24x10
/////////////////////////////////////////////////////////
*/
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include "sprite.h"
#include "lcd_12864_dfrobot.h"
#include "joystick.h"
#include "bitmap.h"

#include "Sound.h"

//player, enemy, missile, drone
volatile PlayerStruct mPlayer;
static EnemyStruct mEnemy[NUM_ENEMY];
static MissileStruct mEnemyMissile[NUM_MISSILE];
static MissileStruct mPlayerMissile[NUM_MISSILE];
static DroneStruct mDrone;

//flags
static uint8_t mPlayerMissileLaunchFlag;		//set in button isr
static uint16_t mGameScore;						//score
static uint16_t mGameLevel;						//level
static uint8_t mGameOverFlag = 0;				//set when last player killed

///////////////////////////////////////////
//Dummy delay for showing sequence of image
//events.  ie, player explode, etc
//
void Sprite_DummyDelay(uint32_t delay)
{
    volatile uint32_t time = delay;

    while (time > 0)
        time--;
}

/////////////////////////////////////
//init all sprites in the game
void Sprite_Init(void)
{
    mPlayerMissileLaunchFlag = 0x00;
    mGameScore = 0x00;
    mGameLevel = 1;

    Sprite_Player_Init();
    Sprite_Enemy_Init();
    Sprite_Missile_Init();
	Sprite_Drone_Init();

    //init the random numbers
    //rand() % (max_number + 1 - minimum_number) + minimum_number
    //random enemy array index.
	int temp = rand() % (NUM_ENEMY) + 0;            
}




//////////////////////////////////
//Init player struct
void Sprite_Player_Init(void)
{
    mPlayer.numLives = PLAYER_DEFAULT_LIVES;
    mPlayer.image = &imagePlayer1;
    mPlayer.x = PLAYER_DEFAULT_X;
    mPlayer.y = PLAYER_DEFAULT_Y;
    mPlayer.sizeX = imagePlayer1.xSize;
    mPlayer.sizeY = imagePlayer1.ySize;
}


////////////////////////////////////
//Init enemy struct array
void Sprite_Enemy_Init(void)
{
    uint8_t count = 0;
    for (int i = 0 ; i < NUM_ENEMY_ROWS ; i++)
    {
        for (int j = 0 ; j < NUM_ENEMY_COLS ; j++)
        {
            mEnemy[count].life = 1;                               //life - 1 = alive, 0 = dead
            mEnemy[count].image = &imageEnemy1;                   //pointer to image data
            mEnemy[count].points = 30;                            //points
            mEnemy[count].x = j * imageEnemy1.xSize;              //x position
            mEnemy[count].y = i * imageEnemy1.ySize;              //y position
            mEnemy[count].sizeX = imageEnemy1.xSize;              //image width
            mEnemy[count].sizeY = imageEnemy1.ySize;              //image height
            mEnemy[count].horizDirection = SPRITE_DIRECTION_LEFT; //initial direction
            mEnemy[count].vertDirection = SPRITE_VERTICAL_DOWN;   //moving down

            count++;
        }
    }
}


///////////////////////////////////////
//init the missile arrays for player
//and enemy
void Sprite_Missile_Init(void)
{

    for (int i = 0 ; i < NUM_MISSILE ; i++)
    {
        mEnemyMissile[i].life = 0;                      //life - 1 = alive, 0 = dead
        mEnemyMissile[i].image = &imageMissile1;		//pointer to image data
        mEnemyMissile[i].x = 0;
        mEnemyMissile[i].y = 0;
        mEnemyMissile[i].sizeX = imageMissile1.xSize;   //image width
        mEnemyMissile[i].sizeY = imageMissile1.ySize;   //image height

        mPlayerMissile[i].life = 0;                     //life - 1 = alive, 0 = dead
        mPlayerMissile[i].image = &imageMissile1;       //pointer to image data
        mPlayerMissile[i].x = 0;
        mPlayerMissile[i].y = 0;
        mPlayerMissile[i].sizeX = imageMissile1.xSize;  //image width
        mPlayerMissile[i].sizeY = imageMissile1.ySize;  //image height

    }
}


//////////////////////////////////////////
//Init Drone
//Drone struct also has a timeout that 
//ticks down for each game loop.  Drone
//is cleared when moving off screen or timeout
//or shot by player
void Sprite_Drone_Init(void)
{
	mDrone.life = 0;
	mDrone.x = 0;
	mDrone.y = 0;
	mDrone.points = 100;
	mDrone.image = &bmimgDrone1Bmp;
	mDrone.sizeX = bmimgDrone1Bmp.xSize;
	mDrone.sizeY = bmimgDrone1Bmp.ySize;
	mDrone.timeTick = 0;				//current cycle counter
	mDrone.timeout = 100;				//number of game cycles to timeout
	mDrone.horizDirection = SPRITE_DIRECTION_LEFT;
}



//////////////////////////////////////////////////////
//Set the player position x-direction
//Uses the joystick on the LCD shield to get left 
//and right presses.  If pressed left or right, move
//player by 2 pixels.
//Player is left aligned, so max right position
//is LCD_WIDTH - player.sizeX - 1
//
void Sprite_Player_Move(void)
{
	JoystickPosition_t pos = Joystick_GetPosition();
	//move left
	if (pos == JOYSTICK_LEFT)
	{
		if (mPlayer.x > (PLAYER_MIN_X + PLAYER_DX))
			mPlayer.x -= PLAYER_DX;
	}
	else if (pos == JOYSTICK_RIGHT)
	{
		if (mPlayer.x < (PLAYER_MAX_X + PLAYER_DX))
			mPlayer.x += PLAYER_DX;
	}
	mPlayer.y = PLAYER_DEFAULT_Y;		//just in case...

	if (mPlayer.x < PLAYER_MIN_X)
		mPlayer.x = PLAYER_MIN_X;
	if (mPlayer.x > PLAYER_MAX_X)
		mPlayer.x = PLAYER_MAX_X;
}

/////////////////////////////////////
//loop over all enemy.  if enemy
//life == 1, move it dx dy.  
void Sprite_Enemy_Move(void)
{
    int i, j;    
    unsigned char flag = 0;
    
    for(i = 0 ; i < NUM_ENEMY ; i++)
    {
        //moving right
        if (mEnemy[i].horizDirection == SPRITE_DIRECTION_RIGHT)
        {
            //right edge and alive
            if(((mEnemy[i].x + mEnemy[i].sizeX) < SPRITE_MAX_X) && (mEnemy[i].life == 1))     //right edge
                mEnemy[i].x += 2;
        }
        
        //moving left
        else
        {
            if((mEnemy[i].x > SPRITE_MIN_X) && (mEnemy[i].life == 1))     //left edge
                mEnemy[i].x -= 2;
        } 
    }
    
    //check for direction change - left
    flag = 0;
    for (i = 0 ; i < NUM_ENEMY ; i++)
    {
        if (((mEnemy[i].x + mEnemy[i].sizeX) >= SPRITE_MAX_X) && (mEnemy[i].life == 1))
            flag = 1;
    }
    
    if (flag == 1)
    {
        for (j = 0 ; j < NUM_ENEMY ; j++)
            mEnemy[j].horizDirection = SPRITE_DIRECTION_LEFT;
    }
    
    //check for direction change - right
    flag = 0;
    for (i = 0 ; i < NUM_ENEMY ; i++)
    {
        if ((mEnemy[i].x <= SPRITE_MIN_X) && (mEnemy[i].life == 1))
            flag = 1;
    }
    if (flag == 1)
    {
        for (j = 0 ; j < NUM_ENEMY ; j++)
        {
            mEnemy[j].horizDirection = SPRITE_DIRECTION_RIGHT;
                        
            //move down on a direction change, if current
            //is moving down... continue moving down
            if (mEnemy[j].vertDirection == SPRITE_VERTICAL_DOWN)
            {
                if(((mEnemy[j].y + mEnemy[j].sizeY) < SPRITE_MAX_Y) && (mEnemy[j].life == 1))
                    mEnemy[j].y++;
            }
            
            //move up on a direction change, if current 
            //is moving up, continue moving up
            else
            {
                if((mEnemy[j].y > SPRITE_MIN_Y) && (mEnemy[j].life == 1))
                    mEnemy[j].y--;
            }             
        }            
    }

    //check for direction change - up
    flag = 0;
    for (i = 0 ; i < NUM_ENEMY ; i++)
    {
        if (((mEnemy[i].y + mEnemy[i].sizeY) >= SPRITE_MAX_Y) && (mEnemy[i].life == 1))
            flag = 1;
    }
    if (flag == 1)
    {
        for (j = 0 ; j < NUM_ENEMY ; j++)
            mEnemy[j].vertDirection = SPRITE_VERTICAL_UP;
    }
    
    //check for direction change - down
    flag = 0;
    for (i = 0 ; i < NUM_ENEMY ; i++)
    {
        if ((mEnemy[i].y <= SPRITE_MIN_Y) && (mEnemy[i].life == 1))
            flag = 1;
    }
    if (flag == 1)
    {
        for (j = 0 ; j < NUM_ENEMY ; j++)
            mEnemy[j].vertDirection = SPRITE_VERTICAL_DOWN;            
    }
}







////////////////////////////////////////////
//loop over all player missile and
//enemy missile.  if life == 1
//move it.  Player missiles move up (y-)
//and enemy missiles move down (y+)
void Sprite_Missle_Move(void)
{
	uint16_t mX, mY, bot, top, left, right = 0x00;

	uint8_t playerHitFlag = 0;

	for (int i = 0 ; i < NUM_MISSILE ; i++)
	{
		////////////////////////////////////////////////
		//player missiles
		//player missile - moving up (y-) - live = 1, move
		if ((mPlayerMissile[i].life == 1) && (mPlayerMissile[i].y > SPRITE_MIN_Y))
			mPlayerMissile[i].y-=2;

		//player missile off the screen?
		if ((mPlayerMissile[i].life == 1) && (mPlayerMissile[i].y <= SPRITE_MIN_Y))
			mPlayerMissile[i].life = 0;

		//Player missile hit enemy or drone
		if (mPlayerMissile[i].life == 1)
		{
			//missile hit drone...
			if (mDrone.life == 1)
			{
				mX = mPlayerMissile[i].x + (mPlayerMissile[i].sizeX / 2);
				mY = mPlayerMissile[i].y;

				bot = mDrone.y + mDrone.sizeY - ENEMY_IMAGE_PADDING;
				top = mDrone.y + ENEMY_IMAGE_PADDING;
				left = mDrone.x + ENEMY_IMAGE_PADDING;
				right = mDrone.x + mDrone.sizeX - ENEMY_IMAGE_PADDING;

				//tip of the missile in the drone box
				if ((mX >= left) && (mX <= right) && (mY <= bot) && (mY >= top))
				{
					//pass missile index to remove the missile
					//play sound, and explosion sequence.
					Sprite_Score_DroneHit(i);
				}
			}

			//missile hit enemy...
			for (int j = 0 ; j < NUM_ENEMY ; j++)
			{
				//if the enemy is alive...
				if (mEnemy[j].life == 1)
				{
					mX = mPlayerMissile[i].x + (mPlayerMissile[i].sizeX / 2);
					mY = mPlayerMissile[i].y;

					bot = mEnemy[j].y + mEnemy[j].sizeY - ENEMY_IMAGE_PADDING;
					top = mEnemy[j].y + ENEMY_IMAGE_PADDING;
					left = mEnemy[j].x + ENEMY_IMAGE_PADDING;
					right = mEnemy[j].x + mEnemy[j].sizeX - ENEMY_IMAGE_PADDING;

					//tip of the missile in the enemy box?
					if ((mX >= left) && (mX <= right) && (mY <= bot) && (mY >= top))
					{
						//score hit!! - pass enemy index and missile index
						//returns remaining
						int rem = Sprite_Score_EnemyHit(j, i);

						//if !rem, all enemy is cleared and reset
						if (!rem)
						{
							Sound_Play_LevelUp();           //play a sound
							mGameLevel++;                   //increment game level
							Sprite_Enemy_Init();            //reset the enemy
						}
					}
				}
			}
		}


		///////////////////////////////////////////////////
		//enemy missile - these go all the way to the bottom
		//of the screen - LCD_HEIGHT
		if ((mEnemyMissile[i].life == 1) && ((mEnemyMissile[i].y + mEnemyMissile[i].sizeY) < (LCD_HEIGHT - 1)))
			mEnemyMissile[i].y+=2;	

		//enemy missile off the screen?
		if ((mEnemyMissile[i].life == 1) && ((mEnemyMissile[i].y + mEnemyMissile[i].sizeY) >= (LCD_HEIGHT - 1)))
			mEnemyMissile[i].life = 0;

		//enemy missile hit the player... evaluate bottom of missile
		//with player box
		if (mEnemyMissile[i].life == 1)
		{
			mX = mEnemyMissile[i].x + (mEnemyMissile[i].sizeX / 2);
			mY = mEnemyMissile[i].y + mEnemyMissile[i].sizeY;

			bot = mPlayer.y + mPlayer.sizeY - PLAYER_IMAGE_PADDING;
			top = mPlayer.y + PLAYER_IMAGE_PADDING;
			left = mPlayer.x + PLAYER_IMAGE_PADDING;
			right = mPlayer.x + mPlayer.sizeX - PLAYER_IMAGE_PADDING;

			if ((mX >= left) && (mX <= right) && (mY <= bot) && (mY >= top))
			{
				//score hit!! - pass the enemy missile index
				//returns the num players remaining
				int remaining = 0;
				if (!playerHitFlag)
				{
					remaining = Sprite_Score_PlayerHit(i);
					if (!remaining)
						mGameOverFlag = 1;
					i = NUM_MISSILE - 1;
					break;
				}

				//set a flag here - first time only
				playerHitFlag = 1;
			}
		}
	}
}


/////////////////////////////////////////////////
//If the drone is active, life=1, then move
//the drone.  decrement the timer tick
//kill off drone if off screen, timeTick = 0,
//Drones move left to right,??
void Sprite_Drone_Move(void)
{
	static uint16_t leftCounter = 21;
	static uint16_t rightCounter = 21;

	//drone is alive
	if (mDrone.life == 1)
	{
		if (mDrone.horizDirection == SPRITE_DIRECTION_LEFT)
		{
			//moving left
			if ((mDrone.x + mDrone.sizeX) < (SPRITE_MAX_X - 4))
			{
				if (!(leftCounter % 9))
				{
					//fire missile
					Sprite_Drone_Missle_Launch();
				}

				mDrone.x += 3;
				mDrone.y--;
				leftCounter++;
			}
			else
			{
				//remove
				mDrone.life = 0;
				mDrone.timeTick = 0;
				mDrone.x = 0;
				mDrone.y = 0;
			}
		}
		else
		{
			//moving right
			if ((mDrone.x) > (SPRITE_MIN_X + 4))
			{
				if (!(leftCounter % 13))
				{
					//fire missile
					Sprite_Drone_Missle_Launch();
				}

				mDrone.x -= 3;
				rightCounter++;
			}
			else
			{
				//remove
				mDrone.life = 0;
				mDrone.timeTick = 0;
				mDrone.x = 0;
				mDrone.y = 0;
			}
		}

		//cycle counter timeout
		if ((mDrone.timeTick > 0) && (mDrone.timeTick <= mDrone.timeout))
		{
			mDrone.timeTick--;			//decrement
		}
		else
		{
			//kill off
			mDrone.life = 0;
			mDrone.timeTick = 0;
			mDrone.x = 0;
			mDrone.y = 0;
		}
	}

	else
	{
		mDrone.timeTick = 0;			//hold
	}
}



/////////////////////////////////////////////////////
//Launch the drone into the player area.
//Makes drone struct active, resets a timeout.
//life = 0 of offscreen or timeout.
//
void Sprite_Drone_Launch(void)
{
	static SpriteDirection_t dir = SPRITE_DIRECTION_RIGHT;

	if (!mDrone.life)
	{
		mDrone.life = 1;
		mDrone.timeTick = mDrone.timeout;
		mDrone.x = 10;
		mDrone.y = 38;
		mDrone.points = 100;

		mDrone.horizDirection = dir;

		if (dir == SPRITE_DIRECTION_LEFT)
		{
			dir = SPRITE_DIRECTION_RIGHT;
			mDrone.x = 10;
		}
		else
		{
			dir = SPRITE_DIRECTION_LEFT;
			mDrone.x = 100;
		}
	}
}




/////////////////////////////////////////
//Initiate missile from the player,
//moving in the y-- direction.  
//Get the next available missile from
//missile live = 0.  Set the live =1
//put the missile position at the center
//x of the player, bottom of the missile at the
//top of the player.
void Sprite_Player_Missle_Launch(void)
{
    int nextMissile = Sprite_Player_GetNextMissile();

    //set the missile in the array as live
    mPlayerMissile[nextMissile].life = 1;
    mPlayerMissile[nextMissile].x = mPlayer.x + (mPlayer.sizeX / 2) - (mPlayerMissile[nextMissile].sizeX / 2);
    mPlayerMissile[nextMissile].y = mPlayer.y - mPlayerMissile[nextMissile].sizeY;

    //play a sound
    Sound_Play_PlayerFire();
}


////////////////////////////////////////
//Launch missile from enemy to player
//get the num enemy remaining and get
//a random number.  Fire missile from
//enemy location.  
void Sprite_Enemy_Missle_Launch(void)
{
    int nextMissile = Sprite_Enemy_GetNextMissile();    //next missile
    int index = Sprite_GetRandomEnemy();                //index of random enemy

    if (index >= 0)
    {        
        //set the missile in the array as live
        mEnemyMissile[nextMissile].life = 1;
        mEnemyMissile[nextMissile].x = mEnemy[index].x + (mEnemy[index].sizeX / 2) - (mEnemyMissile[nextMissile].sizeX / 2);
        mEnemyMissile[nextMissile].y = mEnemy[index].y + mEnemy[index].sizeY;

        Sound_Play_EnemyFire();
    }
 
}



///////////////////////////////////////////////
//Launch missle from drone.  Drone is assumed to
//be onscreen, alive.. etc.  Use enemy missile array
//for missiles
void Sprite_Drone_Missle_Launch(void)
{
	int nextMissile = Sprite_Enemy_GetNextMissile();    //next missile

	//set the missile in the array as live
	mEnemyMissile[nextMissile].life = 1;
	mEnemyMissile[nextMissile].x = mDrone.x + (mDrone.sizeX / 2) - (mEnemyMissile[nextMissile].sizeX / 2);
	mEnemyMissile[nextMissile].y = mDrone.y + mDrone.sizeY;

	Sound_Play_EnemyFire();
}




////////////////////////////////////////
//returns the index of the array element
//containing the next missile with live = 0
//returns -1 for no available missiles
int Sprite_Player_GetNextMissile(void)
{
    for (int i = 0 ; i < NUM_MISSILE ; i++)
    {
        if (!(mPlayerMissile[i].life))
            return i;
    }

    return - 1;
}


////////////////////////////////////////
//returns the index of the array element
//containing the next missile with live = 0
//returns -1 for no available missiles
int Sprite_Enemy_GetNextMissile(void)
{
    for (int i = 0 ; i < NUM_MISSILE ; i++)
    {
        if (!(mEnemyMissile[i].life))
            return i;
    }

    return - 1;
}


/////////////////////////////////////////////
void Sprite_SetPlayerMissileLaunchFlag(void)
{
    mPlayerMissileLaunchFlag = 1;
}

/////////////////////////////////////////////
uint8_t Sprite_GetPlayerMissileLaunchFlag(void)
{
    return mPlayerMissileLaunchFlag;
}

void Sprite_ClearPlayerMissileLaunchFlag(void)
{
    mPlayerMissileLaunchFlag = 0;
}


uint8_t Sprite_GetGameOverFlag(void)
{
    return mGameOverFlag;
}

void Sprite_ClearGameOverFlag(void)
{
    mGameOverFlag = 0;
}

void Sprite_SetGameOverFlag(void)
{
	mGameOverFlag = 1;
}



//////////////////////////////////////////////
//Missile from player hit an enemy
//play a sound, remove the missile
//remove the enemy.  returns the number of 
//enemy remaining.
int Sprite_Score_EnemyHit(uint8_t enemyIndex, uint8_t missileIndex)
{
    Sound_Play_EnemyExplode();                                      //play sound
    mGameScore += mEnemy[enemyIndex].points;                        //increment the score
    mEnemy[enemyIndex].life = 0;                                    //remove enemy
    mEnemy[enemyIndex].horizDirection = SPRITE_DIRECTION_RIGHT;     //reset
    mEnemy[enemyIndex].vertDirection = SPRITE_VERTICAL_DOWN;        //reset
    
    mPlayerMissile[missileIndex].life = 0;                          //remove missile
    mPlayerMissile[missileIndex].x = 0;                             //reset x
    mPlayerMissile[missileIndex].y = 0;                             //reset y

    int remaining = Sprite_GetNumEnemy();

    return remaining;
}

///////////////////////////////////////////////////
//Player hit drone
void Sprite_Score_DroneHit(uint8_t missileIndex)
{
	Sound_Play_EnemyExplode();                    		//play sound
	Sprite_Drone_Explode(mDrone.x, mDrone.y);			//show explosion
	mGameScore += mDrone.points;                        //increment the score
	mDrone.life = 0;
	mDrone.x = 0;
	mDrone.y = 0;
	mDrone.horizDirection = SPRITE_DIRECTION_RIGHT;

	mPlayerMissile[missileIndex].life = 0;     			//remove missile
	mPlayerMissile[missileIndex].x = 0;                 //reset x
	mPlayerMissile[missileIndex].y = 0;                 //reset y
}




////////////////////////////////////////////////
//Enemy or Drone Missile Hit Player
//remove one player, set enemy missile[index] live
//to 0, play a sound... 
int Sprite_Score_PlayerHit(uint8_t missileIndex)
{
    mEnemyMissile[missileIndex].life = 0;      //remove missile
    mEnemyMissile[missileIndex].x = 0;         //reset x
    mEnemyMissile[missileIndex].y = 0;         //reset y

    //remove the player
    if (mPlayer.numLives > 1)
    {
        //play explosion sequence at player x and y
        Sprite_Player_Explode(mPlayer.x, mPlayer.y);    //play explosion
        mPlayer.numLives--;                             //decrement
		Sound_Play_PlayerExplode();                     //play small explosion
    }

    else if (mPlayer.numLives == 1)
    {
        Sprite_Player_Explode(mPlayer.x, mPlayer.y);    //play explosion
        mPlayer.numLives = 0;                             //decrement
		Sound_Play_PlayerExplode();                     //play small explosion
    }

    return mPlayer.numLives;
}


uint16_t Sprite_GetGameScore(void)
{
    return mGameScore;
}

uint16_t Sprite_GetGameLevel(void)
{
    return mGameLevel;
}

void Sprite_ResetGameScore(void)
{
    mGameScore = 0;
}

uint8_t Sprite_GetNumPlayers(void)
{
    return mPlayer.numLives;
}

int Sprite_GetNumEnemy(void)
{
    int num = 0;
    for (int i = 0 ; i < NUM_ENEMY ; i++)
    {
        if (mEnemy[i].life == 1)
            num++;
    }

    return num;
}


////////////////////////////////////
//returns the index of a live random
//enemy for use in shooting missile
int Sprite_GetRandomEnemy(void)
{
    int numEnemy = Sprite_GetNumEnemy();    //number of enemy

    if (numEnemy > 0)
    {
        //get the random index
        //rand() % (max_number + 1 - minimum_number) + minimum_number
        //use index as the counter in the live enemy array
        //0 - first live, 1 - second live,... etc
        int index = rand() % (numEnemy - 1 + 1 - 0) + 0;
        int counter = 0;

        for (int i = 0 ; i < NUM_ENEMY ; i++)
        {
            if (mEnemy[i].life == 1)
            {
                if (index == counter)
                    return counter;

                counter++;      //increment only for live enemy
            }
        }
    }
    
    return -1;
}




///////////////////////////////////
//draw player, enemy, all missiles
//and update the display
void Sprite_UpdateDisplay(void)
{
    uint8_t buffer[32];

    memset(frameBuffer, 0x00, FRAME_BUFFER_SIZE);

    Sprite_Player_Draw();
    Sprite_Enemy_Draw();
    Sprite_Missle_Draw();
	Sprite_Drone_Draw();
    LCD_Update(frameBuffer);

    int n = sprintf((char*)buffer, "L:%2d S:%6d  P:%d", mGameLevel, mGameScore, mPlayer.numLives);
    LCD_DrawStringKernLength(0, 1, buffer, n);
}

/////////////////////////////////////
//Draw the player icon at the player
//x and y position.  Do this only
//if the num lives are > 0
void Sprite_Player_Draw(void)
{
    if (mPlayer.numLives > 0)
    {
        LCD_DrawIcon(mPlayer.x, mPlayer.y, mPlayer.image, 0);
    }
}

////////////////////////////////////////////
//Loop through enemy array and draw those
//enemy that have a life = 1
//
void Sprite_Enemy_Draw(void)
{
    for (int i = 0 ; i < NUM_ENEMY ; i++)
    {
        if (mEnemy[i].life == 1)
        {
            LCD_DrawIcon(mEnemy[i].x, mEnemy[i].y, mEnemy[i].image, 0);
        }
    }
}



///////////////////////////////////////
//Draw the player missile array and the 
//enemy missile array 
void Sprite_Missle_Draw(void)
{
    for (int i = 0 ; i < NUM_MISSILE ; i++)
    {
        //check enemy missile
        if (mEnemyMissile[i].life == 1)
            LCD_DrawIcon(mEnemyMissile[i].x, mEnemyMissile[i].y, mEnemyMissile[i].image, 0);

        //check player missile
        if (mPlayerMissile[i].life == 1)
            LCD_DrawIcon(mPlayerMissile[i].x, mPlayerMissile[i].y, mPlayerMissile[i].image, 0);
    }
}


/////////////////////////////////////////
//Draw the drone if the drone is alive
void Sprite_Drone_Draw(void)
{
	if (mDrone.life == 1)
		LCD_DrawIcon(mDrone.x, mDrone.y, mDrone.image, 0);
}



////////////////////////////////////////////////
//Play explosion sequence at player x and y
//flash through a few images on a delay
//toggle the backlight
void Sprite_Player_Explode(uint16_t x, uint16_t y)
{
    LCD_DrawIcon(mPlayer.x, mPlayer.y, &bmimgPlayerExp1Bmp, 1);
	LCD_BacklightOff();
    Sprite_DummyDelay(1200000);
    LCD_DrawIcon(mPlayer.x, mPlayer.y, &bmimgPlayerExp2Bmp, 1);
	LCD_BacklightOn();
    Sprite_DummyDelay(1200000);
    LCD_DrawIcon(mPlayer.x, mPlayer.y, &bmimgPlayerExp3Bmp, 1);
	LCD_BacklightOff();
    Sprite_DummyDelay(1200000);
    LCD_DrawIcon(mPlayer.x, mPlayer.y, &bmimgPlayerExp4Bmp, 1);
	LCD_BacklightOn();
    Sprite_DummyDelay(1200000);
	LCD_BacklightOff();
	Sprite_DummyDelay(1200000);
	LCD_BacklightOn();
	Sprite_DummyDelay(1200000);
}


///////////////////////////////////////////////////////
//Play explosion sequence for drone
//x and y are the coordinates of the drone
//use force update to LCD
void Sprite_Drone_Explode(uint16_t x, uint16_t y)
{
	LCD_DrawIcon(x, y, &bmimgDroneExp1Bmp, 1);
	Sprite_DummyDelay(600000);
	LCD_DrawIcon(x, y, &bmimgDroneExp2Bmp, 1);
	Sprite_DummyDelay(600000);
	LCD_DrawIcon(x, y, &bmimgDroneExp3Bmp, 1);
	Sprite_DummyDelay(600000);
	LCD_DrawIcon(x, y, &bmimgDroneExp4Bmp, 1);
	Sprite_DummyDelay(600000);
}


