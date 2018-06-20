/*
 * gpio_driver.c
 *
 * Created: 6/16/2018 12:58:16 AM
 *  Author: danao

 Configure GPIO pins for user LED and button
 Configure button with interrupts.  Joystick
 button is handled through the joystick_driver file.

 */ 


 
#include "asf.h"
#include "conf_board.h"
#include "conf_clock.h"
#include "gpio_driver.h"

#include "sprite.h"
#include "lcd_12864_dfrobot.h"


 ////////////////////////////////////////////////////
 //GPIO_Config(void)
 //LED0 - PIO_PC8_IDX
 void GPIO_Config(void)
 {
	 //enable the clocks
	 pmc_enable_periph_clk(ID_PIOA);												//enable the clock
	 pmc_enable_periph_clk(ID_PIOC);												//enable the clock

	 //Configure PC19 - D6
	 //	ioport_set_pin_dir(PIO_PC19_IDX, IOPORT_DIR_OUTPUT);
	 //	ioport_set_pin_level(PIO_PC19_IDX, false);

	 //Configure PD11 - D5
	 //	ioport_set_pin_dir(PIO_PD11_IDX, IOPORT_DIR_OUTPUT);
	 //	ioport_set_pin_level(PIO_PD11_IDX, false);

	 //Configure PD27 - D4
	 //	ioport_set_pin_dir(PIO_PD27_IDX, IOPORT_DIR_OUTPUT);
	 //	ioport_set_pin_level(PIO_PD27_IDX, false);

	 //configure PC8 as output - LED0
	 ioport_set_pin_dir(PIO_PC8_IDX, IOPORT_DIR_OUTPUT);
	 ioport_set_pin_level(PIO_PC8_IDX, false);
 }


//////////////////////////////////////////////////
//Button Config
//Setup user button on PA11 as input, interrupt
//enabled, falling edge, pullup enabled.
//Setup D2 on PA5 as input, interrupt enabled, falling
//edge, pullup enabled.
//
//Note: D2 on the LCD shield is jumpered to the reset
//button.  The reset pin is clipped.  Therefore, when 
//the shield is connected, you can use D2 as the userbutton.
//NOTE:
//PA11 is not the same as PA11_IDX.
//PIO_PA11 = 1 << 11	bitmask
//PIO_PA11_IDX = 11 - ie, index
//Handler function: Button_Handler
void Button_Config()
{
	pmc_enable_periph_clk(ID_PIOA);												//enable the clock

	//PA11
	pio_set_input(PIOA, PIO_PA11, PIO_PULLUP);									//PA11 as input with pullup enabled
	pio_set_debounce_filter(PIOA, PIO_PA11, 1000000);							//debounce
	pio_handler_set(PIOA, ID_PIOA, PIO_PA11, PIO_IT_FALL_EDGE, Button_Handler);	//configure interrupt event
	pio_enable_interrupt(PIOA, PIO_PA11);										//enable the line interrupts

	//PA5
	pio_set_input(PIOA, PIO_PA5, PIO_PULLUP);									//PA11 as input with pullup enabled
	pio_set_debounce_filter(PIOA, PIO_PA5, 1000000);							//debounce
	pio_handler_set(PIOA, ID_PIOA, PIO_PA5, PIO_IT_FALL_EDGE, Button_Handler);	//configure interrupt event
	pio_enable_interrupt(PIOA, PIO_PA5);										//enable the line interrupts

	//NVIC
	NVIC_DisableIRQ(PIOA_IRQn);
	NVIC_ClearPendingIRQ(PIOA_IRQn);
	NVIC_SetPriority(PIOA_IRQn, 15);				//low priority - same as systic
	NVIC_EnableIRQ(PIOA_IRQn);						//interrupt enable - PORTA
}


//////////////////////////////////////////////////////////////
//Button Handler for SW0, PA11
//Button handler for user button on shield on PA5
//
//id = GPIO Port, ie, PIOA, index = pin, ie, PIO_PA11
//Set button flag = polled in main loop
void Button_Handler(const uint32_t id, const uint32_t index)
{
	//PA11 - button
	if ((id == ID_PIOA) && (index == PIO_PA11))
	{
		//if game over flag is set, clear it
		//and no missle launch
		if (Sprite_GetGameOverFlag() == 1)
			Sprite_ClearGameOverFlag();
		else
			Sprite_SetPlayerMissileLaunchFlag();

		pio_get(PIOA, PIO_TYPE_PIO_INPUT, PIO_PA11);	//clear interrupt
	}

	//PA5 - button
	if ((id == ID_PIOA) && (index == PIO_PA5))
	{
		if (Sprite_GetGameOverFlag() == 1)
			Sprite_ClearGameOverFlag();
		else
			Sprite_SetPlayerMissileLaunchFlag();

		pio_get(PIOA, PIO_TYPE_PIO_INPUT, PIO_PA5);	//clear interrupt
	}
}

