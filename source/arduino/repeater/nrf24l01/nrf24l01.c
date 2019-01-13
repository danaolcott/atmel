/*
NRF24L01 Controller File
12/25/18
Dana Olcott

NRF24L01 Transceiver IC.

Configured to be in TX, RX, or Repeater Modes

NOTES:
All pipes are configured to be on
All pipes are configured with ack off
Packet widths are fixed using NRF24_PIPE_WIDTH

Rx Mode:
    RX_DR interrupt is set such that new data pulls the IRQ pin low
    Packets are read until no data is left to read
    Packets processed using the nrf24_processPacket function.

Tx Mode:
    Send data using the nrf24_transmitData function
    Flag is set in the TX_DS interrupt, polled in the tx function

Repeater Mode:
    RX_DR interrupt is set so that new data triggers the IRQ pin.
    The receiver copies the packet into a buffer and sets a flag
    Flag is polled in main to see if data needs to be re-transmitted.


Interface:
    SPI - idle clock low, leading edge, MSB first
    SPI - Pins 10, 11, 12, 13

    IRQ Pin - PD2 - Pin 2 - Interrupt, falling edge, pullup (INT0)
    CE Pin - PB1 - Pin 9 - Normal output




*/

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <avr/interrupt.h>
#include <avr/io.h>         //macros


#include "nrf24l01.h"
#include "register.h"        //GPIO reg defs
#include "spi.h"
#include "usart.h"           //retransmitting out serial port
#include "utility.h"        //print functions


///////////////////////////////////////////////
//NRF24 Global Variables
static NRF24_Mode_t mNRF24_Mode = NRF24_MODE_RX;
static NRF24_State_t mNRF24_State = NRF24_STATE_RX;

static volatile uint8_t mTransmitCompleteFlag = 0;

static volatile uint8_t mRepeaterBuffer[NRF24_PIPE_WIDTH] = {0x00};   //target buffer
static volatile uint8_t mRepeaterFlag = 0x00;           //set when data is ready to forward

//packet buffers for processing
static volatile uint8_t mActivePacketBuffer = 1;
static volatile uint8_t mPacketBuffer1[NRF24_PIPE_WIDTH] = {0x00};
static volatile uint8_t mPacketBuffer2[NRF24_PIPE_WIDTH] = {0x00};



//transmit addresses for pipes 0 - 5
//LSB First - load the array into reg
//in the order shown.  Note that the
//last 4 bytes on Pipe1 - Pipe5 need 
//to be the same
static const uint8_t mTxAddress_Pipe0[5] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};
static const uint8_t mTxAddress_Pipe1[5] = {0xC2, 0xC2, 0xC2, 0xC2, 0xC2};
static const uint8_t mTxAddress_Pipe2[5] = {0xC3, 0xC2, 0xC2, 0xC2, 0xC2};
static const uint8_t mTxAddress_Pipe3[5] = {0xC4, 0xC2, 0xC2, 0xC2, 0xC2};
static const uint8_t mTxAddress_Pipe4[5] = {0xC5, 0xC2, 0xC2, 0xC2, 0xC2};
static const uint8_t mTxAddress_Pipe5[5] = {0xC6, 0xC2, 0xC2, 0xC2, 0xC2};


//////////////////////////////////////////////
//Dummy delay
void nrf24_dummyDelay(uint32_t delay)
{
    volatile uint32_t temp = delay;
    while (temp > 0)
        temp--;
}


////////////////////////////////////////////////
//Write data to register.
//Combine write reg command with 5 bit reg value.
//Send as write followed by 1 data byte
//
void nrf24_writeReg(uint8_t reg, uint8_t data)
{
    uint8_t regValue = NRF24_CMD_W_REGISTER | (reg & (0x1F));

    SPI_select();                   //CS low
    SPI_tx(regValue);               //Set write reg
    SPI_tx(data);                   //send the data
    SPI_deselect();                 //CS high
}



