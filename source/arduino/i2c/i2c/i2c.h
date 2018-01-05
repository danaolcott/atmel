#ifndef __I2C__H_
#define __I2C__H_


//#define LIGHT_SENSOR_I2C_ADDRESS	(((uint8_t)0x39) << 1)
#define LIGHT_SENSOR_I2C_ADDRESS	(((uint8_t)0x29) << 1)


////////////////////////////////////////////////
//TWSR Defines - Return codes on status register
//
#define START_CONDITION             (0x08)
#define START_REPEAT_CONDITION      (0x10)

#define MT_SLA_ACK                  (0x18)
#define MT_SLA_NACK                 (0x20)

#define MT_DATA_ACK                 (0x28)
#define MT_DATA_NACK                (0x30)


#define MR_SLA_ACK                  (0x40)
#define MR_SLA_NACK                 (0x48)

#define MR_DATA_ACK                 (0x50)
#define MR_DATA_NACK                (0x58)


void i2c_delay(unsigned long t);


void i2c_init(void);
void i2c_errorHandler(void);

void i2c_writeArray(uint8_t add, uint8_t* data, uint16_t length);

void i2c_MemoryWrite(uint8_t add, uint8_t* memAdd, uint8_t numAddBytes,
		uint8_t* memData, uint16_t memDataBytes);
  
void i2c_MemoryRead(uint8_t add, uint8_t* memAdd, uint8_t numAddBytes,
		uint8_t* memData, uint16_t memDataBytes);

int i2c_startCondition(void);
int i2c_startRepeatCondition(void);

void i2c_stopCondition(void);
int i2c_addressWrite(uint8_t address);
int i2c_addressRead(uint8_t address);

//write or read a byte with ack or nack bit
int i2c_writeDataByteAck(uint8_t data);
int i2c_readDataByteAck(uint8_t* data);
int i2c_writeDataByteNack(uint8_t data);
int i2c_readDataByteNack(uint8_t* data);






#endif

