////////////////////////////////////////////////////
/*
Baremetal Programming on the Atmel
ATMega328P (Arduino) processor.
Dana Olcott
12/15/17

A simple program that initializes spi on
pins 10-13 (PB2 - PB5) and pin 8 (PB0).

SPI interface is used to drive 128x64
Electronic Assembly graphic display.
PN# EA DOGL128x-6

The demo simply clears the screen with
0x00, 0xFF, and writes text at various
spacing.

Delay function from timer0 that
interrupts at 1khz.

Extra Pins:
PB0 (Pin 8) - Generic LED Pin
PD7 (Pin 7) - LCD Reset
PD6 (Pin 6) - LCD CMD/Data


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
#include "lcd_driver.h"

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
    spi_init();			//init spi
    spi_setSpeed(SPI_SPEED_1_MHZ);
    LCD_init();
    LCD_on();
    LCD_clear(0x00);


    while(1)
    {
		PORTB_DATA_R ^= BIT0;
	    LCD_clear(0x00);
        Delay(500);

		PORTB_DATA_R ^= BIT0;
	    LCD_clear(0xFF);
        Delay(500);

		PORTB_DATA_R ^= BIT0;
	    LCD_clear(0x00);
	    LCD_DrawString(1, 0, "Hello Line1");
	    LCD_DrawString(2, 1, "Hello Line2");
	    LCD_DrawString(3, 2, "Hello Line3");
	    LCD_DrawString(4, 3, "Hello Line4");
	    LCD_DrawString(5, 4, "Hello Line5");
	    LCD_DrawString(6, 5, "Hello Line6");

        Delay(4000);

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
   PORTB_DIR_R |= BIT1; 	//pin 9 as output

   //lcd cmd and reset pin
   PORTD_DIR_R |= BIT6;		//pin 6 - pd6
   PORTD_DIR_R |= BIT7;		//pin 7 - pd7

   //set values
   PORTB_DATA_R &=~ BIT0;	//clear pin 8

   PORTD_DATA_R &=~ BIT6;	//cmd pin
   PORTD_DATA_R &=~ BIT7;	//reset pin

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



