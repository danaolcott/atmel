////////////////////////////////////////////////////
/*
Programming the Atmel ATtimy85 processor.
Dana Olcott
3/28/18

Temperature Sensor State Machine Program
with output over a serial port at 9600 baud.

Temp Sensor - Microchip # MCP9700A-E/T0
Pinout - looking at flat face - 1 2 3
1 - Vdd
2 - Vout
3 - GND


Sensor Transfer Function:
Vout = Tc x Ta + V0c    where:
    Vout    = output voltage on pin2
    Tc      = Temp Coeff                    10mV / C
    Ta      = Ambient Temp
    V0c     = Output voltage at 0 deg C     500mv

Ta = (Vout - V0c) / Tc

NOTE: These temp characteristics are the same ones
      for the other temp IC I was using. TGZ 3? something....


User Button - PB0 - one button
LEDs - PB3 and PB4
Temp Sensor / ADC - PB2
Serial Output - PB1 - serial output
Timers - Timer1 - Systick timer
Timers - Timer0 - Controls serial port.


State Machine Operation:
Press user button to change states
State0 - Display absolute temp
State1 - Display relative temp from initial powerup
State3 - output ADC reading in 0.1 volts.

All states output the following over serial port
a##\r\n - absolute temp in deg F

Hardware config:
LEDs on PB3 and PB4 as output - Red = PB3, Blue = PB4
Button on PB0 as input with interrupt
ADC Channel ADC1 on PB2 (Pin 7)
Serial output on PB1
Timer1 with interrupt to trigger at about 1khz as system timer
Timer0 - configured to run the serial output.

Some added notes / components:
Add load resistor from analog out to ground - 22k
Add cap from hot to ground

General FSM Definition:

State_t struct:
stateID                -index in the state table, 0,1, ...
frequency              -delay routine for state evaluation
*fptr                  -run to completion function
nextState[]            -array of next states

State0: Abs temp measured from 70 deg.  Flashes red
        number of deg. offset from 70.  Flashes blue
        number of deb. cooler than offsets from 70.

State1: Relative meaurement from referene temp.
        Flashes red if current measurement is higher
        than ref by number of deg F.  Flashes blue if
        current measurement is lower than ref by number 
        of deg F.

State2: Flashes red led indicating the ADC reading
        at the analog out pin on the temp sensor.  ie,
        if analog out is 0.5 volts, it flashes red 5 times.
        Blue led is low if ADC reading less than mid-scale 
        (ie, < 512) and high if above.

User Buttons:
        User button changes state

Serial Output
        Output absolute temperature over serial on PB1


*/
//
////////////////////////////////////////////
//
//
#include <avr/io.h>         //macros
#include <avr/interrupt.h>
#include <stdio.h>
#include <string.h>


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
#define _CEI            (SREG_R &=~ BIT7)

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

//Timer1
#define TCCR1_R         (*((volatile unsigned char*)(0x30 + IO_OFFSET)))
#define GTCCR_R         (*((volatile unsigned char*)(0x2C + IO_OFFSET)))
#define TCNT1_R         (*((volatile unsigned char*)(0x2F + IO_OFFSET)))
#define OCR1A_R         (*((volatile unsigned char*)(0x2E + IO_OFFSET)))
#define OCR1B_R         (*((volatile unsigned char*)(0x2B + IO_OFFSET)))
#define OCR1C_R         (*((volatile unsigned char*)(0x2D + IO_OFFSET)))
#define TIMSK_R         (*((volatile unsigned char*)(0x39 + IO_OFFSET)))
#define TIFR_R          (*((volatile unsigned char*)(0x38 + IO_OFFSET)))

//ADC
#define ADCMUX_R         (*((volatile unsigned char*)(0x07 + IO_OFFSET)))
#define ADCSRA_R         (*((volatile unsigned char*)(0x06 + IO_OFFSET)))
#define ADCH_R           (*((volatile unsigned char*)(0x05 + IO_OFFSET)))       //bits 01 - top 2 in 10bit
#define ADCL_R           (*((volatile unsigned char*)(0x04 + IO_OFFSET)))       //bits 0-7

//temp defines
#define DEFAULT_REF_TEMP        ((unsigned int)70)

#define SERIAL_BUFFER_SIZE      32

//////////////////////////////////////////
//FSM items
//
#define FSM_NUM_STATES      3

//updated in the button isr
volatile unsigned char flashState = 0x00;

//state functions
void function0(void);
void function1(void);
void function2(void);