/////////////////////////////////////////////
//Write reg with more than 1 byte of data
//Used for setting tx/rx addresses
void nrf24_writeRegArray(uint8_t reg, uint8_t* data, uint8_t length)
{
    uint8_t i = 0x00;
    uint8_t regValue = NRF24_CMD_W_REGISTER | (reg & (0x1F));

    SPI_select();                   //CS low
    SPI_tx(regValue);               //Set write reg

    for (i = 0 ; i < length ; i++)    
        SPI_tx(data[i]);

    SPI_deselect();                 //CS high
}




//////////////////////////////////////////////////
//Write command byte followed by length data bytes
void nrf24_writeCmd(uint8_t command, uint8_t* data, uint8_t length)
{
    uint8_t i = 0x00;

    SPI_select();           //CS low
    SPI_tx(command);

    if (length > 0)
    {
        for (i = 0 ; i < length ; i++)
            SPI_tx(data[i]);
    }

    SPI_deselect();
}


////////////////////////////////////////////////////
//Read register value.  Combine read reg command
//with register bits.  Read one byte
uint8_t nrf24_readReg(uint8_t reg)
{
    uint8_t data = 0x00;
    uint8_t regValue = NRF24_CMD_R_REGISTER | (reg & (0x1F));

    SPI_select();                   //CS low
    SPI_tx(regValue);               //Set read reg
    data = SPI_rx();                //read the data
    SPI_deselect();                 //CS high

    return data;
}





////////////////////////////////
//Pin 9 - PB1
void nrf24_ce_high(void)
{
   PORTB_DATA_R |= BIT1;
}

////////////////////////////////
//Pin 9 - PB1
void nrf24_ce_low(void)
{
   PORTB_DATA_R &=~ BIT1;
}


/////////////////////////////////////////////////
//Pulse the CE pin - for use with transmit mode
//pulse for at least 10us
//NRF24_CE_PULSE_LENGTH = 10000
void nrf24_ce_pulse(void)
{
    volatile uint32_t time = NRF24_CE_PULSE_LENGTH;

    PORTB_DATA_R |= BIT1;

    while (time > 0)
        time--;

    PORTB_DATA_R &=~ BIT1;
}


///////////////////////////////////////
//Set or Clear the PRIM_RX bit in the
//CONFIG REG
void nrf24_prime_rx_bit(uint8_t value)
{
    uint8_t result = nrf24_readReg(NRF24_REG_CONFIG);

    if (value == 1)
        result |= NRF24_BIT_PRIM_RX;
    else
        result &=~ NRF24_BIT_PRIM_RX;

    nrf24_writeReg(NRF24_REG_CONFIG, result);
}


void nrf24_power_up(void)
{
    uint8_t config = nrf24_readReg(NRF24_REG_CONFIG);
    config |= NRF24_BIT_PWR_UP;
    nrf24_writeReg(NRF24_REG_CONFIG, config);    
    nrf24_dummyDelay(100000);
}

void nrf24_power_down(void)
{
    uint8_t config = nrf24_readReg(NRF24_REG_CONFIG);
    config &=~ NRF24_BIT_PWR_UP;
    nrf24_writeReg(NRF24_REG_CONFIG, config);    
    nrf24_dummyDelay(100000);
}






