////////////////////////////////////////////////////
/*
Programming the Atmel ATtimy85 processor.
Dana Olcott
4/16/18

Simple ADC Meter Project

The purpose of this project is to create a simple
meter that outputs the ADC level on ADC2
(PB4, Pin 3) using 4 leds.  The ADC source is
from a microphone connected to a OPA344 op-amp.
Background is about 1.6v +/-, or about half the 
supply voltage.  ADC readings are stored as the
difference from average level and current reading
and stored in a buffer.  The max value of the buffer
is polled in main loop.  Max value is used to set 
the appropriate LEDs.

Hardware Config:
ADC2    PB4 - Pin 3

LEDs    PB0 - Pin 5
        PB1 - Pin 6
        PB2 - Pin 7
        PB3 - Pin 2


*/
//
////////////////////////////////////////////
//
//
#include <avr/io.h>         //macros
#include <avr/interrupt.h>


#define BIT0            (1u << 0)
#define BIT1            (1u << 1)
#define BIT2            (1u << 2)
#define BIT3            (1u << 3)
#define BIT4            (1u << 4)
#define BIT5            (1u << 5)
#define BIT6            (1u << 6)
#define BIT7            (1u << 7)


//check datasheet for this, compare reg listed
//in datasheet with that listed in io.h files.
#define IO_OFFSET       0x20

//EXT interrupts
#define SREG_R          (*((volatile unsigned char*)(0x3F + IO_OFFSET)))
#define _SEI            (SREG_R |= BIT7)
#define _CEI            (SREG_R &=~ BIT7)

#define GIMSK_R         (*((volatile unsigned char*)(0x3B + IO_OFFSET)))
#define PCMSK_R         (*((volatile unsigned char*)(0x15 + IO_OFFSET)))
#define GIFR_R          (*((volatile unsigned char*)(0x3A + IO_OFFSET)))

//IO
#define PORTB_DATA_R    (*((volatile unsigned char*)(0x18 + IO_OFFSET)))
#define PORTB_DIR_R     (*((volatile unsigned char*)(0x17 + IO_OFFSET)))
#define PINB_R          (*((volatile unsigned char*)(0x16 + IO_OFFSET)))

//timer 0
#define OCCR0A_R         (*((volatile unsigned char*)(0x29 + IO_OFFSET)))
#define TCCR0A_R         (*((volatile unsigned char*)(0x2A + IO_OFFSET)))
#define TCCR0B_R         (*((volatile unsigned char*)(0x33 + IO_OFFSET))) 
#define TIMSK_R          (*((volatile unsigned char*)(0x39 + IO_OFFSET)))
#define TCNT0_R          (*((volatile unsigned char*)(0x32 + IO_OFFSET)))
#define TIFR_R           (*((volatile unsigned char*)(0x38 + IO_OFFSET)))

//ADC
#define ADMUX_R          (*((volatile unsigned char*)(0x07 + IO_OFFSET)))
#define ADCSRA_R         (*((volatile unsigned char*)(0x06 + IO_OFFSET)))
#define ADCH_R           (*((volatile unsigned char*)(0x05 + IO_OFFSET)))       //bits 01 - top 2 in 10bit
#define ADCL_R           (*((volatile unsigned char*)(0x04 + IO_OFFSET)))       //bits 0-7
#define ADCSRB_R         (*((volatile unsigned char*)(0x03 + IO_OFFSET)))


//EEPROM
#define EEARH_R           (*((volatile unsigned char*)(0x1F + IO_OFFSET)))       //MSB, bit 0
#define EEARL_R           (*((volatile unsigned char*)(0x1E + IO_OFFSET)))       //EE addr low
#define EEDR_R           (*((volatile unsigned char*)(0x1D + IO_OFFSET)))       //data register
#define EECR_R           (*((volatile unsigned char*)(0x1C + IO_OFFSET)))       //EE addr low

//EEPROM address definitions
#define EEPROM_REF_ADC_LOW     ((uint8_t)0x10)
#define EEPROM_REF_ADC_HIGH     ((uint8_t)0x11)

//temp defines
#define DEFAULT_REF_TEMP        ((unsigned int)70)

//FIFO size
#define FIFO_SIZE               64

#define ADC_LIMIT0              12
#define ADC_LIMIT1              20
#define ADC_LIMIT2              35
#define ADC_LIMIT3              80


///////////////////////////////////////////////////
//Delay items
void Delay(volatile unsigned int val);
void GPIO_init(void);               //GPIO - leds
void ADC2_init(void);               //ADC1 on PB2 - Pin 7
unsigned int ADC2_read(void);
void SortArray(unsigned int *data, unsigned int size);
unsigned int fifo_add(unsigned int value);
unsigned int fifo_clear(void);
unsigned int fifo_getAverage(void);
unsigned int fifo_getMax(void);
unsigned int fifo_getMin(void);


void ADC_SetLeds(unsigned int reading);
void Timer0_init(void);

