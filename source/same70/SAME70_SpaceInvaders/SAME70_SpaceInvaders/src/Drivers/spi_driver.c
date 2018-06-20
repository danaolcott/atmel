/*
 * spi_driver.c
 *
 * Created: 6/4/2018 12:09:37 AM
 *  Author: danao

 Configure SPI to run on PD20, PD21, PD22, and PD25
 These are the same pins on the arduino headers
 as D10, D11, D12, and D13


 */ 

 #include "asf.h"
 #include "stdio_serial.h"
 #include "conf_board.h"
 #include "conf_clock.h"

 #include "spi.h"
 #include "spi_driver.h"


 static uint32_t gs_ul_spi_clock = 500000;		//SPI clock speed - use 500khz


 ////////////////////////////////////////////////////////
 //Configure SPI peripheral to run on
 //PD20, PD21, PD22, PD25
 //Same as arduino header pins, D10 - D13
 //Configure CS pin as normal GPIO
 //
 void SPI_Config(void)
 {
	 //configure gpio - PD20, 21, 22
	 //see same70_xplained.h for definitions
	 ioport_set_pin_peripheral_mode(SPI0_MISO_GPIO, SPI0_MISO_FLAGS);
	 ioport_set_pin_peripheral_mode(SPI0_MOSI_GPIO, SPI0_MOSI_FLAGS);
	 ioport_set_pin_peripheral_mode(SPI0_SPCK_GPIO, SPI0_SPCK_FLAGS);

	 //cs pin - PD25 - output, start high - D10
	 ioport_set_pin_dir(PIO_PD25_IDX, IOPORT_DIR_OUTPUT);
	 ioport_set_pin_level(PIO_PD25_IDX, true);

	 spi_enable_clock(SPI_MASTER_BASE);
	 spi_disable(SPI_MASTER_BASE);
	 spi_reset(SPI_MASTER_BASE);
	 spi_set_lastxfer(SPI_MASTER_BASE);
	 spi_set_master_mode(SPI_MASTER_BASE);
	 spi_disable_mode_fault_detect(SPI_MASTER_BASE);
	 //	spi_set_peripheral_chip_select_value(SPI_MASTER_BASE, SPI_CHIP_PCS);
	 spi_set_clock_polarity(SPI_MASTER_BASE, SPI_CHIP_SEL, SPI_CLK_POLARITY);
	 spi_set_clock_phase(SPI_MASTER_BASE, SPI_CHIP_SEL, SPI_CLK_PHASE);
	 spi_set_bits_per_transfer(SPI_MASTER_BASE, SPI_CHIP_SEL,
	 SPI_CSR_BITS_8_BIT);

	 //peripheral clock - returns 150mhz
	 //for 1mhz, use div = 150, or
	 volatile uint32_t peripheralClock = 0x00;
	 peripheralClock = sysclk_get_peripheral_hz();

	 spi_set_baudrate_div(SPI_MASTER_BASE, SPI_CHIP_SEL, (peripheralClock / gs_ul_spi_clock));
	 spi_set_transfer_delay(SPI_MASTER_BASE, SPI_CHIP_SEL, SPI_DLYBS,
	 SPI_DLYBCT);
	 spi_enable(SPI_MASTER_BASE);
 }


 void SPI_Select()
 {
	 ioport_set_pin_level(PIO_PD25_IDX, false);

 }

 void SPI_Deselect()
 {
	 ioport_set_pin_level(PIO_PD25_IDX, true);
 }


 ////////////////////////////////////////////////////
 //write 1 byte to SPI0.
 //checks for tx and rx empty
 void SPI_tx(uint8_t data)
 {
	 //check if tx is empty
	 while (!spi_is_tx_empty(SPI0));		//check if tx is empty
	 while (!spi_is_tx_ready(SPI0));		//check if tx is empty
	 spi_put(SPI0, data);				//write the data to SPI0
	 while (!spi_is_tx_ready(SPI0));		//check if tx is empty
	 while (!spi_is_rx_ready(SPI0));		//ready to receive... ie, all data is shifted out
 }

 ///////////////////////////////////////
 //get one byte from spi0
 uint8_t SPI_rx()
 {
	 //wait while rx is not ready
	 while (!spi_is_rx_ready(SPI0));		//check if rx is empty
	 while (!spi_is_tx_ready(SPI0));		//check if tx is empty

	 uint16_t data = spi_get(SPI0);

	 //wait until spi rx is full
	 while (!spi_is_rx_full(SPI0));

	 return data;

 }


 //////////////////////////////////////////////////
 //send 1 byte over spi, including cs
 void SPI_writeByte(uint8_t data)
 {
	 SPI_Select();
	 SPI_tx(data);
	 SPI_Deselect();
 }

 void SPI_writeArray(uint8_t* data, uint16_t length)
 {
	 SPI_Select();

	 for (int i = 0 ; i < length ; i++)
	 SPI_tx(data[i]);
	 
	 SPI_Deselect();
 }