////////////////////////////////////
//nrf24_init()
//Configure the GPIO pins, radio mode, registers
//
//IRQ Pin - PD2 - Pin 2 - Interrupt, falling edge, pullup (INT0)
//CE Pin - PB1 - Pin 9 - Normal output
//
//Pass the final mode
void nrf24_init(NRF24_Mode_t initialMode)
{
    //Pin 9 - PB1 - CE Pin
    PORTB_DIR_R |= BIT1; 	//pin 9 as output
    PORTB_DATA_R &=~ BIT1;	//clear pin 9

    //PD2 - Pin 2 - INT0 as input, interrupt, falling edge
    PORTD_DIR_R &=~ BIT2;   //pin 2 as input
    PORTD_DATA_R |= BIT2;   //enable pullup

    //configure interrupt 0 INT0 - PD2
    //configure INT0 - EICRA 0x02 falling edge
    EICRA_R |= 1u << 1;
    EICRA_R &=~ 0x01;

    //interrupt mask - EIMSK - 0x3D - set bit 0
    EIMSK_R |= 0X01;

    //clear pending interrupt - EIFR - 0X3C - bit 0
    EIFR_R |= 0x01;

    //enable global interrupts - set the I-bit in SREG
    SREG_R |= 1u << 7;

    sei();



    /////////////////////////////////////////////////
    //Register Configuration
    nrf24_writeReg(NRF24_REG_CONFIG, 0x00);         //No CRC, Enable All Interrupts    
    nrf24_writeReg(NRF24_REG_EN_AA, 0x00);          //Disable auto ack - if enable, CRC forced high
    nrf24_writeReg(NRF24_REG_CONFIG, 0x00);         //send again
        
    nrf24_dummyDelay(100000);                       //wait a while
    
    nrf24_writeReg(NRF24_REG_EN_RXADDR, 0x3F);      //enable all data pipes    
    nrf24_writeReg(NRF24_REG_SETUP_RETR, 0x00);     //disable retry / resend data
    
    nrf24_writeReg(NRF24_REG_RF_SETUP, 0x06);           //set power = 0dm, data rate = 1mbs
    
    nrf24_writeReg(NRF24_REG_RF_CH, NRF24_CHANNEL);     //see .h for channel def.
        
    //Set the payload widths on all data pipes
    nrf24_writeReg(NRF24_REG_RX_PW_P0, NRF24_PIPE_WIDTH & 0x3F);
    nrf24_writeReg(NRF24_REG_RX_PW_P1, NRF24_PIPE_WIDTH & 0x3F);
    nrf24_writeReg(NRF24_REG_RX_PW_P2, NRF24_PIPE_WIDTH & 0x3F);
    nrf24_writeReg(NRF24_REG_RX_PW_P3, NRF24_PIPE_WIDTH & 0x3F);
    nrf24_writeReg(NRF24_REG_RX_PW_P4, NRF24_PIPE_WIDTH & 0x3F);
    nrf24_writeReg(NRF24_REG_RX_PW_P5, NRF24_PIPE_WIDTH & 0x3F);
    
    //STATUS - Clear all pending interrupts
    nrf24_writeReg(NRF24_REG_STATUS, NRF24_BIT_RX_DR | NRF24_BIT_TX_DS | NRF24_BIT_MAX_RT);
    
    //flush rx and tx
    nrf24_flushTx();
    nrf24_flushRx();
    
    //Initial Mode - TX / RX / Repeater
    if (initialMode == NRF24_MODE_RX)
    {
        mNRF24_Mode = NRF24_MODE_RX;            //Receive only mode
        nrf24_setState(NRF24_STATE_RX);         //set to listen 
    }    
    else if (initialMode == NRF24_MODE_TX)
    {
        mNRF24_Mode = NRF24_MODE_TX;            //Transmit only mode
        nrf24_setState(NRF24_STATE_TX);         //set state to transmit
    }

    else if (initialMode == NRF24_MODE_REPEATER)
    {
        mNRF24_Mode = NRF24_MODE_REPEATER;      //RX/TX mode
        nrf24_setState(NRF24_STATE_RX);         //set to listen 
    }
    
    nrf24_dummyDelay(50000);                    //wait a while
}



NRF24_Mode_t nrf24_getMode(void)
{
    return mNRF24_Mode;
}

void nrf24_setState(NRF24_State_t state)
{
    if (state == NRF24_STATE_RX)
    {
        mNRF24_State = NRF24_STATE_RX;  //listening state
        nrf24_writeReg(NRF24_REG_STATUS, NRF24_BIT_RX_DR | NRF24_BIT_TX_DS);
        nrf24_prime_rx_bit(1);      //set the prim-rx bit high
        nrf24_power_up();           //set the power up bit
        nrf24_ce_high();            //set the ce pin high
    }

    else if (state == NRF24_STATE_TX)
    {
        mNRF24_State = NRF24_STATE_TX;  //transmit state
        nrf24_writeReg(NRF24_REG_STATUS, NRF24_BIT_RX_DR | NRF24_BIT_TX_DS);
        nrf24_prime_rx_bit(0);      //set the prim-rx bit low
        nrf24_power_up();           //set the power up bit
        nrf24_ce_low();             //set the ce pin low
    }
}


