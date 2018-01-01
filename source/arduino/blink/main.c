////////////////////////////////////////////////////
/*
Baremetal Programming on the Atmel
ATMega328P (Arduino) processor.
Dana Olcott
12/15/17

Ex: A simple program that flashes Pins 0 to 13.

Pin Descriptions:
Pins 0 to 13 reside on two ports: PORTD and PORTB

Output Registers:
Pins 0 to 7 - PORTD: PORTD0 - PORTD7
Pins 8 to 13 - PORTB: PORTB0 - PORTB5

Direction Register (set bit to enable output):
Pins 0 to 7 - DDRD _BV(DDD0) - DRD _BV(DDD7)
Pins 8 to 13 - DDRB _BV(DDB0) - DRB _BV(DDB5)

Defines (see makefile)  __AVR_ATmega328P__
Inludes:  /usr/lib/avr/include
*/
///////////////////////////////////////////////

#include <avr/io.h>         //macros
#include <util/delay.h>     //_delay_ms function

#define DELAY_MS       100


int main()
{
	//register init - DDRB - PortB, DDB5 - Pin5
    //Pins 13 to 8
	DDRB |= _BV(DDB5);      //PB5 - pin 13 output
	


    while(1)
    {

        //Set Pins 0 to 13
        PORTB |= _BV(PORTB5);       //pin 13
      

        _delay_ms(DELAY_MS);

        //Clear Pins 0 to 13
        PORTB &=~ _BV(PORTB5);       //pin 13
        
        _delay_ms(DELAY_MS);
    }

	return 0;
}

