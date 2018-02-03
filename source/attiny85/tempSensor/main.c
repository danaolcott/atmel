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

150 = 73deg
155 = 78deg




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


//ADC
#define ADCMUX_R         (*((volatile unsigned char*)(0x07 + IO_OFFSET)))
#define ADCSRA_R         (*((volatile unsigned char*)(0x06 + IO_OFFSET)))
#define ADCH_R           (*((volatile unsigned char*)(0x05 + IO_OFFSET)))       //bits 01 - top 2 in 10bit
#define ADCL_R           (*((volatile unsigned char*)(0x04 + IO_OFFSET)))       //bits 0-7


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
//gReferenceADC reading = base reading
//for all comparisons
static unsigned int gReferenceADC;
static unsigned int gReferenceMV;
static unsigned int gReferenceTEMP;



/////////////////////////////
//Delay items
void Delay(volatile unsigned int val);
static volatile unsigned long gTimeTick = 0x00;

void GPIO_init(void);
void Button_init(void);
void Timer0_init(void);
void Timer0_OCCA_init(void);

void ADC1_init(void);       //ADC1 on PB2 - Pin 7
unsigned int ADC1_read(void);


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


/////////////////////////////////////////
//ADC Interrupt
//see the following .h file
//for isr definitions:
//  /usr/lib/avr/include/avr/iotnx4.h
// 
//ISR(ADC_vect)
//{
//    ADCSRA_R |= BIT4;       //clear interrupt
//}


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
    Timer0_OCCA_init();
    Button_init();
    ADC1_init(); 

    //init adc variables
    gReferenceADC = 0;
    gReferenceMV = 0x00;
    gReferenceTEMP = 0x00;

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





///////////////////////////////////////////////
//ADC1 on PB2 on Pin7
//From what I can tell, the main registers 
//of interest are:
//PORTB reg - Bit 2 - config as input

//starting page 122 of the datasheet
//registers defined on page 134, Section 17.13
//
//
//general process:
//set the ref voltage in ADCMUX - REFS 2:0
//enable the ADC by setting the ADC enable bit in ADCSRA (ADEN)
//data is available in registers ADCH and ADCL
//data is right justified
//can left justify using the ADLAR bit in ADMUX
//
//ADCSRA
//ADCSRB - not sure we need this one.
//DIDR0 - digital input disable register

//ADMUX
//ADCH and ADCL - resultss go here
//
//1 - ADMUX - 0x07 - write 0000 0001 = 0x01
//bit 7-6 - REFS - set to 000 - use Vcc as ref
//bit 5 - ADLAR - justify - set as 0 - rt justify
//bits 0 - 3 - MUX - ADC1 - single ended input - write 0001
//
//2 - ADCSRA - Control and status register A - 0x06
//bit 7 - ADEN - enable adc, write to 1 to enable adc
//bit 6 - ADSC - start the conversion - write 1 to start conversion. 
//              poll it till it goes low.  when low, the conversion is done
//
//bit 5 - ADATE - ADC auto trigger enable.  write this bit one to auto trigger (not sure...)
//bit 4 - ADIF - interrupt flag, set when conversion is complete.  
//bit 3 - ADIE - interrupt enable - set to 0
//bits 0-2 - prescaler - set to 111, prescale = 128.  
//
// result = 1010 0111       initial value = 0xA7
//start the conversion -  1 << 6
//poll bit 6 until it goes low.
//


//#define ADCMUX_R         (*((volatile unsigned char*)(0x07 + IO_OFFSET)))
//#define ADCSRA_R         (*((volatile unsigned char*)(0x06 + IO_OFFSET)))
//#define ADCH_R           (*((volatile unsigned char*)(0x05 + IO_OFFSET)))       //bits 01 - top 2 in 10bit
//#define ADCL_R           (*((volatile unsigned char*)(0x04 + IO_OFFSET)))       //bits 0-7

//if using auto trigger, the source is selected
//by setting the trigger bits in ADCSRB


void ADC1_init(void)
{
   
    //configure PB2 as input 
    //PORTB_DIR_R &=~ BIT2;

    //set up the MUX register,        
    //1 - ADCMUX - 0x07 - write 0000 0001 = 0x01
    //bit 7-6 - REFS - set to 000 - use Vcc as ref
    //bit 5 - ADLAR - justify - set as  0 - rt justify
    //bits 0 - 3 - MUX - ADC1 - single ended input - write 0001
    //

//    ADMUX = 0x01;      //Vcc as ref, rt justify, ADC1 as single ended input

    //left justify, vcc as ref
  //  ADMUX = 0x21;      //Vcc as ref, rt justify, ADC1 as single ended input


    //left justify, 1.1v ref (bit 7 high, bit 6 low)
    //1010 0001
    ADMUX = 0xA1;      //Vcc as ref, rt justify, ADC1 as single ended input


//    ADCMUX_R = 0x01;      //Vcc as ref, rt justify, ADC1 as single ended input

//    ADCSRA = ((1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0));  //prescale 128
//    ADCSRA = ((1 << ADPS2) | (1 << ADPS1) | (0 << ADPS0));  //prescale 64

//prescale 8 works with left justify and using the adc h reg

    ADCSRA = ((0 << ADPS2) | (1 << ADPS1) | (1 << ADPS0));  //prescale 8

//    ADCSRA = ((1 << ADPS2) | (0 << ADPS1) | (0 << ADPS0));  //prescale 16


    ADCSRA |= (1 << ADEN);



}


