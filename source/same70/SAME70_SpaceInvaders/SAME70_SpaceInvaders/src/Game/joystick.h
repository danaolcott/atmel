/*
 * joystick.h
 *
 * Created: 6/16/2018 1:14:35 AM
 *  Author: danao

 Joystick 5 position attached to the LCD shield
 Read from A0 or A1.  Since A0 is not on the board,
 A1 is connected to A0 on the shield.  This is the same
 as AFE1_AD6

 limits for up, down, left, right, and button press


 */ 


#ifndef JOYSTICK_H_
#define JOYSTICK_H_

//////////////////////////////////////////
//Joystick limits for reading various positions.
//on ADC channel 6, labeled as AD1.  These 
//set as approx. halfway points from joystick presses.
//
#define JOYSTICK_LIMIT_0		600
#define JOYSTICK_LIMIT_1		1300
#define JOYSTICK_LIMIT_2		2000
#define JOYSTICK_LIMIT_3		3000
#define JOYSTICK_LIMIT_4		3500


typedef enum
{
	JOYSTICK_UP,
	JOYSTICK_DOWN,
	JOYSTICK_LEFT,
	JOYSTICK_RIGHT,
	JOYSTICK_PRESS,
	JOYSTICK_NONE
}JoystickPosition_t;


#include "asf.h"
#include "conf_board.h"
#include "conf_clock.h"

JoystickPosition_t Joystick_GetPosition(void);
uint16_t Joystick_GetRawData(void);



#endif /* JOYSTICK_H_ */