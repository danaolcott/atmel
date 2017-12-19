////////////////////////////////////////////////////
/*
Baremetal Programming on the Atmel
ATMega328P (Arduino) processor.
Dana Olcott
12/15/17

Ex: A simple program that uses PB5 (Pin13 - led)
and PD2 (Pin2 - interrupt INT0), and a timer.

This builds on interrupt1 with adding a timer into
the routine.  The purpose is to add a systick counter
with a 1ms timebase.  This will be used for the delay
function so we can get rid of the libs.

Defines (see makefile)  __AVR_ATmega328P__
Inludes:  /usr/lib/avr/include


system("stty -F /dev/ttyUSB0 115200");




*/
///////////////////////////////////////////////

#include <avr/io.h>         //macros
#include <avr/interrupt.h>

#include <stdio.h>
#include <string.h>

#include "register.h"
#include "usart.h"



//////////////////////////////////////
//prototypes
void GPIO_init(void);
void Interrupt_init(void);
void Timer0_init(void);




void Waste_CPU(unsigned int temp);
void Dummy_Function(void);


/////////////////////////////
//Delay items
void Delay(volatile unsigned int val);
static volatile unsigned long gTimeTick = 0x00;




///////////////////////////////////////////
//ISR - Button
//
//waste some CPU time, check the button.
//if pressed, update the flash routine.
//The interrupt flag is self clearing by 
//calling this function.
//
ISR(INT0_vect)
{
    unsigned char val = 0x00;
    Waste_CPU(10000);

    //check button press after killing cpu time
    val = PIND_R & 0x04;
    if (!val)
    {
        //do something...
    }
}


///////////////////////////////
//Timer0 Overflow Interrupt ISR
//Configured to run at 1khz, it's
//pretty close, runs a little slow.
//
ISR(TIMER0_OVF_vect)
{
    gTimeTick++;        //used by Delay
    PINB_R |= 0x01;     //toggle PB0 (Pin 8)

    //clear interrupt - datasheet shows
    //this bit has to be set to run timer
    TIFR0 |= 0x01;
}



//////////////////////////////////////
//usart rx interrupt
//24.8.3 - using interrupt driven data
//requires one to read the udr to clear 
//the flag, or else it will continue
//to generate another interrupt
//
ISR(USART_RX_vect)
{
    unsigned char data = UDR0_R;

    Usart_isr(data);
}




///////////////////////////////////////
int main()
{
    int n, counter = 0;
    char output[50];
    GPIO_init();        //configure led and button
    Interrupt_init();   //falling edge trigger  
    Timer0_init();      //Timer0 Counter Overflow
    Usart_init(9600);   //configure usart

    while(1)
    {
        PORTB_DATA_R ^= 1u << 5;
        Delay(2000);

        Usart_sendString("Hello String This is a new String\r\n");
        n = sprintf(output, "Data to Send: %d\r\n", counter++);
        Usart_sendArray((unsigned char*)output, n);
    }

	return 0;
}



///////////////////////////////////////////
//GPIO_init
//Configure led and button.  Note, according
//to datasheet, pullup can source current,
//so might be a good idea to add series 
//resistor.  either way works and not blew
//up anything yet.
//
void GPIO_init(void)
{    
   //Pin 13 - PB5
   PORTB_DIR_R |= 1u << 5; //pin 5 as output
   PORTB_DATA_R &=~ 1u << 5;   //clear

   //pin8
   PORTB_DIR_R |= 1u << 0; //pin 0 as output
   PORTB_DATA_R &=~ 1u << 0;   //clear

   //Pin 2 as input - PD2
   PORTD_DIR_R &=~ 1u << 2; //pin 2 as input
   PORTD_DATA_R &=~ 1u << 2; //clear for no pull
}


/////////////////////////////////
//Button PD2, falling edge trigger
//
void Interrupt_init(void)
{

    //configure INT0 - EICRA 0x02 falling edge
    EICRA_R |= 1u << 1;
    EICRA_R &=~ 0x01;

    //interrupt mask - EIMSK - 0x3D - set bit 0
    EIMSK_R |= 0X01;

    //clear pending interrupt - EIFR - 0X3C - bit 0
    EIFR_R |= 0x01;

    //enable global interrupts
    sei();
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
    TCCR0A = 0x00;

    //TCCR0B - Control = 0x45 - bits 0-2 confiure the prescaler.  dont care for remaining
    TCCR0B = 0x03;
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
    TIFR0 |= 0x01;

    //enable global interrupts
    sei();

}



/////////////////////////////////////
void Waste_CPU(unsigned int temp)
{
    volatile unsigned int val = temp;
    while (val > 0)
    {
        val--;
        Dummy_Function();
    }
}


/////////////////////////////////////
void Dummy_Function(void)
{

}

////////////////////////////////////
//timeTick is increased in timer isr
void Delay(volatile unsigned int val)
{
    gTimeTick = 0x00;           //upcounter
    while (val > gTimeTick){};
}












