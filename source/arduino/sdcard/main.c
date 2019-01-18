////////////////////////////////////////////////////
/*
Baremetal Programming on the Atmel
ATMega328P (Arduino) processor.
Dana Olcott
1/7/19

SDCard using FatFS

Pinout:
SPI - pins 10-13 (PB2 - PB5)
LED - PB0 - Pin 8
IRQ Pin - PD2 (Pin 2) - INT0 - Interrupt, falling edge, pullup

Control Lines:
CE Pin - PB1 - Pin 9 - Normal output
UART - PD0 and PD1 - Pins 0 and 1

INT1 - PD3 - Also config as interrupt, falling, pullup


*/
///////////////////////////////////////////////

#include <avr/interrupt.h>
#include <avr/io.h>         //macros
#include <stdio.h>
#include <string.h>
#include "sdcard.h"
#include "register.h"
#include "spi.h"
#include "usart.h"

//includes for fat fs - call int SD_Init(void);

//////////////////////////////////////
//prototypes
void GPIO_init(void);
void LED_on(void);
void LED_off(void);
void LED_toggle(void);
void Timer0_init(void);
void Timer2_init(void);
void Interrupt_init(void);


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


////////////////////////////////////
//Timer2 - Dedicated Timer for SDCard
//Calls the timer management function
//for the fatfs
ISR(TIMER2_OVF_vect)
{
    //fatfs timer management
 //   disk_timerproc();

    //clear interrupt - datasheet shows
    //this bit has to be set to run timer
    TIFR2_R |= 0x01;
}



//////////////////////////////////
//INT0 ISR
//Connected to IRQ Pin - Falling edge,
//call nrf24_ISR
//
//ISR(INT0_vect)
//{
//    nrf24_ISR();
//}


//////////////////////////////////
//INT1 ISR
//Connected to User Button - Pin 3
//PD3
//
//ISR(INT1_vect)
//{
    //do something
//}


/////////////////////////////////////
//UART_RX Handler
ISR(USART_RX_vect)
{
    unsigned char data = UDR0_R;

    Usart_isr(data);
}


uint8_t buffer[100] = {0x00};
uint8_t tx[2] = {0xAA, 0xCC};

int n = 0;

///////////////////////////////////////
int main()
{
    GPIO_init();                    //configure led and button
    Timer0_init();                  //Timer0 Counter Overflow
    Timer2_init();                  //Timer2 Counter Overflow
    SPI_init();			            //init spi
    Usart_init(9600);    
//    SD_Init();

    while(1)
    {

        //void SPI_setSpeed(SPISpeed_t speed)
        LED_toggle();

        SPI_setSpeed(SPI_SPEED_125_KHZ);
        SPI_writeArray(tx, 2);

        SPI_setSpeed(SPI_SPEED_250_KHZ);
        SPI_writeArray(tx, 2);

        SPI_setSpeed(SPI_SPEED_1_MHZ);
        SPI_writeArray(tx, 2);

        SPI_setSpeed(SPI_SPEED_4_MHZ);
        SPI_writeArray(tx, 2);


        Delay(100);
    }

	return 0;
}



///////////////////////////////////////////
//GPIO_init
//Configure pin 8, PB0 as output
//Configure PD3 as input - pullup, interrupted, falling edge
//PD3 will be connected to a button
//
void GPIO_init(void)
{   
    //////////////////////////////////
    //User LED
    //Pin 8 - PB0
    PORTB_DIR_R |= BIT0; 	//pin 8 as output
    PORTB_DATA_R &=~ BIT0;	//clear pin 8

    //////////////////////////////////////
    //User Button
    //PD3 - INT1 - Pin 3 - input, pullup
//    PORTD_DIR_R &=~ BIT3; 	//pin 3 as input
//    PORTD_DATA_R |= BIT3;	//enable pullup

    //configure interrupt INT1 - PD3 - falling trigger
//    EICRA_R |= BIT3;
//    EICRA_R &=~ BIT2;

    //interrupt mask - EIMSK - 0x3D - set bit 1
 //   EIMSK_R |= BIT1;

    //clear pending interrupt - EIFR - 0X3C - bit 1
//    EIFR_R |= BIT1;

    //enable global interrupts - set the I-bit in SREG
    SREG_R |= 1u << 7;

//    sei();

}


void LED_on(void)
{
  PORTB_DATA_R |= BIT0;
}

void LED_off(void)
{
  PORTB_DATA_R &=~ BIT0;
}

void LED_toggle(void)
{
  PORTB_DATA_R ^= BIT0;
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



//////////////////////////////////////////
//Configure Timer2 with Overflow Interrupt
//The configuration should be similar to Timer0
//in that both are 8 bit and will also be 
//configured to overflow at 1khz
//
void Timer2_init(void)
{
    //Timer Control - 2 registers:
    TCCR2A_R = 0x00;      //disable all pin outputs on compare
    TCCR2B_R = 0x04;      //set the prescaler - bits 0-2

    //NOTE: Prescale values for Timer2 are not the same
    //as Timer0.  ie, to get clk / 64, need to write 0x04

    //    000 - no clock - timer disabled
    //    001 - no prescale
    //    010 - clk/8
    //    011 - clk/32
    //    100 - clk/64
    //    101 - clk/128
    //    110 - clk/256
    //    111 - clk/1024

    TIMSK2_R |= 0x01;       //bit 0 - overflow interrupt
    TIFR2_R |= 0x01;        //clear the overflow flag

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



