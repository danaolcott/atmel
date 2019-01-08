
#include <avr/interrupt.h>
#include <avr/io.h>         //macros

#include <string.h>
#include <stdio.h>
#include <stddef.h>


#include "register.h"
#include "usart.h"
#include "command.h"




static volatile unsigned char rxIndex = 0x00;
static volatile unsigned char rxActiveBuffer = 0;
static volatile unsigned char rxBuffer0[RX_BUFFER_SIZE];
static volatile unsigned char rxBuffer1[RX_BUFFER_SIZE];



///////////////////////////////////////
//Configure USART on Pins 0 and 1 as
//rx and tx.  No need to set direction if
//enabling the usart.  Baud rates supported
//include 9600, 57600, 115200.  Baud config
//values from Table 24-7.  Register defs 
//around page 244+
//defaults to 9600
void Usart_init(unsigned long baud)
{

    rxIndex = 0x00;
    rxActiveBuffer = 0;
    memset((char*)rxBuffer0, 0x00, RX_BUFFER_SIZE);
    memset((char*)rxBuffer1, 0x00, RX_BUFFER_SIZE);


    //configure the baud rate
    UBRR0H_R = 0x00;

    switch(baud)
    {
        case 9600:
        {
            UBRR0L_R = 207;
            UCSR0A_R |= 0x02;        //U2Xn = double speed
            break;
        }

        //57600 works better single speed, despite table 24-7
        case 57600:
        {
            UBRR0L_R = 16;
            UCSR0A_R &=~ 0x02;        //U2Xn = single speed
            break;
        }

        case 115200:
        {
            UBRR0L_R = 16;
            UCSR0A_R |= 0x02;        //U2Xn = double speed
            break;
        }

        default:        //9600:
        {
            UBRR0L_R = 207;
            UCSR0A_R |= 0x02;        //U2Xn = double speed
            break;
        }
    }

    


    //UCSR0B_R - 0xC1 - config interrupts and tx/rx enable
    //bit 7 - RX complete interrupt enable - set high
    //bit 6 - tx complete interrupt - 0
    //bit 5 - 0
    //bit 4 - rx enable - 1
    //bit 3 - tx enable - 1
    //bit 2 - 1 - dont care
    //bit 0 - 9th bit on a 9 bit tx = 0
    //ie, write 0x98

    UCSR0B_R = 0x98;

    //UCSR0C_R - 0xC2 - config usart params
    //bit 7-6 = 00 - async
    //bit 5-4 = 00 - no parity
    //bit 3   = 0 - 1 stop bit
    //bit 2-0 = 011 - 8 bit - note: this defaults to 110, ??
    //since these are listed as reserved, leave bits 0-2 alone
    UCSR0C_R = 0x06;

    //enable global interrupts
    __SEI;

    //poll the rx...
    while (RXC0_FLAG){};        //complete any incoming
    while (TXC0_FLAG){};        //complete any outgoing 
}



///////////////////////////////////
//Usart_isr
//Call this function from ISR(), passing
//the read data byte as function arg.
//Uses two buffers and flips between them
//process command splits into argc, argv
//
void Usart_isr(unsigned char c)
{
    if ((c != 0x00) && (rxIndex < (RX_BUFFER_SIZE - 1)))
    {
        if (!rxActiveBuffer)
            rxBuffer0[rxIndex] = c;
        else
            rxBuffer1[rxIndex] = c;

        rxIndex++;

        //test char c for \n
        if (c == '\n')
        {
            if (!rxActiveBuffer)
            {
                rxBuffer0[rxIndex] = 0x00;    
                Usart_processCommand((unsigned char*)rxBuffer0, rxIndex);                
                rxActiveBuffer = 1;
                rxIndex = 0x00;
                memset((char*)rxBuffer1, 0x00, RX_BUFFER_SIZE);
            }

            else
            {
                rxBuffer1[rxIndex] = 0x00;
                Usart_processCommand((unsigned char*)rxBuffer1, rxIndex);                
                rxActiveBuffer = 0;
                rxIndex = 0x00;
                memset((char*)rxBuffer0, 0x00, RX_BUFFER_SIZE);
            }
        }
    }
}


/////////////////////////////////////
//poll the tx ready bit to get an empty
//tx buffer.

void Usart_sendByte(unsigned char data)
{
    while(!(UCSR0A_R & (1u << 5))){};   //wait for empty
    UDR0_R = data;                      //write data
    while(!(UCSR0A_R & (1u << 6))){};   //wait for complete
}


void Usart_sendString(char *data)
{
    char *p = data;
    while (*p != 0x00)
    {
        Usart_sendByte((unsigned char)*p);
        p++;
    }
}

void Usart_sendArray(unsigned char *data, unsigned int length)
{
    unsigned int i = 0x00;
    for (i = 0 ; i < length ; i++)
        Usart_sendByte(data[i]);
}



//////////////////////////////////////////////
//process command function for incoming
//data over usart.  Parse args into argc
//argv[*]
//tokanize buffer and assign array of char*
//and get num args.
void Usart_processCommand(unsigned char *data, unsigned int length)
{
	//arg buffs, ptr and size
	char* argv[ARG_BUFFER_SIZE];
	int i, argc = 0;
    int result = 0x00;
    char outBuffer[64];

	memset(argv, 0x00, ARG_BUFFER_SIZE);

    //clean up array by removing \r\n
    for (i = 0 ; i < length ; i++)
    {
        if ((data[i] == '\r') || (data[i] == '\n'))
            data[i] = 0x00;
    }

    Usart_sendString("Orig Rx String: ");
    Usart_sendArray(data, length);
    Usart_sendString("\r\n");


    //parse data* into argv argc
    Usart_parseArgs((char*)data, &argc, argv);

    //pass argv/arc into the cli table
    result = Command_ExeCommand(argc, argv);

    if (result >= 0)
    {
        i = sprintf(outBuffer, "Success: Elem: %d\r\n", result);
        Usart_sendArray((unsigned char*)outBuffer, i);    
    }
    else
    {
        Usart_sendString("Error No Match\r\n");
    }




}


/////////////////////////////////////////
//parse input buffer into args by replacing
//all white space with null chars, and 
//populating array of pointers pointing
//to each arg.
//
void Usart_parseArgs(char *in, int *pargc, char** argv)
{
	int argc = 0;
	char* ptr;

	//get the first arg - pass input buffer
	ptr = strtok(in, " ,.-");
	argv[argc++] = ptr;

	//subsequent args, pass NULL
	while ((ptr != NULL) && (argc < ARG_BUFFER_SIZE))
	{
		ptr = strtok(NULL, " ,.-");
		if(ptr != NULL)
			argv[argc++] = ptr;
	}

	*pargc = argc;
}





