/*
1/6/18
Dana Olcott

I2C Driver file
Function definitions for control of
I2C peripheral on the ATMega328p processor.

Set up as master using pins PC4 and PC5
as SCL and SDA lines.  These are labeled
as A4 and A5 on the Arduino Uno.  Clock
configured to run at about 400khz (not exactly).

Test functions provided for testing the I2C interface
using the TSL2561 light sensor breakout board from
Adafruit.

Notes:
The light sensor was pretty sensitive to the
clock speed.  If not set right, the nack would
get set and terminate the transmision.  Will
need to test this with other ICs with i2c.

*/

#include <stdio.h>
#include <stdint.h>
#include <avr/io.h>         //macros
#include "i2c.h"
#include "register.h"


/////////////////////////////////////
void i2c_delay(unsigned long t)
{
	volatile unsigned long val = t;
	while (val > 0)
		val--;
}

/////////////////////////////////////////////////
//Configure I2C peripheral.
//Uses pins PC4 and PC5 as scl and sda.
//Setting the TWEN bit in the TWCR register to 1
//enables i2c and disconnects portc.  This bit
//is set on every function call to i2c.  Therefore,
//just set up the clock speeds here.
//
void i2c_init(void)
{
	//configure the prescaler bits = 0 and 1
	//on the TWSR register, remaining bits
	//are read only

	//the following clock settings work well for the
	//tsl2561 light sensor, clock speed just under 400khz.
	//

	//TWSR |= 0x01;	//prescale 4
	//TWBR = 0x4;		//0x03 works


	//clock looks really jittery when using
	//a different i2c, so try changing this...

	//  0101 0010 101

	TWSR |= 0x01;	//prescale 4

	//configure the .... TWBR - TW bit rate generator
	//see 21.5.2, standard i2c speed = 100kbits/sec
	//cpu = 16000000, prescale = 16

	//scl = cpu / (16 + (2 x TWBR x prescaler))
	//scl ~ 100kbits/sec for TWBR = 4
//	TWBR = 0x10;

	TWBR = 0x2;		//0x03 works

//	TWBR = 0x0A;


}

/////////////////////////////////////////////
//error_handler.
//Called with value on TWSR does not match
//expected values listed in the datatsheet
//after an i2c opperation is performed.
//code - status code
//value - actual read value in TWSR
//target = desired value read from TWSR
//Note: top 5 bits of TWSR captured
//
void i2c_errorHandler(I2C_StatusCode_t code, uint8_t value)
{
	/*
	switch(code)
	{
		case I2C_STATUS_START: 			i2c_errorFlash((uint8_t)code, 1000);	break;
		case I2C_STATUS_RESTART: 		i2c_errorFlash((uint8_t)code, 1000);	break;
		case I2C_STATUS_MT_SLA_ACK: 	i2c_errorFlash((uint8_t)code, 1000);	break;
		case I2C_STATUS_MT_SLA_NACK: 	i2c_errorFlash((uint8_t)code, 1000);	break;
		case I2C_STATUS_MT_DATA_ACK: 	i2c_errorFlash((uint8_t)code, 1000);	break;
		case I2C_STATUS_MT_DATA_NACK: 	i2c_errorFlash((uint8_t)code, 1000);	break;
		case I2C_STATUS_MR_SLA_ACK: 	i2c_errorFlash((uint8_t)code, 1000);	break;
		case I2C_STATUS_MR_SLA_NACK: 	i2c_errorFlash((uint8_t)code, 1000);	break;
		case I2C_STATUS_MR_DATA_ACK: 	i2c_errorFlash((uint8_t)code, 1000);	break;
		case I2C_STATUS_MR_DATA_NACK: 	i2c_errorFlash((uint8_t)code, 1000);	break;
		default:						i2c_errorFlash(1, 10000);				break;
	}
	*/

}



