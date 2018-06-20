/*
 * gpio_driver.h
 *
 * Created: 6/16/2018 12:58:37 AM
 *  Author: danao

 Configure GPIO pins for user LED and button.  
 Joystick button is handled through the joystick_driver
 file.
 */ 


#ifndef GPIO_DRIVER_H_
#define GPIO_DRIVER_H_


#include "asf.h"
#include "conf_board.h"
#include "conf_clock.h"

//////////////////////////////////////////////////

void GPIO_Config(void);			//gpio config
void Button_Config(void);		//configure user button on PA11
void Button_Handler(const uint32_t id, const uint32_t index);



#endif /* GPIO_DRIVER_H_ */