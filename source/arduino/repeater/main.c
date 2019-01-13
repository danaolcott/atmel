////////////////////////////////////////////////////
/*
Baremetal Programming on the Atmel
ATMega328P (Arduino) processor.
Dana Olcott
1/7/19

NRF24L01 Transceiver Project - Repeater Mode

Use existing nrf24l01 tx/rx project and add
repeater mode.  The idea is that the radio 
will be configured to listen for incoming 
messages using the standard RX mode.  When a new
message arrives, it will forward it out.

Functions added for set channel (freq) so that
it can listen on one channel and send on a different
one.  This is so that there can be a main receiver
that only listens for repeater signals.


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

#include "register.h"
#include "spi.h"
#include "nrf24l01.h"
#include "usart.h"

//////////////////////////////////////
//prototypes
void GPIO_init(void);
void LED_on(void);
void LED_off(void);
void LED_toggle(void);
void Timer0_init(void);
void Interrupt_init(void);


/////////////////////////////
//Delay items
void Delay(unsigned long val);
volatile unsigned long gTimeTick = 0x00;

uint8_t forwardBuffer[32] = {0x00};
uint32_t loopCounter = 0x00;

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



//////////////////////////////////
//INT0 ISR
//Connected to IRQ Pin - Falling edge,
//call nrf24_ISR
//
ISR(INT0_vect)
{
    nrf24_ISR();
}


//////////////////////////////////
//INT1 ISR
//Connected to User Button - Pin 3
//PD3
//
ISR(INT1_vect)
{
    //do something
}


/////////////////////////////////////
//UART_RX Handler
ISR(USART_RX_vect)
{
    unsigned char data = UDR0_R;

    Usart_isr(data);
}


///////////////////////////////////////
int main()
{
    GPIO_init();                    //configure led and button
    Timer0_init();                  //Timer0 Counter Overflow
    SPI_init();			            //init spi
    SPI_setSpeed(SPI_SPEED_1_MHZ);
    Usart_init(9600);    
 //   nrf24_init(NRF24_MODE_REPEATER);
    nrf24_init(NRF24_MODE_RX);

    while(1)
    {
        LED_toggle();

        //Mode: NRF24_MODE_REPEATER
        if (nrf24_getMode() == NRF24_MODE_REPEATER)
        {
            if (nrf24_getRepeaterFlag() == 1)
            {
                Usart_sendString("Repeater: Flag Set - Forward Packet\r\n");
                uint8_t size = nrf24_getRepeaterBuffer(forwardBuffer);      //load the contents
                nrf24_setState(NRF24_STATE_TX);                             //set to tx state
                nrf24_transmitData(8, forwardBuffer, size);                 //send the data
                nrf24_setState(NRF24_STATE_RX);                             //return to rx state
                nrf24_setRepeaterFlag(0);                                   //clear the flag
            }

            if (!(loopCounter % 10))
                Usart_sendString("Waiting for Incoming Data...\r\n");
        }

        else if (nrf24_getMode() == NRF24_MODE_RX)
        {
            if (!(loopCounter % 10))
                Usart_sendString("Waiting for Incoming Data...\r\n");
        }

        else if (nrf24_getMode() == NRF24_MODE_TX)
        {
            //send something
        }

       

        loopCounter++;
        Delay(500);
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
    PORTD_DIR_R &=~ BIT3; 	//pin 3 as input
    PORTD_DATA_R |= BIT3;	//enable pullup

    //configure interrupt INT1 - PD3 - falling trigger
    EICRA_R |= BIT3;
    EICRA_R &=~ BIT2;

    //interrupt mask - EIMSK - 0x3D - set bit 1
    EIMSK_R |= BIT1;

    //clear pending interrupt - EIFR - 0X3C - bit 1
    EIFR_R |= BIT1;

    //enable global interrupts - set the I-bit in SREG
    SREG_R |= 1u << 7;

    sei();

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



////////////////////////////////////
//timeTick is increased in timer isr
void Delay(unsigned long val)
{
	volatile unsigned long t = val;
    gTimeTick = 0x00;           //upcounter
    while (t > gTimeTick){};
}



