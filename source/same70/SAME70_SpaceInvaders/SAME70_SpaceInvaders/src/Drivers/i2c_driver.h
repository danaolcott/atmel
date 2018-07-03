/*
 * i2c_driver.h
 *
 * Created: 6/21/2018 2:23:34 AM
 *  Author: danao
 */ 


#ifndef I2C_DRIVER_H_
#define I2C_DRIVER_H_

#define I2C_BUFFER_SIZE		64

/** Definition of TWI interrupt ID on board. */
#define BOARD_TWIHS_IRQn          TWIHS2_IRQn
#define BOARD_TWIHS_Handler    TWIHS2_Handler

/** Configure TWI2 pins */
#define CONF_BOARD_TWIHS0



/** EEPROM Wait Time */
#define WAIT_TIME   10
/** TWI Bus Clock 400kHz */
#define TWIHS_CLK     400000
/** Address of AT24C chips */
//depending on where you look, the address is one of the
//ones listed below....
//#define AT24C_ADDRESS           0x40		//this one?
//#define AT24C_ADDRESS           0x50		//base address - a0, a1, a2 floating
//#define AT24C_ADDRESS           0x37		//datasheet - error

#define AT24C_ADDRESS           (0x57)		//datasheet

#define EEPROM_MEM_ADDR         0
#define EEPROM_MEM_ADDR_LENGTH  1


//////////////////////////////////////////////////
//located on I2C 0
//
/** TWI ID for simulated EEPROM application to use */
#define BOARD_ID_TWIHS_EEPROM         ID_TWIHS0
/** TWI Base for simulated TWI EEPROM application to use */
#define BOARD_BASE_TWIHS_EEPROM       TWIHS0


void I2C_DummyDelay(uint32_t delay);
void I2C_Config(void);
void I2C_EEPROM_WP_Enable(void);
void I2C_EEPROM_WP_Disable(void);
void I2C_EEPROM_Write(uint16_t memaddress, uint8_t memaddresslength, uint8_t* data, uint16_t length);
void I2C_EEPROM_Read(uint16_t memaddress, uint8_t memaddresslength, uint8_t* data, uint16_t length);

#endif /* I2C_DRIVER_H_ */