////////////////////////////////////
//flash a pin numFlashes with delay
//for now, use pin 8, or port b, bit 0
void i2c_errorFlash(uint8_t numFlashes, uint16_t delay)
{
	uint8_t i = 0x00;
	for (i = 0 ; i < numFlashes ; i++)
	{
		PORTB_DATA_R |= BIT0;
		i2c_delay(delay);
		PORTB_DATA_R &=~ BIT0;
		i2c_delay(delay);
	}
}





////////////////////////////////////////////
//TSL2561 Light Sensor
//Test function to read product code.
//returns value 0x50
//
uint8_t i2c_lightSensorCode(void)
{
	uint8_t readCommandByte = 0x80; 		//1000 + ...
	uint8_t productCode = readCommandByte | 0x0A;
	uint8_t data = 0x00;

	//start condition
	i2c_startCondition();
	//setup for master write to slave
	i2c_addressWrite(I2C_ADDRESS);
	//send the product code address - 0x0A - See Table 2
	//page 13 in datasheet
	i2c_writeDataByteAck(productCode);
	//resend start and set up for a read
	i2c_startRepeatCondition();
	//send slave address as a read
	i2c_addressRead(I2C_ADDRESS);
	//read the byte
	i2c_readDataByteNack(&data);
	i2c_stopCondition();

	return data;
}


/////////////////////////////////////////
//TSL2561 Light Sensor
//Generic read function
//
//Read size number bytes starting at address
//add (0x00 - 0x0F).  If reading a single byte,
//it issues byte read command (0x80 | Add).
//For multiple byte reads, it issues the block
//read command (0x90 | Add).
//
void i2c_lightSensorRead(uint8_t add, uint8_t* data, uint8_t size)
{

	uint8_t commandByte = 0x80;
	uint8_t commandBlock = 0x90;
	uint8_t i = 0x00;
	uint8_t t = 0x00;

	//default single byte read
	uint8_t start = commandByte | (add & 0x0F);

	//reading many bytes - set up for block read
	if (size > 1)
		start = commandBlock | (add & 0x0F);

	//set up for a write to slave at starting address

	i2c_startCondition();			//start condition
	i2c_addressWrite(I2C_ADDRESS);	//setup master write
	i2c_writeDataByteAck(start);	//send starting address - ack
	i2c_startRepeatCondition();		//resend start condition
	i2c_addressRead(I2C_ADDRESS);	//send slave address as a read

	//read # size bytes into *data

	//single byte - read with nack
	if (size == 1)
		i2c_readDataByteNack(data);

	//2 bytes - 1 ack, 1 nack
	else if (size == 2)
	{
		i2c_readDataByteAck(data++);
		i2c_readDataByteNack(data);
	}

	//many bytes - read all with ack up to
	//size -1, read last as nack
	else
	{
		for (i = 0 ; i < size - 1 ; i++)
		{
			i2c_readDataByteAck(&t);
			data[i] = t;
		}

		i2c_readDataByteNack(&t);
		data[size - 1] = t;
	}

	//stop condition
	i2c_stopCondition();
}




/////////////////////////////////////////
//TSL2561 Light Sensor
//Generic write function
//
//Read size number bytes starting at address
//add (0x00 - 0x0F).  If reading a single byte,
//it issues byte read command (0x80 | Add).
//For multiple byte reads, it issues the block
//read command (0x90 | Add).
//
void i2c_lightSensorWrite(uint8_t add, uint8_t* data, uint8_t size)
{
	uint8_t commandByte = 0x80;
	uint8_t commandBlock = 0x90;
	uint8_t i = 0x00;

	//default single byte
	uint8_t start = commandByte | (add & 0x0F);

	//many bytes - set up for block w/r
	if (size > 1)
		start = commandBlock | (add & 0x0F);

	//set up for a write to slave at starting address
	i2c_startCondition();			//start condition
	i2c_addressWrite(I2C_ADDRESS);	//setup master write
	i2c_writeDataByteAck(start);	//send starting address - ack

	//send the data - no need for a restart since
	//no change in direction
	for (i = 0 ; i < size ; i++)
		i2c_writeDataByteAck(data[i]);

	//stop condition
	i2c_stopCondition();
}


