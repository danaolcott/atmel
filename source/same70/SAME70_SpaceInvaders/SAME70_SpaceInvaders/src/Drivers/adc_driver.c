/*
 * adc_driver.c
 *
 * Created: 6/6/2018 1:11:48 AM
 *  Author: danao
 */ 

 /*
 ADC Pinout on the atmel board:
From datasheet:
Header J502

Label		Pin			Function
AD0			PD26		TD
AD1			PC31		AFE1_AD6
AD2			PD30		AFE0_AD0
AD3			PA19		AFE0_AD8
AD4			PC13		AFE1_AD1

NOTE: Don't use pin 6-8 they are not connected
to the controller until you install the 0ohm resistors.
see datasheet


Link to ADC - might be doing this wrong
https://community.atmel.com/forum/same70-afec-automatic-gain-offset-error-compensation


NOTE:
When using two channels on the same AFEXX, you can't 
initialize separately.  the second one will cancel the 
initialization of the first one.  Use the adc_sequence.... something.

Note:
Conflict when trying to read AD2 and tempsensor
I think it's because they are on the same AFE0
Channel 11 (temp sensor)??? and Channel 0 (AD2)

probably because it was configured for temp sensor
first, then the AD2 channel, both on the same AFE0

probably need to use the sequence configure.

Try to reproduce this using AFE0, AD2 and AD3
channels AD0 and AD8

Configure this next....
Pin AD3 - PA19, AFE0_AD8



*/

#include "asf.h"
#include "conf_board.h"
#include "conf_clock.h"

#include "afec.h"			//timer
#include "adc_driver.h"
#include "pindefs.h"		//conversion from D# to chip pin#



///////////////////////////////////////////////
//ADC Globals
volatile bool is_tempConversionDone = false;
volatile uint32_t g_adcTempRawData = 0;

volatile bool is_ADC_CH6_Done = false;
volatile uint32_t g_adc_channel6_rawData = 0x00;

volatile bool is_ADC_CH0_Done = false;
volatile uint32_t g_adc_channel0_rawData = 0x00;

volatile bool is_ADC_CH8_Done = false;
volatile uint32_t g_adc_channel8_rawData = 0x00;


//init ch0 and ch8 on afec0
volatile bool is_ADC_AFEC0_Done = false;
volatile uint32_t g_adc_afec_data[2] = {0x00, 0x00};



////////////////////////////////////////////////////
//Callback function for when the ADC conversion
//is complete.  Sets a flag for polling in main loop
//This callback is continuously called.  From the ADC
//temp example, appears to be called every second?
static void afec_temp_sensor_end_conversion(void)
{
	g_adcTempRawData = afec_channel_get_value(AFEC0, AFEC_TEMPERATURE_SENSOR);
	is_tempConversionDone = true;
}


/////////////////////////////////////////////////////
//Callback function for ADC6
static void ADC_Channel6_Callback(void)
{
	g_adc_channel6_rawData = afec_channel_get_value(AFEC1, AFEC_CHANNEL_6);		//read the data
	is_ADC_CH6_Done = true;														//set the flag
}


/////////////////////////////////////////////////////
//Callback function for ADC0
static void ADC_Channel0_Callback(void)
{
	g_adc_channel0_rawData = afec_channel_get_value(AFEC0, AFEC_CHANNEL_0);		//read the data
	is_ADC_CH0_Done = true;														//set the flag
}

/////////////////////////////////////////////////////
//Callback function for ADC8
static void ADC_Channel8_Callback(void)
{
	g_adc_channel8_rawData = afec_channel_get_value(AFEC0, AFEC_CHANNEL_8);		//read the data
	is_ADC_CH8_Done = true;														//set the flag
}


/////////////////////////////////////////////////////////
//Callback for AFEC0 - ch0 and ch8
//map both interrupts to this and test
//the source
//
//static inline uint32_t afec_get_latest_value(Afec *const afec)
//static inline uint32_t afec_get_latest_chan_num(Afec *const afec)

