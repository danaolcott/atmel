/*
 * spi_driver.h
 *
 * Created: 6/4/2018 12:09:58 AM
 *  Author: danao

 Configure SPI to run on PD20, PD21, PD22, and PD25
 These are the same pins on the arduino headers
 as D10, D11, D12, and D13


 */ 


#ifndef SPI_DRIVER_H_
#define SPI_DRIVER_H_


//////////////////////////////////////////////
//SPI Defines
#define SPI_MASTER_BASE      SPI0
#define SPI_CHIP_SEL 0							//NA
#define SPI_CHIP_PCS spi_get_pcs(SPI_CHIP_SEL)	//NA
#define SPI_CLK_POLARITY 1						//idle clock low
#define SPI_CLK_PHASE 0							//0 = trailing edge, 1 = leading
#define SPI_DLYBS 0x40							//delay before clock - NA
#define SPI_DLYBCT 0x10							//delay between consecutive transfers
#define	MASTER_MODE   0
#define SLAVE_MODE	  1

void SPI_Config(void);			//configure SPI peripheral
void SPI_Select(void);			//CS pin
void SPI_Deselect(void);		//CS pin
void SPI_tx(uint8_t data);
uint8_t SPI_rx(void);
void SPI_writeByte(uint8_t data);
void SPI_writeArray(uint8_t* data, uint16_t length);



#define ioport_set_port_peripheral_mode(port, masks, mode) \
do {\
	ioport_set_port_mode(port, masks, mode);\
	ioport_disable_port(port, masks);\
} while (0)


#define ioport_set_pin_peripheral_mode(pin, mode) \
do {\
	ioport_set_pin_mode(pin, mode);\
	ioport_disable_pin(pin);\
} while (0)





#endif /* SPI_DRIVER_H_ */