/*
 * timer_driver.h
 *
 * Created: 6/4/2018 12:01:06 AM
 *  Author: danao
 */ 


#ifndef TIMER_DRIVER_H_
#define TIMER_DRIVER_H_


#include "asf.h"
#include "stdio_serial.h"
#include "conf_board.h"
#include "conf_clock.h"

void Timer_Delay(uint32_t delay);

//timers
void Timer0_Config(void);
void Timer3_Config(void);
void Timer4_Config(void);
void Timer5_Config(void);

void Timer0_Start(void);
void Timer0_Stop(void);




#endif /* TIMER_DRIVER_H_ */