static void ADC_AFEC0_Callback(void)
{
	//get the last channel
	uint32_t lastChannel = afec_get_latest_chan_num(AFEC0);

	if (lastChannel == AFEC_CHANNEL_0)
		g_adc_afec_data[0] = afec_get_latest_value(AFEC0);

	else if (lastChannel == AFEC_CHANNEL_8)
		g_adc_afec_data[1] = afec_get_latest_value(AFEC0);

	
	//read the data
	g_adc_afec_data[0] = afec_channel_get_value(AFEC0, AFEC_CHANNEL_0);
	g_adc_afec_data[1] = afec_channel_get_value(AFEC0, AFEC_CHANNEL_8);

	is_ADC_AFEC0_Done = true;
}


/////////////////////////////////////////////
//Configures the temp sensor
//Uses AFEC0, so if anything else is on
//AFEC0, you can't use this function
void ADC_Config()
{
	struct afec_config afec_cfg;							//config struct
	struct afec_ch_config afec_ch_cfg;						//channel config struct
	struct afec_temp_sensor_config afec_temp_sensor_cfg;	//temp sensor config struct

	afec_enable(AFEC0);
	afec_get_config_defaults(&afec_cfg);
	afec_init(AFEC0, &afec_cfg);

//	afec_set_trigger(AFEC0, AFEC_TRIG_SW);			//about 1 second conversion time
	afec_set_trigger(AFEC0, AFEC_TRIG_FREERUN);		//2-6 uS conversion time

	afec_ch_get_config_defaults(&afec_ch_cfg);

	afec_ch_cfg.gain = AFEC_GAINVALUE_0;

	afec_ch_set_config(AFEC0, AFEC_TEMPERATURE_SENSOR, &afec_ch_cfg);

	//internal adc has offset 0x200, so apply offset here to cancel it out
	afec_channel_set_analog_offset(AFEC0, AFEC_TEMPERATURE_SENSOR, 0x200);

	afec_temp_sensor_get_config_defaults(&afec_temp_sensor_cfg);
	afec_temp_sensor_cfg.rctc = true;
	afec_temp_sensor_set_config(AFEC0, &afec_temp_sensor_cfg);

	//configure the callback function when conversion is complete:
	//afec_temp_sensor_end_conversion
	afec_set_callback(AFEC0, AFEC_INTERRUPT_EOC_11, afec_temp_sensor_end_conversion, 1);
}




//////////////////////////////////////////
//Configure ADC Channel6
//Datasheet: 
//Label			Pin			Channel
//AD1			PC31		AFE1_AD6
//
//Channel AD6 uses AFEC1.  Make sure to use AFEC1
//and not AFEC0 or else the callback won't get called.
//See datasheet / user guide for other channels on pins
//AD1, AD2... AD4
//
//Callback: ADC_Channel6_Callback()
//
//The following example results in continuous conversion
//of ADC Channel 6.
//
///////////////////////////////////////////////////////////
//Notes on Analog Offset
//For offset = 0xFFF and 1k resistor in series (AD1 to 0/3.3)
//
//Grounded connection = ground
//3.3v connection = about 1.7v, or reading = 2071
//
//For offset = 0x00 and 1k resistor in series
//3.3v connection: reading 4095
//0v connection: 2099
//
//For offset = 0x200 (example value):
//low - 80 to 85
//high - 4091 to 4095
//
//NOTES on setting the callback function
//set callback function for AFEC1, Channel 6 (Pin AD1 as labeled):
//NOTE: IRQ = AFEC_INTERRUPT_EOC_6
//Value 1 is the priority, not the IRQ number suggested by
//variable name in one of the API functions
//
////////////////////////////////////////////////////////
void ADC_ConfigAD6(void)
{
	struct afec_config afec_cfg;							//config struct
	struct afec_ch_config afec_ch_cfg;						//channel config struct

	//configure PC31 as input, default, enable clock
	pmc_enable_periph_clk(ID_PIOC);									//enable the clock	
	pio_set_input(PIOC, PIO_PC31, PIO_DEFAULT);						//input, default config
	
	afec_enable(AFEC1);												//enable AFEC1
	afec_get_config_defaults(&afec_cfg);							//populate the afec structure
	afec_init(AFEC1, &afec_cfg);									//init the AFEC1 peripheral
	afec_set_trigger(AFEC1, AFEC_TRIG_FREERUN);						//2-6 uS conversion time
	afec_ch_get_config_defaults(&afec_ch_cfg);						//populate the channel struct with default values
	afec_ch_cfg.gain = AFEC_GAINVALUE_0;							//set the gain - check the linearity
	afec_ch_set_config(AFEC1, AFEC_CHANNEL_6, &afec_ch_cfg);		//configure channel 6
	afec_channel_set_analog_offset(AFEC1, AFEC_CHANNEL_6, 0x200);	//set the offset - use 0x200 from example

	afec_set_callback(AFEC1, AFEC_INTERRUPT_EOC_6, ADC_Channel6_Callback, 1);

	afec_channel_enable(AFEC1, AFEC_CHANNEL_6);				//enable the channel

}









