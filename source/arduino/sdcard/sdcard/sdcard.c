////////////////////////////////////////////
//SD Card Driver File
//
//SD card is driven via the SPI on pins
//10, 11, 12, 13.  + one more for the CS pin.
//
//Need another function in the SPI for setting
//the speed.
//
//All calls to the high level FATFS functions are 
//contained in this driver file, as well as members 
//for FATFS, FIL, etc....
//
#include <string.h>
#include <stdio.h>

#include "sdcard.h"
#include "spi.h"            //spi control
#include "usart.h"          //serial output

#include "diskio.h"	     //defines - CMD0
#include "ff.h"

#include "register.h"

//////////////////////////////////////////////
//FatFs stuff - read/write buffers for sdcard

uint8_t SD_readBuffer[SD_READ_BUFFER_SIZE];
uint8_t SD_writeBuffer[SD_WRITE_BUFFER_SIZE];


//////////////////////////////////////////////////
//FATFS globals - accessed from sdcard_driver only

volatile FATFS fs32;     //mount this one

#if _USE_LFN
    static char lfn[_MAX_LFN + 1];
    fno.lfname = lfn;
    fno.lfsize = sizeof lfn;
#endif




/////////////////////////////////////////
//Initializes the sd card
//Procedure:
//Puts the card into idle state.  Returns -255
//if can't go idle with 255 attempts.
//Initializes the disk using FatFS function calls
//by calling fmount.  If can't mount the drive, 
//returns neg value of the error code.  
//Builds the directory file on the sd card.
//returns negative value of the error code if it
//can't build the directory, returns the size
//of the directory file in bytes if everything
//worked ok.  
//
//
//Also inits all members associated with sd card
//buffers, read, write buffers, etc.
int SD_Init(void)
{
    FRESULT res;            //fatfs results

    //clear the read and write buffers
    memset(SD_readBuffer, 0x00, SD_READ_BUFFER_SIZE);
    memset(SD_writeBuffer, 0x00, SD_WRITE_BUFFER_SIZE);

    //put sd card in idle state - result should be 0x01
    //if it worked.  
//    uint8_t status = SD_GoIdleState();

    //mount the drive
    memset(&fs32, 0, sizeof(FATFS));

//    if (status != 0x01)
  //  {
        //invalid response, the valid response for
        //CMD0 is 0x01..  return -255
        //return value beyond range of fat error codes
   //     return -255;
   // }

    res = f_mount(&fs32, "", 0);
   
    //test the result - not ok, print something
    if (res != FR_OK)
    {
        SD_ErrorHandler(res);
        return (-1*((int)res));
    }

    if (res == FR_OK)
    {
        Usart_sendString("Fmount is FR OK\r\n");
        //drive mounte - build the sd card directory file
        //function returns the size of the directory
        //or the negative value of the error code
        //if something did not work.
        //
        //make a file

        //int SD_PrintFileToBuffer(char* name, uint8_t* dest, uint32_t maxbytes);

//        int len = SD_PrintFileToBuffer("DIR.TXT", SD_readBuffer, 32);
//        Usart_sendString("Reading DIR.TXT\r\n");

 //       if (len > 64)
 //           Usart_sendArray(SD_readBuffer, 64);
 //       else
  //          Usart_sendArray(SD_readBuffer, len);


//        SD_FileCreate("IT_WORKS.TXT");

        return 1;
    }

    return (-1*((int)res));
}





///////////////////////////////////////
//Generic error handler
//Toggle Pin 8 a bunch of times
void SD_ErrorHandler(FRESULT result)
{
    int i = 0;
    uint8_t buffer[32];
    int n = sprintf(buffer, "Error: %d\r\n", result);    
    Usart_sendArray(buffer, n);

    for (i = 0 ; i < 10 ; i++)
    {
        PORTB_DATA_R ^= BIT0;
        SPI_delay(10);
    }
}