NRF24_State_t nrf24_getState(void)
{
    return mNRF24_State;
}



uint8_t nrf24_getRepeaterFlag(void)
{
    return mRepeaterFlag;
}

void nrf24_setRepeaterFlag(uint8_t value)
{
    mRepeaterFlag = value;    
}


////////////////////////////////////////////
//loads the contents of buffer with contents 
//of mRepeaterBuffer, returns the number of bytes
uint8_t nrf24_getRepeaterBuffer(uint8_t *buffer)
{
    int i = 0;
    for (i = 0 ; i < NRF24_PIPE_WIDTH ; i++)
    {
        buffer[i] = mRepeaterBuffer[i];
    }
}





///////////////////////////////////////
//send single byte 0xFF and return the 
//contents of the SPI_DR
uint8_t nrf24_getStatus(void)
{
    uint8_t status = 0x00;
    SPI_select();
    status = SPI_tx(0xFF);
    SPI_deselect();
    return status;    
}


uint8_t nrf24_getFifoStatus(void)
{
    uint8_t status = nrf24_readReg(NRF24_REG_FIFO_STATUS);
    return status;
}



//////////////////////////////////////////////
//Check if the rx fifo has more data to read.
//Read the fifo status reg - bit 0 - RX_EMPTY bit
//if high, no more data to read.
//
uint8_t nrf24_RxFifoHasData(void)
{
    uint8_t bit, status = 0x00;
    status = nrf24_readReg(NRF24_REG_FIFO_STATUS);
    bit = status & NRF24_BIT_RX_EMPTY;

    //bit value = 0 - Not Empty
    if (!bit)
        return 1;
    else
        return 0;
}



///////////////////////////////////////////
//TxFifo Has Space
//returns 1 if there is an empty space
uint8_t nrf24_TxFifoHasSpace(void)
{
    //full = high bit
    if (nrf24_getStatus() & NRF24_BIT_TX_FULL)
        return 0;
    else
        return 1;
}



///////////////////////////////////////////
//Send command only - flush rx buffers
void nrf24_flushRx(void)
{
    uint8_t dummy = 0x00;
    nrf24_writeCmd(NRF24_CMD_FLUSH_RX, &dummy, 0);
}


///////////////////////////////////////////
//Send command only - flush tx buffers
void nrf24_flushTx(void)
{
    uint8_t dummy = 0x00;
    nrf24_writeCmd(NRF24_CMD_FLUSH_TX, &dummy, 0);
}



///////////////////////////////////////////
//Set the transmit address
//tx addresses are defined at top for
//each pipe.  NOTE: Address arrays are 
//reversed from the datasheet, so that LSB 
//is sent first
//
void nrf24_setTxPipe(uint8_t pipe)
{
    const uint8_t* pipeAddress = mTxAddress_Pipe0;

    if (pipe <= 5)
    {
        switch(pipe)
        {
            case 0:     pipeAddress = mTxAddress_Pipe0;     break;
            case 1:     pipeAddress = mTxAddress_Pipe1;     break;
            case 2:     pipeAddress = mTxAddress_Pipe2;     break;
            case 3:     pipeAddress = mTxAddress_Pipe3;     break;
            case 4:     pipeAddress = mTxAddress_Pipe4;     break;
            case 5:     pipeAddress = mTxAddress_Pipe5;     break;
            default:    pipeAddress = mTxAddress_Pipe0;     break;
        }

        //write pipeAddress into the TX_ADDR register
        nrf24_writeRegArray(NRF24_REG_TX_ADDR, (uint8_t*)pipeAddress, 5);
    }
}



//////////////////////////////////////////////////////
//Write data buffer to the tx payload register.
//
void nrf24_writeTXPayLoad(uint8_t* buffer, uint8_t length)
{
    nrf24_writeCmd(NRF24_CMD_W_TX_PAYLOAD, buffer, length);
}

