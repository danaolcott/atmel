////////////////////////////////////////////////////
/*
Baremetal Programming on the Atmel
ATMega328P (Arduino) processor.
Dana Olcott
12/15/17
A simple program that initializes spi on
pins 10-13 (PB2 - PB5) and pin 8 (PB0).

uses timer0 to interrupt at 1khz for use
with delay.


Defines (see makefile)  __AVR_ATmega328P__
Inludes:  /usr/lib/avr/include


system("stty -F /dev/ttyUSB0 115200");




*/
///////////////////////////////////////////////

#include <avr/interrupt.h>
#include <avr/io.h>         //macros
#include <stdio.h>
#include <string.h>

#include "register.h"
#include "spi.h"

//////////////////////////////////////
//prototypes
void GPIO_init(void);
void Timer0_init(void);


/////////////////////////////
//Delay items
void Delay(unsigned long val);
volatile unsigned long gTimeTick = 0x00;





///////////////////////////////
//Timer0 Overflow Interrupt ISR
//Configured to run at 1khz, it's
//pretty close, runs a little slow.
//
ISR(TIMER0_OVF_vect)
{
    gTimeTick++;        //used by Delay

    //clear interrupt - datasheet shows
    //this bit has to be set to run timer
    TIFR0_R |= 0x01;
}


///////////////////////////////////////
int main()
{
    GPIO_init();        //configure led and button
    Timer0_init();      //Timer0 Counter Overflow
    spi_init();

    while(1)
    {

        PORTB_DATA_R ^= BIT0;

        spi_write(0xAA);

        Delay(50);
    }

	return 0;
}



///////////////////////////////////////////
//GPIO_init
//Configure pin 8, PB0 as output
//
void GPIO_init(void)
{    
   //Pin 8 - PB0
   PORTB_DIR_R |= BIT0; 	//pin 8 as output
   PORTB_DATA_R &=~ BIT0;	//clear pin 8

}



//////////////////////////////////////////
//Configure Timer0 with Overflow Interrupt
//
void Timer0_init(void)
{

    //enable timer TC0 is enabled with writing 1 to PRR.PRTIM0 -> reg 0x64, bit 5 - set
    //PRR_R |= 1u << 5;
    //Note - don't do this...
    //i saw in the datasheet where this bit has to be set but it kills it
   

    //Timer Control - 2 registers:
    //TCCR0A - Control = 0x44 - write 0x00 to disable all pin outputs on compare.
    TCCR0A_R = 0x00;

    //TCCR0B - Control = 0x45 - bits 0-2 confiure the prescaler.  dont care for remaining
    TCCR0B_R = 0x03;
    //    000 - no clock - timer disabled
    //    001 - no prescale
    //    010 - clk/8
    //    011 - clk/64
    //    100 - clk/256
    //    101 - clk/1024

    //TIMSK0 - int mask reg = 0x6E - bit 0 is overflow interrupt => write 0x01
    TIMSK0_R |= 0x01;
    
    //clear interrupt
    //TIFR0 - interrupt flag.  0x15    TOV - write 1 to clear interrupt
    TIFR0_R |= 0x01;

    //enable global interrupts
    __SEI;
}



////////////////////////////////////
//timeTick is increased in timer isr
void Delay(unsigned long val)
{
	volatile unsigned long t = val;
    gTimeTick = 0x00;           //upcounter
    while (t > gTimeTick){};
}