//////////////////////////////////////////////
//char* SD_GetStringFromFatCode(FRESULT result);
//Get print string from FRESULT list
//returns the string from the result
/*
char* SD_GetStringFromFatCode(FRESULT result)
{
	//scan the list of results and return the
	//cooresponding string
	switch(result)
	{
		case 	FR_OK:					return "Success\r\n";
		case 	FR_DISK_ERR:			return "(1) A hard error occurred in the low level disk I/O layer\r\n";
		case 	FR_INT_ERR:				return "(2) Assertion failed\r\n";
		case	FR_NOT_READY:			return "(3) The physical drive cannot work\r\n";
		case 	FR_NO_FILE:				return "(4) Could not find the file\r\n";
		case	FR_NO_PATH:				return "(5) Could not find the path\r\n";
		case 	FR_INVALID_NAME:		return "(6) The path name format is invalid\r\n";
		case 	FR_DENIED:				return "(7) Access denied due to prohibited access or directory full\r\n";
		case	FR_EXIST:				return "(8) Access denied due to prohibited access\r\n";
		case 	FR_INVALID_OBJECT:		return "(9) The file/directory object is invalid\r\n";
		case 	FR_WRITE_PROTECTED:		return "(10) The physical drive is write protected\r\n";
		case 	FR_INVALID_DRIVE:		return "(11) The logical drive number is invalid\r\n";
		case 	FR_NOT_ENABLED:			return "(12) The volume has no work area\r\n";
		case 	FR_NO_FILESYSTEM:		return "(13) There is no valid FAT volume\r\n";
		case 	FR_MKFS_ABORTED:		return "(14) The f_mkfs() aborted due to any parameter error\r\n";
		case 	FR_TIMEOUT:				return "(15) Could not get a grant to access the volume within defined period\r\n";
		case	FR_LOCKED:				return "(16) The operation is rejected according to the file sharing policy\r\n";
		case 	FR_NOT_ENOUGH_CORE:		return "(17) LFN working buffer could not be allocated\r\n";
		case 	FR_TOO_MANY_OPEN_FILES: return "(18) Number of open files > _FS_SHARE\r\n";
		case 	FR_INVALID_PARAMETER:	return "(19) Given parameter is invalid\r\n";
		default:						return "Unknown error, not listed in FRESULT\r\n";
	}
}

*/





///////////////////////////////////////////
//CS LOW - Pin 10
void SD_CS_Assert()
{
	SPI_select();
}

///////////////////////////////////////////
//CS HIGH - Pin 10
void SD_CS_Deassart()
{
	SPI_deselect();
}

///////////////////////////////////////////
//Send a byte - no CS control, querry
//the rx fifo to clear it
void SPI_transmit(uint8_t data)
{
	SPI_tx(data);
}


uint8_t SPI_receive()
{
	uint8_t rx = SPI_rx();
    return rx;
}


//******************************************************************
//Function: to initialize the SD card in SPI mode
//Arguments: none
//return: unsigned char; will be 0 if no error,
// otherwise the response byte will be sent
//Result:  Puts the sd card into idle state
//disk_initialize is called after this to complete
//the init procedure.  Eventually, this code will get moved
//into that function or the problem with that function will
//get fixed.  
//******************************************************************
unsigned char SD_GoIdleState()
{
    uint8_t buf[64];
    int n;

    //set the SPI speed to low
    FCLK_SLOW();
    
    //void SPI_delay(unsigned long t)
    SPI_delay(1000000);                       //wait a bit

    uint8_t i = 0x00;
    volatile uint8_t response = 0x00;
    volatile uint8_t retry = 0x00;

    Usart_sendString("SD_GoIdleState\r\n");

    SD_CS_Assert();

    while (response != 0x01)
    {
        //send 80 clock cycles
        for(i=0 ; i < 10 ; i++)
        {
            SPI_transmit(0xff);
        }

        //send CMD0 - Go Idle state - continue
        //until the response is valid = 0x01

        response = SD_sendCommand(CMD0, 0);

        //print the response
        n = sprintf(buf, "Attempt: %d,  CMD: %d   Response: %d\r\n", retry, CMD0, response);
        Usart_sendArray(buf, n);

        retry++;

        if( retry > 0xfe)
        {
            //error, could not init the card, 
            //write something to the uart...
            Usart_sendString("SD_Init: CMD0 - Failure to Receive 0x01\r\n");
            return 0;

        }//time out
    }


    SD_CS_Deassart();

    return 1; //normal return
}