//////////////////////////////////////////////////////
//Transmit Data
//Uses TX_DS interrupt to set the transmit complete flag
//Clear the interrupt bits
//flush the tx register
//write data to the tx payload register
//pulse the ce pin and wait...
//
//Added check - if the mode is repeater mode,
//return to receive state
void nrf24_transmitData(uint8_t pipe, uint8_t* buffer, uint8_t length)
{
    uint16_t timeout = NRF24_TX_TIMEOUT;    //reset the counter    
    mTransmitCompleteFlag = 0;              //clear the tx complete flag
    uint8_t hexBuffer[64] = {0x00};

    //set the transmit pipe
    nrf24_setTxPipe(pipe);

    //clear the tx_ds and max_rt bits
    nrf24_writeReg(NRF24_REG_STATUS, NRF24_BIT_TX_DS | NRF24_BIT_TX_DS);

    nrf24_flushTx();                                    //flush the tx fifo
    nrf24_writeTXPayLoad(buffer, NRF24_PIPE_WIDTH);     //write the data to the payload reg
    nrf24_ce_pulse();                                   //pulse the ce pin
    
    while ((!mTransmitCompleteFlag) && (timeout > 0))
    {
        timeout--;
    }
    
    //if timeout == 0 or transmit complete flag
    if ((!timeout) || (!mTransmitCompleteFlag))
    {
        Usart_sendString("Timeout - Counter Expired - Transmit Aborted\r\n");
        mTransmitCompleteFlag = 0x00;
        nrf24_flushTx();
        nrf24_writeReg(NRF24_REG_STATUS, NRF24_BIT_TX_DS | NRF24_BIT_MAX_RT);
    }
    
    else if(mTransmitCompleteFlag == 1)
    {
        Usart_sendString("Polling - Transmit Complete: ");

        //convert the buffer to a hex output buffer
        //            n = utility_data2HexBuffer(rxBuffer, 8, output);
        int n = utility_data2HexBuffer(buffer, length, hexBuffer);
        Usart_sendArray(hexBuffer, n);
        Usart_sendString("\r\n");
    }
    
    else
    {
        Usart_sendString("Polling - Transmit Flag Never Set - Timeout\r\n");    
    }
    
    /////////////////////////////////////////
    //Test the tx_ds and max_rt bits
    if (nrf24_getStatus() & (NRF24_BIT_TX_DS | NRF24_BIT_TX_DS ))
    {
        //tx ds bit high - clearing it
        Usart_sendString("TX_DS and or MAX_RT Bit(s) High - Clearing It...\r\n");
        nrf24_writeReg(NRF24_REG_STATUS, NRF24_BIT_TX_DS | NRF24_BIT_MAX_RT);
    }   

    //if mode = repeater - return to listen state
    if (mNRF24_Mode == NRF24_MODE_REPEATER)
    {
        nrf24_setState(NRF24_STATE_RX);
    }
}



//////////////////////////////////////////////////
//Receiver functions

/////////////////////////////////////////////////////
//Set the size of the RX payload.  The size of the pipe
//is the number of bytes that triggers the RX_DR interrupt.
//Max size = NRF24_PIPE_WIDTH_MAX
//base reg = NRF24_REG_RX_PW_P0
//reg = base + pipe assuming pipe 0-5
void nrf24_setRxPayLoadSize(uint8_t pipe, uint8_t numBytes)
{
    uint8_t reg = 0x00;
    
    //valid pipe # 0-5, max width = 32 bytes
    if ((pipe <= 5) && (numBytes <= NRF24_PIPE_WIDTH_MAX))
    { 
        reg = NRF24_REG_RX_PW_P0 + pipe;                    //pipe address continuous
        nrf24_writeReg(reg, (numBytes & 0x3F));             //write num bytes to bits 0:5
    }
}



//////////////////////////////////////////////////
//Get the pipe width for a particular pipe.  
//Pipe Range: 0-5
uint8_t nrf24_getRxPayLoadSize(uint8_t pipe)
{
    uint8_t reg = 0x00;
    uint8_t width = 0x00;

    if (pipe <= 5)
    {
        reg = NRF24_REG_RX_PW_P0 + pipe;                    //pipe address continuous
        width = (nrf24_readReg(reg) & 0x3F);

        return width;
    }
    
    return 0;
}