//////////////////////////////////////////
//Configure ADC Channel0
//Datasheet:
//Label			Pin			Channel
//AD2			PD30		AFE0_AD0
//
//Channel AD0 uses AFEC0, located on AD2
//Pin PD30.
//
//Callback: ADC_Channel0_Callback()
//
////////////////////////////////////////////////////////
void ADC_ConfigAD0(void)
{
	struct afec_config afec_cfg;							//config struct
	struct afec_ch_config afec_ch_cfg;						//channel config struct
	//struct afec_temp_sensor_config afec_temp_sensor_cfg;	//temp sensor config struct

	//configure PD30 as input, default, enable clock
	pmc_enable_periph_clk(ID_PIOD);							//enable the clock
	pio_set_input(PIOD, PIO_PD30, PIO_DEFAULT);				//input, default config
	
	afec_enable(AFEC0);										//enable AFEC0
	afec_get_config_defaults(&afec_cfg);					//populate the afec structure
	afec_init(AFEC0, &afec_cfg);							//init the AFEC0 peripheral
	afec_set_trigger(AFEC0, AFEC_TRIG_FREERUN);				//2-6 uS conversion time
	afec_ch_get_config_defaults(&afec_ch_cfg);				//populate the channel struct with default values
	afec_ch_cfg.gain = AFEC_GAINVALUE_0;					//set the gain - check the linearity
	afec_ch_set_config(AFEC0, AFEC_CHANNEL_0, &afec_ch_cfg);		//configure channel 0
	afec_channel_set_analog_offset(AFEC0, AFEC_CHANNEL_0, 0x200);	//set the offset - use 0x200 from example

	afec_set_callback(AFEC0, AFEC_INTERRUPT_EOC_0, ADC_Channel0_Callback, 1);
	afec_channel_enable(AFEC0, AFEC_CHANNEL_0);				//enable the channel
}





//////////////////////////////////////////
//Configure ADC Channel8
//Datasheet:
//Label			Pin			Channel
//AD3			PA19		AFE0_AD8
//
//Channel AD8 uses AFEC0, located on AD8
//Pin PA19.
//
//Callback: ADC_Channel8_Callback()
//
////////////////////////////////////////////////////////
void ADC_ConfigAD8(void)
{
	struct afec_config afec_cfg;							//config struct
	struct afec_ch_config afec_ch_cfg;						//channel config struct

	//configure PA19 as input, default, enable clock
	pmc_enable_periph_clk(ID_PIOA);							//enable the clock
	pio_set_input(PIOA, PIO_PA19, PIO_DEFAULT);				//input, default config
	
	afec_enable(AFEC0);										//enable AFEC0
	afec_get_config_defaults(&afec_cfg);					//populate the afec structure
	afec_init(AFEC0, &afec_cfg);							//init the AFEC0 peripheral
	afec_set_trigger(AFEC0, AFEC_TRIG_FREERUN);				//2-6 uS conversion time
	afec_ch_get_config_defaults(&afec_ch_cfg);				//populate the channel struct with default values
	afec_ch_cfg.gain = AFEC_GAINVALUE_0;					//set the gain - check the linearity
	afec_ch_set_config(AFEC0, AFEC_CHANNEL_8, &afec_ch_cfg);		//configure channel
	afec_channel_set_analog_offset(AFEC0, AFEC_CHANNEL_8, 0x200);	//set the offset - use 0x200 from example

	afec_set_callback(AFEC0, AFEC_INTERRUPT_EOC_8, ADC_Channel8_Callback, 1);
	afec_channel_enable(AFEC0, AFEC_CHANNEL_8);				//enable the channel
}


