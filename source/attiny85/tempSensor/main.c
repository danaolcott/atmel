////////////////////////////////////////////////////
/*
Programming the Atmel ATtimy85 processor.
Dana Olcott
2/1/18

Temperature Sensor State Machine Program
using the TMP36GZ temp sensor.

Press the user button to set a base temperature 
reading.  While in a certain state, the red 
led flashes number of deg hotter than the base 
temp.  The blue led flashes the number of measured
degrees cooler than the base temp.  ie, it's a 
relative temperature measurment device.

Hardware config:
LEDs on PB3 and PB4 as output - Red = PB3, Blue = PB4
Buttons on PB0 and PB1 as input with interrupts.
Timer0 as OCC with interrupt trigger at 1khz.
ADC Channel ADC1 on PB2 (Pin 7)

Some added notes / components:
Add load resistor from analog out to ground - 22k
Add cap from hot to ground

General FSM Definition:

State_t struct:
stateID                -index in the state table, 0,1, ...
frequency              -delay routine for state evaluation
*fptr                  -run to completion function
nextState[]            -array of next states

State0: Calibration state - press and hold user
        button to set the reference temp.  Stores
        reference temp in EEPROM

State1: Relative meaurement from referene temp.
        Flashes red if current measurement is higher
        than ref by number of deg F.  Flashes blue if
        current measurement is lower than ref by number 
        of deg F.

State2: Abs temp measured from 70 deg.  Flashes red
        number of deg. offset from 70.  Flashes blue
        number of deb. cooler than offsets from 70.

State3: Flashes red led indicating the ADC reading
        at the analog out pin on the temp sensor.  ie,
        if analog out is 0.5 volts, it flashes red 5 times.
        Blue led is low if ADC reading less than mid-scale 
        (ie, < 512) and high if above.

User Buttons:
        User button 1 changes state
        User button 2 valid in State 0, used to 
        set the reference temp.


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

//ADC
#define ADCMUX_R         (*((volatile unsigned char*)(0x07 + IO_OFFSET)))
#define ADCSRA_R         (*((volatile unsigned char*)(0x06 + IO_OFFSET)))
#define ADCH_R           (*((volatile unsigned char*)(0x05 + IO_OFFSET)))       //bits 01 - top 2 in 10bit
#define ADCL_R           (*((volatile unsigned char*)(0x04 + IO_OFFSET)))       //bits 0-7


//EEPROM
#define EEARH_R           (*((volatile unsigned char*)(0x1F + IO_OFFSET)))       //MSB, bit 0
#define EEARL_R           (*((volatile unsigned char*)(0x1E + IO_OFFSET)))       //EE addr low
#define EEDR_R           (*((volatile unsigned char*)(0x1D + IO_OFFSET)))       //data register
#define EECR_R           (*((volatile unsigned char*)(0x1C + IO_OFFSET)))       //EE addr low

//EEPROM address definitions
#define EEPROM_REF_ADC_LOW     ((uint8_t)0x10)
#define EEPROM_REF_ADC_HIGH     ((uint8_t)0x11)

//temp defines
#define DEFAULT_REF_TEMP        ((unsigned int)70)

//////////////////////////////////////////
//FSM items
//
#define FSM_NUM_STATES      4

//updated in the button isr
volatile unsigned char flashState = 0x00;

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

//////////////////////////////////////////////////
//State Machine Definition
//Simple state pattern - all states have next
//state in sequence. ie, 0 to 1, 1 to 2.. etc.
//
State_t fsm[FSM_NUM_STATES] = 
{
    {STATE_0, 10, function0, {STATE_0, STATE_1, STATE_2, STATE_3}},
    {STATE_1, 10, function1, {STATE_0, STATE_1, STATE_2, STATE_3}},
    {STATE_2, 10, function2, {STATE_0, STATE_1, STATE_2, STATE_3}},
    {STATE_3, 10, function3, {STATE_0, STATE_1, STATE_2, STATE_3}},
};

State_t currentState;


////////////////////////////////////////////////
//ADC Variables, flags... etc
//gReferenceADC reading = base reading
//for all comparisons
static unsigned int gReferenceADC;
static float gReferenceMV;
static float gReferenceTEMP;



///////////////////////////////////////////////////
//Delay items
void Delay(volatile unsigned int val);
static volatile unsigned long gTimeTick = 0x00;


void GPIO_init(void);               //GPIO - leds
void Button_init(void);             //Buttons PB0 and PB1
void Timer0_init(void);             //Time tick
void Timer0_OCCA_init(void);        //Time tick
void ADC1_init(void);               //ADC1 on PB2 - Pin 7
unsigned int ADC1_read(void);
void EEPROM_init(void);             //EEPROM
void EEPROM_write(uint8_t address, uint8_t data);
uint8_t EEPROM_read(uint8_t address);

//Button debounce
void Waste_CPU(unsigned int temp);
void Dummy_Function(void);

//Sorting
void SortArray(unsigned int *data, unsigned int size);

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
//
ISR(PCINT0_vect)
{
    unsigned char val0, val1 = 0x00;
    Waste_CPU(1000);

    //check button press after killing cpu time
    val0 = PINB_R & BIT0;
    val1 = PINB_R & BIT1;   //left

    //PB0 - down - do nothing
    if (!val0)
    {
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



////////////////////////////////////////////////
//Main loop
//
int main()
{
    uint16_t low, high;
    
    ///////////////////////////////////////////
    //initialize peripherals
    GPIO_init();
    Timer0_OCCA_init();
    Button_init();
    ADC1_init(); 
    EEPROM_init();

    ////////////////////////////////////////////
    //Get base readings from EEPROM
    low = (EEPROM_read(EEPROM_REF_ADC_LOW)) & 0xFF;
    high = (EEPROM_read(EEPROM_REF_ADC_HIGH)) & 0xFF;
    high = high << 8;

    gReferenceADC = high | low;
    gReferenceMV = ((float)gReferenceADC / 1024.0) * 1100.0;
    gReferenceTEMP = (gReferenceMV - 500.0) / 10.0;     //centigrade
    gReferenceTEMP = ((gReferenceTEMP * 9.0 / 5.0) + 32.0);


    ///////////////////////////////////////////////////
    //Run the state machine
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
//PB0 and PB1 on interrupts, no pull.
//Buttons have 10k pullup added.
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




//////////////////////////////////////////////////
//Configure Timer0 with Overflow Interrupt
//
void Timer0_init(void)
{  
    //Timer Control - 2 registers:
    //TCCR0A - output compare modes
    //Section 11.9.2
    //bits 7-4 - no output compare
    //no waveform gen - 00
    TCCR0A_R = 0x00;

    //TCCR0B - prescale
    //    000 - no clock - timer disabled
    //    001 - no prescale
    //    010 - clk/8
    //    011 - clk/64
    //    100 - clk/256
    //    101 - clk/1024

    TCCR0B_R |= BIT0;

    //TIMSK - int mask reg - bit 1 overflow interrupt en.
    TIMSK_R |= BIT1;
    
    //clear interrupt
    //TIFR - overflow flag - TOV0 - bit 1 
    TIFR_R |= BIT1;

    //enable global interrupts
    _SEI;

}





//////////////////////////////////////////////
//Timer0 - Compare Capture
//Generates an interrupt at 1khz
//Run Timer0 as Compare Capture
//Alternative to previous configuration.
//
void Timer0_OCCA_init(void)
{
    //Timer Control - 2 registers:
    //TCCR0A - output compare modes
    //Section 11.9.2
    //bits 7-4 - no output compare
    //no waveform gen - 00
    TCCR0A_R = 0x00;

    //TCCR0B - prescale - no prescale
    //    000 - no clock - timer disabled
    //    001 - no prescale
    //    010 - clk/8
    //    011 - clk/64
    //    100 - clk/256
    //    101 - clk/1024
    TCCR0B_R |= BIT1;

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




///////////////////////////////////////////
//State0 - Function to Run
//
//Calibration State
//Press and hold user button to store the 
//current ADC1 reading as the reference.
//If stored, the LEDs will alternate fast
//for 10 times.
//
void function0(void)
{
    unsigned char i, dataL, dataH;
    
    if (!(PINB_R & BIT0))
    {
        gReferenceADC = (ADC1_read() & 0x3FF);
        gReferenceMV = ((float)gReferenceADC / 1024.0) * 1100.0;
        gReferenceTEMP = (gReferenceMV - 500.0) / 10.0;     //centigrade
        gReferenceTEMP = ((gReferenceTEMP * 9.0 / 5.0) + 32.0);

        //write the reference ADC to eeprom
        dataH = (gReferenceADC >> 8) & 0xFF;
        dataL = gReferenceADC & 0xFF;

        EEPROM_write(EEPROM_REF_ADC_HIGH, dataH);
        EEPROM_write(EEPROM_REF_ADC_LOW, dataL);
              
        //flash something to indicate stored
        for (i = 0 ; i < 10 ; i++)
        {
            PORTB_DATA_R |= BIT3;
            PORTB_DATA_R &=~ BIT4;
            Delay(100);
            PORTB_DATA_R |= BIT4;
            PORTB_DATA_R &=~ BIT3;
            Delay(100);
        }
    }


    PORTB_DATA_R |= BIT3;
    PORTB_DATA_R &=~ BIT4;
    Delay(500);
    PORTB_DATA_R |= BIT4;
    PORTB_DATA_R &=~ BIT3;
    Delay(500);

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
}


////////////////////////////////////////////////
//State2 - Function to Run
//
//Absolute Temperature Display
//Flash blue or red led indicating the 
//temperature difference from 70 deg. F.
//For ex: 74, flash red led 4 times.
//
void function2(void)
{
    int diff = 0;
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
}



///////////////////////////////////////////////////
//State3 - Function to Run
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
void function3(void)
{
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
        
    Delay(2000);
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




/////////////////////////////////////////////////////////////
//EEPROM_init()
//Configure EEPROM registers for storing data.
//
//General summary of how it works:
//EEDR_R  - contains the data for read or write
//EEARH_R - MSB bit of the address, set to 0
//EEARL_R - lower 8 bits of address

//EECR_R - EE control register
//bits 7 - read - reserved
//bits 6 - read - reserved
//bits 5-4 - prog. mode - set 00 to do erase and write in 1 opp.
//bits 3 - IE - interrupt enable - set to 0
//bits 2 - EEMPE - master program enable - set to 1 then write
//         EEPE to 1 within 4 clock cycles to program EEPROM
//bits 1 - EEPE - write to 1 within 4 clock cycles to program
//         EEPROM.  Data is written to the specified address
//bits 0 - EERE - read enable bit.  Set to one to read the data at
//         the specified address.  Poll the EEPE bit prior to
//         reading to to make sure a write is not in progress.  
//         Data is available at the specified address immediately
//         after setting the read bit.
//
//General Notes / Obsevations:
//
//Disable all interrupts during any
//read/write opperation using _SEI and _CEI opperations.
//
//Reflashing the Attiny85 using ISP over SPI erases the EEPROM.
//
//
void EEPROM_init(void)
{
    _CEI;               //disable interrupts
    EEARH_R &=~ BIT0;   //set address to 0x00
    EEARL_R = 0x00;     //set address to 0x00
 
   //programming mode - erase and write in 1 opperation
    EECR_R &=~ BIT5;    //prog. mode
    EECR_R &=~ BIT4;    //prog. mode
    EECR_R &=~ BIT3;    //disable interrupts

    //poll EEPE bit1 to make sure no write is taking place
    while (EECR_R & BIT1){};

    _SEI;               //enable interrupts
}


///////////////////////////////////////////////////
//EEPROM write
//Process:
//disable interrupts
//poll EEPE bit 1 in the control reg
//write address to address low.
//write data to EEDR_R
//write 1 to EEMPE - bit 2
//write 1 to EEPE - bit 1 within 4 clock cycles
//poll the EEPE bit until write complete
//enable interrupts
void EEPROM_write(uint8_t address, uint8_t data)
{
    _CEI;

    //poll EEPE bit1 to complete write opperation
    while (EECR_R & BIT1){};

    EECR_R &=~ BIT5;        //prog. mode - erase and write
    EECR_R &=~ BIT4;        //prog. mode - erase and write

    EEARH_R &=~ BIT0;       //address high - 0, max 0xFF
    EEARL_R = address;      //address low - address

    EEDR_R = data;          //write the data

    EECR_R |= BIT2;         //set bit 2 - master prog. enable
    EECR_R |= BIT1;         //set bit 1 - EEPE within 4 clock cycles.

    while (EECR_R & BIT1){};    //poll EEPE bit to complete write

    _SEI;                   //enable interrupts
}

////////////////////////////////////////////////////
//EEPROM read
//Process:
//disable interrupts
//poll EEPE bit 1 in EECR_R to confirm no write in progress
//set the target address.
//set the EERE bit 0 in EECR_R reg.
//data is read in the EEDR_R
//enable interrupts
//
uint8_t EEPROM_read(uint8_t address)
{
    _CEI;                       //disable interrupts

    while (EECR_R & BIT1){};    //poll the EEPE bit

    EEARH_R &=~ BIT0;           //set address high 0 (restrict 0xFF max)
    EEARL_R = address;          //set address low - max 0xFF
    EECR_R |= BIT0;             //read enable bit 0 high

    _SEI;                       //enable interrupts

    return EEDR_R;
}