//variables
volatile unsigned int fifo[FIFO_SIZE] = {0x00};


///////////////////////////////
//Timer0 Overflow Interrupt ISR
//Read the ADC2 value and add to 
//fifo.  Compares against the current
//average of the fifo to always store
//that way, only the noise above the
//current background is stored.
//allow only 50? from average
ISR(TIMER0_OVF_vect)
{
    unsigned int read = ADC2_read();
    
    //normalize to average level - 512
    if (read < 512)
        read = 512 - read;
    else
        read = read - 512;

    //remove readings that are likely from
    //noise due to toggling leds amplified.
    //high spike.
    if (read > (4 * ADC_LIMIT3))
        read = 0;

    //add the new reading
    fifo_add(read);

    TIFR_R |= BIT1;     //clear interrupt
}




////////////////////////////////////////////////
//Main loop
//
int main()
{
    
    ///////////////////////////////////////////
    //initialize peripherals
    GPIO_init();        //init pb0-pb3
    ADC2_init();        //configure ADC 
    Timer0_init();      //configure timer

    fifo_clear();

    ///////////////////////////////////////////////////
    //Run the state machine
    while(1)
    {
        //fifo - compute the max
        unsigned int max = fifo_getMax();

        //only set the leds if it's not a spike
        if (max < (4 * ADC_LIMIT3))
            ADC_SetLeds(max);

        Delay(4500);

    }

	return 0;
}




/////////////////////////////////////////////
//Delay - gTimeTick incremented in timer isr
void Delay(volatile unsigned int val)
{
    volatile unsigned int temp = val;
    while (temp > 0)
        temp--;
}




////////////////////////////////////////////
//GPIO_init
//Configure LEDs as output
//LEDs on PB0 - PB3

void GPIO_init(void)
{
    PORTB_DIR_R |= BIT0;
    PORTB_DIR_R |= BIT1;
    PORTB_DIR_R |= BIT2;
    PORTB_DIR_R |= BIT3;

    //set initial value - low
    PORTB_DATA_R &=~ BIT0;
    PORTB_DATA_R &=~ BIT1;
    PORTB_DATA_R &=~ BIT2;
    PORTB_DATA_R &=~ BIT3;
}





////////////////////////////////////////////////////////
//ADC2 Configuration
//Configure ADC2 to run on PB4 (Pin 3)
//
//References: Page 122 - 134 of the datasheet
//
//General Process:
//Set up the reference voltage - use Vdd - ADMUX - Bits 2-0
//Enable the ADC by setting the ADC enable bit - ADCSRA, ADEN
//Data is available in registers ADCH and ADCL
//Left justify by using the ADLAR bit in ADMUX
//
//ADCSRA - Control and status register A
//bit 7 - ADEN - enable adc, write to 1 to enable adc
//bit 6 - ADSC - start the conversion - write 1 to start. 
//               poll it till it goes low indicating complete.
//bit 5 - ADATE - ADC auto trigger enable.  leave this 0
//bit 4 - ADIF - interrupt flag, set when conversion is complete.  
//bit 3 - ADIE - interrupt enable - set to 0
//bits 0-2 - prescaler - set based on the CPU speed, for 1mhz, use prescale 8
//
void ADC2_init(void)
{
    //ADMUX_R register - 
    //Configure as Vdd ref - bits 76 = 00, bit 4 - X
    //left justify - ADLAR = 1, 
    //ADC2 single ended input - bits 3-0 = 0010, 0x02
    //0010 0010
    ADMUX_R = 0x22;

    //prescale bits - use prescale = 8
    ADCSRA = ((0 << ADPS2) | (1 << ADPS1) | (1 << ADPS0));  //prescale 8

    //enable the ADC
    ADCSRA |= (1 << ADEN);

}


///////////////////////////////////////////////////////////
//ADC2_read
//Read data from ADC2.  Process consists of:
//Set bit 6 high in ADCSRA_R to start conversion
//Poll bit 6 till it goes low indicating complete.
//Read the results. in data high and low registers.
//Note:
//Seems order these are read is important.  
//For left justify, reading data low then data high.
//Repeat this 5 times, sort, and take the average.
//
unsigned int ADC2_read(void)
{
    unsigned int result = 0x00;
    unsigned int adc[5] = {0x00};
    unsigned char i = 0;
    unsigned int temp = 0x00;

    for (i = 0 ; i < 5 ; i++)
    {
        //write bit 6 high to start the conversion
        ADCSRA_R |= BIT6;

        //wait - poll BIT6 - clear when conversion is complete
        while (ADCSRA_R & BIT6);

        //read the result in ADCL then ADCH
        //it seems the order is important and
        //depends on if the data is left or 
        //right justified. This is for left justified
        
        result = (ADCL_R & 0x03);
        temp = (ADCH_R & 0xFF) << 2;
        result |= temp;

        adc[i] = result;
    }

    //sort the results - bubble sort
    SortArray(adc, 5);

    //average of middle 3
    result = adc[1] + adc[2] + adc[3];
    result = result / 3;

    return result;
}