///////////////////////////////////////////////////////////
//Configure ADC Channels that reside on AFEC0
//Configure the following:
//AD2			PD30		AFE0_AD0
//AD3			PA19		AFE0_AD8
//
//Configuring one channel at a time and mapping to
//interrupt does not work as expected.  
//
//Try using the configure_sequence equation:
//
//void afec_configure_sequence(Afec *const afec,
//const enum afec_channel_num ch_list[], const uint8_t uc_num);
//
//Steps:
//Make a channel list
//pass the list as argument into configure sequence
//
//This Does not work, it keeps reading channel 0
//and not channel 8!!
//
void ADC_Config_AFEC0(void)
{

	struct afec_config afec_cfg;							//config struct
	struct afec_ch_config afec_ch_cfg;						//channel config struct

	//PD30
	pmc_enable_periph_clk(ID_PIOD);							//enable the clock
	pio_set_input(PIOD, PIO_PD30, PIO_DEFAULT);				//input, default config

	//PA19
	pmc_enable_periph_clk(ID_PIOA);							//enable the clock
	pio_set_input(PIOA, PIO_PA19, PIO_DEFAULT);				//input, default config

	afec_enable(AFEC0);										//enable AFEC0
	afec_get_config_defaults(&afec_cfg);					//populate the afec structure

	afec_cfg.useq = true;									//use sequence enable

	afec_init(AFEC0, &afec_cfg);							//init the AFEC0 peripheral
	afec_set_trigger(AFEC0, AFEC_TRIG_FREERUN);				//2-6 uS conversion time

	//all channels
	afec_ch_get_config_defaults(&afec_ch_cfg);				//populate the channel struct with default values
	afec_ch_cfg.gain = AFEC_GAINVALUE_0;					//set the gain - check the linearity

	//ch0
	afec_ch_set_config(AFEC0, AFEC_CHANNEL_0, &afec_ch_cfg);		//configure channel
	afec_channel_set_analog_offset(AFEC0, AFEC_CHANNEL_0, 0x200);	//set the offset - use 0x200 from example

	//ch8
	afec_ch_set_config(AFEC0, AFEC_CHANNEL_8, &afec_ch_cfg);		//configure channel
	afec_channel_set_analog_offset(AFEC0, AFEC_CHANNEL_8, 0x200);	//set the offset - use 0x200 from example

	//make a channel sequence
	const enum afec_channel_num ch_list[2] = {AFEC_CHANNEL_0, AFEC_CHANNEL_8};
	afec_configure_sequence(AFEC0, ch_list, 2);

	//interrupts - use common interrupt handler
	afec_set_callback(AFEC0, AFEC_INTERRUPT_DATA_READY, ADC_AFEC0_Callback, 1);

//	afec_set_callback(AFEC0, AFEC_INTERRUPT_EOC_0, ADC_AFEC0_Callback, 1);
//	afec_set_callback(AFEC0, AFEC_INTERRUPT_EOC_8, ADC_AFEC0_Callback, 1);

	//enable
	afec_channel_enable(AFEC0, AFEC_CHANNEL_0);				//enable the channel
	afec_channel_enable(AFEC0, AFEC_CHANNEL_8);				//enable the channel

}






