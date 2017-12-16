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
	DDRB |= _BV(DDB4);      //PB4 - pin 12 output
	DDRB |= _BV(DDB3);      //PB3 - pin 11 output
	DDRB |= _BV(DDB2);      //PB2 - pin 10 output
	DDRB |= _BV(DDB1);      //PB1 - pin 9 output
	DDRB |= _BV(DDB0);      //PB0 - pin 8 output

    //port d - pins 7 - 0
	DDRD |= _BV(DDD7);      //PD7 - pin 7 output
	DDRD |= _BV(DDD6);      //PD6 - pin 6 output
	DDRD |= _BV(DDD5);      //PD5 - pin 5 output
	DDRD |= _BV(DDD4);      //PD4 - pin 4 output
	DDRD |= _BV(DDD3);      //PD3 - pin 3 output
	DDRD |= _BV(DDD2);      //PD2 - pin 2 output
	DDRD |= _BV(DDD1);      //PD1 - pin 1 output
	DDRD |= _BV(DDD0);      //PD0 - pin 0 output


    while(1)
    {

        //Set Pins 0 to 13
        PORTB |= _BV(PORTB5);       //pin 13
        PORTB |= _BV(PORTB4);       //pin 12
        PORTB |= _BV(PORTB3);       //pin 11
        PORTB |= _BV(PORTB2);       //pin 10
        PORTB |= _BV(PORTB1);       //pin 9
        PORTB |= _BV(PORTB0);       //pin 8

        PORTD |= _BV(PORTD7);       //pin 7
        PORTD |= _BV(PORTD6);       //pin 6
        PORTD |= _BV(PORTD5);       //pin 5
        PORTD |= _BV(PORTD4);       //pin 4
        PORTD |= _BV(PORTD3);       //pin 3
        PORTD |= _BV(PORTD2);       //pin 2
        PORTD |= _BV(PORTD1);       //pin 1
        PORTD |= _BV(PORTD0);       //pin 0

        _delay_ms(DELAY_MS);

        //Clear Pins 0 to 13
        PORTB &=~ _BV(PORTB5);       //pin 13
        PORTB &=~ _BV(PORTB4);       //pin 12
        PORTB &=~ _BV(PORTB3);       //pin 11
        PORTB &=~ _BV(PORTB2);       //pin 10
        PORTB &=~ _BV(PORTB1);       //pin 9
        PORTB &=~ _BV(PORTB0);       //pin 8

        PORTD &=~ _BV(PORTD7);       //pin 7
        PORTD &=~ _BV(PORTD6);       //pin 6
        PORTD &=~ _BV(PORTD5);       //pin 5
        PORTD &=~ _BV(PORTD4);       //pin 4
        PORTD &=~ _BV(PORTD3);       //pin 3
        PORTD &=~ _BV(PORTD2);       //pin 2
        PORTD &=~ _BV(PORTD1);       //pin 1
        PORTD &=~ _BV(PORTD0);       //pin 0

        _delay_ms(DELAY_MS);
    }

	return 0;
}