////////////////////////////////////////
//Set the leds based on the adc reading
//
void ADC_SetLeds(unsigned int reading)
{
   
    if (reading < ADC_LIMIT0)
    {
        PORTB_DATA_R &=~ BIT0;
        PORTB_DATA_R &=~ BIT1;
        PORTB_DATA_R &=~ BIT2;
        PORTB_DATA_R &=~ BIT3;        
    }

    else if ((reading >= ADC_LIMIT0) && (reading < ADC_LIMIT1))
    {
        PORTB_DATA_R |= BIT0;
        PORTB_DATA_R &=~ BIT1;
        PORTB_DATA_R &=~ BIT2;
        PORTB_DATA_R &=~ BIT3;        
    }
    else if ((reading >= ADC_LIMIT1) && (reading < ADC_LIMIT2))
    {
        PORTB_DATA_R |= BIT0;
        PORTB_DATA_R |= BIT1;
        PORTB_DATA_R &=~ BIT2;
        PORTB_DATA_R &=~ BIT3;        
    }
    else if ((reading >= ADC_LIMIT2) && (reading < ADC_LIMIT3))
    {
        PORTB_DATA_R |= BIT0;
        PORTB_DATA_R |= BIT1;
        PORTB_DATA_R |= BIT2;
        PORTB_DATA_R &=~ BIT3;        
    }

    //saturated.... flash
    else if (reading > ADC_LIMIT3)
    {
        PORTB_DATA_R |= BIT0;
        PORTB_DATA_R |= BIT1;
        PORTB_DATA_R |= BIT2;
        PORTB_DATA_R |= BIT3;
    }

}











///////////////////////////////////////////
//Sort array of unsigned ints from lowest
//to highest using bubble sort
void SortArray(unsigned int *data, unsigned int size)
{
    unsigned int i, j, val;

    for (i = 0 ; i < size ; i++)
    {
        for (j = i+ 1; j < size ; j++)
        {
            if(data[i] > data[j])
            {
                val = data[i];
                data[i] = data[j];
                data[j] = val;
            }
        }
    }
}




/////////////////////////////////////////
//clear the fifo
unsigned int fifo_clear(void)
{
    unsigned int i = 0;
    for (i = 0 ; i < FIFO_SIZE ; i++)
        fifo[i] = 0x00;

    return 0;
}



unsigned int fifo_getAverage(void)
{
    unsigned long sum = 0x00;
    unsigned int i, ave = 0x00;

    for (i = 0 ; i < FIFO_SIZE ; i++)
        sum += fifo[i];

    ave = sum / FIFO_SIZE;

    return ave;
}

unsigned int fifo_getMax(void)
{
    unsigned int i, max = 0x00;

    for (i = 0 ; i < FIFO_SIZE ; i++)
    {
        if (fifo[i] >= max)
            max = fifo[i];
    }

    return max;
}

unsigned int fifo_getMin(void)
{
    unsigned int i, min = 0xFFFF;

    for (i = 0 ; i < FIFO_SIZE ; i++)
    {
        if (fifo[i] <= min)
            min = fifo[i];
    }

    return min;
}




/////////////////////////////////////////
//add element to the fifo, shift all elements
//compute and return the average
//element added to the front - lowest address
unsigned int fifo_add(unsigned int value)
{
    unsigned long sum = 0x00;
    unsigned int i, ave = 0x00;

    for (i = 0 ; i < FIFO_SIZE - 1 ; i++)
    {
        fifo[FIFO_SIZE - 1 - i] = fifo[FIFO_SIZE - 2 - i];
    }

    fifo[0] = value;

    for (i = 0 ; i < FIFO_SIZE ; i++)
        sum += fifo[i];

    ave = sum / FIFO_SIZE;

    return ave;
    
}





//////////////////////////////////////////
//Configure Timer0 with Overflow Interrupt
//configure timer to run at prescale = 8,
//overflow interrupt enable.
//
//no presceale = 4khz
//prescale = 8, 500hz
void Timer0_init(void)
{
  
    //Timer Control - 2 registers:
    //TCCR0A - output compare modes
    //Section 11.9.2
    //bits 7-4 - no output compare
    //no waveform gen - 00
    TCCR0A_R = 0x00;

    //TCCR0B - prescale - 
    TCCR0B_R |= BIT1;

    //    000 - no clock - timer disabled
    //    001 - no prescale
    //    010 - clk/8
    //    011 - clk/64
    //    100 - clk/256
    //    101 - clk/1024

    //TIMSK - int mask reg - bit 1 overflow interrupt en.
    TIMSK_R |= BIT1;
    
    //clear interrupt
    //TIFR - overflow flag - TOV0 - bit 1 
    TIFR_R |= BIT1;

    //enable global interrupts
    _SEI;

}












