/*
 * timer_driver.c
 *
 * Created: 6/4/2018 12:00:47 AM
 *  Author: danao
 */ 

//Notes on Timers:
//There are 4 timer modules, TC0, TC1, TC2, TC3.  Each one has
//three channels for a total of 12 timers.  Each Timer has it's
//own interrupt and IRQ.  ie, if you want to use Timer 3, you need
//to enable TC1, channel 0, and use TC3_Handler function.
//
//More Info Here:
//https://community.atmel.com/forum/please-help-me-get-sam-e70-timer-modules-work
//


#include "asf.h"
#include "conf_board.h"
#include "conf_clock.h"

#include "tc.h"				//timer
#include "timer_driver.h"	//timer function prototypes
#include "pindefs.h"		//conversion from D# to chip pin#
#include "Sound.h"			//sound files
 
static volatile uint32_t gTimerTick = 0x00;

//////////////////////////////////////////////
//Delay that uses Timer3
void Timer_Delay(uint32_t delay)
{
	gTimerTick = 0x00;

	volatile uint32_t temp = delay;
	while (gTimerTick < temp);

}

///////////////////////////////////////////////
//Timer 0 Config
//Timer0 ID = Timer0, Channel 0
//Compare Capture - Interrupt Enabled
//Configure to Trigger at 11khz
//
void Timer0_Config(void)
{
	uint32_t ul_div;
	uint32_t ul_tcclks;
	uint32_t ul_sysclk;
	uint32_t Timer0_Frequency = 11000;

	ul_sysclk = sysclk_get_cpu_hz();
	pmc_enable_periph_clk(ID_TC0);			//peripheral clock for Timer0

	//compute the clock divider	 
	tc_find_mck_divisor(1000, ul_sysclk, &ul_div, &ul_tcclks, ul_sysclk);

	/////////////////////////////////////////////////////////////
	//Configure the compare/capture value
	tc_write_rc(TC0, 0, (ul_sysclk / ul_div) / (2*Timer0_Frequency));
	tc_init(TC0, 0, ul_tcclks | TC_CMR_CPCTRG);

	////////////////////////////////////////////////////////////
	//Configure and enable interrupt on RC compare
	NVIC_EnableIRQ((IRQn_Type) ID_TC0);				//ID0
	tc_enable_interrupt(TC0, 0, TC_IER_CPCS);		//Timer 0, Channel 0	 
	tc_start(TC0, 0);								//enable the timer
}




///////////////////////////////////////////////
//Timer 3 Config
//Timer3 ID = Timer1, Channel 0
//Compare Capture - Interrupt Enabled
//Configure to Trigger at 1khz
//
void Timer3_Config()
{
	uint32_t ul_div;
	uint32_t ul_tcclks;
	uint32_t ul_sysclk;
	uint32_t Timer3_Frequency = 1000;

	ul_sysclk = sysclk_get_cpu_hz();
	pmc_enable_periph_clk(ID_TC3);		//Enable Clock Timer 3
	 
	//Clock divider for Timer3_Frequency
	tc_find_mck_divisor(Timer3_Frequency, ul_sysclk, &ul_div, &ul_tcclks, ul_sysclk);

	//Configure compare/capture
	//Timer3 ID = Timer1, Ch0
	tc_write_rc(TC1, 0, (ul_sysclk / ul_div) / (2*Timer3_Frequency));
	tc_init(TC1, 0, ul_tcclks | TC_CMR_CPCTRG);

	//////////////////////////////////////////////////
	//Configure and enable interrupt on RC compare
	NVIC_EnableIRQ((IRQn_Type) ID_TC3);				//ID3
	tc_enable_interrupt(TC1, 0, TC_IER_CPCS);		//Timer 1, Channel 0
	tc_start(TC1, 0);								//start the timer
}