//////////////////////////////////////////////////
//Read ADC Channel
//Assumes the ADC is configured.  Clear the conversion
//done flag, wait till set, return the value
//NOTE: using software trigger results in about 1 second
//per conversion.  using free run mode takes about 2-6 uS 
//per conversion
//Conversion is set to freerun mode
//read it 5 times, drop the high and low and return the average
//
uint32_t ADC_readTemp()
{
	uint32_t data[5] = {0x00, 0x00, 0x00, 0x00, 0x00};
	uint32_t average = 0x00;
	uint32_t sum = 0x00;

	for (int i = 0 ; i < 5 ; i++)
	{
		is_tempConversionDone = false;			//clear the flag - set in callback
		while(is_tempConversionDone == false);	//wait till cleared
		data[i] = g_adcTempRawData;				//record the data
	}

	//sort the data array
	ADC_sort(data, 5);

	//take the average of the middle 3
	sum = data[1] + data[2] + data[3];	
	average = sum / 3;

	return average;
}



////////////////////////////////////////////////////////
//Read Channel 6 on Pin AD1 5 times, sort,
//remove the high and low, and return the
//average value
//
uint32_t ADC_readChannel6()
{
	uint32_t data[5] = {0x00, 0x00, 0x00, 0x00, 0x00};
	uint32_t average = 0x00;
	uint32_t sum = 0x00;

	for (int i = 0 ; i < 5 ; i++)
	{
		is_ADC_CH6_Done = false;			//clear the flag - set in callback
		while(is_ADC_CH6_Done == false);	//wait till cleared
		data[i] = g_adc_channel6_rawData;
	}

	//sort the data array
	ADC_sort(data, 5);

	//take the average of the middle 3
	sum = data[1] + data[2] + data[3];	
	average = sum / 3;
		
	return average;
}





////////////////////////////////////////////////////////
//Read Channel 0 on Pin AD2 5 times, sort,
//remove the high and low, and return the
//average value
//
uint32_t ADC_readChannel0()
{
	uint32_t data[5] = {0x00, 0x00, 0x00, 0x00, 0x00};
	uint32_t average = 0x00;
	uint32_t sum = 0x00;

	for (int i = 0 ; i < 5 ; i++)
	{
		is_ADC_CH0_Done = false;			//clear the flag - set in callback
		while(is_ADC_CH0_Done == false);	//wait till cleared
		data[i] = g_adc_channel0_rawData;
	}

	//sort the data array
	ADC_sort(data, 5);

	//take the average of the middle 3
	sum = data[1] + data[2] + data[3];
	average = sum / 3;
	
	return average;
}


////////////////////////////////////////////////////////
//Read Channel 8 on Pin AD3 5 times, sort,
//remove the high and low, and return the
//average value
//
uint32_t ADC_readChannel8()
{
	uint32_t data[5] = {0x00, 0x00, 0x00, 0x00, 0x00};
	uint32_t average = 0x00;
	uint32_t sum = 0x00;

	for (int i = 0 ; i < 5 ; i++)
	{
		is_ADC_CH8_Done = false;			//clear the flag - set in callback
		while(is_ADC_CH8_Done == false);	//wait till cleared
		data[i] = g_adc_channel8_rawData;
	}

	//sort the data array
	ADC_sort(data, 5);

	//take the average of the middle 3
	sum = data[1] + data[2] + data[3];
	average = sum / 3;
	
	return average;
}



////////////////////////////////////////////////////////
//Read ADC data captured on AFEC0
//length = num channels, data is pointer to outside world
/*
use these equations:  
static inline uint32_t afec_get_latest_value(Afec *const afec)
static inline uint32_t afec_get_latest_chan_num(Afec *const afec)


*/
void ADC_read_AFEC0(uint32_t* data, uint32_t length)
{

	is_ADC_AFEC0_Done = false;			//clear the flag - set in callback
	while(is_ADC_AFEC0_Done == false);	//wait till cleared

	data[0] = g_adc_afec_data[0];
	data[1] = g_adc_afec_data[1];

}







//////////////////////////////////////////////////////
//sort array temp of size size
//
void ADC_sort(uint32_t arr[], uint32_t n)
{
	for (uint32_t i = 0; i < n; i++) 
	{
		for (uint32_t j = 0; j < n - i - 1; j++) 
		{
			if (arr[j] > arr[j + 1]) 
			{
				uint32_t temp = arr[j];
				arr[j] = arr[j + 1];
				arr[j + 1] = temp;
			}
		}
	}
}

