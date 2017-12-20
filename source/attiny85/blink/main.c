////////////////////////////////////////////////////
/*
Programming the Atmel ATtimy85 processor.
Dana Olcott
12/19/17

Ex: A simple program that toggles PB3 and PB4

ATTiny85 programmed using Arduino as ISP

A few notes:
use baud rate 19200

if writing to registers directly, be sure to 
use the offset to get into io space.  The datasheets
for the 328p and the attiny85 appear to differ how
registers are listed.  ie. 328p shows io reg that includes
the 0x20 offet, whereas attiny85 does not.

Digital IO - looks like we can get PB0-PB4 ok, PB5
as the reset pin seems not work.


Defines:
__AVR_ATtiny85___
F_CPU = 1000000

See Makefile


Inludes:  /usr/lib/avr/include
*/
///////////////////////////////////////////////

#include <avr/io.h>         //macros
#include <avr/interrupt.h>


#define IO_OFFSET       0x20

//IO
#define PORTB_DATA_R    (*((volatile unsigned char*)(0x18 | IO_OFFSET)))
#define PORTB_DIR_R     (*((volatile unsigned char*)(0x17 | IO_OFFSET)))
#define PINB_R          (*((volatile unsigned char*)(0x16 | IO_OFFSET)))

//EXT interrupts
#define SREG_R          (*((volatile unsigned char*)(0x3F | IO_OFFSET)))
#define PCMSK_R         (*((volatile unsigned char*)(0x15 | IO_OFFSET)))

//timer counter0
#define TCNT0_R         (*((volatile unsigned char*)(0x15 | IO_OFFSET)))



#define BIT0            (1u << 0)
#define BIT1            (1u << 1)
#define BIT2            (1u << 2)
#define BIT3            (1u << 3)
#define BIT4            (1u << 4)
#define BIT5            (1u << 5)
#define BIT6            (1u << 6)
#define BIT7            (1u << 7)


void Delay(unsigned int temp);
void GPIO_init(void);


int main()
{
    //init pins as output
    GPIO_init();

    while(1)
    {

        //Set Pins 0 to 13
        PORTB_DATA_R |= BIT0;
        PORTB_DATA_R |= BIT1;
        PORTB_DATA_R |= BIT2;
        PORTB_DATA_R |= BIT3;
        PORTB_DATA_R |= BIT4;

        Delay(10000);

        PORTB_DATA_R &=~ BIT0;
        PORTB_DATA_R &=~ BIT1;
        PORTB_DATA_R &=~ BIT2;
        PORTB_DATA_R &=~ BIT3;
        PORTB_DATA_R &=~ BIT4;

        Delay(10000);

    }

	return 0;
}



//////////////////////////////////////
//Delay - 
//Since no timer yet, the delay is pretty
//arbitrary
//
void Delay(unsigned int temp)
{
    volatile unsigned int delay = temp;
    while(delay > 0)
    {
        delay--;
    }      
}


//////////////////////////////////////
void GPIO_init(void)
{
    PORTB_DIR_R |= BIT0;
    PORTB_DIR_R |= BIT1;
    PORTB_DIR_R |= BIT2;
    PORTB_DIR_R |= BIT3;
    PORTB_DIR_R |= BIT4;

    //set initial value - low
    PORTB_DATA_R &=~ BIT0;
    PORTB_DATA_R &=~ BIT1;
    PORTB_DATA_R &=~ BIT2;
    PORTB_DATA_R &=~ BIT3;
    PORTB_DATA_R &=~ BIT4;
}