//////////////////////////////////////////////////////////
//Read which pipe has data available to read from
//the RX Fifo.  STATUS register - bits 3:1, 111 = RX FIFO
//empty, 110 not used. ie, 0 - 5
uint8_t nrf24_getRxPipeToRead(void)
{
    uint8_t pipe = (nrf24_getStatus() >> 1) & 0x07;
    
    //test for valid, if 111, return 0xFF;
    if (pipe == 0x07)           //empty
        return 0xFF;
     else if (pipe == 0x06)     //not used, error
        return 0xFE;
    else
        return pipe;
}




////////////////////////////////////////////////////////
//Read the payload at the top of the Rx FIFO
//Uses command: NRF24_CMD_R_RX_PAYLOAD - reading 1 to 32 bytes
//#define NRF24_CMD_R_RX_PAYLOAD          0x61
void nrf24_readRxPayLoad(uint8_t* data, uint8_t length)
{
    uint8_t i = 0x00;
    uint8_t cmd = NRF24_CMD_R_RX_PAYLOAD;

    SPI_select();       //CS low
    SPI_tx(cmd);        //Set the read rx fifo cmd

    //read data bytes into buffer
    for (i = 0 ; i < length ; i++)
        data[i] = SPI_rx();

    SPI_deselect();
}



///////////////////////////////////////////////////
//Read Data From RX FIFO
//Reading the data removes it from the FIFO.
//This function is called from the ISR when
//RX_DR interrupt is enabled.
//
//data - buffer populated with data
//pipe - pipe from which data is read
//size - pipe width
//returns the length of bytes in the pipe.
//
uint8_t nrf24_readRxData(uint8_t* data, uint8_t* pipe)
{
    uint8_t length = 0x00;    
    uint8_t pipeNum = nrf24_getRxPipeToRead();    //returns pipeNum

    if (pipeNum <= 0x05)                         //pipe 5 max
    {
        *pipe = pipeNum;                            //set the pipe read
        length = nrf24_getRxPayLoadSize(pipeNum);   //pipe width
        nrf24_readRxPayLoad(data, length);          //read the data

        return length;
    }

    return 0xFF;        //error
}