/////////////////////////////////////////////////
//Write data array to i2c device starting at address
//i2c Address: add
//address size in bytes: numAddBytes
//starting address = *memAdd
//data size = numMemBytes
//data = memData
//
//Note: will need a memory chip to test the multibyte
//address read/write
void i2c_MemoryWrite(uint8_t add, uint8_t* memAdd, uint8_t numAddBytes,
		uint8_t* memData, uint16_t numMemBytes)
{
	uint8_t i = 0x00;

	//setup for a write to slave address memAdd
	i2c_startCondition();			//start condition
	i2c_addressWrite(add);	//setup master write

	//send the memory address
	for (i = 0 ; i < numAddBytes ; i++)
		i2c_writeDataByteAck(memAdd[i]);

	//send the data - no direction change
	for (i = 0 ; i < numMemBytes ; i++)
		i2c_writeDataByteAck(memData[i]);

	//stop condition
	i2c_stopCondition();
}

/////////////////////////////////////////////////
//Read data array to i2c device starting at address
//i2c Address: add
//address size in bytes: numAddBytes
//starting address = *memAdd
//data size = numMemBytes
//data = memData
//
//Note: will need a memory chip to test the multibyte
//address read/write
void i2c_MemoryRead(uint8_t add, uint8_t* memAdd, uint8_t numAddBytes,
		uint8_t* memData, uint16_t numMemBytes)
{
	uint8_t i = 0x00;
	uint8_t t = 0x00;

	//set up for a write to slave at starting address
	i2c_startCondition();			//start condition
	i2c_addressWrite(add);	//setup master write

	//send the memory address bytes with ack
	for (i = 0 ; i < numAddBytes ; i++)
		i2c_writeDataByteAck(memAdd[i]);

	//send the start to change directions
	i2c_startRepeatCondition();
	i2c_addressRead(add);

	//read #memBytes into memData
	//1 byte - read with a nack
	if (numMemBytes == 1)
		i2c_readDataByteNack(memData);

	//multibyte - read all but last
	//with ack, last byte with nack
	else
	{
		for (i = 0 ; i < numMemBytes - 1 ; i++)
		{
			i2c_readDataByteAck(&t);
			memData[i] = t;
		}

		i2c_readDataByteNack(&t);
		memData[numMemBytes - 1] = t;
	}

	//stop condition
	i2c_stopCondition();
}


////////////////////////////////
//start condition:
//clock high with falling data line
//Table 21-2
int i2c_startCondition(void)
{
    //clear the interrupt flag by writing a 1 to the TWINT bit
    //enable the start condition
    //enable the i2c lines
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN) | (1 << TWEA);

    //wait till the TWINT bit is set
    //to indicate a start condition complete
    while (!(TWCR & (1 << TWINT)))

    //check TWI status for an error condition
    //in the TWSR register
    if ((TWSR & 0xF8) != START_CONDITION)
    {
        i2c_errorHandler(I2C_STATUS_START, (TWSR & 0xF8));
        return -1;
    }

    return 0;
}



////////////////////////////////
//repeated start condition:
//Table 21-2
int i2c_startRepeatCondition(void)
{
    //clear the interrupt flag by writing a 1 to the TWINT bit
    //enable the start condition
    //enable the i2c lines
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN) | (1 << TWEA);

    //wait till the TWINT bit is set
    //to indicate a start condition complete
    while (!(TWCR & (1 << TWINT)))

    //check TWI status doe an error condition
    //in the TWSR register
    if ((TWSR & 0xF8) != START_REPEAT_CONDITION)
    {
        i2c_errorHandler(I2C_STATUS_RESTART, (TWSR & 0xF8));
        return -1;
    }

    return 0;
}




////////////////////////////////////
//stop condition - high clock with
//a rising data line
void i2c_stopCondition(void)
{
    //clear the TWINT flag, enable the i2c
    //lines, and set the stop condition bit
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
}