//state names to make it more readable
//enum names coorespond to the index in array
//to keep it simple.
typedef enum
{
    STATE_0 = 0,
    STATE_1 = 1,
    STATE_2 = 2,
}StateName_t;


typedef struct
{
    StateName_t name;
    unsigned int frequency;
    void (*fptr) (void);
    StateName_t nextState[FSM_NUM_STATES];   
}State_t;

//////////////////////////////////////////////////
//State Machine Definition
//Simple state pattern - all states have next
//state in sequence. ie, 0 to 1, 1 to 2.. etc.
//
State_t fsm[FSM_NUM_STATES] = 
{
    {STATE_0, 10, function0, {STATE_0, STATE_1, STATE_2}},
    {STATE_1, 10, function1, {STATE_0, STATE_1, STATE_2}},
    {STATE_2, 10, function2, {STATE_0, STATE_1, STATE_2}},
};

State_t currentState;



////////////////////////////////////
//Serial Port 
volatile struct {
  uint8_t dataByte, bitsLeft,
          pin, done;   
} txData = {0, 0, 0, 0};




////////////////////////////////////////////////
//ADC Variables, flags... etc
//gReferenceADC reading = base reading
//for all comparisons
static unsigned int gReferenceADC;
static float gReferenceMV;
static float gReferenceTEMP;

char serialBuffer[SERIAL_BUFFER_SIZE];


///////////////////////////////////////////////////
//Delay items
void Delay(volatile unsigned int val);
static volatile unsigned long gTimeTick = 0x00;


void GPIO_init(void);               //GPIO - leds
void Button_init(void);             //Buttons PB0 and PB1
void Timer1_init(void);             //time tick

void Serial_init(uint8_t pin);              //configure serial output
void SerialWriteByte(const uint8_t data);   //send byte
void SerialWriteString(char *p, int length);    //send string

void ADC1_init(void);               //ADC1 on PB2 - Pin 7
unsigned int ADC1_read(void);




//Button debounce
void Waste_CPU(unsigned int temp);
void Dummy_Function(void);

//Sorting
void SortArray(unsigned int *data, unsigned int size);



///////////////////////////////////////
//Timer0 CCR0A
//Runs when sending data over serial
//
ISR(TIMER0_COMPA_vect) 
{

    uint8_t bitVal;

    switch (txData.bitsLeft) 
    {

        case 10:
            bitVal = 0;
            break;
        case  1:
            bitVal = 1;
            break;
        case  0:
            TIMSK &= ~(1 << OCIE0A);
            txData.done = 1;
            return;

        default:
            bitVal = txData.dataByte & 1;
            txData.dataByte >>= 1;
    }


    if (bitVal)
        PORTB |= (1 << txData.pin);

    else
        PORTB &= ~(1 << txData.pin);

    --txData.bitsLeft;

}


///////////////////////////////
//Timer1 Overflow Interrupt ISR
//For now, toggle something to get
//the rate of the timer
ISR(TIMER1_OVF_vect)
{
    gTimeTick++;            //used by Delay

    TCNT1_R = 0x14;         //jump up a bit
    TIFR_R &=~ BIT2;        //clear overflow flag
    TIMSK_R |= BIT2;        //enable overflow interrupt

}


///////////////////////////////////////////
//ISR - Button on PCINT0
//
ISR(PCINT0_vect)
{
    unsigned char val0 = 0x00;
    Waste_CPU(1000);

    //check button press after killing cpu time
    val0 = PINB_R & BIT0;

    //PB0 - down - do nothing
    if (!val0)
    {
        if (flashState < FSM_NUM_STATES -1)
            flashState++;
        else
            flashState = 0;
    }

}



