////////////////////////////////////////////////////
/*
Programming the Atmel ATtimy85 processor.
Dana Olcott
12/19/17

Ex: A simple program that toggles PB3 and PB4
and captures interrupts on PB1 and PB2 (PCINT1, PCINT2)

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
#define _SEI            (SREG_R |= BIT7)

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




/////////////////////////////
//Delay items
void Delay(volatile unsigned int val);
static volatile unsigned long gTimeTick = 0x00;

void GPIO_init(void);
void Button_init(void);
void Timer0_init(void);
void Timer0_OCCA_init(void);

//button debounce
void Waste_CPU(unsigned int temp);
void Dummy_Function(void);


//main loop
static volatile unsigned char flashRoutine = 0;
void flash0(void);
void flash1(void);



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



///////////////////////////////////////////
//ISR - Buttons on PCINT1 and PCINT2
//shared interrupt??
//
//
ISR(PCINT0_vect)
{
    unsigned char val1, val2 = 0x00;
    Waste_CPU(1000);

    //check button press after killing cpu time
    val1 = PINB_R & BIT1;
    val2 = PINB_R & BIT2;   //left

    //PB1 - down
    if (!val1)
    {
        PORTB_DATA_R ^= BIT3;
    }

    if (!val2)
    {
        if (!flashRoutine)
            flashRoutine = 1;
        else
            flashRoutine = 0;
    }

}





//////////////////////////////////////////
//Main loop - calls flash routine o or 1
//base on flashroutine variable set in 
//button interrupt.
//
int main()
{
    //init pins as output
    GPIO_init();

    //init one or the other
    Timer0_OCCA_init();
    //Timer0_init();

    Button_init();

    while(1)
    {
        switch(flashRoutine)
        {
            case 0:  flash0(); break;
            case 1:  flash1(); break;
            default: flash0(); break;
        }
    }

	return 0;
}




void flash0(void)
{
    PORTB_DATA_R ^= BIT4;
    Delay(100);
}


void flash1(void)
{
    PORTB_DATA_R ^= BIT4;
    Delay(500);
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



///////////////////////////////////////////
//Configure PB1 ans PB2 on user buttons
//as interrupts, no pull.  Buttons have 10k
//pullup added.
//Configure to run on PCINT1 and PCINT2
//
void Button_init(void)
{
    //general interrupt mask reg - bit 5 - PCIE
    GIMSK_R |= BIT5;

    //PCMSK - enable line specific interrupt
    //PCINT1 and PCINT2
    PCMSK_R |= BIT1;
    PCMSK_R |= BIT2;

    GIFR_R |= BIT5;     //clear pending interrupt flag

    _SEI;       //enable global interrupts
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










