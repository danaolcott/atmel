////////////////////////////////////////////////////
/*
Baremetal Programming on the Atmel
ATMega328P (Arduino) processor.
Dana Olcott
12/15/17

Simple Tasker example.
Make 2 tasks, sender and receiver.  Sender
sends toggle signal to 

Defines (see makefile)  __AVR_ATmega328P__
Inludes:  /usr/lib/avr/include
*/
///////////////////////////////////////////////

#include <avr/io.h>         //macros
#include <avr/interrupt.h>
#include "./task/task.h"

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

#define PRR_R         (*((volatile unsigned char*)0x64))

//timer 0
#define TCCR0A_R         (*((volatile unsigned char*)0x44))
#define TCCR0B_R         (*((volatile unsigned char*)0x45))
#define TIMSK0_R         (*((volatile unsigned char*)0x6E))
#define TCNT0_R          (*((volatile unsigned char*)0x46))
#define TIFR0_R          (*((volatile unsigned char*)0x15))


//////////////////////////////////////
//prototypes
void GPIO_init(void);
void Interrupt_init(void);
void Timer0_init(void);
void Waste_CPU(unsigned int temp);
void Dummy_Function(void);


/////////////////////////////
//Delay items
//Don't use if using task.h/.c
//
void Delay(volatile unsigned int val);
static volatile unsigned long gTimeTick = 0x00;


//////////////////////////////////////
//task items
//
#define TASK_TX_NAME		((char*)"tx")
#define TASK_RX_NAME		((char*)"rx")

//Task functions
void TaskFunction_Tx(void);
void TaskFunction_Rx(void);




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
    TaskMessage msg = {TASK_SIG_BUTTON, 0x00};
	uint8_t index = 0x00;

    unsigned char val = 0x00;
    Waste_CPU(10000);

    //check button press after killing cpu time
    //PIND_R is for reading
    val = PIND_R & 0x04;
    if (!val)
    {
        //send a toggle msg to the rx task
        index = Task_GetIndexFromName("rx");
        Task_SendMessage(index, msg);
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

    //run the scheduler - See Task.c/.h
    Task_TimerISRHandler();

    //clear interrupt - datasheet shows
    //this bit has to be set to run timer
    TIFR0 |= 0x01;
}




///////////////////////////////////////
int main()
{
    GPIO_init();        //configure led and button
    Interrupt_init();   //falling edge trigger  
    Timer0_init();      //Timer0 Counter Overflow
 

    //Task
    Task_Init();

	//add the tasks
	Task_AddTask(TASK_TX_NAME, TaskFunction_Tx, 500, 0);
	Task_AddTask(TASK_RX_NAME, TaskFunction_Rx, 100, 1);

	//start the scheduler
	Task_StartScheduler();

	while (1){};

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




//////////////////////////////////////////
//Task Definitions - Tx Task
//send toggle to the receiver task
//
void TaskFunction_Tx(void)
{

	TaskMessage msg = {TASK_SIG_TOGGLE, 0x00};

	uint8_t index = Task_GetIndexFromName("rx");

	Task_SendMessage(index, msg);

}


///////////////////////////////////////////
//Task Definitions - Rx Task
//
//
void TaskFunction_Rx(void)
{
	TaskMessage msg = {TASK_SIG_NONE, 0x00};
	uint8_t index = Task_GetIndexFromName("rx");

    //read all incoming messages into msg address
    //
	while (Task_GetNextMessage(index, &msg) > 0)
	{
		switch(msg.signal)
		{
			case TASK_SIG_ON:
			{
                PORTB_DATA_R |= 1u << 5;
				break;
			}
			case TASK_SIG_OFF:
			{
                PORTB_DATA_R &=~ 1u << 5;
				break;
			}

			case TASK_SIG_TOGGLE:
			{
                PORTB_DATA_R ^= 1u << 5;
				break;
			}

            //signal posted from button
            //delay to create a blip
			case TASK_SIG_BUTTON:
			{
                PORTB_DATA_R ^= 1u << 5;
                Delay(50);
                PORTB_DATA_R ^= 1u << 5;
                Delay(50);
				break;
			}

			default:
				break;
		}
	}
}













