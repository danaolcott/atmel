/*
 * dac_driver.h
 *
 * Created: 6/4/2018 1:33:29 PM
 *  Author: danao
 */ 


#ifndef DAC_DRIVER_H_
#define DAC_DRIVER_H_

#include "asf.h"
#include "conf_board.h"
#include "conf_clock.h"

/////////////////////////////////////////
//Defines for DAC output
#define DACC_CHANNEL_0        0 // (PB13)
#define DACC_CHANNEL_1        1 // (PD00)

#define DACC_BASE           DACC		//DAC base ID
#define DACC_ID             ID_DACC

/** Analog control value */
#define DACC_ANALOG_CONTROL (DACC_ACR_IBCTLCH0(0x02) | DACC_ACR_IBCTLCH1(0x02))


typedef enum
{
	DAC_Channel_0 = 0,
	DAC_Channel_1 = 1,

}DAC_Channel_t;


void DAC_Config(void);
void DAC_write(DAC_Channel_t ch, uint16_t data);





#endif /* DAC_DRIVER_H_ */