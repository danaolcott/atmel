////////////////////////////////////////////////////
/*
Programming the Atmel ATtimy85 processor.
Dana Olcott
12/19/17

Ex: A simple program that toggles PB3 and PB4

ATTiny85 programmed using Arduino as ISP

A few notes:
use baud rate 19200

If writing to registers directly, be sure to account
for the offset.  It appears the datasheets for the
m328p ans tiny differ, one includes the offset and 
other does not.
ie way, the offset is 0x20



Defines:
__AVR_ATtiny85___
F_CPU = 1000000

See Makefile


Inludes:  /usr/lib/avr/include
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
#define PCMSK_R         (*((volatile unsigned char*)(0x15 + IO_OFFSET)))
#define _SEI            (SREG_R |= BIT7)

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




/////////////////////////////
//Delay items
void Delay(volatile unsigned int val);
static volatile unsigned long gTimeTick = 0x00;

void GPIO_init(void);
void Timer0_init(void);
void Timer0_OCCA_init(void);


static unsigned long gCycleCounter = 0x00;



///////////////////////////////
//Timer0 Overflow Interrupt ISR
//Runs when init timer includes overflow interrupts
ISR(TIMER0_OVF_vect)
{
    gTimeTick++;        //used by Delay
}



///////////////////////////////////////
//Timer0 CCR0A
//Runs when init timer includes compare interrupts
ISR(TIMER0_COMPA_vect)
{
    gTimeTick++;        //used by Delay
    TCNT0_R = 0x00;     //reset the counter
}



//////////////////////////////////////////
//
int main()
{
    //init pins as output
    GPIO_init();

    //init one or the other
    Timer0_OCCA_init();
    //Timer0_init();


    while(1)
    {       
        PORTB_DATA_R ^= BIT3;

        if (!(gCycleCounter % 2))        
            PORTB_DATA_R ^= BIT4;

        Delay(100);
        gCycleCounter++;

    }

	return 0;
}



////////////////////////////////////
//timeTick is increased in timer isr
void Delay(volatile unsigned int val)
{
    gTimeTick = 0x00;           //upcounter
    while (val > gTimeTick){};
}




//////////////////////////////////////
void GPIO_init(void)
{
    PORTB_DIR_R |= BIT3;
    PORTB_DIR_R |= BIT4;

    //set initial value - low
    PORTB_DATA_R &=~ BIT3;
    PORTB_DATA_R &=~ BIT4;
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
    TCCR0B_R |= BIT0;

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





//////////////////////////////////
//Compare Capture
//Generates an interrupt at 1khz
//
void Timer0_OCCA_init(void)
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

    //trigger ~2x and adjust as needed
    OCCR0A_R = 120;

    //TIMSK - int mask reg - bit 4 - compare capture A
    TIMSK_R |= BIT4;
    
    //clear interrupt
    //TIFR - overflow flag - OCF0A - bit 4
    TIFR_R |= BIT4;

    //enable global interrupts
    _SEI;

}