////////////////////////////////////////////////////////
//Called from the interrupt IRQ pin handler.
//Mapped to pin PA2 - input, falling edge
//Three interrupt sources: data transmitted successfully,
//max retransmissions reached, and data arrived in RX fifo.
//each it's own bit.  Write one to clear the bit.  
//
//For now, only enable the RX_DR interrupt and TX_DS interrupt
//
void nrf24_ISR(void)
{
    int n = 0x00;
    uint8_t len = 0x00;
    uint8_t pipe = 0x00;
    uint8_t rxBuffer[NRF24_PIPE_WIDTH] = {0x00};
    uint8_t status = nrf24_getStatus();
    uint8_t output[64] = {0x00};

    //RX_DR Interrupt - Data Received
    if (status & NRF24_BIT_RX_DR)
    {
        //read the rx fifo while packets are available
        while (nrf24_RxFifoHasData())
        {
            len = nrf24_readRxData(rxBuffer, &pipe);            //read the packet and pipe

            //output result
            n = sprintf(output, "RX(%d): ", pipe);
            Usart_sendArray(output, n);                   //forward it to the uart

            n = utility_data2HexBuffer(rxBuffer, NRF24_PIPE_WIDTH, output);
            Usart_sendArray(output, n);                   //forward it to the uart

            Usart_sendString("\r\n");
            
            //Test for a valid packet
            if ((rxBuffer[0] == 0xFE) && (rxBuffer[NRF24_PIPE_WIDTH - 1] == 0xFE))
            {
                //NRF24_MODE_REPEATER: Repeater Mode - Forward Data
                //Copy to mRepeaterBuffer and set a flag for
                //polling in the main loop
                if (mNRF24_Mode == NRF24_MODE_REPEATER)
                {
                    Usart_sendString("Repeater::Forwarding:: ");

                    n = sprintf(output, "RX(%d): ", pipe);
                    Usart_sendArray(output, n);                   //forward it to the uart

                    n = utility_data2HexBuffer(rxBuffer, NRF24_PIPE_WIDTH, output);
                    Usart_sendArray(output, n);                   //forward it to the uart
                    Usart_sendString("\r\n");

                    rxBuffer[1] = STATION_REPEATER_1;       //update the source
                    memcpy(mRepeaterBuffer, rxBuffer, NRF24_PIPE_WIDTH);   //copy the buffer
                    mRepeaterFlag = 1;                      //set the flag - polled in main loop
                }

                //NRF24_MODE_RX: Receive Only
                else if (mNRF24_Mode == NRF24_MODE_RX)
                {
                    if (mActivePacketBuffer == 1)
                    {
                        memcpy(mPacketBuffer1, rxBuffer, NRF24_PIPE_WIDTH);        //copy into buffer
                        mActivePacketBuffer = 2;
                        nrf24_processPacket(pipe, mPacketBuffer1, NRF24_PIPE_WIDTH);     //pipe, buffer, size
                    }

                    else
                    {
                        memcpy(mPacketBuffer2, rxBuffer, NRF24_PIPE_WIDTH);        //copy into buffer
                        mActivePacketBuffer = 1;
                        nrf24_processPacket(pipe, mPacketBuffer2, NRF24_PIPE_WIDTH);     //pipe, buffer, size
                    }
                }
            }

            else
            {
                //bad / missing data - don't forward it
                Usart_sendString("Bad Data / Corrupt Packet\r\n");
            }

            //clear the interrupt
            nrf24_writeReg(NRF24_REG_STATUS, NRF24_BIT_RX_DR);
        }
    }

    //TX_DS Interrupt - Data Sent
    else if (status & NRF24_BIT_TX_DS)
    {
        mTransmitCompleteFlag = 1;      //set the transmit complete flag
        //write 1 to clear the transmit complete flag        
        nrf24_writeReg(NRF24_REG_STATUS, NRF24_BIT_TX_DS | NRF24_BIT_MAX_RT);
    }
    
    //MX_RT Interrupt - Max Retransmissions - For Ack Only
    else if (status & NRF24_BIT_MAX_RT)
    {
//        UART_sendString("Max Retries Failed...\n");        
        nrf24_writeReg(NRF24_REG_STATUS, NRF24_BIT_TX_DS | NRF24_BIT_MAX_RT);
    }    
}






///////////////////////////////////////////////////
//Process the packet
//Assumes in RX mode, start and stop bytes are correct.
void nrf24_processPacket(uint8_t pipe, uint8_t* buffer, uint8_t size)
{
    int n = 0x00;
    uint8_t output[64] = {0x00};
    uint16_t adcValue, adcLSB, adcMSB = 0x00;
    uint8_t tempInt, tempFrac = 0x00;

    Usart_sendString("Processing Packet\r\n");

    n = sprintf(output, "RX(%d): ", pipe);
    Usart_sendArray(output, n);                   //forward it to the uart

    n = utility_data2HexBuffer(buffer, NRF24_PIPE_WIDTH, output);
    Usart_sendArray(output, n);                   //forward it to the uart
    Usart_sendString("\r\n");

    //MID ADC
    if (buffer[2] == MID_TEMP_MCP9700A)
    {
        adcLSB = (uint16_t)buffer[3];
        adcMSB = (uint16_t)buffer[4];
        adcValue = (adcMSB << 8) | (adcLSB & 0xFF);

        tempInt = buffer[5];
        tempFrac = buffer[6];

        //output the result....
        Usart_sendString("ADC: ");
        n = utility_decimal2Buffer(adcValue, output);
        Usart_sendArray(output, n);
        Usart_sendString("\r\n");

        Usart_sendString("TEMP: ");
        n = utility_decimal2Buffer(tempInt, output);
        Usart_sendArray(output, n);

        Usart_sendString(".");
        n = utility_decimal2Buffer(tempFrac, output);
        Usart_sendArray(output, n);

        Usart_sendString("\r\n");
    }
}