////////////////////////////////////////////////
//Main loop
//
int main()
{    
    ///////////////////////////////////////////
    //initialize peripherals
    GPIO_init();
    Timer1_init();
    Button_init();
    ADC1_init(); 

    Serial_init(PB1);

    memset(serialBuffer, 0x00, SERIAL_BUFFER_SIZE);

    ////////////////////////////////////////////
    //Get initial ADC reading for reference.

    gReferenceADC = (ADC1_read() & 0x3FF);
    gReferenceMV = ((float)gReferenceADC / 1024.0) * 1100.0;
    gReferenceTEMP = (gReferenceMV - 500.0) / 10.0;     //centigrade
    gReferenceTEMP = ((gReferenceTEMP * 9.0 / 5.0) + 32.0);

    ///////////////////////////////////////////////////
    //Run the state machine - set the initial state
    flashState = STATE_0;

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




/////////////////////////////////////////////
//Delay - gTimeTick incremented in timer isr
void Delay(volatile unsigned int val)
{
    gTimeTick = 0x00;
    while (val > gTimeTick){};
}




////////////////////////////////////////////
//GPIO_init
//Configure LEDs as output
//RED = BIT3, Blue = BIT4
void GPIO_init(void)
{
    PORTB_DIR_R |= BIT3;
    PORTB_DIR_R |= BIT4;

    //set initial value - low
    PORTB_DATA_R &=~ BIT3;
    PORTB_DATA_R &=~ BIT4;
}



/////////////////////////////////////////////////
//Configure User Buttons.  
//PB0 is the user button.
//Buttons have 10k pullup added.
//Configure to run on PCINT0
//
void Button_init(void)
{
    //general interrupt mask reg - bit 5 - PCIE
    GIMSK_R |= BIT5;

    //PCMSK - enable line specific interrupt - PCINT0
    PCMSK_R |= BIT0;

    GIFR_R |= BIT5;     //clear pending interrupt flag

    _SEI;       //enable global interrupts
}






//////////////////////////////////////////////////
//Configure Timer1 with Overflow Interrupt
//Setup Timer1 to interrupt at about 1khz 
//for use as the sytem timer.  
//
void Timer1_init(void)
{ 
    //Timer1 Control register - bits 3-0 - 0011
    //See Table 12-5.  Use prescale 4 and reload value
    //to arrive at overflow rate at 1000hz.  Tune 
    //as needed.
    TCCR1_R &=~ BIT3;       //0011 - CK / 4
    TCCR1_R &=~ BIT2;       //
    TCCR1_R |=  BIT1;
    TCCR1_R |=  BIT0;
    
    TCNT1_R = 0x14;         //set counter reload to 1000hz for CK/4

    //enable interrrupts and clear flags
    TIMSK_R |= BIT2;        //enable overflow interrupt
    TIFR_R &=~ BIT2;        //clear overflow flag

    _SEI;                   //enable global interrupts
}



////////////////////////////////
//Configure any pin on the ATTiny85
//as output and use as a serial 
//port at 9600 baud.  Assumes Timer0
//as timer source and internal clock
//running at default speed of 1mhz

void Serial_init(uint8_t serialPin)
{
    txData.pin = serialPin;       //set port pin
    DDRB |= 1 << txData.pin;      //config as output
    PORTB |= 1 << txData.pin;     //initial state high

    //config output compare counter interrupt value
    //this value configures the baud rate.  
    //for no prescale, use 106 to get about 9600 baud
    //note: original code is missing the clock config,
    //so I think the CPU is actually running at 1mhz
  
    OCR0A = 106;          //about 104??  pretty close

    // No A/B match output; just CTC mode
    TCCR0A = 1 << WGM01;

    //no prescale
    TCCR0B = 1 << (CS00);

}



//---------------------------------------------------------
void SerialWriteByte(const uint8_t data) 
{

    txData.dataByte = data;
    txData.bitsLeft = 10;
    txData.done = 0;
    // Reset counter
    TCNT0 = 0;
    // Activate timer0 A match interrupt
    TIMSK |= 1 << OCIE0A; 
}


 
/////////////////////////////////////////
//SerialWriteString
void SerialWriteString(char *p, int length)
{
    int i = 0;

    for (i = 0 ; i < length ; i++)
    {
        //disable the global timer interrupt - Timer1
        TIMSK_R &=~ BIT2;           //TOIE1 - Timer1 overflow interrupt enable

        SerialWriteByte(p[i]);
        while (!txData.done);

        //resume the global timer interrupt Timer1
        TIMSK_R |= BIT2;           //TOIE1 - Timer1 overflow interrupt enable

    } 


}




////////////////////////////////////////////////////////
//ADC1 Configuration
//Configure ADC1 to run on PB2 (Pin 7)
//
//References: Page 122 - 134 of the datasheet
//
//General Process:
//Set up the reference voltage (use 1.1v) using
//the ADCMUX register, Bits 2-0
//Enable the ADC by setting the ADC enable bit
//in ADCSRA (ADEN).
//Data is available in registers ADCH and ADCL
//Left justify by using the ADLAR bit in ADMUX
//
//ADCSRA - Control and status register A
//bit 7 - ADEN - enable adc, write to 1 to enable adc
//bit 6 - ADSC - start the conversion - write 1 to start. 
//               poll it till it goes low indicating complete.
//bit 5 - ADATE - ADC auto trigger enable.  leave this 0
//bit 4 - ADIF - interrupt flag, set when conversion is complete.  
//bit 3 - ADIE - interrupt enable - set to 0
//bits 0-2 - prescaler - set based on the CPU speed, for 1mhz, use prescale 8
//
void ADC1_init(void)
{
    //ADCMUX_R register - Configure as 1.1v reference
    //left justify, ADC1 single ended input
    //left justify, 1.1v ref (bit 7 high, bit 6 low)
    //1010 0001
    ADMUX = 0xA1;

    //prescale bits - use prescale = 8
    ADCSRA = ((0 << ADPS2) | (1 << ADPS1) | (1 << ADPS0));  //prescale 8

    //enable the ADC
    ADCSRA |= (1 << ADEN);

}


///////////////////////////////////////////////////////////
//ADC1_read
//Read data from ADC1.  Process consists of:
//Set bit 6 high in ADCSRA_R to start conversion
//Poll bit 6 till it goes low indicating complete.
//Read the results. in data high and low registers.
//Note:
//Seems order these are read is important.  
//For left justify, reading data low then data high.
//Repeat this 5 times, sort, and take the average.
//
unsigned int ADC1_read(void)
{
    unsigned int result = 0x00;
    unsigned int adc[5] = {0x00};
    unsigned char i = 0;
    unsigned int temp = 0x00;

    for (i = 0 ; i < 5 ; i++)
    {
        //write bit 6 high to start the conversion
        ADCSRA_R |= BIT6;

        //wait - poll BIT6 - clear when conversion is complete
        while (ADCSRA_R & BIT6);

        //read the result in ADCL then ADCH
        //it seems the order is important and
        //depends on if the data is left or 
        //right justified. This is for left justified
        
        result = (ADCL_R & 0x03);
        temp = (ADCH_R & 0xFF) << 2;
        result |= temp;

        adc[i] = result;
    }

    //sort the results - bubble sort
    SortArray(adc, 5);

    //average of middle 3
    result = adc[1] + adc[2] + adc[3];
    result = result / 3;

    return result;
}





///////////////////////////////////////////
//Dummy delay used for switch debounce
void Waste_CPU(unsigned int temp)
{
    volatile unsigned int val = temp;
    while (val > 0)
    {
        val--;
        Dummy_Function();
    }
}


//////////////////////////////////////////
void Dummy_Function(void)
{

}







//////////////////////////////////////////////////
//State0 - Function to Run
//
//Absolute Temperature Display
//Flash blue or red led indicating the 
//temperature difference from 70 deg. F.
//For ex: 74, flash red led 4 times.
//
void function0(void)
{
    int diff = 0;
    int n = 0;
    unsigned char i = 0;
    unsigned int currentADC = (ADC1_read() & 0x3FF);
    float currentMV = ((float)currentADC / 1024.0) * 1100.0;
    float currentTEMP = (currentMV - 500.0) / 10.0;     //centigrade
    currentTEMP = ((currentTEMP * 9.0 / 5.0) + 32.0);

    if (currentTEMP < DEFAULT_REF_TEMP)
    {
        //flash blue based on diff
        diff = DEFAULT_REF_TEMP - (unsigned int)currentTEMP;

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


    else
    {
        //flash red based on diff
        diff = (unsigned int)currentTEMP - DEFAULT_REF_TEMP;

        for (i = 0 ; i < diff ; i++)
        {
            PORTB_DATA_R &=~ BIT4;
            PORTB_DATA_R |= BIT3;
            Delay(40);
            PORTB_DATA_R &=~ BIT4;
            PORTB_DATA_R &=~ BIT3;
            Delay(400);
        }

        Delay(1000);
    }

    //output abs temp
    memset(serialBuffer, 0x00, SERIAL_BUFFER_SIZE);
    n = sprintf(serialBuffer, "a:%d\r\n", (int)currentTEMP);
    SerialWriteString(serialBuffer, n);

    //output relative temp
    memset(serialBuffer, 0x00, SERIAL_BUFFER_SIZE);

    if (currentTEMP > gReferenceTEMP)
    {
        n = sprintf(serialBuffer, "r:+%d\r\n", (int)(currentTEMP - gReferenceTEMP));
        SerialWriteString(serialBuffer, n);
    }
    else
    {
        n = sprintf(serialBuffer, "r:-%d\r\n", (int)(gReferenceTEMP - currentTEMP));
        SerialWriteString(serialBuffer, n);
    }

    //output the raw ADC reading
    memset(serialBuffer, 0x00, SERIAL_BUFFER_SIZE);
    n = sprintf(serialBuffer, "d:%d\r\n", currentADC);
    SerialWriteString(serialBuffer, n);

}








//////////////////////////////////////////////////
//State1 - Function to Run
//Relaive Temperature Display
//
//Flash the blue or red led based on whether
//the current temp is colder or hotter than
//the referene temp.  Flashes number of degrees
//(F) colder or hotter than the reference.
//
void function1(void)
{
    int diff = 0;
    int n;
    unsigned char i = 0;
    unsigned int currentADC = (ADC1_read() & 0x3FF);
    float currentMV = ((float)currentADC / 1024.0) * 1100.0;
    float currentTEMP = (currentMV - 500.0) / 10.0;     //centigrade
    currentTEMP = ((currentTEMP * 9.0 / 5.0) + 32.0);

    if (currentTEMP < gReferenceTEMP)
    {
        //flash blue based on diff
        diff = (int)gReferenceTEMP - (int)currentTEMP;

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

    else
    {
        //flash red based on diff
        diff = (int)currentTEMP - (int)gReferenceTEMP;

        for (i = 0 ; i < diff ; i++)
        {
            PORTB_DATA_R &=~ BIT4;
            PORTB_DATA_R |= BIT3;
            Delay(40);
            PORTB_DATA_R &=~ BIT4;
            PORTB_DATA_R &=~ BIT3;
            Delay(400);
        }

        Delay(1000);
    }



    //output abs temp
    memset(serialBuffer, 0x00, SERIAL_BUFFER_SIZE);
    n = sprintf(serialBuffer, "a:%d\r\n", (int)currentTEMP);
    SerialWriteString(serialBuffer, n);

    //output relative temp
    memset(serialBuffer, 0x00, SERIAL_BUFFER_SIZE);

    if (currentTEMP > gReferenceTEMP)
    {
        n = sprintf(serialBuffer, "r:+%d\r\n", (int)(currentTEMP - gReferenceTEMP));
        SerialWriteString(serialBuffer, n);
    }
    else
    {
        n = sprintf(serialBuffer, "r:-%d\r\n", (int)(gReferenceTEMP - currentTEMP));
        SerialWriteString(serialBuffer, n);
    }

    //output the raw ADC reading
    memset(serialBuffer, 0x00, SERIAL_BUFFER_SIZE);
    n = sprintf(serialBuffer, "d:%d\r\n", currentADC);
    SerialWriteString(serialBuffer, n);
    
}



///////////////////////////////////////////////////
//State2 - Function to Run
//
//Measurement of ADC1 Voltage
//Display the ADC1 reading in tenths of a volt.
//The voltage is displayed by flashing the 
//red led number of 1/10ths of a volt.  
//Blue led indicates if it's exceeded 1/2
//of full scale reference temp.  For example:
//if the ADC reading was 0.3XX volts, the red
//led flashes 3 times and the blue led is off
//
void function2(void)
{
    int n;
    unsigned int result = 0x00;
    result = ADC1_read();
    result = result & 0x3FF;        //10bit only
    unsigned int i, val = 0;
    float adc = 0.0;

    //blue
    if (result < 512)
        PORTB_DATA_R &=~ BIT4;
    else
        PORTB_DATA_R |= BIT4;

    //adc in 10th's of a volt
    adc = (float)result / 1024.0;
    adc = adc * 11;

    val = (unsigned int)adc;


    for (i = 0 ; i < val ; i++)
    {
        PORTB_DATA_R |= BIT3;
        Delay(50);
        PORTB_DATA_R &=~ BIT3;
        Delay(400);
    }

    //output the raw ADC reading
    memset(serialBuffer, 0x00, SERIAL_BUFFER_SIZE);
    n = sprintf(serialBuffer, "d:%d\r\n", result);
    SerialWriteString(serialBuffer, n);

    Delay(1000);

}



///////////////////////////////////////////
//Sort array of unsigned ints from lowest
//to highest using bubble sort
void SortArray(unsigned int *data, unsigned int size)
{
    unsigned int i, j, val;

    for (i = 0 ; i < size ; i++)
    {
        for (j = i+ 1; j < size ; j++)
        {
            if(data[i] > data[j])
            {
                val = data[i];
                data[i] = data[j];
                data[j] = val;
            }
        }
    }
}