//////////////////////////////////////////////
//Initiate connection with slave address
//indicting a write (bit 0 = 0)
//clear the TWINT flag to send address
//NOTE:  address is assumed to be upshifted!!
//ie, the address bit 0 is not populated
//or a don't care.
int i2c_addressWrite(uint8_t address)
{
    //put address into TWDR
    TWDR = address;

    //Clear the int flag enable the i2c lines
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);

    //wait until address has been sent
    while (!(TWCR & (1 << TWINT)))

    //read the status - check for MT_SLA_ACK
    //in the top 5 bits, ignoring prescale bits
    if ((TWSR & 0xF8) != MT_SLA_ACK)
    {
        i2c_errorHandler(I2C_STATUS_MT_SLA_ACK, (TWSR & 0xF8));
        return -1;
    }

    return 0;
}


///////////////////////////////////////
//Initialize comunication with 
//slave as a read opperation
//Similar to a write with bit 0 = 1
//
int i2c_addressRead(uint8_t address)
{
    //bit 0 high for a read
    uint8_t add = address | 0x01;

    //put address into TWDR
    TWDR = add;

    //Clear the int flag enable the i2c lines
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);

    //wait until address has been sent
    while (!(TWCR & (1 << TWINT)))

    //read status - check for a read ack
    //in top 5 bits...
    if ((TWSR & 0xF8) != MR_SLA_ACK)
    {
        i2c_errorHandler(I2C_STATUS_MR_SLA_ACK, (TWSR & 0xF8));
        return -1;
    }

    return 0;
}

///////////////////////////////////////
//send data byte to slave
//and request ack
int i2c_writeDataByteAck(uint8_t data)
{
    //load data into TWDR register
    TWDR = data;

    //Clear the int flag enable the i2c lines
    //to start data transmission
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);

    //wait until data has been sent
    while (!(TWCR & (1 << TWINT)))

    //read status - check for a data write ack
    if ((TWSR & 0xF8) != MT_DATA_ACK)
    {
        i2c_errorHandler(I2C_STATUS_MT_DATA_ACK, (TWSR & 0xF8));
        return -1;
    }

    return 0;
}

//////////////////////////////////////
//read data byte with ack
int i2c_readDataByteAck(uint8_t* data)
{
    //Clear the int flag enable the i2c lines
    //to start data transmission
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);

    //wait until data is received
    while (!(TWCR & (1 << TWINT)))

    //read status - check for a data write ack
    if ((TWSR & 0xF8) != MR_DATA_ACK)
    {
        i2c_errorHandler(I2C_STATUS_MR_DATA_ACK, (TWSR & 0xF8));
        *data = 0xFF;
        return -1;
    }

    *data = TWDR;

    return 0;
}



/////////////////////////////////////
//send data byte to slave
//and request NACK
int i2c_writeDataByteNack(uint8_t data)
{
    //load data into TWDR register
    TWDR = data;

    //Clear the int flag enable the i2c lines
    //to start data transmission
    TWCR = (1 << TWINT) | (1 << TWEN);

    //wait until data has been sent
    while (!(TWCR & (1 << TWINT)))

    //read status - check for a data write nack
    if ((TWSR & 0xF8) != MT_DATA_NACK)
    {
        i2c_errorHandler(I2C_STATUS_MT_DATA_NACK, (TWSR & 0xF8));
        return -1;
    }

    return 0;
}



//////////////////////////////////////
//read data byte with NACK
int i2c_readDataByteNack(uint8_t* data)
{
    //Clear the int flag enable the i2c lines
    //to start data transmission
    TWCR = (1 << TWINT) | (1 << TWEN);

    //wait until data is received
    while (!(TWCR & (1 << TWINT)))

    //read status - check for a data write ack
    if ((TWSR & 0xF8) != MR_DATA_NACK)
    {
        i2c_errorHandler(I2C_STATUS_MR_DATA_NACK, (TWSR & 0xF8));
        *data = 0xFF;
        return -1;
    }

    *data = TWDR;

    return 0;
}