//******************************************************************
//Function: to send a command to SD card
//Arguments: unsigned char (8-bit command value)
// & unsigned long (32-bit command argument)
//return: unsigned char; response byte
//******************************************************************
unsigned char SD_sendCommand(unsigned char cmd, unsigned long arg)
{
    unsigned char response, retry=0;
    uint8_t buf[50];
    int n = 0x00;

    SD_CS_Assert();
    
    SPI_transmit(cmd | 0x40); //send command, first two bits always '01'
    SPI_transmit(arg>>24);
    SPI_transmit(arg>>16);
    SPI_transmit(arg>>8);
    SPI_transmit(arg);
    
    uint8_t crc = 0x01;
    if (cmd == CMD0)    crc = 0x95;
    if (cmd == CMD8)    crc = 0x87;

    SPI_transmit(crc);

    while((response = SPI_receive()) == 0xff)
    {
        //print the command and response
        n = sprintf(buf, "CMD: 0x%x   RES: 0x%x\r\n", cmd, response);
        Usart_sendArray(buf, n);

        retry++;

        //wait response
        if(retry > 0xfe)
        {
            //error - command failed 
            Usart_sendString("Command Failed - Continued RX 0xFF after 0xFE retries\r\n");
            break; //time out error
        }
    }

    //print the last response
    n = sprintf(buf, "CMD: 0x%x   RES: 0x%x\r\n", cmd, response);
    Usart_sendArray(buf, n);
    
    SPI_receive(); //extra 8 CLK
    
    SD_CS_Deassart();

    ////////////////////////////////////////
    //some more code follows that was removed from
    //the routine.  Stops here to simply
    //put the card into the idle state
    //FatFs diskinit code handles the rest.
    
    return response; //return state
}






//////////////////////////////////////////////
//function defintions
void CS_HIGH()
{
	SPI_deselect();
}

void CS_LOW()
{
	SPI_select();
}

void sd_deselect()
{
	CS_HIGH();		/* Set CS# high */
	xchg_spi(0xFF);	/* Dummy clock (force DO hi-z for multiple slave SPI) */
}

int sd_select()
{
	CS_LOW();		/* Set CS# low */
	xchg_spi(0xFF);	/* Dummy clock (force DO enabled) */
	if (wait_ready(500)) return 1;	/* Wait for card ready */

	sd_deselect();
	return 0;	/* Timeout */

}


int wait_ready(uint32_t wt)
{
	BYTE d;
    
	Timer2 = wt;
	do {
		d = xchg_spi(0xFF);
		/* This loop takes a time. Insert rot_rdq() here for multitask envilonment. */
	} while (d != 0xFF && Timer2);	/* Wait for card goes ready or timeout */

	return (d == 0xFF) ? 1 : 0;
}



/*-----------------------------------------------------------------------*/
/* Device timer function                                                 */
/*-----------------------------------------------------------------------*/
/* This function must be called from timer interrupt routine in period
/  of 1 ms to generate card control timing.
*/

void disk_timerproc ()
{
	WORD n;
	BYTE s;

	n = Timer1;						/* 1kHz decrement timer stopped at 0 */
	if (n) Timer1 = --n;
	n = Timer2;
	if (n) Timer2 = --n;

	s = Stat;
	if (MMC_WP) {	/* Write protected */
		s |= STA_PROTECT;
	} else {		/* Write enabled */
		s &= ~STA_PROTECT;
	}
	if (MMC_CD) {	/* Card is in socket */
		s &= ~STA_NODISK;
	} else {		/* Socket empty */
		s |= (STA_NODISK | STA_NOINIT);
	}
	Stat = s;
}



void init_spi()
{
    //do nothing, it's already initialized
}

//////////////////////////
//400khz clock
//void SPI_setSpeed(SPISpeed_t speed)

void FCLK_SLOW()
{
	SPI_setSpeed(SPI_SPEED_250_KHZ);
}

