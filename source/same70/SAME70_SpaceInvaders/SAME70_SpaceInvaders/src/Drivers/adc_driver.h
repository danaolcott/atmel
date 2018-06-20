/*
 * adc_driver.h
 *
 * Created: 6/6/2018 1:12:12 AM
 *  Author: danao
 */ 


#ifndef ADC_DRIVER_H_
#define ADC_DRIVER_H_


#include "asf.h"
#include "conf_board.h"
#include "conf_clock.h"

#include "afec.h"			//timer
#include "pindefs.h"		//conversion from D# to chip pin#


void ADC_Config(void);
uint32_t ADC_readTemp(void);

void ADC_ConfigAD6(void);
uint32_t ADC_readChannel6(void);

void ADC_ConfigAD0(void);
uint32_t ADC_readChannel0(void);

void ADC_ConfigAD8(void);
uint32_t ADC_readChannel8(void);

void ADC_Config_AFEC0(void);
void ADC_read_AFEC0(uint32_t* data, uint32_t length);

void ADC_sort(uint32_t arr[], uint32_t n);




#endif /* ADC_DRIVER_H_ */