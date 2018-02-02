////////////////////////////////////////////////////
/*
Programming the Atmel ATtimy85 processor.
Dana Olcott
2/1/18

Ex:  Temperature Sensor State Machine Program

This project follows along with the Moore State 
Machine project with modifications to use a
temperature sensor.

Use ADC1 on PB2 (Pin 7)

Buttons: shift these to PB0 and PB1
on pins 5 and 6.  

Leave LEDs alone


General Concept:

Press the user button to set a base temperature 
reading.  While in a certain state, the red 
led flashes number of deg hotter than the base 
temp.  The blue led flashes the number of measured
degrees cooler than the base temp.  ie, it's a 
relative temperature measurment device.

Hardware config:
LEDs on PB3 and PB4 as output
Buttons on PB0 and PB1 as input with interrupts.
Timer0 as OCC with interrupt trigger at 1khz.
ADC Channel ADC1 on PB2 (Pin 7)

General FSM Definition:

State_t struct:

stateID                -index in the state table, 0,1, ...
frequency              -delay routine for state evaluation
*fptr                  -run to completion function
nextState[]            -array of next states

For every input value there is a cooresponding state
listed in nextState

State_t fsm[num states]




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



//////////////////////////////////////////
//FSM items
//
#define FSM_NUM_STATES      4

unsigned char flashState = 0x00;

//state functions
void function0(void);
void function1(void);
void function2(void);
void function3(void);

//state names to make it more readable
//enum names coorespond to the index in array
//to keep it simple.
typedef enum
{
    STATE_0 = 0,
    STATE_1 = 1,
    STATE_2 = 2,
    STATE_3 = 3,
}StateName_t;


typedef struct
{
    StateName_t name;
    unsigned int frequency;
    void (*fptr) (void);
    StateName_t nextState[FSM_NUM_STATES];   
}State_t;


//0123 - transitions to the cooresponding state
//ie, if flashState == 1, run function 1, if 2, then
//run 2, etc.  Runs the statemachine every 10ms
//for all states.
//
State_t fsm[FSM_NUM_STATES] = 
{
    {STATE_0, 10, function0, {STATE_0, STATE_1, STATE_2, STATE_3}},
    {STATE_1, 10, function1, {STATE_0, STATE_1, STATE_2, STATE_3}},
    {STATE_2, 10, function2, {STATE_0, STATE_1, STATE_2, STATE_3}},
    {STATE_3, 10, function3, {STATE_0, STATE_1, STATE_2, STATE_3}},
};

State_t currentState;


///////////////////////////////
//ADC Variables, flags... etc
//flag to check if the basetemp needs
//to get set for temp sensor adc reading
unsigned char gBaseTempFlag = 0;    



/////////////////////////////
//Delay items
void Delay(volatile unsigned int val);
static volatile unsigned long gTimeTick = 0x00;

void GPIO_init(void);
void Button_init(void);
void Timer0_init(void);
void Timer0_OCCA_init(void);

void ADC1_init(void);       //ADC1 on PB2 - Pin 7

//button debounce
void Waste_CPU(unsigned int temp);
void Dummy_Function(void);

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
//ISR - Buttons on PCINT0 and PCINT1
//shared interrupt??
//
//
ISR(PCINT0_vect)
{
    unsigned char val0, val1 = 0x00;
    Waste_CPU(1000);

    //check button press after killing cpu time
    val0 = PINB_R & BIT0;
    val1 = PINB_R & BIT1;   //left

    //PB0 - down
    if (!val0)
    {
        PORTB_DATA_R ^= BIT3;

        flashState = 0;
    }

    //input value for flash state
    //0 to num states - 1
    if (!val1)
    {
        if (flashState < FSM_NUM_STATES -1)
            flashState++;
        else
            flashState = 0;
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
        //get next state from current and flashState input
        StateName_t next = currentState.nextState[flashState];

        //update state
        currentState = fsm[next];
        
        //run the current state function
        currentState.fptr();

        //wait
        Delay(currentState.frequency);        
    }

	return 0;
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
//Configure PB0 and PB1 on user buttons
//as interrupts, no pull.  Buttons have 10k
//pullup added.
//Configure to run on PCINT0 and PCINT1
//
void Button_init(void)
{
    //general interrupt mask reg - bit 5 - PCIE
    GIMSK_R |= BIT5;

    //PCMSK - enable line specific interrupt
    //PCINT0 and PCINT1
    PCMSK_R |= BIT0;
    PCMSK_R |= BIT1;

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




void function0(void)
{
    PORTB_DATA_R ^= BIT4;
    Delay(100);
}

void function1(void)
{
    PORTB_DATA_R ^= BIT4;
    Delay(200);
}

void function2(void)
{
    PORTB_DATA_R ^= BIT4;
    Delay(500);
}

void function3(void)
{
    PORTB_DATA_R ^= BIT4;
    Delay(1000);
}









