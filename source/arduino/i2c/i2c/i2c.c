/*
I2C Driver file

See page 183 in datasheet
PC4 and PC5 - SDA and SCL (Labeled as analog pins A4 and A5)

page 199


*/

#include <stdio.h>
#include <stdint.h>
#include <avr/io.h>         //macros

#include "i2c.h"
#include "register.h"



void i2c_delay(unsigned long t)
{
	volatile unsigned long val = t;
	while (val > 0)
		val--;
}

/////////////////////////////////////////////////
//Configure pins XXX as i2c, two wire serial
//See page 68 of datasheet
//Set TWEN bit in the TWCR register to 1 to
//enable the 2-wire serial interface
//This disconnects PC4 and PC5 from port c
//and enables 2 wire periheral.
void i2c_init(void)
{
	//configure the prescaler bits = 0 and 1
	//on the TWSR register, remaining bits
	//are read only

	TWSR |= 0x01;	//prescale 4


	//configure the .... TWBR - TW bit rate generator
	//see 21.5.2, standard i2c speed = 100kbits/sec
	//cpu = 16000000, prescale = 16

	//scl = cpu / (16 + (2 x TWBR x prescaler))
	//scl ~ 100kbits/sec for TWBR = 4
//	TWBR = 0x10;

	TWBR = 0x4;		//0x03 works

//	TWBR = 0x0A;


}


void i2c_errorHandler(void)
{
	//while (1){}
	PORTB_DATA_R &=~ BIT0;
}


//////////////////////////////////////////////////
//send array of size length over i2c
//bus to slave address add.
//following along with Table 21-2 in datasheet
void i2c_writeArray(uint8_t add, uint8_t* data, uint16_t length)
{
	int result = 0x00;
	int i;

	//start condition
	result = i2c_startCondition();

	//setup for master write to slave
	result = i2c_addressWrite(add);

	if (length == 1)
	{
		result = i2c_writeDataByteAck(*data);
		i2c_stopCondition();
	}
	else if (length == 2)
	{
		result = i2c_writeDataByteAck(data[0]);
		result = i2c_writeDataByteAck(data[1]);
		i2c_stopCondition();
	}

	else
	{
		//write first length - 1 bytes with ack
		for (i = 0 ; i < (length - 1) ; i++)
			result = i2c_writeDataByteAck(data[i]);

		//send the last byte
		result = i2c_writeDataByteNack(data[length - 1]);
		i2c_stopCondition();
	}


}

void i2c_MemoryWrite(uint8_t add, uint8_t* memAdd, uint8_t numAddBytes,
		uint8_t* memData, uint16_t numMemBytes)
{

}
  
void i2c_MemoryRead(uint8_t add, uint8_t* memAdd, uint8_t numAddBytes,
		uint8_t* memData, uint16_t numMemBytes)
{

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
   // TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN) | (1 << TWEA);
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);


    //wait till the TWINT bit is set
    //to indicate a start condition complete
    while (!(TWCR & (1 << TWINT)))

    //check TWI status doe an error condition
    //in the TWSR register
    if ((TWSR & 0xF8) != START_CONDITION)
    {
        i2c_errorHandler();
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
        i2c_errorHandler();
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

    //twint is not set on the stop condition
    //according to some reading.
    //check the stop bit
   // while ((TWCR & (1 << TWSTO)))
   // {}


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
    //TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);

    TWCR = (1 << TWINT) | (1 << TWEN);

    //wait until address has been sent
    while (!(TWCR & (1 << TWINT)))
    {}


    //i2c_delay(50);

    //read the status - check for MT_SLA_ACK
    //in the top 5 bits, ignoring prescale bits
    if ((TWSR & 0xF8) != MT_SLA_ACK)
    {
        i2c_errorHandler();
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
    {}

    //read status - check for a read ack
    //in top 5 bits...
    if ((TWSR & 0xF8) != MR_SLA_ACK)
    {
        i2c_errorHandler();
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
//    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);

    TWCR = (1 << TWINT) | (1 << TWEN);

    //wait until data has been sent
    while (!(TWCR & (1 << TWINT)))
    {}

    //read status - check for a data write ack
    if ((TWSR & 0xF8) != MT_DATA_ACK)
    {
        i2c_errorHandler();
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
    {}

    //read status - check for a data write ack
    if ((TWSR & 0xF8) != MR_DATA_ACK)
    {
        i2c_errorHandler();

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
    {}

    //read status - check for a data write ack
    if ((TWSR & 0xF8) != MT_DATA_NACK)
    {
        i2c_errorHandler();
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
    {}

    //read status - check for a data write ack
    if ((TWSR & 0xF8) != MR_DATA_NACK)
    {
        i2c_errorHandler();

        *data = 0xFF;
        return -1;
    }

    *data = TWDR;

    return 0;

}

