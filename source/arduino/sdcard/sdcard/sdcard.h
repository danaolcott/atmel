////////////////////////////////////////////
//SD Card Driver File
//SPI Interface using SPI - atmel
//Pins 10, 11, 12, 13
//
//
#ifndef SDCARD__H
#define SDCARD__H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "ff.h"         //FRESULT typdedefs
//////////////////////////////////////////
//Defines and externs
#define SD_READ_BUFFER_SIZE    256
#define SD_WRITE_BUFFER_SIZE   256

extern uint8_t SD_readBuffer[SD_READ_BUFFER_SIZE];
extern uint8_t SD_writeBuffer[SD_WRITE_BUFFER_SIZE];


////////////////////////////////////////////
//Typedefs




////////////////////////////////////////
//Function prototypes
//

int SD_Init(void);
void SD_ErrorHandler(FRESULT result);       


//lowest level spi functions
void SD_CS_Assert(void);
void SD_CS_Deassart(void);
void SPI_transmit(uint8_t data);
uint8_t SPI_receive(void);
unsigned char SD_GoIdleState(void);
unsigned char SD_sendCommand(unsigned char cmd, unsigned long arg);

//functions and commands from fat fs example
void CS_HIGH(void);		/* Set CS# high */
void CS_LOW(void);		/* Set CS# low */
void sd_deselect(void);
int sd_select(void);

int wait_ready(uint32_t wt);
void disk_timerproc (void);

void init_spi(void);
void FCLK_SLOW(void);
void FCLK_FAST(void);
uint8_t xchg_spi(uint8_t data);
uint8_t send_cmd(BYTE cmd, DWORD arg);
void rcvr_spi_multi (BYTE *buff, UINT btr);
#if _USE_WRITE
void xmit_spi_multi (const BYTE *buff, UINT btx);
#endif
int rcvr_datablock (BYTE *buff,	UINT btr);
#if _USE_WRITE
int xmit_datablock (const BYTE *buff, BYTE token);
#endif

char *dec32(unsigned long i);




//file write, read, append, create, delete, etc
int SD_PrintFileToBuffer(char* name, uint8_t* dest, uint32_t maxbytes);
int SD_AppendData(char* name, uint8_t* data, uint32_t size);

int SD_FileCreate(char *name);



#endif