//////////////////////////
//12mhz clock speed
void FCLK_FAST()
{
	SPI_setSpeed(SPI_SPEED_4_MHZ);
}

/////////////////////////////////
//send data over the spi - no chip select
//
uint8_t xchg_spi(uint8_t data)
{
	uint8_t result = SPI_tx(data);
	return result;
}




uint8_t send_cmd(BYTE cmd, DWORD arg)
{
	BYTE n, res;

	if (cmd & 0x80) {	/* Send a CMD55 prior to ACMD<n> */
		cmd &= 0x7F;
		res = send_cmd(CMD55, 0);
		if (res > 1) return res;
	}

	/* Select the card and wait for ready except to stop multiple block read */
	if (cmd != CMD12) {
		sd_deselect();
		if (!sd_select()) return 0xFF;
	}

	/* Send command packet */
	xchg_spi(0x40 | cmd);				/* Start + command index */
	xchg_spi((BYTE)(arg >> 24));		/* Argument[31..24] */
	xchg_spi((BYTE)(arg >> 16));		/* Argument[23..16] */
	xchg_spi((BYTE)(arg >> 8));			/* Argument[15..8] */
	xchg_spi((BYTE)arg);				/* Argument[7..0] */
	n = 0x01;							/* Dummy CRC + Stop */
	if (cmd == CMD0) n = 0x95;			/* Valid CRC for CMD0(0) */
	if (cmd == CMD8) n = 0x87;			/* Valid CRC for CMD8(0x1AA) */
	xchg_spi(n);

	/* Receive command resp */
	if (cmd == CMD12) xchg_spi(0xFF);	/* Diacard following one byte when CMD12 */
	n = 10;								/* Wait for response (10 bytes max) */

	do {
		res = xchg_spi(0xFF);
	} while ((res & 0x80) && --n);

	return res;							/* Return received response */
}





////////////////////////////////////////////////////
/* Receive multiple byte */
//read btr bytes into pre allocated buffer *buff
//FATFS initially had this defined as 16 bit
//transfers, leave it at 8
//Args:
//BYTE *buff - Pointer to the data buffer, assumes this is allocated already
//UINT btr - Number of bytes to read

void rcvr_spi_multi (BYTE *buff, UINT btr)
{
    uint8_t result;

    for (int i = 0 ; i < btr ; i++)
    {
    	result = SPI_rx();

        buff[i] = (uint8_t)(result & 0xFF); //copy to the buffer
    }
}


//////////////////////////////////////////////
////////////////////////////////////////////////
#if _USE_WRITE
/* Send multiple byte */
/////////////////////////////////////////////////
//*buff - pointer to data to send
//btx - bytes to send
//
void xmit_spi_multi (const BYTE *buff, UINT btx)
{
    for (int i = 0 ; i < btx ; i++)
    {
    	SPI_tx(buff[i]);
    }
}
#endif



/*-----------------------------------------------------------------------*/
/* Receive a data packet from the MMC                                    */
/*-----------------------------------------------------------------------*/
//1: OK, 0: Error
//*buff - Pointer to the data buffer
//btr - length of the data buffer (bytes)
//
int rcvr_datablock (BYTE *buff, UINT btr)
{
	BYTE token;

	Timer1 = 200;
	do {							/* Wait for DataStart token in timeout of 200ms */
		token = xchg_spi(0xFF);
		/* This loop will take a time. Insert rot_rdq() here for multitask envilonment. */
	} while ((token == 0xFF) && Timer1);
	if(token != 0xFE) return 0;		/* Function fails if invalid DataStart token or timeout */

	rcvr_spi_multi(buff, btr);		/* Store trailing data to the buffer */
	xchg_spi(0xFF); xchg_spi(0xFF);			/* Discard CRC */

	return 1;						/* Function succeeded */
}



/*-----------------------------------------------------------------------*/
/* Send a data packet to the MMC                                         */
/*-----------------------------------------------------------------------*/

#if _USE_WRITE