/////////////////////////////////
//ADC1 read
//set bit 6 high in the ADCSRA_R
//to start the conversion,
//poll the interrupt bit
//read and return the result
unsigned int ADC1_read(void)
{
    unsigned int result = 0x00;
    unsigned int temp = 0x00;

    //enable the adc
//    ADCSRA |= BIT7;

    //write bit 6 high to start the conversion
    ADCSRA |= (1 << ADSC);

    //wait - poll BIT6 - clear when conversion is complete
    while (ADCSRA & (1 << ADSC));

    //read the result in ADCH and ADCL
//    result = ADCL & 0x03;
//    result = result << 8;

//    result = (ADCH_R & 0xFF);
//    result = result << 2;

    result = (ADCL_R & 0x03);
    temp = (ADCH_R & 0xFF) << 2;
    result |= temp;


    
    //this works with left justify
//    result = ADCH & 0xFF;




    return result;

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
//use function 0 to set the 
//base temp, flash two leds
//alternating.  read the ADC
//and set the base value
void function0(void)
{
    gReferenceADC = ADC1_read() & 0x3FF;
    gReferenceMV = gReferenceADC * 1100 / 1024;
    gReferenceTEMP = (gReferenceMV - 500) / 10;     //centigrade
    gReferenceTEMP = ((gReferenceTEMP * 9 / 5) + 32) & 0x7F;

    PORTB_DATA_R |= BIT3;
    PORTB_DATA_R &=~ BIT4;
    Delay(200);
    PORTB_DATA_R |= BIT4;
    PORTB_DATA_R &=~ BIT3;
    Delay(200);


}



///////////////////////////////////
//function 1 - 
//use this as a comparison with current
//reading and base reading....
//if current is less than base, flash yellow
//if current is higher than base, flash red
void function1(void)
{
    unsigned int diff = 0;
    unsigned char i = 0;
    unsigned int currentADC = ADC1_read() & 0x3FF;
    unsigned int currentMV = currentADC * 1100 / 1024;
    unsigned int currentTEMP = (currentMV - 500) / 10;     //centigrade
    currentTEMP = ((currentTEMP * 9 / 5) + 32) & 0x7F;


    if (currentTEMP < gReferenceTEMP)
    {
        //flash yellow based on diff
        diff = gReferenceTEMP - currentTEMP;

        for (i = 0 ; i < diff ; i++)
        {
            PORTB_DATA_R &=~ BIT4;
            PORTB_DATA_R |= BIT3;
            Delay(50);
            PORTB_DATA_R &=~ BIT4;
            PORTB_DATA_R &=~ BIT3;
            Delay(400);
        }

        Delay(600);
    }


    else
    {
        //flash red based on diff
        diff = currentTEMP - gReferenceTEMP;

        for (i = 0 ; i < diff ; i++)
        {
            PORTB_DATA_R &=~ BIT3;
            PORTB_DATA_R |= BIT4;
            Delay(50);
            PORTB_DATA_R &=~ BIT3;
            PORTB_DATA_R &=~ BIT4;
            Delay(400);
        }

        Delay(600);
    }
}


/////////////////////////////////////
//flash yellow or red based on diff with
//base temp 65 deg.
//
void function2(void)
{
    unsigned int diff = 0;
    unsigned char i = 0;
    unsigned int currentADC = ADC1_read() & 0x3FF;
    unsigned int currentMV = currentADC * 1100 / 1024;
    unsigned int currentTEMP = (currentMV - 500) / 10;     //centigrade
    currentTEMP = ((currentTEMP * 9 / 5) + 32) & 0x7F;      //128 F max


    if (currentTEMP < 80)
    {
        //flash yellow based on diff
        diff = 80 - currentTEMP;

        for (i = 0 ; i < diff ; i++)
        {
            PORTB_DATA_R &=~ BIT4;
            PORTB_DATA_R |= BIT3;
            Delay(50);
            PORTB_DATA_R &=~ BIT4;
            PORTB_DATA_R &=~ BIT3;
            Delay(400);
        }

        Delay(600);
    }


    else
    {
        //flash red based on diff
        diff = currentTEMP - 80;

        for (i = 0 ; i < diff ; i++)
        {
            PORTB_DATA_R &=~ BIT3;
            PORTB_DATA_R |= BIT4;
            Delay(50);
            PORTB_DATA_R &=~ BIT3;
            PORTB_DATA_R &=~ BIT4;
            Delay(400);
        }

        Delay(600);
    }
}

///////////////////////////////
//read the value on the adc
//turn led off if low, led on
//if high
void function3(void)
{
    unsigned int result = 0x00;
    result = ADC1_read();
    result = result & 0x3FF;        //10bit only
    unsigned int i, val = 0;

    //yellow
    if (result < 512)
        PORTB_DATA_R &=~ BIT3;
    else
        PORTB_DATA_R |= BIT3;

    //flash the red led based on 100x the 
    //adc reading
    val = result / 100;
    for (i = 0 ; i < val ; i++)
    {
        PORTB_DATA_R |= BIT4;
        Delay(50);
        PORTB_DATA_R &=~ BIT4;
        Delay(200);        
    }
        
    //flash freq set by the ADC reading.
    Delay(1000);


}









