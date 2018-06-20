/*
 * dac_driver.c
 *
 * Created: 6/4/2018 1:33:46 PM
 *  Author: danao
 */ 

#include "asf.h"
#include "conf_board.h"
#include "conf_clock.h"

#include "dacc.h"			//timer
#include "dac_driver.h"
#include "pindefs.h"		//conversion from D# to chip pin#


//////////////////////////////////////////////////////
//DAC_Config
//Configure DAC output on Channels 0 and 1
//on PB13 and PD0
//
//NOTE: Can't get Channel 1 on PD0 working
//Channel 0 works fine.
//changing from half word to word also did not fix
//also, updating the max channel did not fix it (see dacc.h)
//# define MAX_CH_NB        2				//channel 1 did not work
//
void DAC_Config()
{
	sysclk_enable_peripheral_clock(DACC_ID);
	dacc_reset(DACC_BASE);										//reset the DAC registers
	dacc_set_writeprotect(DACC_BASE, 0);						//0 = disable, 1 = enable
	dacc_set_transfer_mode(DACC_BASE, 0);						//half word transfer
	dacc_enable_channel(DACC_BASE, DACC_CHANNEL_0);				//enable output channel 0 - PB13
	dacc_enable_channel(DACC_BASE, DACC_CHANNEL_1);				//enable output channel 0 - PD0
	dacc_set_analog_control(DACC_BASE, DACC_ANALOG_CONTROL);	//setup analog current
}


//////////////////////////////////////////////////////
//DAC Write to Channels 0 or 1.
//Assumes that both channels are already configured
//Writing to channel 1 does not work.  Tried multiple
//approaches and it's not working.  just toggles at 
//high freq, not peak to peak, noise.  Something else
//is probably configured.  toggle 1v pk-pk at about 200hz.
//PWM output?....
//
void DAC_write(DAC_Channel_t channel, uint16_t data)
{
	uint32_t status = dacc_get_interrupt_status(DACC_BASE);

	if (channel == DAC_Channel_0)
	{
		if ((status & DACC_ISR_TXRDY0) == DACC_ISR_TXRDY0)
			dacc_write_conversion_data(DACC_BASE, (data & 0xFFF), DACC_CHANNEL_0);
	}

	if (channel == DAC_Channel_1)
	{
		if ((status & DACC_ISR_TXRDY1) == DACC_ISR_TXRDY1)
			dacc_write_conversion_data(DACC_BASE, (data & 0xFFF), DACC_CHANNEL_1);
	}
}