int xmit_datablock (	/* 1:OK, 0:Failed */
	const BYTE *buff,	/* Ponter to 512 byte data to be sent */
	BYTE token			/* Token */
)
{
	BYTE resp;

	if (!wait_ready(500)) return 0;		/* Wait for card ready */

	xchg_spi(token);					/* Send token */
	if (token != 0xFD) {				/* Send data if token is other than StopTran */
		xmit_spi_multi(buff, 512);		/* Data */
		xchg_spi(0xFF); xchg_spi(0xFF);	/* Dummy CRC */

		resp = xchg_spi(0xFF);				/* Receive data resp */
		if ((resp & 0x1F) != 0x05) return 0;	/* Function fails if the data packet was not accepted */
	}
	return 1;
}
#endif


///////////////////////////////////////////
//I think this is used for setting up the
//directory function.  
char *dec32(unsigned long i)
{
  static char str[16];
  char *s = str + sizeof(str);

  *--s = 0;

  do
  {
    *--s = '0' + (char)(i % 10);
    i /= 10;
  }
  while(i);

  return(s);
}






////////////////////////////////////////////////////
//SD_PrintFileToBuffer
//Write contents of a file to a buffer
//Assumes the file exists
//Args:
//name - filename
//dest - pointer to destination buffer
//      if the file does not fit in the dest.
//      buffer..... not sure
//maxbytes - size of the destination buffer
//return: number of bytes read into the destintion buffer
//returns negative value of error code if error

int SD_PrintFileToBuffer(char* name, uint8_t* dest, uint32_t maxbytes)
{
	FRESULT res;
	FIL fil;
    UINT bytesRead = 0x00;

    res = f_open(&fil, name, FA_READ);

    if (res != FR_OK)
    {
        f_close(&fil);
        SD_ErrorHandler(res);
        return (-1*((int)res));
    }

    if (res == FR_OK)
    {
        //read the contents of the file, not to exceed
        //maxbytes, total bytes read into address of bytesRead

        res = f_read(&fil, dest, maxbytes, &bytesRead);

        if (res != FR_OK)
        {
            f_close(&fil);
            SD_ErrorHandler(res);
            return (-1*((int)res));
        }
            
        if (res == FR_OK)
        {
            dest[bytesRead] = 0x00;
            //close file
            f_close(&fil);
            return (int)bytesRead;
        }
    }

    res = f_close(&fil);
    return -255;
}







/////////////////////////////////////////////////
//file write, read, append, create, delete, etc

//Append data to the end of a file
//Args:
//name = pointer to the file name to open, append
//data = pointer to the data to be appended
//size = number of data bytes
//return neg value of the error code if error
int SD_AppendData(char* name, uint8_t* data, uint32_t size)
{
	FRESULT res;
	FIL fil;
    UINT bytesWritten = 0x00;

    res = f_open(&fil, name, FA_WRITE | FA_OPEN_APPEND);

    if (res != FR_OK)
    {
        f_close(&fil);
        SD_ErrorHandler(res);
        return (-1*((int)res));
    }

    if (res == FR_OK)
    {
        //write the data to the end of the file,
        //compare size and bytesWritten
        res = f_write(&fil, data, size, &bytesWritten);

        if (res != FR_OK)
        {
            f_close(&fil);
            SD_ErrorHandler(res);
            return (-1*((int)res));
        }

        if (res == FR_OK)
        {
            //compare the number of bytesWritten
            //to the size
            if (bytesWritten == size)
            {
                //ok, good
                res = f_close(&fil);
                return (int)bytesWritten;
            }
            else
            {
                //didn't write everything
                res = f_close(&fil);
                return -255;
            }
        }
    }

    res = f_close(&fil);
    return -255;   
}



/////////////////////////////////////////
//Make a new file and close it
//If it exits, it clears it
//If it does not exit, it creates it.
//returns -neg value of the error code if
//there is an error, returns 1 if ok.
int SD_FileCreate(char *name)
{
	FRESULT res;
	FIL fil;

    res = f_open(&fil, name, FA_WRITE | FA_CREATE_ALWAYS);

    if (res != FR_OK)
    {
        f_close(&fil);
        SD_ErrorHandler(res);
        return (-1*((int)res));     //neg value of fresult
    }

    if (res == FR_OK)
    {
        f_close(&fil);
    }

    return 1;    
}





