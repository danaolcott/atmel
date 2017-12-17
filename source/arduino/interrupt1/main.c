////////////////////////////////////////////////////
/*
Baremetal Programming on the Atmel
ATMega328P (Arduino) processor.
Dana Olcott
12/15/17

Ex: A simple program that uses PB5 (Pin13 - led)
and PD2 (Pin2 - interrupt INT0).  Press the user
button to change the flash state.  There are 4 flash
states.  Pressing the user button toggles the flash
state.  The button is configured as input, pullup
enabled, falling edge trigger.

Defines (see makefile)  __AVR_ATmega328P__
Inludes:  /usr/lib/avr/include
*/
///////////////////////////////////////////////

#include <avr/io.h>         //macros
#include <util/delay.h>     //_delay_ms function
#include <avr/interrupt.h>


//////////////////////////////////////////
//register defines
#define PORTB_DATA_R    (*((volatile unsigned char*)0x25))
#define PORTB_DIR_R     (*((volatile unsigned char*)0x24))
#define PINB_R          (*((volatile unsigned char*)0x23))

#define PORTD_DATA_R    (*((volatile unsigned char*)0x2B))
#define PORTD_DIR_R     (*((volatile unsigned char*)0x2A))
#define PIND_R          (*((volatile unsigned char*)0x29))

#define EICRA_R         (*((volatile unsigned char*)0x69))
#define EIMSK_R         (*((volatile unsigned char*)0x3D))
#define EIFR_R          (*((volatile unsigned char*)0x3C))
#define SREG_R          (*((volatile unsigned char*)0x5F))


//////////////////////////////////////
//prototypes
void GPIO_init(void);
void Interrupt_init(void);
void Waste_CPU(unsigned int temp);
void Dummy_Function(void);


void Flash0(void);
void Flash1(void);
void Flash2(void);
void Flash3(void);

unsigned char flashRoutine = 0x00;     //0 - 3

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
        if (flashRoutine < 3)
            flashRoutine++;
        else
            flashRoutine = 0;
    }
}



///////////////////////////////////////
int main()
{
    GPIO_init();            //configure led and button
    Interrupt_init();       //falling edge trigger

    while(1)
    {
        switch(flashRoutine)
        {
            case 0: Flash0();   break;
            case 1: Flash1();   break;
            case 2: Flash2();   break;
            case 3: Flash3();   break;
        }
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
//Button PD2, falling edge
//clear pending by writing high
//sei() enables global interrupts.
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

    //enable global interrupts - set the I-bit in SREG
    SREG_R |= 1u << 7;

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

/////////////////////////////////////
//Flash 50ms
void Flash0(void)
{
    PORTB_DATA_R |= 1u << 5;
    _delay_ms(50);
    PORTB_DATA_R &=~ 1u << 5;
    _delay_ms(50);
}

//Flash 100ms
void Flash1(void)
{
    PORTB_DATA_R |= 1u << 5;
    _delay_ms(100);
    PORTB_DATA_R &=~ 1u << 5;
    _delay_ms(100);

}

//Flash 200ms
void Flash2(void)
{
    PORTB_DATA_R |= 1u << 5;
    _delay_ms(200);
    PORTB_DATA_R &=~ 1u << 5;
    _delay_ms(200);

}

//Flash 500ms
void Flash3(void)
{
    PORTB_DATA_R |= 1u << 5;
    _delay_ms(500);
    PORTB_DATA_R &=~ 1u << 5;
    _delay_ms(500);

}