///////////////////////////////////////////////
//Timer 4 Config
//Timer4 ID = Timer1, Channel 1
//Compare Capture - Interrupt Enabled
//Configure to Trigger at 500hz
//
void Timer4_Config()
{
	uint32_t ul_div;
	uint32_t ul_tcclks;
	uint32_t ul_sysclk;
	uint32_t Timer4_Frequency = 500;

	ul_sysclk = sysclk_get_cpu_hz();
	pmc_enable_periph_clk(ID_TC4);		//Enable Clock Timer 4
	
	//Clock divider for Timer4_Frequency
	tc_find_mck_divisor(Timer4_Frequency, ul_sysclk, &ul_div, &ul_tcclks, ul_sysclk);

	//Configure compare/capture
	//Timer4 ID = Timer1, Ch1
	tc_write_rc(TC1, 1, (ul_sysclk / ul_div) / (2*Timer4_Frequency));
	tc_init(TC1, 1, ul_tcclks | TC_CMR_CPCTRG);

	//////////////////////////////////////////////////
	//Configure and enable interrupt on RC compare
	NVIC_EnableIRQ((IRQn_Type) ID_TC4);				//ID4
	tc_enable_interrupt(TC1, 1, TC_IER_CPCS);		//Timer 1, Channel 1
	tc_start(TC1, 1);								//start the timer
}




///////////////////////////////////////////////////
//Timer 5 Config
//Timer5 ID = Timer1, Channel 2
//Compare Capture - Interrupt Enabled
//Configure to Trigger at 100hz
//
void Timer5_Config()
{
	uint32_t ul_div;
	uint32_t ul_tcclks;
	uint32_t ul_sysclk;
	uint32_t Timer5_Frequency = 100;

	ul_sysclk = sysclk_get_cpu_hz();
	pmc_enable_periph_clk(ID_TC5);		//Enable Clock Timer 5
	
	//Clock divider for Timer5_Frequency
	tc_find_mck_divisor(Timer5_Frequency, ul_sysclk, &ul_div, &ul_tcclks, ul_sysclk);

	//Configure compare/capture
	//Timer5 ID = Timer1, Ch2
	tc_write_rc(TC1, 2, (ul_sysclk / ul_div) / (2*Timer5_Frequency));
	tc_init(TC1, 2, ul_tcclks | TC_CMR_CPCTRG);

	//////////////////////////////////////////////////
	//Configure and enable interrupt on RC compare
	NVIC_EnableIRQ((IRQn_Type) ID_TC5);				//ID5
	tc_enable_interrupt(TC1, 2, TC_IER_CPCS);		//Timer 1, Channel 2
	tc_start(TC1, 2);								//start the timer
}




/////////////////////////////////////////////////
//ISR for Timer 0
//Runs at 11khz, used to process sound data
//
void TC0_Handler()
{
	//process sound bytes	
	Sound_InterruptHandler();

	uint32_t dummy = tc_get_status(TC0, 0);
	UNUSED(dummy);
}






/////////////////////////////////////////////////
//ISR for Timer 3
//
void TC3_Handler()
{
	//do something
	ioport_toggle_pin_level(GPIO_D6);	//D6 - PC19

	gTimerTick++;
	
	uint32_t dummy = tc_get_status(TC1, 0);
	UNUSED(dummy);
}


/////////////////////////////////////////////////
//ISR for Timer 4
//
void TC4_Handler()
{
	//do something
	ioport_toggle_pin_level(GPIO_D5);
	
	uint32_t dummy = tc_get_status(TC1, 1);
	UNUSED(dummy);
}


/////////////////////////////////////////////////
//ISR for Timer 5
//100hz timer.  Use to toggle gpio pin D4
//and toggle the high speed timer 11khz 
//on and off.  simulates starting and stopping
//sound output.
void TC5_Handler()
{
	static uint8_t sound = 0;

	//do something
	ioport_toggle_pin_level(GPIO_D4);

	if (!sound)
	{
		sound = 1;
		tc_start(TC0, 0);		//start the HS timer
	}
	else
	{
		sound = 0;
		tc_stop(TC0, 0);		//stop the HS timer
	}

	uint32_t dummy = tc_get_status(TC1, 2);
	UNUSED(dummy);
}



///////////////////////////////////////
//Timer0 - TC0
void Timer0_Start(void)
{
	tc_start(TC0, 0);
}

void Timer0_Stop(void)
{
	tc_stop(TC0, 0);
}
