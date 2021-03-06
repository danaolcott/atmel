/**
 * \file
 *
 * \brief PWM LED example for SAM.
 *
 * Copyright (c) 2011-2015 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */

/**
 * \mainpage PWM LED Example
 *
 * \par Purpose
 *
 * This example demonstrates a simple configuration of 2 PWM channels to
 * generate variable duty cycle signals.
 * The 2 LEDs on the evaluation kit will glow repeatedly.
 *
 * \section Requirements
 *
 * This example can be used on SAM boards. The 2 required leds need to
 * be connected to PWM output pins, else consider probing the PWM output pins
 * with an oscilloscope.
 *
 * \par Usage
 *
 * -# Initialize system clock and pins setting on board
 * -# Initialize PWM clock
 * -# Configure PIN_PWM_LED0_CHANNEL
 * -# Configure PIN_PWM_LED1_CHANNEL
 * -# Enable interrupt of counter event and PIN_PWM_LED0_CHANNEL
 * & PIN_PWM_LED1_CHANNEL
 * -# Change duty cycle in ISR
 *
 */
/*
 * Support and FAQ: visit <a href="http://www.atmel.com/design-support/">Atmel Support</a>
 */

#include "asf.h"
#include "stdio_serial.h"
#include "conf_board.h"
#include "conf_clock.h"

#include "timer_driver.h"			//timebase and 11khz timer
#include "spi_driver.h"				//spi - lcd control
#include "dac_driver.h"				//dac- - sound output
#include "adc_driver.h"				//adc - read joystick
#include "gpio_driver.h"			//gpio and buttons
#include "i2c_driver.h"				//eeprom
#include "lcd_12864_dfrobot.h"		//lcd driver
#include "bitmap.h"					//images
#include "joystick.h"				//joystick left/right
#include "sprite.h"					//game engine
#include "Sound.h"					//sound engine
#include "score.h"					//high score, level, etc, EEPROM

////////////////////////////////////////////////////////
//Thankyou so much Atmel for creating the test project
//for PWM demo.  I decided however to remove all instances
//of PWM for the time being and use the project as a 
//starting point for my new game - SpaceInvaders.  
//
//
///////////////////////////////////////////////////////
//Space Invaders!!!!
//Another version of SpaceInvaders using the Atmel
//SAME70-xplained devboard and 128x64 black/white LCD.
//
//The game uses the following components:
//SPI - Controlling LCD 128x64 from DFRobot.  
//
//LCD is from NewHaven Display.  NOTE: I jumpered A0 and A1
//on the board and clipped the A0 pin on the shield.  The 
//dev board labels this pin as something other than an ADC channel.
//Link to LCD Shield:
//https://www.bananarobotics.com/shop/LCD12864-Graphic-LCD-Shield
//
//Timer0 - TC0, CH0 - 11khz timer for sound
//Timer3 - TC1, CH0 - 1khz timer for system tick
//ADC - Channel 6 - located on A1 - Reading user joystick
//User button - PA11 - onBoard button
//
//
/////////////////////////////////////////////////////
//Globals
uint32_t gCounter = 0x00;			//game loop counter


int main(void)
{
	/* Initialize the SAM system */
	sysclk_init();
	board_init();

	GPIO_Config();			//LED
	Button_Config();		//user button
	SPI_Config();			//LCD SPI control
	Timer0_Config();		//11khz timer
	Timer3_Config();		//1000hz - required
	DAC_Config();			//configure DAC output on DAC0, PB13
	ADC_ConfigAD6();		//setup channel 6 - AD1 pin
	I2C_Config();			//configure I2C for EEPROM - pins on Arduino headers
	LCD_Config();			//setup lcd shield
	Sprite_Init();			//initialize the game engine
	Sound_Init();			//init the sound engine

	//comment this out of score and player are set
	//Score_Init();			//init high score, level, name
	LCD_BacklightOn();		//turn on the backlight

	Sprite_SetGameOverFlag();		//start with press button to begin

	/////////////////////////////////////////
	//Main loop

	while (1) 
	{
		//Game Over??
        if (Sprite_GetGameOverFlag() == 1)
        {
	        Sound_Play_GameOver();

			//evaluate the high score and current score
			if (Sprite_GetGameScore() > Score_GetHighScore())
			{
				while(Sprite_GetGameOverFlag() == 1)
				{
					Score_DisplayNewHighScore(Sprite_GetGameScore(), Sprite_GetGameLevel());
					Timer_Delay(1000);
					LCD_Clear(0x00);
					Timer_Delay(1000);
				}

				//update the high score and level here
				Score_SetHighScore(Sprite_GetGameScore());
				Score_SetMaxLevel(Sprite_GetGameLevel());

				Sprite_SetGameOverFlag();
			}

	        Timer_Delay(2000);
        }

		//High Score Flag - Was a new high score achieved??
		//if a new high score flag... signal the 
		//state machine to enter into high score entry state
		//the state will handle up, down, left right, etc
		//signals.
		//these get posted in any button or joystick polling
		//as usual.
		//ie, they are always posted.


		//Add the following:
		//flag for high score
		//flag for state to enter the player name
		//function for adding the new player name
		//add simple HSM??
		//two states - high score enter state
		//and offstate


		//display game over and high score.  flag is
		//cleared from button press if the flag
		//is set
        while (Sprite_GetGameOverFlag() == 1)
        {
			uint8_t buffer[SCORE_PLAYER_NAME_SIZE] = {0x00};
			uint8_t buffer2[16] = {0x00};
			uint16_t highScore = Score_GetHighScore();
			uint8_t level = Score_GetMaxLevel();
			uint8_t len = Score_GetPlayerName(buffer);

			LCD_DrawStringKernLength(1, 3, buffer, len);

			int n = sprintf((char*)buffer2, "Score:%d", highScore);
			LCD_DrawStringKernLength(2, 3, buffer2, n);

			n = sprintf((char*)buffer2, "Level:%d", level);
			LCD_DrawStringKernLength(3, 3, buffer2, n);

	        LCD_DrawStringKern(5, 3, " Press Button");

	        Timer_Delay(1000);
			LCD_Clear(0x00);
//	        LCD_DrawStringKern(2, 3, "                ");
	        Timer_Delay(1000);

	        Sprite_Init();                  //reset and clear all flags
        }

        //launch any new missiles from player?
        if (Sprite_GetPlayerMissileLaunchFlag() == 1)
        {
	        Sprite_ClearPlayerMissileLaunchFlag();  //clear flag
	        Sprite_Player_Missle_Launch();          //launch missile
        }

        //launch any new missiles from enemy?
        int16_t interval = 30 - (2 * Sprite_GetGameLevel());
		if (interval < 0)
			interval = interval * (-1);

        if (interval < 5)
			interval = 5;

        if (!(gCounter % interval))
        {
	        Sprite_Enemy_Missle_Launch();
        }

		////////////////////////////////////////////////
		//launch drone - function of the game level
		//if the level is less than 10, launch every
		//20.  if the level is more than 10, launch
		//every 15 game cycles
		if (Sprite_GetGameLevel() < 10)
		{
			if (!(gCounter % 20))
			{
				Sprite_Drone_Launch();
			}
		}
		else
		{
			if (!(gCounter % 15))
			{
				Sprite_Drone_Launch();
			}
		}


		Sprite_Player_Move();		//move player
		Sprite_Enemy_Move();		//move enemy
		Sprite_Missle_Move();		//move missile
		Sprite_Drone_Move();		//move the drone
		Sprite_UpdateDisplay();		//update the display
        
        gCounter++;

        Timer_Delay(200);
	}

}



